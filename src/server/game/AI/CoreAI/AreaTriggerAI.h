/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ACORE_AREATRIGGERAI_H
#define ACORE_AREATRIGGERAI_H

#include "Define.h"

class AreaTrigger;
class Unit;

class AreaTriggerAI
{
    uint32 _scriptId;
    protected:
        AreaTrigger* const at;
    public:
        explicit AreaTriggerAI(AreaTrigger* a, uint32 scriptId = {});
        virtual ~AreaTriggerAI();

        // Called when the AreaTrigger has just been initialized, just before added to map
        virtual void OnInitialize() { }

        // Called when the AreaTrigger has just been created
        virtual void OnCreate() { }

        // Called on each AreaTrigger update
        virtual void OnUpdate(uint32 /*diff*/) { }

        // Called on each AreaTrigger proc, timer defined by at->SetPeriodicProcTimer(uint32)
        virtual void OnPeriodicProc() { }

        // Called when the AreaTrigger reach splineIndex
        virtual void OnSplineIndexReached(int /*splineIndex*/) { }

        // Called when the AreaTrigger reach its destination
        virtual void OnDestinationReached() { }

        // Called when an unit enter the AreaTrigger
        virtual void OnUnitEnter(Unit* /*unit*/) { }

        // Called when an unit exit the AreaTrigger, or when the AreaTrigger is removed
        virtual void OnUnitExit(Unit* /*unit*/) { }

        // Called when the AreaTrigger is removed
        virtual void OnRemove() { }

        // Pass parameters between AI
        virtual void DoAction(int32 /*param*/) { }

        uint32 GetId() const { return _scriptId; }
};

class NullAreaTriggerAI : public AreaTriggerAI
{
    public:
        using AreaTriggerAI::AreaTriggerAI;
};

#endif
