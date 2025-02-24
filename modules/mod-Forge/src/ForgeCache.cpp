#pragma once
#define OUT

#include <string>
#include "AreaTriggerDataStore.h"
#include "ScriptMgr.h"
#include "Player.h"
#include "Config.h"
#include "Chat.h"
#include "WorldPacket.h"
#include "DatabaseEnv.h"
#include "SharedDefines.h"
#include "Gamemode.h"
#include <unordered_map>
#include <list>
#include <tuple>

enum CharacterPointType
{
    TALENT_TREE = 0,
    FORGE_SKILL_TREE = 1,
    PRESTIGE_TREE = 3,
    RACIAL_TREE = 4,
    SKILL_PAGE = 5,
    PRESTIGE_COUNT = 6,
    CLASS_TREE = 7,
    PET_TALENT = 8,
};

enum NodeType
{
    AURA = 0,
    SPELL = 1,
    CHOICE = 2
};

enum ForgeSettingIndex
{
    FORGE_SETTING_SPEC_SLOTS = 0
};

#define FORGE_SETTINGS "ForgeSettings"
#define ACCOUNT_WIDE_TYPE CharacterPointType::PRESTIGE_TREE
#define ACCOUNT_WIDE_KEY 0xfffffffe

enum class SpecVisibility
{
    PRIVATE = 0,
    FRIENDS = 1,
    GUILD = 2,
    PUBLIC = 3
};

enum class PereqReqirementType
{
    ALL = 0,
    ONE = 1
};

struct ForgeCharacterPoint
{
    CharacterPointType PointType;
    uint32 SpecId;
    uint32 Sum;
    uint32 Max;
};

struct ClassSpecDetail
{
    std::string Name;
    uint32 SpellIconId;
    uint32 SpecId;
};

struct ForgeCharacterTalent
{
    uint32 SpellId;
    uint32 TabId;
    uint8 CurrentRank;
    uint8 type;
};

struct ForgeTalentPrereq
{
    uint32 reqId;
    uint32 Talent;
    uint32 TalentTabId;
    uint32 RequiredRank;
};

struct ForgeCharacterSpec
{
    uint32 Id;
    ObjectGuid CharacterGuid;
    std::string Name;
    std::string Description;
    bool Active;
    uint32 SpellIconId;
    SpecVisibility Visability;
    uint32 CharacterSpecTabId; // like holy ret pro
    // TabId, Spellid
    std::unordered_map<uint32, std::unordered_map<uint32, ForgeCharacterTalent*>> Talents;
    // tabId
    std::unordered_map<uint32, uint8> PointsSpent;
    std::unordered_map<uint32 /*node id*/, uint32/*spell picked*/> ChoiceNodesChosen;
};

struct ForgeTalentChoice
{
    uint32 spellId;
    bool active;
};

struct ForgeTalent
{
    uint32 SpellId;
    uint32 TalentTabId;
    uint32 ColumnIndex;
    uint32 RowIndex;
    uint8 RankCost;
    uint16 TabPointReq;
    uint8 RequiredLevel;
    CharacterPointType TalentType;
    NodeType nodeType;
    uint8 nodeIndex;

    uint8 NumberOfRanks;
    PereqReqirementType PreReqType;
    std::list<ForgeTalentPrereq*> Prereqs;
    std::map<uint8 /*index*/, ForgeTalentChoice*> Choices;
    std::list<uint32> UnlearnSpells;
    // rank number, spellId
    std::unordered_map<uint32, uint32> Ranks;
    // SpellId, Rank Number
    std::unordered_map<uint32, uint32> RanksRev; // only in memory
};

struct ForgeTalentTab
{
    uint32 Id;
    uint32 ClassMask;
    uint32 RaceMask;
    std::string Name;
    uint32 SpellIconId;
    std::string Background;
    std::string Description;
    uint8 Role;
    std::string SpellString;
    CharacterPointType TalentType;
    uint32 TabIndex;
    std::unordered_map<uint32 /*spellId*/, ForgeTalent*> Talents;
};

// XMOG
struct ForgeCharacterXmog
{
    std::string name;
    std::unordered_map<uint8, uint32> slottedItems;
};

// hater: custom stats
#define MAINSTATS 3
#define TANKDIST 0

class ForgeCache : public DatabaseScript
{
public:
    

    static ForgeCache *instance()
    {
        static ForgeCache* cache;

        if (cache == nullptr)
            cache = new ForgeCache();

        return cache;
    }

    ForgeCache() : DatabaseScript("ForgeCache")
    {
        RACE_LIST =
        {
            RACE_HUMAN,
            RACE_ORC,
            RACE_DWARF,
            RACE_NIGHTELF,
            RACE_UNDEAD_PLAYER,
            RACE_TAUREN,
            RACE_GNOME,
            RACE_TROLL,
            RACE_BLOODELF,
            RACE_DRAENEI
        };

        CLASS_LIST =
        {
            CLASS_WARRIOR,
            CLASS_PALADIN,
            CLASS_HUNTER,
            CLASS_ROGUE,
            CLASS_PRIEST,
            CLASS_DEATH_KNIGHT,
            CLASS_SHAMAN,
            CLASS_MAGE,
            CLASS_WARLOCK,
            CLASS_DRUID
        };

        TALENT_POINT_TYPES =
        {
            CharacterPointType::TALENT_TREE,
            CharacterPointType::CLASS_TREE
        };
    }

    bool IsDatabaseBound() const override
    {
        return true;
    }

    void OnAfterDatabasesLoaded(uint32 updateFlags) override
    {
        BuildForgeCache();
    }

    bool TryGetTabIdForSpell(Player* player, uint32 spellId, OUT uint32& tabId)
    {
        auto tabItt = SpellToTalentTabMap.find(spellId);

        if (tabItt == SpellToTalentTabMap.end())
            return false;

        tabId = tabItt->second;
        return true;
    }

    bool TryGetSpellIddForTab(Player* player, uint32 tabId, OUT uint32& skillId)
    {
        auto tabItt = TalentTabToSpellMap.find(tabId);

        if (tabItt == TalentTabToSpellMap.end())
            return false;

        skillId = tabItt->second;
        return true;
    }

    bool TryGetCharacterTalents(Player* player, uint32 tabId, OUT std::unordered_map<uint32, ForgeCharacterTalent*>& spec)
    {
        ForgeCharacterSpec* charSpec;

        if (!TryGetCharacterActiveSpec(player, charSpec))
            return false;

        auto tabItt = charSpec->Talents.find(tabId);

        if (tabItt == charSpec->Talents.end())
            return false;

        spec = tabItt->second;
        return true;
    }

    bool TryGetAllCharacterSpecs(Player* player, OUT std::list<ForgeCharacterSpec*>& specs)
    {
        auto charSpecItt = CharacterSpecs.find(player->GetGUID());

        if (charSpecItt == CharacterSpecs.end())
            return false;

        for (auto& specKvp : charSpecItt->second)
            specs.push_back(specKvp.second);

        return true;
    }

    bool TryGetCharacterActiveSpec(Player* player, OUT ForgeCharacterSpec*& spec)
    {
        auto cas = CharacterActiveSpecs.find(player->GetGUID());

        if (cas == CharacterActiveSpecs.end())
            return false;

        return TryGetCharacterSpec(player, cas->second, spec);
    }

    bool TryGetCharacterSpec(Player* player, uint32 specId, OUT ForgeCharacterSpec*& spec)
    {
        auto charSpecItt = CharacterSpecs.find(player->GetGUID());

        if (charSpecItt == CharacterSpecs.end())
            return false;

        auto sp = charSpecItt->second.find(specId);

        if (sp == charSpecItt->second.end())
            return false;

        spec = sp->second;
        return true;
    }

    ForgeCharacterTalent* GetTalent(Player* player, uint32 spellId)
    {
        auto tabItt = SpellToTalentTabMap.find(spellId);

        if (tabItt == SpellToTalentTabMap.end())
            return nullptr;

        auto* tab = TalentTabs[tabItt->second];

        ForgeCharacterSpec* spec;
        if (TryGetCharacterActiveSpec(player, spec))
        {
            auto talTabItt = spec->Talents.find(tab->Id);

            if (talTabItt == spec->Talents.end())
                return nullptr;

            auto spellItt = talTabItt->second.find(spellId);

            if (spellItt == talTabItt->second.end())
                return nullptr;

            return spellItt->second;
        }

        return nullptr;
    }

    ForgeCharacterPoint* GetSpecPoints(Player* player, CharacterPointType pointType)
    {
        ForgeCharacterSpec* spec;

        if (TryGetCharacterActiveSpec(player, spec))
        {
            return GetSpecPoints(player, pointType, spec->Id);
        }

        AddCharacterSpecSlot(player);

        return GetSpecPoints(player, pointType);
    }

    ForgeCharacterPoint* GetSpecPoints(Player* player, CharacterPointType pointType, uint32 specId)
    {
        if (ACCOUNT_WIDE_TYPE == pointType)
        {
            auto ptItt = AccountWidePoints.find(player->GetSession()->GetAccountId());

            if (ptItt != AccountWidePoints.end())
            {
                auto p = ptItt->second.find(pointType);

                if (p != ptItt->second.end())
                    return p->second;
            }

            return CreateAccountBoundCharPoint(player, pointType);
        }

        auto charGuid = player->GetGUID();
        auto cpItt = CharacterPoints.find(charGuid);

        if (cpItt != CharacterPoints.end())
        {
            auto ptItt = cpItt->second.find(pointType);

            if (ptItt != cpItt->second.end())
            {
                auto talItt = ptItt->second.find(specId);

                if (talItt != ptItt->second.end())
                    return talItt->second;
            }
        }

    
        ForgeCharacterPoint* fcp = new ForgeCharacterPoint();
        fcp->PointType = pointType;
        fcp->SpecId = specId;


        UpdateCharPoints(player, fcp);

        return fcp;
    }

