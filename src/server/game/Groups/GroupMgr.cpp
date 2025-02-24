/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "GroupMgr.h"
#include "Common.h"
#include "DBCStores.h"
#include "InstanceSaveMgr.h"
#include "Log.h"
#include "World.h"

GroupMgr::GroupMgr()
{
    _nextGroupId = 0;
}

GroupMgr::~GroupMgr()
{
    for (GroupContainer::iterator itr = GroupStore.begin(); itr != GroupStore.end(); ++itr)
        delete itr->second;
}

uint32 GroupMgr::GenerateNewGroupDbStoreId()
{
    uint32 newStorageId = NextGroupDbStoreId;

    for (uint32 i = ++NextGroupDbStoreId; i < 0xFFFFFFFF; ++i)
    {
        if ((i < GroupDbStore.size() && GroupDbStore[i] == nullptr) || i >= GroupDbStore.size())
        {
            NextGroupDbStoreId = i;
            break;
        }
    }

    if (newStorageId == NextGroupDbStoreId)
    {
        LOG_ERROR("misc", "Group storage ID overflow!! Can't continue, shutting down server. ");
        World::StopNow(ERROR_EXIT_CODE);
    }

    return newStorageId;
}

void GroupMgr::RegisterGroupDbStoreId(uint32 storageId, Group* group)
{
    // Allocate space if necessary.
    if (storageId >= uint32(GroupDbStore.size()))
        GroupDbStore.resize(storageId + 1);

    GroupDbStore[storageId] = group;
}

void GroupMgr::FreeGroupDbStoreId(Group* group)
{
    uint32 storageId = group->GetDbStoreId();

    if (storageId < NextGroupDbStoreId)
        NextGroupDbStoreId = storageId;

    GroupDbStore[storageId] = nullptr;
}

Group* GroupMgr::GetGroupByDbStoreId(uint32 storageId) const
{
    if (storageId < GroupDbStore.size())
        return GroupDbStore[storageId];

    return nullptr;
}

GroupMgr* GroupMgr::instance()
{
    static GroupMgr instance;
    return &instance;
}

void GroupMgr::InitGroupIds()
{
    _nextGroupId = 1;

    QueryResult result = CharacterDatabase.Query("SELECT MAX(guid) FROM `groups`");
    if (result)
    {
        uint32 maxId = (*result)[0].Get<uint32>();
        _groupIds.resize(maxId + 1);
    }
}

void GroupMgr::RegisterGroupId(uint32 groupId)
{
    // Allocation was done in InitGroupIds()
    _groupIds[groupId] = true;

    // Groups are pulled in ascending order from db and _nextGroupId is initialized with 1,
    // so if the instance id is used, increment
    if (_nextGroupId == groupId)
        ++_nextGroupId;
}

ObjectGuid::LowType GroupMgr::GenerateGroupId()
{
    ObjectGuid::LowType newGroupId = _nextGroupId;

    // find the lowest available id starting from the current _nextGroupId
    while (_nextGroupId < 0xFFFFFFFF && ++_nextGroupId < _groupIds.size() && _groupIds[_nextGroupId]);

    if (_nextGroupId == 0xFFFFFFFF)
    {
        LOG_ERROR("server.worldserver", "Group ID overflow!! Can't continue, shutting down server.");
        World::StopNow(ERROR_EXIT_CODE);
    }

    return newGroupId;
}

Group* GroupMgr::GetGroupByGUID(ObjectGuid::LowType groupId) const
{
    GroupContainer::const_iterator itr = GroupStore.find(groupId);
    if (itr != GroupStore.end())
        return itr->second;

    return nullptr;
}

void GroupMgr::AddGroup(Group* group)
{
    GroupStore[group->GetGUID().GetCounter()] = group;
}

void GroupMgr::RemoveGroup(Group* group)
{
    GroupStore.erase(group->GetGUID().GetCounter());
}