    ForgeCharacterPoint* GetCommonCharacterPoint(Player* player, CharacterPointType pointType)
    {
        if (pointType != ACCOUNT_WIDE_TYPE)
            return GetSpecPoints(player, pointType, UINT_MAX);
        else
            return GetSpecPoints(player, pointType);
    }

    bool TryGetTabPointType(uint32 tabId, CharacterPointType& pointType)
    {
        auto fttItt = TalentTabs.find(tabId);

        if (fttItt == TalentTabs.end())
            return false;

        pointType = fttItt->second->TalentType;
        return true;
    }

    bool TryGetTalentTab(Player* player, uint32 tabId, OUT ForgeTalentTab*& tab)
    {
        auto charRaceItt = RaceAndClassTabMap.find(player->getRace());

        if (charRaceItt == RaceAndClassTabMap.end())
            return false;

        auto charClassItt = charRaceItt->second.find(player->getClass());

        if (charClassItt == charRaceItt->second.end())
            return false;

        // all logic before this is to ensure player has access to the tab.

        auto fttItt = charClassItt->second.find(tabId);

        if (fttItt == charClassItt->second.end())
            return false;

        tab = TalentTabs[tabId];
        return true;
    }

    bool TryGetForgeTalentTabs(Player* player, CharacterPointType cpt, OUT std::list<ForgeTalentTab*>& talentTabs)
    {
        auto race = player->getRace();
        auto pClass = player->getClass();

        auto charRaceItt = RaceAndClassTabMap.find(race);

        if (charRaceItt == RaceAndClassTabMap.end())
            return false;

        auto charClassItt = charRaceItt->second.find(pClass);

        if (charClassItt == charRaceItt->second.end())
            return false;

        auto ptItt = CharacterPointTypeToTalentTabIds.find(cpt);

        if (ptItt == CharacterPointTypeToTalentTabIds.end())
            return false;

        for (auto iter : charClassItt->second)
        {
            if (ptItt->second.find(iter) != ptItt->second.end())
                talentTabs.push_back(TalentTabs[iter]);
        }

        return true;
    }


    void AddCharacterSpecSlot(Player* player)
    {
        uint32 act = player->GetSession()->GetAccountId();
        uint8 num = player->GetSpecsCount();

        num = num + 1;
        player->UpdatePlayerSetting(FORGE_SETTINGS, FORGE_SETTING_SPEC_SLOTS, num);

        ForgeCharacterSpec* spec = new ForgeCharacterSpec();
        spec->Id = num;
        spec->Active = num == 1;
        spec->CharacterGuid = player->GetGUID();
        spec->Name = "Specialization " + std::to_string(num);
        spec->Description = "Skill Specilization";
        spec->Visability = SpecVisibility::PRIVATE;
        spec->SpellIconId = 133743;
        spec->CharacterSpecTabId = _playerClassFirstSpec[player->getClassMask()];

        player->SetSpecsCount(num);

        if (spec->Active)
            player->ActivateSpec(num);

        auto actItt = AccountWideCharacterSpecs.find(act);

        if (actItt != AccountWideCharacterSpecs.end())
        {
            for (auto& talent : actItt->second->Talents)
                for (auto& tal : talent.second)
                    spec->Talents[talent.first][tal.first] = tal.second;


            for (auto& pointsSpent : actItt->second->PointsSpent)
                spec->PointsSpent[pointsSpent.first] = pointsSpent.second;
        }

        std::list<ForgeTalentTab*> tabs;
        if (TryGetForgeTalentTabs(player, CharacterPointType::TALENT_TREE, tabs)) {
            for (auto tab : tabs) {
                for (auto talent : tab->Talents) {
                    ForgeCharacterTalent* ct = new ForgeCharacterTalent();
                    ct->CurrentRank = 0;
                    ct->SpellId = talent.second->SpellId;
                    ct->TabId = tab->Id;
                    ct->type = talent.second->nodeType;

                    spec->Talents[tab->Id][ct->SpellId] = ct;
                }
            }
        }
        if (TryGetForgeTalentTabs(player, CharacterPointType::CLASS_TREE, tabs)) {
            for (auto tab : tabs) {
                for (auto talent : tab->Talents) {
                    ForgeCharacterTalent* ct = new ForgeCharacterTalent();
                    ct->CurrentRank = 0;
                    ct->SpellId = talent.second->SpellId;
                    ct->TabId = tab->Id;
                    ct->type = talent.second->nodeType;

                    spec->Talents[tab->Id][ct->SpellId] = ct;
                }
            }
        }

        for (auto pt : TALENT_POINT_TYPES)
        {
            if (ACCOUNT_WIDE_TYPE == pt)
            {
                if (actItt == AccountWideCharacterSpecs.end() || AccountWidePoints[act].find(pt) == AccountWidePoints[act].end())
                    CreateAccountBoundCharPoint(player, pt);

                continue;
            }

            ForgeCharacterPoint* fpt = GetCommonCharacterPoint(player, pt);
            ForgeCharacterPoint* maxCp = GetMaxPointDefaults(pt);

            ForgeCharacterPoint* newFp = new ForgeCharacterPoint();
            newFp->Max = maxCp->Max;
            newFp->PointType = pt;
            newFp->SpecId = spec->Id;
            newFp->Sum = fpt->Sum;

            UpdateCharPoints(player, newFp);
        }

        UpdateCharacterSpec(player, spec);
    }

    void ReloadDB()
    {
        BuildForgeCache();
    }

    void UpdateCharPoints(Player* player, ForgeCharacterPoint*& fp)
    {
        auto charGuid = player->GetGUID();
        auto acct = player->GetSession()->GetAccountId();
        bool isNotAccountWide = ACCOUNT_WIDE_TYPE != fp->PointType;

        if (isNotAccountWide)
            CharacterPoints[charGuid][fp->PointType][fp->SpecId] = fp;
        else
        {
            AccountWidePoints[acct][fp->PointType] = fp;

            auto pItt = PlayerCharacterMap.find(acct);

            if (pItt != PlayerCharacterMap.end())
                for (auto& ch : pItt->second)
                    for (auto& spec : CharacterSpecs[ch])
                        CharacterPoints[ch][fp->PointType][spec.first] = fp;
        }

        auto trans = CharacterDatabase.BeginTransaction();

        if (isNotAccountWide)
            trans->Append("INSERT INTO `forge_character_points` (`guid`,`type`,`spec`,`sum`,`max`) VALUES ({},{},{},{},{}) ON DUPLICATE KEY UPDATE `sum` = {}, `max` = {}", charGuid.GetCounter(), (int)fp->PointType, fp->SpecId, fp->Sum, fp->Max, fp->Sum, fp->Max);
        else
            trans->Append("INSERT INTO `forge_character_points` (`guid`,`type`,`spec`,`sum`,`max`) VALUES ({},{},{},{},{}) ON DUPLICATE KEY UPDATE `sum` = {}, `max` = {}", acct, (int)fp->PointType, ACCOUNT_WIDE_KEY, fp->Sum, fp->Max, fp->Sum, fp->Max);

        CharacterDatabase.CommitTransaction(trans);
    }

    void UpdateCharacterSpec(Player* player, ForgeCharacterSpec* spec)
    {
        uint32 charId = player->GetGUID().GetCounter();
        uint32 acct = player->GetSession()->GetAccountId();

        auto trans = CharacterDatabase.BeginTransaction();

        if (spec->Active)
            CharacterActiveSpecs[player->GetGUID()] = spec->Id;

        UpdateForgeSpecInternal(player, trans, spec);

        for (auto& tabIdKvp : spec->Talents)
            for (auto& tabTypeKvp : tabIdKvp.second)
                UpdateCharacterTalentInternal(acct, charId, trans, spec->Id, tabTypeKvp.second->SpellId, tabTypeKvp.second->TabId, tabTypeKvp.second->CurrentRank);

        CharacterDatabase.CommitTransaction(trans);
    }

    void UpdateCharacterSpecDetailsOnly(Player* player, ForgeCharacterSpec*& spec)
    {
        uint32 charId = player->GetGUID().GetCounter();
        auto trans = CharacterDatabase.BeginTransaction();
        UpdateForgeSpecInternal(player, trans, spec);

        CharacterDatabase.CommitTransaction(trans);
    }

    void ApplyAccountBoundTalents(Player* player)
    {
        ForgeCharacterSpec* currentSpec;

        if (TryGetCharacterActiveSpec(player, currentSpec))
        {
            uint32 playerLevel = player->getLevel();
            auto modes = Gamemode::GetGameMode(player);

            for (auto charTabType : TALENT_POINT_TYPES)
            {
                if (ACCOUNT_WIDE_TYPE != charTabType)
                    continue;

                std::list<ForgeTalentTab*> tabs;
                if (TryGetForgeTalentTabs(player, charTabType, tabs))
                    for (auto* tab : tabs)
                    {
                        auto talItt = currentSpec->Talents.find(tab->Id);

                        for (auto spell : tab->Talents)
                        {

                            if (playerLevel != 80 && modes.size() == 0 && talItt != currentSpec->Talents.end())
                            {
                                auto spellItt = talItt->second.find(spell.first);

                                if (spellItt != talItt->second.end())
                                {
                                    uint32 currentRank = spell.second->Ranks[spellItt->second->CurrentRank];

                                    for (auto rank : spell.second->Ranks)
                                        if (currentRank != rank.second)
                                            player->removeSpell(rank.second, SPEC_MASK_ALL, false);

                                    if (!player->HasSpell(currentRank))
                                        player->learnSpell(currentRank, false, false);
                                }
                            }
                        }
                    }
            }

            player->SendInitialSpells();
        }
    }