void GroupMgr::LoadGroups()
{
    {
        uint32 oldMSTime = getMSTime();

        // Delete all groups whose leader does not exist
        CharacterDatabase.DirectExecute("DELETE FROM `groups` WHERE leaderGuid NOT IN (SELECT guid FROM characters)");

        // Delete all groups with less than 2 members
        CharacterDatabase.DirectExecute("DELETE FROM `groups` WHERE guid NOT IN (SELECT guid FROM group_member GROUP BY guid HAVING COUNT(guid) > 1)");

        //                                                        0              1           2             3                 4      5          6      7         8       9
        QueryResult result = CharacterDatabase.Query("SELECT g.leaderGuid, g.lootMethod, g.looterGuid, g.lootThreshold, g.icon1, g.icon2, g.icon3, g.icon4, g.icon5, g.icon6"
                             //  10         11          12         13              14                  15            16        17          18
                             ", g.icon7, g.icon8, g.groupType, g.difficulty, g.raidDifficulty, g.masterLooterGuid, g.guid, lfg.dungeon, lfg.state FROM `groups` g LEFT JOIN lfg_data lfg ON lfg.guid = g.guid ORDER BY g.guid ASC");

        if (!result)
        {
            LOG_WARN("server.loading", ">> Loaded 0 group definitions. DB table `groups` is empty!");
            LOG_INFO("server.loading", " ");
        }
        else
        {
            uint32 count = 0;
            do
            {
                Field* fields = result->Fetch();
                Group* group = new Group;
                group->LoadGroupFromDB(fields);
                AddGroup(group);

                // Get the ID used for storing the group in the database and register it in the pool.
                uint32 storageId = group->GetDbStoreId();

                RegisterGroupDbStoreId(storageId, group);
                if (storageId == NextGroupDbStoreId)
                    NextGroupDbStoreId++;

                ++count;
            } while (result->NextRow());

            LOG_INFO("server.loading", ">> Loaded {} group definitions in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
            LOG_INFO("server.loading", " ");
        }
    }

    LOG_INFO("server.loading", "Loading Group Members...");
    {
        uint32 oldMSTime = getMSTime();

        // Delete all rows from group_member with no group
        CharacterDatabase.DirectExecute("DELETE FROM group_member WHERE guid NOT IN (SELECT guid FROM `groups`)");
        CharacterDatabase.DirectExecute("DELETE FROM group_instance WHERE guid NOT IN (SELECT guid FROM `groups`)");
        // Delete all members that does not exist
        CharacterDatabase.DirectExecute("DELETE FROM group_member WHERE memberGuid NOT IN (SELECT guid FROM characters)");

        //                                                    0        1           2            3       4
        QueryResult result = CharacterDatabase.Query("SELECT guid, memberGuid, memberFlags, subgroup, roles FROM group_member ORDER BY guid");
        if (!result)
        {
            LOG_WARN("server.loading", ">> Loaded 0 group members. DB table `group_member` is empty!");
            LOG_INFO("server.loading", " ");
        }
        else
        {
            uint32 count = 0;
            do
            {
                Field* fields = result->Fetch();
                Group* group = GetGroupByDbStoreId(fields[0].Get<uint32>());

                if (group)
                    group->LoadMemberFromDB(fields[1].Get<uint32>(), fields[2].Get<uint8>(), fields[3].Get<uint8>(), fields[4].Get<uint8>());
                else
                    LOG_ERROR("misc", "GroupMgr::LoadGroups: Consistency failed, can't find group (storage id: {})", fields[0].Get<uint32>());

                ++count;
            } while (result->NextRow());

            LOG_INFO("server.loading", ">> Loaded {} group members in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
            LOG_INFO("server.loading", " ");
        }
    }

    LOG_INFO("server.loading", "Loading Group instance saves...");
    {
        uint32 oldMSTime = getMSTime();

        //                                                   0        1      2            3             4             5            6
        QueryResult result = CharacterDatabase.Query("SELECT gi.guid, i.map, gi.instance, gi.permanent, i.difficulty, i.resettime, i.entranceId, "
            "(select COUNT(1) FROM `character_instance` ci LEFT JOIN `groups` g ON ci.guid = g.leaderGuid WHERE ci.`instance` = gi.`instance` AND ci.`permanent` = 1 LIMIT 1)"
            "FROM group_instance gi LEFT JOIN instance i ON gi.instance = i.id ORDER BY guid");

        if (!result)
        {
            LOG_INFO("server.loading", ">> Loaded 0 group-instance saves. DB table `group_instance` is empty!");
            return;
        }

        uint32 count = 0;
        do
        {
            Field* fields = result->Fetch();
            Group* group = GetGroupByDbStoreId(fields[0].Get<uint32>());
            // group will never be NULL (we have run consistency sql's before loading)

            MapEntry const* mapEntry = sMapStore.LookupEntry(fields[1].Get<uint16>());
            if (!mapEntry || !mapEntry->IsDungeon())
            {
                LOG_INFO("sql.sql", "Incorrect entry in group_instance table : no dungeon map %d", fields[1].Get<uint16>());
                continue;
            }

            uint32 diff = fields[4].Get<uint8>();
            auto difficultyEntry = GetMapDifficultyData(mapEntry->MapID, Difficulty(diff));
            if (!difficultyEntry)
                continue;

            InstanceSave* save = sInstanceSaveMgr->AddInstanceSave(mapEntry->MapID, fields[2].Get<uint32>(), Difficulty(diff), time_t(fields[5].Get<uint32>()), fields[6].Get<uint32>(), fields[7].Get<uint64>() == 0, true);
            group->BindToInstance(save, fields[3].Get<bool>(), true);
            ++count;
        } while (result->NextRow());

        LOG_INFO("server.loading", ">> Loaded %u group-instance saves in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
    }
}