    ForgeCharacterPoint* GetMaxPointDefaults(CharacterPointType cpt)
    {
        auto fpd = MaxPointDefaults.find(cpt);

        // Get default skill max and current. Happens at level 10. Players start with 10 forge points. can be changed in DB with UINT_MAX entry.
        if (fpd == MaxPointDefaults.end())
        {
            ForgeCharacterPoint* fp = new ForgeCharacterPoint();
            fp->PointType = cpt;
            fp->SpecId = UINT_MAX;
            fp->Max = 0;

            if (cpt == CharacterPointType::FORGE_SKILL_TREE)
                fp->Sum = GetConfig("level10ForgePoints", 30);
            else
                fp->Sum = 0;

            return fp;
        }
        else
            return fpd->second;
    }

    void AddCharacterPointsToAllSpecs(Player* player, CharacterPointType pointType,  uint32 amount)
    {
        ForgeCharacterPoint* m = GetMaxPointDefaults(pointType);
        ForgeCharacterPoint* ccp = GetCommonCharacterPoint(player, pointType);

        if (m->Max > 0)
        {
            auto maxPoints = amount + ccp->Sum;

            if (maxPoints >= m->Max)
            {
                maxPoints = maxPoints - m->Max;
                amount = amount - maxPoints;
            }
        }

        if (amount > 0)
        {
            ccp->Sum += amount;
            UpdateCharPoints(player, ccp);

            if (pointType != ACCOUNT_WIDE_TYPE)
                for (auto& spec : CharacterSpecs[player->GetGUID()])
                {
                    ForgeCharacterPoint* cp = GetSpecPoints(player, pointType, spec.first);
                    cp->Sum += amount;
                    UpdateCharPoints(player, cp);
                }

            ChatHandler(player->GetSession()).SendSysMessage("|cff8FCE00You have been awarded " + std::to_string(amount) + " " + GetpointTypeName(pointType) + " point(s).");
        }
    }

    std::string GetpointTypeName(CharacterPointType t)
    {
        switch (t)
        {
        case CharacterPointType::PRESTIGE_TREE:
            return "Prestige";
        case CharacterPointType::TALENT_TREE:
            return "Talent";
        case CharacterPointType::RACIAL_TREE:
            return "Racial";
        case CharacterPointType::FORGE_SKILL_TREE:
            return "Forge";
        default:
            return "";
        }
    }

    bool TryGetClassSpecilizations(Player* player, OUT std::vector<ClassSpecDetail*>& list)
    {
        auto raceItt = ClassSpecDetails.find(player->getRace());


        if (raceItt == ClassSpecDetails.end())
            return false;

        auto classItt = raceItt->second.find(player->getClass());

        if (classItt == raceItt->second.end())
            return false;

        list = classItt->second;
        return true;
    }

    void DeleteCharacter(ObjectGuid guid, uint32 accountId)
    {
        if (GetConfig("PermaDeleteForgecharacter", 0) == 1)
        {
            auto trans = CharacterDatabase.BeginTransaction();
            trans->Append("DELETE FROM forge_character_specs WHERE guid = {}", guid.GetCounter());
            trans->Append("DELETE FROM character_modes WHERE guid = {}", guid.GetCounter());
            trans->Append("DELETE FROM forge_character_points WHERE guid = {} AND spec != {}", guid.GetCounter(), ACCOUNT_WIDE_KEY);
            trans->Append("DELETE FROM forge_character_talents WHERE guid = {} AND spec != {}", guid.GetCounter(), ACCOUNT_WIDE_KEY);
            trans->Append("DELETE FROM forge_character_talents_spent WHERE guid = {} AND spec != {}", guid.GetCounter(), ACCOUNT_WIDE_KEY);
            CharacterDatabase.CommitTransaction(trans);
        }
    }

    uint32 GetConfig(std::string key, uint32 defaultValue)
    {
        auto valItt = CONFIG.find(key);

        if (valItt == CONFIG.end())
        {
            auto trans = WorldDatabase.BeginTransaction();
            trans->Append("INSERT INTO forge_config (`cfgName`,`cfgValue`) VALUES ('{}',{})", key, defaultValue);
            WorldDatabase.CommitTransaction(trans);

            CONFIG[key] = defaultValue;
            return defaultValue;
        }

        return valItt->second;
    }


    bool isNumber(const std::string& s)
    {
        return std::ranges::all_of(s.begin(), s.end(), [](char c) { return isdigit(c) != 0; });
    }

    void UpdateCharacters(uint32 account, Player* player)
    {
        if (PlayerCharacterMap.find(account) != PlayerCharacterMap.end())
            PlayerCharacterMap.erase(account);

        QueryResult query = CharacterDatabase.Query("SELECT guid, account FROM characters where account = {}", account);

        if (!query)
            return;

        do
        {
            Field* field = query->Fetch();
            PlayerCharacterMap[field[1].Get<uint32>()].push_back(ObjectGuid::Create<HighGuid::Player>(field[0].Get<uint32>()));

        } while (query->NextRow());
    }

    ForgeCharacterPoint* CreateAccountBoundCharPoint(Player* player, const CharacterPointType& pt)
    {
        ForgeCharacterPoint* maxCp = GetMaxPointDefaults(pt);

        ForgeCharacterPoint* newFp = new ForgeCharacterPoint();
        newFp->Max = maxCp->Max;
        newFp->PointType = pt;
        newFp->SpecId = ACCOUNT_WIDE_KEY;
        newFp->Sum = 0;

        UpdateCharPoints(player, newFp);
        return newFp;
    }


    std::string BuildXmogSetsMsg(Player* player) {
        std::string out = "";

        auto sets = XmogSets.find(player->GetGUID().GetCounter());
        if (sets != XmogSets.end())
            for (auto set : sets->second)
                out += std::to_string(set.first) + "^" + set.second->name + ";";
        else
            out += "empty";

        return out;
    }

    void SaveXmogSet(Player* player, uint32 setId) {
        auto sets = XmogSets.find(player->GetGUID().GetCounter());
        if (sets != XmogSets.end()) {
            auto set = sets->second.find(setId);
            if (set != sets->second.end()) {
                for (int i : xmogSlots)
                    if (auto item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                        set->second->slottedItems[i] = item->GetTransmog();
                    else
                        set->second->slottedItems[i] = 0;

                SaveXmogSetInternal(player, setId, set->second);
            }
        }
    }

    void AddXmogSet(Player* player, uint32 setId, std::string name) {
        ForgeCharacterXmog* xmog = new ForgeCharacterXmog();
        xmog->name = name;

        auto newSetId = FirstOpenXmogSlot(player);

        for (int i : xmogSlots)
            if (auto item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                xmog->slottedItems[i] = item->GetTransmog();
            else
                xmog->slottedItems[i] = 0;

        XmogSets[player->GetGUID().GetCounter()][newSetId] = xmog;
        SaveXmogSetInternal(player, newSetId, xmog);
    }

    std::string BuildXmogFromEquipped(Player* player) {
        std::string out = "noname^";
        for (auto slot : xmogSlots) {
            auto item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
            out += item == nullptr ? "0^" : std::to_string(item->GetTransmog()) + "^";
        }
        return out + ";";
    }

    std::string BuildXmogFromSet(Player* player, uint8 setId) {
        std::string out = "";
        auto sets = XmogSets.find(player->GetGUID().GetCounter());

        if (sets != XmogSets.end()) {
            auto set = sets->second.find(setId);
            if (set != sets->second.end()) {
                out += set->second->name + "^";
                for (auto slot : xmogSlots) {
                    out += std::to_string(set->second->slottedItems[slot]) + "^";
                }
                out += ";";

                return out;
            }
        }

        return BuildXmogFromEquipped(player);
    }

    void UnlearnFlaggedSpells(Player* player) {
        auto found = FlaggedForUnlearn.find(player->GetGUID().GetCounter());
        if (found != FlaggedForUnlearn.end()) {
            if (player->HasSpell(found->second))
                player->postCheckRemoveSpell(found->second);
        }
    }

    void LearnExtraSpellsIfAny(Player* player, uint32 spell) {
        auto found = SpellLearnedAdditionalSpells.find(spell);
        if (found != SpellLearnedAdditionalSpells.end()) {
            for (auto spell : found->second) {
                if (!player->HasSpell(spell))
                    player->learnSpell(spell);
            }
        }
    }

    void LoadLoadoutActions(Player* player)
    {
        ForgeCharacterSpec* spec;
        if (TryGetCharacterActiveSpec(player, spec)) {
            auto foundActions = _playerActiveTalentLoadouts.find(player->GetGUID().GetCounter());
            if (foundActions != _playerActiveTalentLoadouts.end()) {
                // SELECT button, action, type FROM forge_character_action WHERE guid = ? AND spec = ? and loadout = ? ORDER BY button;
                CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_ACTIONS_SPEC_LOADOUT);
                stmt->SetData(0, player->GetGUID().GetCounter());
                stmt->SetData(1, spec->CharacterSpecTabId);
                stmt->SetData(2, foundActions->second->id);

                WorldSession* mySess = player->GetSession();
                mySess->GetQueryProcessor().AddCallback(CharacterDatabase.AsyncQuery(stmt)
                    .WithPreparedCallback([mySess](PreparedQueryResult result)
                        {
                            // safe callback, we can't pass this pointer directly
                            // in case player logs out before db response (player would be deleted in that case)
                            if (Player* thisPlayer = mySess->GetPlayer())
                                thisPlayer->LoadActions(result);
                        }));
            }
        }
    }

    std::vector<uint32> RACE_LIST;
    std::vector<uint32> CLASS_LIST;
    std::vector<CharacterPointType> TALENT_POINT_TYPES;

    // tabId
    std::unordered_map<uint32, ForgeTalentTab*> TalentTabs;

    // choiceNodeId is the id of the node in forge_talents
    std::unordered_map<uint32 /*nodeid*/, std::vector<uint32/*choice spell id*/>> _choiceNodes;
    std::unordered_map<uint32 /*choice spell id*/, uint32 /*nodeid*/> _choiceNodesRev;
    std::unordered_map<uint8, uint32> _choiceNodeIndexLookup;

    uint32 GetChoiceNodeFromindex(uint8 index) {
        auto out = _choiceNodeIndexLookup.find(index);
        if (out != _choiceNodeIndexLookup.end())
            return out->second;

        return 0;
    }

    std::unordered_map<uint8 /*class*/, std::unordered_map<uint32 /*racemask*/, std::unordered_map<uint8/*level*/, std::vector<uint32 /*spell*/>>>> _levelClassSpellMap;

    /* hater: cached tree meta data */
    struct NodeMetaData {
        uint32 spellId;
        uint8 row;
        uint8 col;
        uint8 pointReq;
        uint8 nodeIndex;
        std::vector<NodeMetaData*> unlocks;
    };
    struct TreeMetaData {
        uint32 TabId;
        uint8 MaxXDim = 0;
        uint8 MaxYDim = 0;
        std::unordered_map<uint8/*row*/, std::unordered_map<uint8 /*col*/, NodeMetaData*>> nodes;
        std::unordered_map<uint32/*spellId*/, NodeMetaData*> nodeLocation;
    };
    std::unordered_map<uint32 /*tabId*/, TreeMetaData*> _cacheTreeMetaData;

    void ForgetTalents(Player* player, ForgeCharacterSpec* spec, CharacterPointType pointType) {
        std::list<ForgeTalentTab*> tabs;
        if (TryGetForgeTalentTabs(player, pointType, tabs))
            for (auto* tab : tabs) {
                for (auto spell : tab->Talents) {
                    if (spell.second->nodeType == NodeType::CHOICE) {
                        for (auto choice : _choiceNodesRev)
                            if (player->HasSpell(choice.first))
                                player->removeSpell(choice.first, SPEC_MASK_ALL, false);
                    }
                    else
                        for (auto rank : spell.second->Ranks)
                            if (player->HasSpell(rank.second))
                                if (auto spellInfo = sSpellMgr->GetSpellInfo(rank.second)) {
                                    if (spellInfo->HasEffect(SPELL_EFFECT_LEARN_SPELL))
                                        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                                            player->removeSpell(spellInfo->Effects[i].TriggerSpell, SPEC_MASK_ALL, false);

                                    player->removeSpell(rank.second, SPEC_MASK_ALL, false);
                                }

                    auto talent = spec->Talents[tab->Id].find(spell.first);
                    if (talent != spec->Talents[tab->Id].end())
                        talent->second->CurrentRank = 0;
                }
                spec->PointsSpent[tab->Id] = 0;
            }
        ForgeCharacterPoint* fcp = GetSpecPoints(player, pointType, spec->Id);

        auto amount = 0;
        auto level = player->getLevel();
        if (level >= 10)
            level -= 9;

        switch (pointType) {
        case CharacterPointType::TALENT_TREE:
            if (level > 1) {
               int div = level / 2;
               amount = div;
            }
            else
                if (level % 2)
                    amount = 1;
            break;
        case CharacterPointType::CLASS_TREE:
            if (level > 1) {
                int rem = level % 2;
                int div = level / 2;
                if (rem)
                    div += 1;

                amount = div;
            }
            break;
        default:
            break;
        }

        fcp->Max = amount;
        fcp->Sum = amount;

        UpdateCharPoints(player, fcp);
        if (pointType == CharacterPointType::TALENT_TREE)
            return ForgetTalents(player, spec, CharacterPointType::CLASS_TREE);
    }

    std::unordered_map<uint32 /*tabid*/, std::unordered_map<uint8 /*node*/, uint32 /*spell*/>> _cacheSpecNodeToSpell;
    std::unordered_map<uint32 /*class*/, std::unordered_map<uint8 /*node*/, uint32 /*spell*/>> _cacheClassNodeToSpell;
    std::unordered_map<uint32, uint32> _cacheClassNodeToClassTree;

    const std::string base64_char = "|ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; // | is to offset string to base 1

    const uint8 LOADOUT_NAME_MAX = 64;
    const uint8 META_PREFIX = 3;
    const uint8 MAX_LOADOUTS_PER_SPEC = 7;

    struct PlayerLoadout {
        bool active;
        uint8 id;
        uint32 tabId;
        std::string name;
        std::string talentString;
    };
    // hater: player loadout storage
    std::unordered_map<uint32 /*guid*/, std::unordered_map<uint32 /*tabId*/, std::unordered_map<uint8 /*id*/, PlayerLoadout*>>> _playerTalentLoadouts;
    std::unordered_map<uint32 /*guid*/,PlayerLoadout*> _playerActiveTalentLoadouts;

    std::unordered_map<uint32 /*class*/, uint32 /*spec*/> _playerClassFirstSpec;

    // hater: custom items
    std::unordered_map<InventoryType, float /*value*/> _forgeItemSlotValues;
    std::unordered_map<ItemModType, float /*value*/> _forgeItemStatValues;
    std::unordered_map<uint8, std::vector<ItemModType>> _forgeItemSecondaryStatPools;

    void AddDefaultLoadout(Player* player)
    {
        std::list<ForgeTalentTab*> tabs;
        if (TryGetForgeTalentTabs(player, CharacterPointType::TALENT_TREE, tabs)) {
            for (auto tab : tabs) {
                std::string loadout = "A";
                loadout += base64_char.substr(tab->Id, 1);
                loadout += base64_char.substr(player->getClass(), 1);

                auto classMap = _cacheClassNodeToSpell[player->getClassMask()];
                for (int i = 1; i <= classMap.size(); i++)
                    loadout += base64_char.substr(1, 1);

                auto specMap = _cacheSpecNodeToSpell[tab->Id];
                for (int i = 1; i <= specMap.size(); i++) {
                    loadout += base64_char.substr(1, 1);
                }

                auto guid = player->GetGUID().GetCounter();

                PlayerLoadout* plo = new PlayerLoadout();
                plo->active = true;
                plo->id = 1;
                plo->name = "Default";
                plo->tabId = tab->Id;
                plo->talentString = loadout;

                _playerTalentLoadouts[guid][tab->Id][plo->id] = plo;
                _playerActiveTalentLoadouts[guid] = plo;

                CharacterDatabase.Execute("insert into `forge_character_talent_loadouts` (`guid`, `id`, `talentTabId`, `name`, `talentString`, `active`) values ({}, {}, {}, '{}', '{}', {})",
                    guid, plo->id, tab->Id, plo->name, loadout, true);
            }
        }
    }

    void EchosDefaultLoadout(Player* player)
    {
        std::list<ForgeTalentTab*> tabs;
        std::string loadout = "AD";
        if (TryGetForgeTalentTabs(player, CharacterPointType::TALENT_TREE, tabs)) {
            loadout += base64_char.substr(player->getClass(), 1);
            for (auto tab : tabs) {
                auto specMap = _cacheSpecNodeToSpell[tab->Id];
                for (int i = 1; i <= specMap.size(); i++) {
                    loadout += base64_char.substr(1, 1);
                }
            }
            
            auto guid = player->GetGUID().GetCounter();

            PlayerLoadout* plo = new PlayerLoadout();
            plo->active = true;
            plo->id = 1;
            plo->name = "Default";
            plo->tabId = 1;
            plo->talentString = loadout;

            _playerTalentLoadouts[guid][1][plo->id] = plo;
            _playerActiveTalentLoadouts[guid] = plo;

            CharacterDatabase.Execute("insert into `forge_character_talent_loadouts` (`guid`, `id`, `talentTabId`, `name`, `talentString`, `active`) values ({}, {}, {}, '{}', '{}', {})",
                guid, plo->id, 1, plo->name, loadout, true);
        }
    }

private:
    std::unordered_map<ObjectGuid, uint32> CharacterActiveSpecs;
    std::unordered_map<std::string, uint32> CONFIG;

    // charId, specId
    std::unordered_map<ObjectGuid, std::unordered_map<uint32, ForgeCharacterSpec*>> CharacterSpecs;
    std::unordered_map<uint32, ForgeCharacterSpec*> AccountWideCharacterSpecs;

    // charId, PointType, specid
    std::unordered_map<ObjectGuid, std::unordered_map<CharacterPointType, std::unordered_map<uint32, ForgeCharacterPoint*>>> CharacterPoints;
    std::unordered_map<CharacterPointType, ForgeCharacterPoint*> MaxPointDefaults;
    std::unordered_map < uint32, std::unordered_map<uint32, ForgeCharacterPoint*>> AccountWidePoints;
    // skillid
    std::unordered_map<uint32, uint32> SpellToTalentTabMap;

    // skillid
    std::unordered_map<uint32, uint32> TalentTabToSpellMap;
    std::unordered_map<CharacterPointType, std::unordered_set<uint32>> CharacterPointTypeToTalentTabIds;

    // Race, class, tabtype
    std::unordered_map<uint8, std::unordered_map<uint8, std::unordered_set<uint32>>> RaceAndClassTabMap;

    // Race, class
    std::unordered_map<uint8, std::unordered_map<uint8, std::vector<ClassSpecDetail*>>> ClassSpecDetails;


    std::unordered_map<uint32, std::vector<ObjectGuid>> PlayerCharacterMap;

    // Flagged for spec reset
    std::vector<uint32 /*guid*/> FlaggedForReset;
    std::unordered_map<uint32 /*guid*/, uint32 /*spell*/> FlaggedForUnlearn;
    std::unordered_map<uint32 /*guid*/, std::vector<uint32 /*spell*/>> SpellLearnedAdditionalSpells;

    // xmog
    std::unordered_map<uint32 /*char*/, std::unordered_map<uint8 /*setId*/, ForgeCharacterXmog*>> XmogSets;
    uint8 xmogSlots[14] = { EQUIPMENT_SLOT_HEAD, EQUIPMENT_SLOT_SHOULDERS, EQUIPMENT_SLOT_BODY, EQUIPMENT_SLOT_CHEST,
        EQUIPMENT_SLOT_WAIST, EQUIPMENT_SLOT_LEGS, EQUIPMENT_SLOT_FEET, EQUIPMENT_SLOT_WRISTS, EQUIPMENT_SLOT_HANDS,
        EQUIPMENT_SLOT_BACK, EQUIPMENT_SLOT_MAINHAND, EQUIPMENT_SLOT_OFFHAND, EQUIPMENT_SLOT_RANGED, EQUIPMENT_SLOT_TABARD };

    void BuildForgeCache()
    {
        CharacterActiveSpecs.clear();
        CharacterSpecs.clear();
        CharacterPoints.clear();
        MaxPointDefaults.clear();
        SpellToTalentTabMap.clear();
        TalentTabToSpellMap.clear();
        TalentTabs.clear();
        RaceAndClassTabMap.clear();
        ClassSpecDetails.clear();
        PRESTIGE_IGNORE_SPELLS.clear();
        CONFIG.clear();
        CharacterPointTypeToTalentTabIds.clear();

        for (const auto& race : RACE_LIST)
        {
            for (const auto& wowClass : CLASS_LIST)
            {
                for (const auto& ptType : TALENT_POINT_TYPES)
                    RaceAndClassTabMap[race][wowClass];
            }
        }

        try {
            GetCharacters();
            GetConfig();
            AddTalentTrees();
            AddTalentsToTrees();
            AddLevelClassSpellMap();
            AddTalentPrereqs();
            AddTalentChoiceNodes();
            AddTalentRanks();
            AddTalentUnlearn();
            AddCharacterSpecs();
            AddTalentSpent();
            AddCharacterTalents();
            //AddCharacterChoiceNodes();

            AddPlayerTalentLoadouts();

            LOG_INFO("server.load", "Loading characters points...");
            AddCharacterPointsFromDB();
            AddCharacterClassSpecs();
            AddCharacterXmogSets();

            LOG_INFO("server.load", "Loading m+ difficulty multipliers...");
            sObjectMgr->LoadInstanceDifficultyMultiplier();
            LOG_INFO("server.load", "Loading m+ difficulty level scales...");
            sObjectMgr->LoadMythicLevelScale();
            LOG_INFO("server.load", "Loading m+ minion values...");
            sObjectMgr->LoadMythicMinionValue();
            LOG_INFO("server.load", "Loading m+ keys...");
            sObjectMgr->LoadMythicDungeonKeyMap();
            LOG_INFO("server.load", "Loading m+ affixes...");
            sObjectMgr->LoadMythicAffixes();

            LOG_INFO("server.load", "Loading npc sounds...");
            sObjectMgr->LoadNpcSounds();

            LOG_INFO("server.load", "Loading character spell unlearn flags...");
            AddSpellUnlearnFlags();
            LOG_INFO("server.load", "Loading character spell learn additional spells...");
            AddSpellLearnAdditionalSpells();
        }
        catch (std::exception & ex) {
            std::string error = ex.what();
            LOG_ERROR("server.load", "ERROR IN FORGE CACHE BUILD: " + error);
            throw ex;
        }
    }

    void GetCharacters()
    {
        QueryResult query = CharacterDatabase.Query("SELECT guid, account FROM characters");

        if (!query)
            return;

        do
        {
            Field* field = query->Fetch();
            PlayerCharacterMap[field[1].Get<uint32>()].push_back(ObjectGuid::Create<HighGuid::Player>(field[0].Get<uint32>()));

        } while (query->NextRow());
    }

    void GetConfig()
    {
        QueryResult query = WorldDatabase.Query("SELECT * FROM forge_config");

        if (!query)
            return;

        do
        {
            Field* field = query->Fetch();
            CONFIG[field[0].Get<std::string>()] = field[1].Get<uint32>();

        } while (query->NextRow());
    }

    void GetMaxLevelQuests()
    {
        QueryResult query = WorldDatabase.Query("SELECT spellid FROM forge_prestige_ignored_spells");

        if (!query)
            return;

        do
        {
            Field* field = query->Fetch();
            PRESTIGE_IGNORE_SPELLS.push_back(field[0].Get<uint32>());

        } while (query->NextRow());
    }

    void UpdateForgeSpecInternal(Player* player, CharacterDatabaseTransaction& trans, ForgeCharacterSpec*& spec)
    {
        ObjectGuid charId = player->GetGUID();
        uint32 actId = player->GetSession()->GetAccountId();
        CharacterSpecs[charId][spec->Id] = spec;

        // check for account wide info, apply to other specs for all other characters of the account in the cache.
        for (auto& actChr : PlayerCharacterMap[actId])
            for (auto& sp : CharacterSpecs[actChr])
            {
                for (auto& ts : spec->PointsSpent)
                {
                    if (TalentTabs[ts.first]->TalentType == ACCOUNT_WIDE_TYPE)
                        CharacterSpecs[charId][sp.first]->PointsSpent[ts.first] = spec->PointsSpent[ts.first];
                }

                for (auto& tals : spec->Talents)
                {
                    if (tals.first > 0) {
                        if (TalentTabs[tals.first]->TalentType == ACCOUNT_WIDE_TYPE)
                            for (auto& tal : tals.second)
                                sp.second->Talents[tals.first][tal.first] = tal.second;
                    }
                }
            }

        auto activeSpecItt = CharacterActiveSpecs.find(charId);

        if (activeSpecItt != CharacterActiveSpecs.end() && spec->Active && activeSpecItt->second != spec->Id)
        {
            ForgeCharacterSpec* activeSpec;
            if (TryGetCharacterSpec(player, activeSpecItt->second, activeSpec))
            {
                activeSpec->Active = false;
                AddCharSpecUpdateToTransaction(actId, trans, activeSpec);
            }
        }

        if (spec->Active)
            CharacterActiveSpecs[charId] = spec->Id;

        AddCharSpecUpdateToTransaction(actId, trans, spec);
    }

    void AddCharSpecUpdateToTransaction(uint32 accountId, CharacterDatabaseTransaction& trans, ForgeCharacterSpec*& spec)
    {
        auto guid = spec->CharacterGuid.GetCounter();
        trans->Append("INSERT INTO `forge_character_specs` (`id`,`guid`,`name`,`description`,`active`,`spellicon`,`visability`,`charSpec`) VALUES ({},{},\"{}\",\"{}\",{},{},{},{}) ON DUPLICATE KEY UPDATE `name` = \"{}\", `description` = \"{}\", `active` = {}, `spellicon` = {}, `visability` = {}, `charSpec` = {}",
            spec->Id, guid, spec->Name, spec->Description, spec->Active, spec->SpellIconId, (int)spec->Visability, spec->CharacterSpecTabId,
            spec->Name, spec->Description, spec->Active, spec->SpellIconId, (int)spec->Visability, spec->CharacterSpecTabId);

        for (auto& kvp : spec->PointsSpent)
        {
            if(TalentTabs[kvp.first]->TalentType != ACCOUNT_WIDE_TYPE)
                trans->Append("INSERT INTO forge_character_talents_spent(`guid`,`spec`,`tabid`,`spent`) VALUES({}, {}, {}, {}) ON DUPLICATE KEY UPDATE spent = {}",
                    guid, spec->Id, kvp.first, kvp.second,
                    kvp.second);
            else
                trans->Append("INSERT INTO forge_character_talents_spent(`guid`,`spec`,`tabid`,`spent`) VALUES({}, {}, {}, {}) ON DUPLICATE KEY UPDATE spent = {}",
                    accountId, ACCOUNT_WIDE_KEY, kvp.first, kvp.second,
                    kvp.second);
        }
    }

    void UpdateCharacterTalentInternal(uint32 account, uint32 charId, CharacterDatabaseTransaction& trans, uint32 spec, uint32 spellId, uint32 tabId, uint8 known)
    {
        if (TalentTabs[tabId]->TalentType != ACCOUNT_WIDE_TYPE)
            trans->Append("INSERT INTO `forge_character_talents` (`guid`,`spec`,`spellid`,`tabId`,`currentrank`) VALUES ({},{},{},{},{}) ON DUPLICATE KEY UPDATE `currentrank` = {}", charId, spec, spellId, tabId, known, known);
        else
            trans->Append("INSERT INTO `forge_character_talents` (`guid`,`spec`,`spellid`,`tabId`,`currentrank`) VALUES ({},{},{},{},{}) ON DUPLICATE KEY UPDATE `currentrank` = {}", account, ACCOUNT_WIDE_KEY, spellId, tabId, known, known);
    }

    void ForgetCharacterPerkInternal(uint32 charId, uint32 spec, uint32 spellId) {
        // TODO trans->Append("DELETE FROM character_perks WHERE spellId = {} and specId = {}", spellId, spec);
    }

    void AddCharacterXmogSets()
    {
        LOG_INFO("server.load", "Loading character xmog sets...");
        QueryResult xmogSets = CharacterDatabase.Query("SELECT * FROM `forge_character_transmogsets`");
        
        if (!xmogSets)
            return;

        XmogSets.clear();

        do
        {
            Field* xmogSet = xmogSets->Fetch();
            auto guid = xmogSet[0].Get<uint32>();
            auto setid = xmogSet[1].Get<uint32>();
            std::string name = xmogSet[2].Get<std::string>();
            uint32 head = xmogSet[3].Get<uint32>();
            uint32 shoulders = xmogSet[4].Get<uint32>();
            uint32 shirt = xmogSet[5].Get<uint32>();
            uint32 chest = xmogSet[6].Get<uint32>();
            uint32 waist = xmogSet[7].Get<uint32>();
            uint32 legs = xmogSet[8].Get<uint32>();
            uint32 feet = xmogSet[9].Get<uint32>();
            uint32 wrists = xmogSet[10].Get<uint32>();
            uint32 hands = xmogSet[11].Get<uint32>();
            uint32 back = xmogSet[12].Get<uint32>();
            uint32 mh = xmogSet[13].Get<uint32>();
            uint32 oh = xmogSet[14].Get<uint32>();
            uint32 ranged = xmogSet[15].Get<uint32>();
            uint32 tabard = xmogSet[16].Get<uint32>();

            ForgeCharacterXmog* set = new ForgeCharacterXmog();
            set->name = name;
            set->slottedItems[EQUIPMENT_SLOT_HEAD] = head;
            set->slottedItems[EQUIPMENT_SLOT_SHOULDERS] = shoulders;
            set->slottedItems[EQUIPMENT_SLOT_BODY] = shirt;
            set->slottedItems[EQUIPMENT_SLOT_CHEST] = chest;
            set->slottedItems[EQUIPMENT_SLOT_WAIST] = waist;
            set->slottedItems[EQUIPMENT_SLOT_LEGS] = legs;
            set->slottedItems[EQUIPMENT_SLOT_FEET] = feet;
            set->slottedItems[EQUIPMENT_SLOT_WRISTS] = wrists;
            set->slottedItems[EQUIPMENT_SLOT_HANDS] = hands;
            set->slottedItems[EQUIPMENT_SLOT_BACK] = back;
            set->slottedItems[EQUIPMENT_SLOT_MAINHAND] = mh;
            set->slottedItems[EQUIPMENT_SLOT_OFFHAND] = oh;
            set->slottedItems[EQUIPMENT_SLOT_RANGED] = ranged;
            set->slottedItems[EQUIPMENT_SLOT_TABARD] = tabard;

            XmogSets[guid][setid] = set;
        } while (xmogSets->NextRow());
    }

    void SaveXmogSetInternal(Player* player, uint32 set, ForgeCharacterXmog* xmog) {
        auto trans = CharacterDatabase.BeginTransaction();
        trans->Append("INSERT INTO acore_characters.forge_character_transmogsets (guid, setid, setname, head, shoulders, shirt, chest, waist, legs, feet, wrists, hands, back, mh, oh, ranged, tabard) VALUES({}, {}, '{}', {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}) on duplicate key update setname = '{}' , head = {}, shoulders = {}, shirt = {}, chest = {}, waist = {}, legs = {}, feet = {}, wrists = {}, hands = {}, back = {}, mh = {}, oh = {}, ranged = {}, tabard = {}",
            player->GetGUID().GetCounter(), set, xmog->name, xmog->slottedItems[EQUIPMENT_SLOT_HEAD], xmog->slottedItems[EQUIPMENT_SLOT_SHOULDERS],
            xmog->slottedItems[EQUIPMENT_SLOT_BODY], xmog->slottedItems[EQUIPMENT_SLOT_CHEST], xmog->slottedItems[EQUIPMENT_SLOT_WAIST],
            xmog->slottedItems[EQUIPMENT_SLOT_LEGS], xmog->slottedItems[EQUIPMENT_SLOT_FEET], xmog->slottedItems[EQUIPMENT_SLOT_WRISTS],
            xmog->slottedItems[EQUIPMENT_SLOT_HANDS], xmog->slottedItems[EQUIPMENT_SLOT_BACK], xmog->slottedItems[EQUIPMENT_SLOT_MAINHAND],
            xmog->slottedItems[EQUIPMENT_SLOT_OFFHAND], xmog->slottedItems[EQUIPMENT_SLOT_RANGED], xmog->slottedItems[EQUIPMENT_SLOT_TABARD],
            xmog->name, xmog->slottedItems[EQUIPMENT_SLOT_HEAD], xmog->slottedItems[EQUIPMENT_SLOT_SHOULDERS],
            xmog->slottedItems[EQUIPMENT_SLOT_BODY], xmog->slottedItems[EQUIPMENT_SLOT_CHEST], xmog->slottedItems[EQUIPMENT_SLOT_WAIST],
            xmog->slottedItems[EQUIPMENT_SLOT_LEGS], xmog->slottedItems[EQUIPMENT_SLOT_FEET], xmog->slottedItems[EQUIPMENT_SLOT_WRISTS],
            xmog->slottedItems[EQUIPMENT_SLOT_HANDS], xmog->slottedItems[EQUIPMENT_SLOT_BACK], xmog->slottedItems[EQUIPMENT_SLOT_MAINHAND],
            xmog->slottedItems[EQUIPMENT_SLOT_OFFHAND], xmog->slottedItems[EQUIPMENT_SLOT_RANGED], xmog->slottedItems[EQUIPMENT_SLOT_TABARD]);
        CharacterDatabase.CommitTransaction(trans);
    }

    uint8 FirstOpenXmogSlot(Player* player) {
        auto playerSets = XmogSets[player->GetGUID().GetCounter()];
        int i = 0;
        for (auto it = playerSets.cbegin(), end = playerSets.cend();
            it != end && i == it->first; ++it, ++i)
        { }
        return i;
    }

    void AddTalentTrees()
    {
        QueryResult talentTab = WorldDatabase.Query("SELECT * FROM forge_talent_tabs order by `id` asc");

        if (!talentTab)
            return;

        _cacheClassNodeToClassTree.clear();
        _cacheClassNodeToSpell.clear();
        _playerClassFirstSpec.clear();

        do
        {
            Field* talentFields = talentTab->Fetch();
            ForgeTalentTab* newTab = new ForgeTalentTab();
            newTab->Id = talentFields[0].Get<uint32>();
            newTab->ClassMask = talentFields[1].Get<uint32>();
            newTab->RaceMask = talentFields[2].Get<uint32>();
            newTab->Name = talentFields[3].Get<std::string>();
            newTab->SpellIconId = talentFields[4].Get<uint32>();
            newTab->Background = talentFields[5].Get<std::string>();
            newTab->Description = talentFields[6].Get<std::string>();
            newTab->Role = talentFields[7].Get<uint8>();
            newTab->SpellString = talentFields[8].Get<std::string>();
            newTab->TalentType = (CharacterPointType)talentFields[9].Get<uint8>();
            newTab->TabIndex = talentFields[10].Get<uint32>();

            auto firstSpec = _playerClassFirstSpec.find(newTab->ClassMask);
            if (firstSpec == _playerClassFirstSpec.end()) {
                _playerClassFirstSpec[newTab->ClassMask] = newTab->Id;
            }

            if (newTab->TalentType == CharacterPointType::CLASS_TREE) {
                _cacheClassNodeToSpell[newTab->ClassMask] = {};
                _cacheClassNodeToClassTree[newTab->ClassMask] = newTab->Id;
            }

            for (auto& race : RaceAndClassTabMap)
            {
                auto bit = (newTab->RaceMask & (1 << (race.first - 1)));

                if (newTab->RaceMask != 0 && bit == 0)
                    continue;

                for (const auto& wowClass : race.second)
                {
                    auto classBit = (newTab->ClassMask & (1 << (wowClass.first - 1)));

                    if (classBit != 0 || newTab->ClassMask == 0)
                    {
                        RaceAndClassTabMap[race.first][wowClass.first].insert(newTab->Id);
                        SpellToTalentTabMap[newTab->SpellIconId] = newTab->Id;
                        TalentTabToSpellMap[newTab->Id] = newTab->SpellIconId;
                        CharacterPointTypeToTalentTabIds[newTab->TalentType].insert(newTab->Id);
                    }
                }
            }

            TalentTabs[newTab->Id] = newTab;
        } while (talentTab->NextRow());

    }

    void AddTalentsToTrees()
    {
        QueryResult talents = WorldDatabase.Query("SELECT * FROM forge_talents order by `talentTabId` asc, `rowIndex` asc, `columnIndex` asc");

        _cacheTreeMetaData.clear();
        _cacheSpecNodeToSpell.clear();
        _cacheClassNodeToSpell.clear();

        if (!talents)
            return;

        int i = 1;
        auto prevTab = -1;
        do
        {
            Field* talentFields = talents->Fetch();
            ForgeTalent* newTalent = new ForgeTalent();
            newTalent->SpellId = talentFields[0].Get<uint32>();
            newTalent->TalentTabId = talentFields[1].Get<uint32>();
            newTalent->ColumnIndex = talentFields[2].Get<uint32>();
            newTalent->RowIndex = talentFields[3].Get<uint32>();
            newTalent->RankCost = talentFields[4].Get<uint8>();
            newTalent->RequiredLevel = talentFields[5].Get<uint8>();
            newTalent->TalentType = (CharacterPointType)talentFields[6].Get<uint8>();
            newTalent->NumberOfRanks = talentFields[7].Get<uint8>();
            newTalent->PreReqType = (PereqReqirementType)talentFields[8].Get<uint8>();
            newTalent->TabPointReq = talentFields[9].Get<uint16>();
            newTalent->nodeType = NodeType(talentFields[10].Get<uint8>());

            if (prevTab != newTalent->TalentTabId) {
                prevTab = newTalent->TalentTabId;
                i = 1;
            }

            newTalent->nodeIndex = i++;
            if (newTalent->TalentType != CharacterPointType::CLASS_TREE)
                _cacheSpecNodeToSpell[newTalent->TalentTabId][newTalent->nodeIndex] = newTalent->SpellId;
            else {
                _cacheClassNodeToSpell[TalentTabs[newTalent->TalentTabId]->ClassMask][newTalent->nodeIndex] = newTalent->SpellId;
            }

            auto tabItt = TalentTabs.find(newTalent->TalentTabId);

            if (tabItt == TalentTabs.end())
            {
                LOG_ERROR("FORGE.ForgeCache", "Error loading talents, invalid tab id: " + std::to_string(newTalent->TalentTabId));
            }
            else
                tabItt->second->Talents[newTalent->SpellId] = newTalent;

            // get treemeta from struct
            auto found = _cacheTreeMetaData.find(newTalent->TalentTabId);
            TreeMetaData* data;

            if (found == _cacheTreeMetaData.end()) {
                TreeMetaData* tree = new TreeMetaData();
                tree->MaxXDim = newTalent->ColumnIndex;
                tree->MaxYDim = newTalent->RowIndex;
                tree->TabId = newTalent->TalentTabId;
                _cacheTreeMetaData[tree->TabId] = tree;
                data = _cacheTreeMetaData[tree->TabId];
            }
            else {
                if (newTalent->RowIndex > found->second->MaxYDim)
                    found->second->MaxYDim = newTalent->RowIndex;
                if (newTalent->ColumnIndex > found->second->MaxXDim)
                    found->second->MaxXDim = newTalent->ColumnIndex;

                data = found->second;
            }
            NodeMetaData* node = new NodeMetaData();
            node->spellId = newTalent->SpellId;
            node->pointReq = newTalent->TabPointReq;
            node->col = newTalent->ColumnIndex;
            node->row = newTalent->RowIndex;
            node->nodeIndex = newTalent->nodeIndex;

            data->nodes[node->row][node->col] = node;
            data->nodeLocation[node->spellId] = node;
        } while (talents->NextRow());

        // load meta data
    }

    void AddTalentPrereqs()
    {
        QueryResult preReqTalents = WorldDatabase.Query("SELECT * FROM forge_talent_prereq");

        if (!preReqTalents)
            return;

        do
        {
            Field* talentFields = preReqTalents->Fetch();
            ForgeTalentPrereq* newTalent = new ForgeTalentPrereq();
            newTalent->reqId = talentFields[0].Get<uint32>();
            newTalent->Talent = talentFields[3].Get<uint32>();
            newTalent->TalentTabId = talentFields[4].Get<uint32>();
            newTalent->RequiredRank = talentFields[5].Get<uint32>();

            auto reqdSpellId = talentFields[1].Get<uint32>();
            auto reqSpelltab = talentFields[2].Get<uint32>();

            if (!TalentTabs.empty()) {
                auto tab = TalentTabs.find(reqSpelltab);
                if (tab != TalentTabs.end()) {
                    auto talent = tab->second->Talents.find(reqdSpellId);
                    if (talent != tab->second->Talents.end()) {
                        ForgeTalent* lt = talent->second;
                        if (lt != nullptr)
                            lt->Prereqs.push_back(newTalent);
                        else
                            LOG_ERROR("FORGE.ForgeCache", "Error loading AddTalentPrereqs, invaild req id: " + std::to_string(newTalent->reqId));

                        auto found = _cacheTreeMetaData.find(reqSpelltab);
                        if (found != _cacheTreeMetaData.end()) {
                            TreeMetaData* tree = found->second;
                            NodeMetaData* node = tree->nodeLocation[newTalent->Talent];
                            node->unlocks.push_back(tree->nodeLocation[reqdSpellId]);
                            tree->nodeLocation[newTalent->Talent] = node;
                            tree->nodes[node->row][node->col] = node;
                        }
                        else
                            LOG_ERROR("FORGE.ForgeCache", "Prereq cannot be mapped to existing talent meta data.");
                    } else {
                        LOG_ERROR("FORGE.ForgeCache", "Prereq spell not found in tab.");
                    }
                } else {
                    LOG_ERROR("FORGE.ForgeCache", "Prereq spell tab not found.");
                }
            } else {
                LOG_ERROR("FORGE.ForgeCache", "Talent tab store is empty.");
            }
        } while (preReqTalents->NextRow());
    }

    void AddTalentChoiceNodes()
    {
        QueryResult exclTalents = WorldDatabase.Query("SELECT * FROM forge_talent_choice_nodes");

        _choiceNodes.clear();
        _choiceNodesRev.clear();
        _choiceNodeIndexLookup.clear();

        if (!exclTalents)
            return;

        do
        {
            Field* talentFields = exclTalents->Fetch();
            uint32 choiceNodeId = talentFields[0].Get<uint32>();
            uint32 talentTabId = talentFields[1].Get<uint32>();
            uint8 choiceIndex = talentFields[2].Get<uint8>();
            uint32 spellChoice = talentFields[3].Get<uint32>();

            ForgeTalentChoice* choice = new ForgeTalentChoice();
            choice->active = false;
            choice->spellId = spellChoice;

            _choiceNodes[choiceNodeId].push_back(spellChoice);
            _choiceNodesRev[spellChoice] = choiceNodeId;
            _choiceNodeIndexLookup[choiceIndex] = spellChoice;

            ForgeTalent* lt = TalentTabs[talentTabId]->Talents[choiceNodeId];
            if (lt != nullptr)
            {
                lt->Choices[choiceIndex] = choice;
            }
            else
            {
                LOG_ERROR("FORGE.ForgeCache", "Error loading AddTalentChoiceNodes, invaild choiceNodeId id: " + std::to_string(choiceNodeId));
            }

        } while (exclTalents->NextRow());
    }

    void AddCharacterChoiceNodes() {
        QueryResult choiceQuery = CharacterDatabase.Query("SELECT * FROM forge_character_node_choices");

        if (!choiceQuery)
            return;

        do
        {
            Field* fields = choiceQuery->Fetch();
            uint32 id = fields[0].Get<uint32>();
            ObjectGuid characterGuid = ObjectGuid::Create<HighGuid::Player>(id);
            uint32 specId = fields[1].Get<uint32>();
            uint32 TabId = fields[2].Get<uint32>();
            uint32 nodeId = fields[3].Get<uint32>();
            uint32 chosenSpell = fields[4].Get<uint32>();

            ForgeTalent* ft = TalentTabs[TabId]->Talents[nodeId];
            if (ft->nodeType == NodeType::CHOICE) {
                ForgeCharacterSpec* spec = CharacterSpecs[characterGuid][specId];
                spec->ChoiceNodesChosen[nodeId] = chosenSpell;
            }

        } while (choiceQuery->NextRow());
    }

    void AddTalentRanks()
    {
        QueryResult talentRanks = WorldDatabase.Query("SELECT * FROM forge_talent_ranks");

        if (!talentRanks)
            return;

        do
        {
            Field* talentFields = talentRanks->Fetch();
            uint32 talentspellId = talentFields[0].Get<uint32>();
            uint32 talentTabId = talentFields[1].Get<uint32>();
            uint32 rank = talentFields[2].Get<uint32>();
            uint32 spellId = talentFields[3].Get<uint32>();

            ForgeTalent* lt = TalentTabs[talentTabId]->Talents[talentspellId];

            if (lt != nullptr)
            {
                lt->Ranks[rank] = spellId;
                lt->RanksRev[spellId] = rank;
            }
            else
            {
                LOG_ERROR("FORGE.ForgeCache", "Error loading AddTalentRanks, invaild talentTabId id: " + std::to_string(talentTabId) + " Rank: " + std::to_string(rank) + " SpellId: " + std::to_string(spellId));
            }

        } while (talentRanks->NextRow());
    }

    void AddTalentUnlearn()
    {
        QueryResult exclTalents = WorldDatabase.Query("SELECT * FROM forge_talent_unlearn");

        if (!exclTalents)
            return;

        do
        {
            Field* talentFields = exclTalents->Fetch();
            uint32 spellId = talentFields[1].Get<uint32>();
            uint32 talentTabId = talentFields[0].Get<uint32>();
            uint32 exclusiveSpellId = talentFields[2].Get<uint32>();

            ForgeTalent* lt = TalentTabs[talentTabId]->Talents[spellId];

            if (lt != nullptr)
            {
                lt->UnlearnSpells.push_back(exclusiveSpellId);
            }
            else
            {
                LOG_ERROR("FORGE.ForgeCache", "Error loading AddTalentUnlearn, invaild talentTabId id: " + std::to_string(talentTabId) + " ExclusiveSpell: " + std::to_string(exclusiveSpellId) + " SpellId: " + std::to_string(spellId));
            }

        } while (exclTalents->NextRow());
    }

    void AddCharacterSpecs()
    {
        QueryResult charSpecs = CharacterDatabase.Query("SELECT * FROM forge_character_specs");

        if (!charSpecs)
            return;
        
        do
        {
            Field* specFields = charSpecs->Fetch();
            ForgeCharacterSpec* spec = new ForgeCharacterSpec();
            spec->Id = specFields[0].Get<uint32>();
            spec->CharacterGuid = ObjectGuid::Create<HighGuid::Player>(specFields[1].Get<uint32>());
            spec->Name = specFields[2].Get<std::string>();
            spec->Description = specFields[3].Get<std::string>();
            spec->Active = specFields[4].Get<bool>();
            spec->SpellIconId = specFields[5].Get<uint32>();
            spec->Visability = (SpecVisibility)specFields[6].Get<uint8>();
            spec->CharacterSpecTabId = specFields[7].Get<uint32>();

            if (spec->Active)
                CharacterActiveSpecs[spec->CharacterGuid] = spec->Id;

            CharacterSpecs[spec->CharacterGuid][spec->Id] = spec;
        } while (charSpecs->NextRow());
    }

    void AddTalentSpent()
    {
        QueryResult exclTalents = CharacterDatabase.Query("SELECT * FROM forge_character_talents_spent");

        if (!exclTalents)
            return;

        do
        {
            Field* talentFields = exclTalents->Fetch();
            uint32 id = talentFields[0].Get<uint32>();
            uint32 spec = talentFields[1].Get<uint32>();
            uint32 tabId = talentFields[2].Get<uint32>();

            if (spec != ACCOUNT_WIDE_KEY)
            {
                ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(id);
               
                CharacterSpecs[guid][spec]->PointsSpent[tabId] = talentFields[3].Get<uint8>();
            }
            else
            {
                auto aws = AccountWideCharacterSpecs.find(id);
                

                if (aws == AccountWideCharacterSpecs.end())
                    AccountWideCharacterSpecs[id] = new ForgeCharacterSpec();

                AccountWideCharacterSpecs[id]->PointsSpent[tabId] = talentFields[3].Get<uint8>();

                for (auto& ch : PlayerCharacterMap[id])
                    for (auto& spec : CharacterSpecs[ch])
                        spec.second->PointsSpent[tabId] = talentFields[3].Get<uint8>();
            }

        } while (exclTalents->NextRow());
    }

    void AddCharacterTalents()
    {
        QueryResult talentsQuery = CharacterDatabase.Query("SELECT * FROM forge_character_talents");

        if (!talentsQuery)
            return;

        do
        {
            Field* talentFields = talentsQuery->Fetch();
            uint32 id = talentFields[0].Get<uint32>();
            ObjectGuid characterGuid = ObjectGuid::Create<HighGuid::Player>(id);
            uint32 specId = talentFields[1].Get<uint32>();
            ForgeCharacterTalent* talent = new ForgeCharacterTalent();
            talent->SpellId = talentFields[2].Get<uint32>();
            talent->TabId = talentFields[3].Get<uint32>();
            talent->CurrentRank = talentFields[4].Get<uint8>();

            auto invalid = false;

            if (specId != ACCOUNT_WIDE_KEY)
            {
                ForgeTalent* ft = TalentTabs[talent->TabId]->Talents[talent->SpellId];
                ForgeCharacterSpec * spec = CharacterSpecs[characterGuid][specId];
                talent->type = ft->nodeType;
                spec->Talents[talent->TabId][talent->SpellId] = talent;
            }
            else
            {
                AccountWideCharacterSpecs[id]->Talents[talent->TabId][talent->SpellId] = talent;

                for (auto& ch : PlayerCharacterMap[id])
                    for (auto& spec : CharacterSpecs[ch])
                    {
                        ForgeTalent* ft = TalentTabs[talent->TabId]->Talents[talent->SpellId];
                        talent->type = ft->nodeType;
                        spec.second->Talents[talent->TabId][talent->SpellId] = talent;
                    }
            }

        } while (talentsQuery->NextRow());
    }

    void AddCharacterPointsFromDB()
    {
        QueryResult pointsQuery = CharacterDatabase.Query("SELECT * FROM forge_character_points");

        if (!pointsQuery)
            return;
        uint32 almostMax = UINT_MAX - 1;

        do
        {
            Field* pointsFields = pointsQuery->Fetch();
            uint32 guid = pointsFields[0].Get<uint32>();
            CharacterPointType pt = (CharacterPointType)pointsFields[1].Get<uint8>();
            ForgeCharacterPoint* cp = new ForgeCharacterPoint();
            cp->PointType = pt;
            cp->SpecId = pointsFields[2].Get<uint32>();
            cp->Sum = pointsFields[3].Get<uint32>();
            cp->Max = pointsFields[4].Get<uint32>();

            if (guid == UINT_MAX)
            {
                MaxPointDefaults[pt] = cp;
            }
            else
            {
                if (cp->SpecId == ACCOUNT_WIDE_KEY)
                {
                    AccountWidePoints[guid][pt] = cp;

                    for (auto& ch : PlayerCharacterMap[guid])
                        for (auto& spec : CharacterSpecs[ch])
                            CharacterPoints[ch][cp->PointType][spec.first] = cp;
                        
                }
                else
                {
                    ObjectGuid og = ObjectGuid::Create<HighGuid::Player>(guid);
                    CharacterPoints[og][cp->PointType][cp->SpecId] = cp;
                }
            }
            
        } while (pointsQuery->NextRow());
    }

    void AddCharacterClassSpecs()
    {
       /* QueryResult specsQuery = WorldDatabase.Query("SELECT sl.DisplayName_Lang_enUS as specName, sl.SpellIconID as specIcon, src.ClassMask, src.RaceMask, sl.id FROM acore_world.db_SkillLine_12340 sl LEFT JOIN acore_world.db_SkillRaceClassInfo_12340 src ON src.SkillID = sl.id WHERE sl.CategoryID = 7 AND ClassMask IS NOT NULL AND sl.SpellIconID != 1 AND sl.SpellIconID != 0 order by src.ClassMask asc, sl.DisplayName_Lang_enUS asc");


        if (!specsQuery)
            return;

        do
        {
            Field* spec = specsQuery->Fetch();
            ClassSpecDetail* cs = new ClassSpecDetail();
            cs->Name = spec[0].Get<std::string>();
            cs->SpellIconId = spec[1].Get<uint32>();
            cs->SpecId = spec[4].Get<uint32>();

            uint32 classMask = spec[2].Get<uint32>();
            std::string raceMMask = spec[3].Get<std::string>();

            if (raceMMask == "-1")
            {
                for (const auto& race : RACE_LIST)
                    ClassSpecDetails[race][classMask].push_back(cs);
            }
            else
            {
                ClassSpecDetails[static_cast<uint32_t>(std::stoul(raceMMask))][classMask].push_back(cs);
            }

        } while (specsQuery->NextRow());*/
    }

    void AddPlayerSpellScaler()
    {
        QueryResult scale = WorldDatabase.Query("SELECT * FROM forge_player_spell_scale");

        if (!scale)
            return;

        do
        {
            Field* talentFields = scale->Fetch();
            uint32 id = talentFields[0].Get<uint32>();
            float data = talentFields[1].Get<float>();

            PlayerSpellScaleMap[id] = data;

        } while (scale->NextRow());
    }

    void AddSpellUnlearnFlags()
    {
        FlaggedForUnlearn.clear();

        std::unordered_map<uint32 /*guid*/, std::vector<uint32 /*spell*/>> SpellLearnedAdditionalSpells;
        QueryResult flag = WorldDatabase.Query("SELECT * FROM `forge_talent_spell_flagged_unlearn`");

        if (!flag)
            return;

        do
        {
            Field* talentFields = flag->Fetch();
            uint32 guid = talentFields[0].Get<uint32>();
            uint32 spell = talentFields[1].Get<float>();

            FlaggedForUnlearn[guid] = spell;

        } while (flag->NextRow());
    }

    void AddSpellLearnAdditionalSpells()
    {
        SpellLearnedAdditionalSpells.clear();

        std::unordered_map<uint32 /*guid*/, std::vector<uint32 /*spell*/>> SpellLearnedAdditionalSpells;
        QueryResult added = WorldDatabase.Query("SELECT * FROM `forge_talent_learn_additional_spell`");

        if (!added)
            return;

        do
        {
            Field* talentFields = added->Fetch();
            uint32 rootSpell = talentFields[0].Get<uint32>();
            uint32 addedSpell = talentFields[1].Get<float>();

            SpellLearnedAdditionalSpells[rootSpell].push_back(addedSpell);

        } while (added->NextRow());
    }

    void AddLevelClassSpellMap()
    {
        _levelClassSpellMap.clear();

        QueryResult maQuery = WorldDatabase.Query("select * from `acore_world`.`forge_character_spec_spells` order by `class` asc, `race` asc, `level` asc, `spell` asc");

        if (!maQuery)
            return;

        do
        {
            Field* mapFields = maQuery->Fetch();
            uint8 pClass = mapFields[0].Get<uint8>();
            uint32 race = mapFields[1].Get<uint32>();
            uint8 level = mapFields[2].Get<uint8>();
            uint32 spell = mapFields[3].Get<uint32>();

            _levelClassSpellMap[pClass][race][level].push_back(spell);
        } while (maQuery->NextRow());
    }

    void AddPlayerTalentLoadouts()
    {
        _playerTalentLoadouts.clear();
        _playerActiveTalentLoadouts.clear();

        QueryResult loadouts = WorldDatabase.Query("select * from `acore_characters`.`forge_character_talent_loadouts`");

        if (!loadouts)
            return;

        do
        {
            Field* loadoutsFields = loadouts->Fetch();
            uint32 guid = loadoutsFields[0].Get<uint32>();
            uint32 id = loadoutsFields[1].Get<uint32>();
            uint32 tabId = loadoutsFields[2].Get<uint32>();
            std::string name = loadoutsFields[3].Get<std::string>();
            std::string talentString = loadoutsFields[4].Get<std::string>();
            bool active = loadoutsFields[5].Get<bool>();

            PlayerLoadout* plo = new PlayerLoadout();
            plo->id = id;
            plo->tabId = tabId;
            plo->name = name;
            plo->talentString = talentString;
            plo->active = active;


            _playerTalentLoadouts[guid][tabId][id] = plo;
            _playerActiveTalentLoadouts[guid] = plo;
        } while (loadouts->NextRow());
    }

    // hater: custom items
    void AddItemSlotValue() {
        _forgeItemSlotValues.clear();

        QueryResult values = WorldDatabase.Query("select * from `acore_world`.`forge_item_slot_value`");

        if (!values)
            return;

        do
        {
            Field* valuesFields = values->Fetch();
            InventoryType inv = InventoryType(valuesFields[0].Get<uint32>());
            float value = valuesFields[1].Get<float>();

            _forgeItemSlotValues[inv] = value;
        } while (values->NextRow());
    }

    void AddItemStatValue() {
        _forgeItemStatValues.clear();

        QueryResult values = WorldDatabase.Query("select * from `acore_world`.`forge_item_stat_value`");

        if (!values)
            return;

        do
        {
            Field* valuesFields = values->Fetch();
            ItemModType stat = ItemModType(valuesFields[0].Get<uint32>());
            float value = valuesFields[1].Get<float>();

            _forgeItemStatValues[stat] = value;
        } while (values->NextRow());
    }

    void AddItemStatPools() {
        _forgeItemSecondaryStatPools.clear();

        QueryResult values = WorldDatabase.Query("select * from `acore_world`.`forge_item_stat_pool`");

        if (!values)
            return;

        do
        {
            Field* valuesFields = values->Fetch();
            uint8 mainstat = valuesFields[0].Get<uint8>();
            ItemModType secondarystat = ItemModType(valuesFields[1].Get<uint32>());

            _forgeItemSecondaryStatPools[mainstat].emplace_back(secondarystat);
        } while (values->NextRow());
    }
};

#define sForgeCache ForgeCache::instance()
