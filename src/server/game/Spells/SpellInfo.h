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

#ifndef _SPELLINFO_H
#define _SPELLINFO_H

#include "DBCStructure.h"
#include "Object.h"
#include "SharedDefines.h"
#include "SpellAuraDefines.h"
#include "Util.h"

class Unit;
class Player;
class Item;
class Spell;
class SpellInfo;
class AuraEffect;
struct SpellChainNode;
struct SpellTargetPosition;
struct SpellDurationEntry;
struct SpellModifier;
struct SpellRangeEntry;
struct SpellRadiusEntry;
struct SpellEntry;
struct SpellCastTimesEntry;
struct Condition;

enum SpellCastTargetFlags
{
    TARGET_FLAG_NONE            = 0x00000000,
    TARGET_FLAG_UNUSED_1        = 0x00000001,               // not used
    TARGET_FLAG_UNIT            = 0x00000002,               // pguid
    TARGET_FLAG_UNIT_RAID       = 0x00000004,               // not sent, used to validate target (if raid member)
    TARGET_FLAG_UNIT_PARTY      = 0x00000008,               // not sent, used to validate target (if party member)
    TARGET_FLAG_ITEM            = 0x00000010,               // pguid
    TARGET_FLAG_SOURCE_LOCATION = 0x00000020,               // pguid, 3 float
    TARGET_FLAG_DEST_LOCATION   = 0x00000040,               // pguid, 3 float
    TARGET_FLAG_UNIT_ENEMY      = 0x00000080,               // not sent, used to validate target (if enemy)
    TARGET_FLAG_UNIT_ALLY       = 0x00000100,               // not sent, used to validate target (if ally)
    TARGET_FLAG_CORPSE_ENEMY    = 0x00000200,               // pguid
    TARGET_FLAG_UNIT_DEAD       = 0x00000400,               // not sent, used to validate target (if dead creature)
    TARGET_FLAG_GAMEOBJECT      = 0x00000800,               // pguid, used with TARGET_GAMEOBJECT_TARGET
    TARGET_FLAG_TRADE_ITEM      = 0x00001000,               // pguid
    TARGET_FLAG_STRING          = 0x00002000,               // string
    TARGET_FLAG_GAMEOBJECT_ITEM = 0x00004000,               // not sent, used with TARGET_GAMEOBJECT_ITEM_TARGET
    TARGET_FLAG_CORPSE_ALLY     = 0x00008000,               // pguid
    TARGET_FLAG_UNIT_MINIPET    = 0x00010000,               // pguid, used to validate target (if non combat pet)
    TARGET_FLAG_GLYPH_SLOT      = 0x00020000,               // used in glyph spells
    TARGET_FLAG_DEST_TARGET     = 0x00040000,               // sometimes appears with DEST_TARGET spells (may appear or not for a given spell)
    TARGET_FLAG_UNUSED20        = 0x00080000,               // uint32 counter, loop { vec3 - screen position (?), guid }, not used so far
    TARGET_FLAG_UNIT_PASSENGER  = 0x00100000,               // guessed, used to validate target (if vehicle passenger)

    TARGET_FLAG_UNIT_MASK = TARGET_FLAG_UNIT | TARGET_FLAG_UNIT_RAID | TARGET_FLAG_UNIT_PARTY
                            | TARGET_FLAG_UNIT_ENEMY | TARGET_FLAG_UNIT_ALLY | TARGET_FLAG_UNIT_DEAD | TARGET_FLAG_UNIT_MINIPET | TARGET_FLAG_UNIT_PASSENGER,
    TARGET_FLAG_GAMEOBJECT_MASK = TARGET_FLAG_GAMEOBJECT | TARGET_FLAG_GAMEOBJECT_ITEM,
    TARGET_FLAG_CORPSE_MASK = TARGET_FLAG_CORPSE_ALLY | TARGET_FLAG_CORPSE_ENEMY,
    TARGET_FLAG_ITEM_MASK = TARGET_FLAG_TRADE_ITEM | TARGET_FLAG_ITEM | TARGET_FLAG_GAMEOBJECT_ITEM,
};

enum SpellTargetSelectionCategories
{
    TARGET_SELECT_CATEGORY_NYI,
    TARGET_SELECT_CATEGORY_DEFAULT,
    TARGET_SELECT_CATEGORY_CHANNEL,
    TARGET_SELECT_CATEGORY_NEARBY,
    TARGET_SELECT_CATEGORY_CONE,
    TARGET_SELECT_CATEGORY_AREA,
    TARGET_SELECT_CATEGORY_TRAJ,
};

enum SpellTargetReferenceTypes
{
    TARGET_REFERENCE_TYPE_NONE,
    TARGET_REFERENCE_TYPE_CASTER,
    TARGET_REFERENCE_TYPE_TARGET,
    TARGET_REFERENCE_TYPE_LAST,
    TARGET_REFERENCE_TYPE_SRC,
    TARGET_REFERENCE_TYPE_DEST,
};

enum SpellTargetObjectTypes
{
    TARGET_OBJECT_TYPE_NONE = 0,
    TARGET_OBJECT_TYPE_SRC,
    TARGET_OBJECT_TYPE_DEST,
    TARGET_OBJECT_TYPE_UNIT,
    TARGET_OBJECT_TYPE_UNIT_AND_DEST,
    TARGET_OBJECT_TYPE_GOBJ,
    TARGET_OBJECT_TYPE_GOBJ_ITEM,
    TARGET_OBJECT_TYPE_ITEM,
    TARGET_OBJECT_TYPE_CORPSE,
    // only for effect target type
    TARGET_OBJECT_TYPE_CORPSE_ENEMY,
    TARGET_OBJECT_TYPE_CORPSE_ALLY,
};

enum SpellTargetCheckTypes
{
    TARGET_CHECK_DEFAULT,
    TARGET_CHECK_ENTRY,
    TARGET_CHECK_ENEMY,
    TARGET_CHECK_ALLY,
    TARGET_CHECK_PARTY,
    TARGET_CHECK_RAID,
    TARGET_CHECK_RAID_CLASS,
    TARGET_CHECK_PASSENGER,
    TARGET_CHECK_CORPSE,
    TARGET_CHECK_SUMMON,
};

enum SpellTargetDirectionTypes
{
    TARGET_DIR_NONE,
    TARGET_DIR_FRONT,
    TARGET_DIR_BACK,
    TARGET_DIR_RIGHT,
    TARGET_DIR_LEFT,
    TARGET_DIR_FRONT_RIGHT,
    TARGET_DIR_BACK_RIGHT,
    TARGET_DIR_BACK_LEFT,
    TARGET_DIR_FRONT_LEFT,
    TARGET_DIR_RANDOM,
    TARGET_DIR_ENTRY,
};

enum SpellEffectImplicitTargetTypes
{
    EFFECT_IMPLICIT_TARGET_NONE = 0,
    EFFECT_IMPLICIT_TARGET_EXPLICIT,
    EFFECT_IMPLICIT_TARGET_CASTER,
};

// Spell clasification
enum SpellSpecificType
{
    SPELL_SPECIFIC_NORMAL                        = 0,
    SPELL_SPECIFIC_SEAL                          = 1,
    SPELL_SPECIFIC_AURA                          = 3,
    SPELL_SPECIFIC_STING                         = 4,
    SPELL_SPECIFIC_CURSE                         = 5,
    SPELL_SPECIFIC_ASPECT                        = 6,
    SPELL_SPECIFIC_TRACKER                       = 7,
    SPELL_SPECIFIC_WARLOCK_ARMOR                 = 8,
    SPELL_SPECIFIC_MAGE_ARMOR                    = 9,
    SPELL_SPECIFIC_ELEMENTAL_SHIELD              = 10,
    SPELL_SPECIFIC_MAGE_POLYMORPH                = 11,
    SPELL_SPECIFIC_JUDGEMENT                     = 13,
    SPELL_SPECIFIC_WARLOCK_CORRUPTION            = 17,
    SPELL_SPECIFIC_FOOD                          = 19,
    SPELL_SPECIFIC_DRINK                         = 20,
    SPELL_SPECIFIC_FOOD_AND_DRINK                = 21,
    SPELL_SPECIFIC_PRESENCE                      = 22,
    SPELL_SPECIFIC_CHARM                         = 23,
    SPELL_SPECIFIC_SCROLL                        = 24,
    SPELL_SPECIFIC_MAGE_ARCANE_BRILLANCE         = 25,
    SPELL_SPECIFIC_PRIEST_DIVINE_SPIRIT          = 26,
    SPELL_SPECIFIC_HAND                          = 27,
};

enum SpellCustomAttributes
{
    SPELL_ATTR0_CU_ENCHANT_PROC                  = 0x00000001,
    SPELL_ATTR0_CU_CONE_BACK                     = 0x00000002,
    SPELL_ATTR0_CU_CONE_LINE                     = 0x00000004,
    SPELL_ATTR0_CU_SHARE_DAMAGE                  = 0x00000008,
    SPELL_ATTR0_CU_NO_INITIAL_THREAT             = 0x00000010,
    SPELL_ATTR0_CU_AURA_CC                       = 0x00000020,
    SPELL_ATTR0_CU_DONT_BREAK_STEALTH            = 0x00000040,
    SPELL_ATTR0_CU_NO_PVP_FLAG                   = 0x00000080,
    SPELL_ATTR0_CU_DIRECT_DAMAGE                 = 0x00000100,
    SPELL_ATTR0_CU_CHARGE                        = 0x00000200,
    SPELL_ATTR0_CU_PICKPOCKET                    = 0x00000400,
    SPELL_ATTR0_CU_IGNORE_EVADE                  = 0x00000800,
    SPELL_ATTR0_CU_NEGATIVE_EFF0                 = 0x00001000,
    SPELL_ATTR0_CU_NEGATIVE_EFF1                 = 0x00002000,
    SPELL_ATTR0_CU_NEGATIVE_EFF2                 = 0x00004000,
    SPELL_ATTR0_CU_IGNORE_ARMOR                  = 0x00008000,
    SPELL_ATTR0_CU_REQ_TARGET_FACING_CASTER      = 0x00010000,
    SPELL_ATTR0_CU_REQ_CASTER_BEHIND_TARGET      = 0x00020000,
    SPELL_ATTR0_CU_ALLOW_INFLIGHT_TARGET         = 0x00040000,
    SPELL_ATTR0_CU_NEEDS_AMMO_DATA               = 0x00080000,
    SPELL_ATTR0_CU_BINARY_SPELL                  = 0x00100000,
    SPELL_ATTR0_CU_NO_POSITIVE_TAKEN_BONUS       = 0x00200000,
    SPELL_ATTR0_CU_SINGLE_AURA_STACK             = 0x00400000, // pussywizard
    SPELL_ATTR0_CU_SCHOOLMASK_NORMAL_WITH_MAGIC  = 0x00800000,
    SPELL_ATTR0_CU_AURA_CANNOT_BE_SAVED          = 0x01000000,
    SPELL_ATTR0_CU_POSITIVE_EFF0                 = 0x02000000,
    SPELL_ATTR0_CU_POSITIVE_EFF1                 = 0x04000000,
    SPELL_ATTR0_CU_POSITIVE_EFF2                 = 0x08000000,
    SPELL_ATTR0_CU_FORCE_SEND_CATEGORY_COOLDOWNS = 0x10000000,
    SPELL_ATTR0_CU_FORCE_AURA_SAVING             = 0x20000800,
    SPELL_ATTR0_CU_ENCOUNTER_REWARD              = 0x40000000, // pussywizard
    SPELL_ATTR0_CU_BYPASS_MECHANIC_IMMUNITY      = 0x80000000,

    SPELL_ATTR0_CU_NEGATIVE                      = SPELL_ATTR0_CU_NEGATIVE_EFF0 | SPELL_ATTR0_CU_NEGATIVE_EFF1 | SPELL_ATTR0_CU_NEGATIVE_EFF2,
    SPELL_ATTR0_CU_POSITIVE                      = SPELL_ATTR0_CU_POSITIVE_EFF0 | SPELL_ATTR0_CU_POSITIVE_EFF1 | SPELL_ATTR0_CU_POSITIVE_EFF2,
};

enum SpellCustomAttributes2
{
    SPELL_ATTR1_CU_REQUIRES_COMBAT                      = 0x00000001,
    SPELL_ATTR1_CU_NO_ATTACK_BLOCK                      = 0x00000002,
    SPELL_ATTR1_CU_SCALE_DAMAGE_EFFECTS_ONLY            = 0x00000004,
    SPELL_ATTR1_CU_SCALE_HEALING_EFFECTS_ONLY           = 0x00000008,
    SPELL_ATTR1_CU_USE_TARGETS_LEVEL_FOR_SPELL_SCALING  = 0x00000010,
    SPELL_ATTR1_CU_REMOVE_OUTSIDE_DUNGEONS_AND_RAIDS    = 0x00000020, //TODO
    SPELL_ATTR1_CU_NOT_USABLE_IN_INSTANCES              = 0x00000040, //TODO
    SPELL_ATTR1_CU_PERIODIC_CAN_CRIT                    = 0x00000080,
    SPELL_ATTR1_CU_RESET_COOLDOWN_ON_ENCOUNTER_END      = 0x00000100, //TODO
    SPELL_ATTR1_CU_IGNORES_CASTER_LEVEL                 = 0x00000200,
    SPELL_ATTR1_CU_ONLY_PROC_FROM_CLASS_ABILITIES       = 0x00000400, //TODO
    SPELL_ATTR1_CU_ACTIVATES_REQUIRED_SHAPESHIFT        = 0x00000800, //TODO

    SPELL_ATTR1_CU_SCALE_DAMAGE_OR_HEALING_EFFECT       = SPELL_ATTR1_CU_SCALE_DAMAGE_EFFECTS_ONLY | SPELL_ATTR1_CU_SCALE_HEALING_EFFECTS_ONLY,
};

uint32 GetTargetFlagMask(SpellTargetObjectTypes objType);

class SpellImplicitTargetInfo
{
private:
    Targets _target;
public:
    SpellImplicitTargetInfo() : _target(Targets(0)) {}
    SpellImplicitTargetInfo(uint32 target);

    bool IsArea() const;
    SpellTargetSelectionCategories GetSelectionCategory() const;
    SpellTargetReferenceTypes GetReferenceType() const;
    SpellTargetObjectTypes GetObjectType() const;
    SpellTargetCheckTypes GetCheckType() const;
    SpellTargetDirectionTypes GetDirectionType() const;
    float CalcDirectionAngle() const;

    Targets GetTarget() const;
    uint32 GetExplicitTargetMask(bool& srcSet, bool& dstSet) const;

private:
    struct StaticData
    {
        SpellTargetObjectTypes ObjectType;    // type of object returned by target type
        SpellTargetReferenceTypes ReferenceType; // defines which object is used as a reference when selecting target
        SpellTargetSelectionCategories SelectionCategory;
        SpellTargetCheckTypes SelectionCheckType; // defines selection criteria
        SpellTargetDirectionTypes DirectionType; // direction for cone and dest targets
    };

    static std::array<StaticData, TOTAL_SPELL_TARGETS> _data;
};

class SpellEffectInfo
{
    SpellInfo const* _spellInfo;
    uint8 _effIndex;
public:
    uint32    Effect;
    uint32    ApplyAuraName;
    uint32    Amplitude;
    int32     DieSides;
    float     RealPointsPerLevel;
    int32     BasePoints;
    float     PointsPerComboPoint;
    float     ValueMultiplier;
    float     DamageMultiplier;
    float     BonusMultiplier;
    int32     MiscValue;
    int32     MiscValueB;
    Mechanics Mechanic;
    SpellImplicitTargetInfo TargetA;
    SpellImplicitTargetInfo TargetB;
    SpellRadiusEntry const* RadiusEntry;
    uint32    ChainTarget;
    uint32    ItemType;
    uint32    TriggerSpell;
    flag96    SpellClassMask;
    std::list<Condition*>* ImplicitTargetConditions;

    SpellEffectInfo() : _spellInfo(nullptr), _effIndex(0), Effect(0), ApplyAuraName(0), Amplitude(0), DieSides(0),
        RealPointsPerLevel(0), BasePoints(0), PointsPerComboPoint(0), ValueMultiplier(0), DamageMultiplier(0),
        BonusMultiplier(0), MiscValue(0), MiscValueB(0), Mechanic(MECHANIC_NONE), RadiusEntry(nullptr), ChainTarget(0),
        ItemType(0), TriggerSpell(0), ImplicitTargetConditions(nullptr) {}
    SpellEffectInfo(SpellEntry const* spellEntry, SpellInfo const* spellInfo, uint8 effIndex);

    bool IsEffect() const;
    bool IsEffect(SpellEffects effectName) const;
    bool IsAura() const;
    bool IsAura(AuraType aura) const;
    bool IsTargetingArea() const;
    bool IsAreaAuraEffect() const;
    bool IsFarUnitTargetEffect() const;
    bool IsFarDestTargetEffect() const;
    bool IsUnitOwnedAuraEffect() const;

    int32 CalcValue(Unit const* caster = nullptr, int32 const* basePoints = nullptr, Unit const* target = nullptr) const;
    int32 CalcBaseValue(int32 value) const;
    float CalcValueMultiplier(Unit* caster, Spell* spell = nullptr) const;
    float CalcDamageMultiplier(Unit* caster, Spell* spell = nullptr) const;

    bool HasRadius() const;
    float CalcRadius(Unit* caster = nullptr, Spell* = nullptr) const;

    uint32 GetProvidedTargetMask() const;
    uint32 GetMissingTargetMask(bool srcSet = false, bool destSet = false, uint32 mask = 0) const;

    SpellEffectImplicitTargetTypes GetImplicitTargetType() const;
    SpellTargetObjectTypes GetUsedTargetObjectType() const;

private:
    struct StaticData
    {
        SpellEffectImplicitTargetTypes ImplicitTargetType; // defines what target can be added to effect target list if there's no valid target type provided for effect
        SpellTargetObjectTypes UsedTargetObjectType; // defines valid target object type for spell effect
    };

    static std::array<StaticData, TOTAL_SPELL_EFFECTS> _data;
};

class AC_GAME_API SpellInfo
{
friend class SpellMgr;

public:
    uint32 Id;
    SpellCategoryEntry const* CategoryEntry;
    uint32 Dispel;
    uint32 Mechanic;
    uint32 Attributes;
    uint32 AttributesEx;
    uint32 AttributesEx2;
    uint32 AttributesEx3;
    uint32 AttributesEx4;
    uint32 AttributesEx5;
    uint32 AttributesEx6;
    uint32 AttributesEx7;
    uint32 AttributesCu;
    uint32 AttributesExCu;
    uint32 Stances;
    uint32 StancesNot;
    uint32 Targets;
    uint32 TargetCreatureType;
    uint32 RequiresSpellFocus;
    uint32 FacingCasterFlags;
    uint32 CasterAuraState;
    uint32 TargetAuraState;
    uint32 CasterAuraStateNot;
    uint32 TargetAuraStateNot;
    uint32 CasterAuraSpell;
    uint32 TargetAuraSpell;
    uint32 ExcludeCasterAuraSpell;
    uint32 ExcludeTargetAuraSpell;
    SpellCastTimesEntry const* CastTimeEntry;
    uint32 RecoveryTime;
    uint32 CategoryRecoveryTime;
    uint32 StartRecoveryCategory;
    uint32 StartRecoveryTime;
    uint32 InterruptFlags;
    uint32 AuraInterruptFlags;
    uint32 ChannelInterruptFlags;
    uint32 ProcFlags;
    uint32 ProcChance;
    uint32 ProcCharges;
    uint32 MaxLevel;
    uint32 BaseLevel;
    uint32 SpellLevel;
    SpellDurationEntry const* DurationEntry;
    uint32 PowerType;
    uint32 ManaCost;
    uint32 ManaCostPerlevel;
    uint32 ManaPerSecond;
    uint32 ManaPerSecondPerLevel;
    uint32 ManaCostPercentage;
    uint32 RuneCostID;
    SpellRangeEntry const* RangeEntry;
    float  Speed;
    uint32 StackAmount;
    std::array<uint32, 2> Totem;
    std::array<int32, MAX_SPELL_REAGENTS>  Reagent;
    std::array<uint32, MAX_SPELL_REAGENTS> ReagentCount;
    int32  EquippedItemClass;
    int32  EquippedItemSubClassMask;
    int32  EquippedItemInventoryTypeMask;
    std::array<uint32, 2> TotemCategory;
    std::array<uint32, 2> SpellVisual;
    uint32 SpellIconID;
    uint32 ActiveIconID;
    uint32 Priority;
    std::array<char const*, 16> SpellName;
    std::array<char const*, 16> Rank;
    uint32 MaxTargetLevel;
    uint32 MaxAffectedTargets;
    uint32 SpellFamilyName;
    flag96 SpellFamilyFlags;
    uint32 DmgClass;
    uint32 PreventionType;
    int32  AreaGroupId;
    uint32 SchoolMask;
    std::array<SpellEffectInfo, MAX_SPELL_EFFECTS> Effects;
    uint32 ExplicitTargetMask;
    SpellChainNode const* ChainEntry;

    // Mine
    AuraStateType _auraState;
    SpellSpecificType _spellSpecific;
    bool _isStackableWithRanks;
    bool _isSpellValid;
    bool _isCritCapable;
    bool _requireCooldownInfo;

    SpellInfo(SpellEntry const* spellEntry);
    ~SpellInfo();

    uint32 GetCategory() const;
    bool HasEffect(SpellEffects effect) const;
    bool CheckFamilyFlagsApply(flag96 flags) const;
    bool CheckFamilyFlagsApply(flag96 flags, uint8 effect) const;
    bool HasAura(AuraType aura) const;
    bool HasAuraPositive(AuraType aura) const;
    bool HasAnyAura() const;
    bool HasAreaAuraEffect() const;

    inline bool HasAttribute(SpellAttr0 attribute) const { return (Attributes & attribute) != 0; }
    inline bool HasAttribute(SpellAttr1 attribute) const { return (AttributesEx & attribute) != 0; }
    inline bool HasAttribute(SpellAttr2 attribute) const { return (AttributesEx2 & attribute) != 0; }
    inline bool HasAttribute(SpellAttr3 attribute) const { return (AttributesEx3 & attribute) != 0; }
    inline bool HasAttribute(SpellAttr4 attribute) const { return (AttributesEx4 & attribute) != 0; }
    inline bool HasAttribute(SpellAttr5 attribute) const { return (AttributesEx5 & attribute) != 0; }
    inline bool HasAttribute(SpellAttr6 attribute) const { return (AttributesEx6 & attribute) != 0; }
    inline bool HasAttribute(SpellAttr7 attribute) const { return (AttributesEx7 & attribute) != 0; }
    inline bool HasAttribute(SpellCustomAttributes customAttribute) const { return (AttributesCu & customAttribute) != 0; }
    inline bool HasAttribute(SpellCustomAttributes2 customAttribute) const { return (AttributesExCu & customAttribute) != 0; }

    bool IsExplicitDiscovery() const;
    bool IsLootCrafting() const;
    bool IsQuestTame() const;
    bool IsProfessionOrRiding() const;
    bool IsProfession() const;
    bool IsPrimaryProfession() const;
    bool IsPrimaryProfessionFirstRank() const;
    bool IsAbilityLearnedWithProfession() const;
    bool IsAbilityOfSkillType(uint32 skillType) const;

    bool IsAffectingArea() const;
    bool IsTargetingArea() const;
    bool NeedsExplicitUnitTarget() const;
    bool NeedsToBeTriggeredByCaster(SpellInfo const* triggeringSpell, uint8 effIndex = MAX_SPELL_EFFECTS) const;
    bool IsChannelCategorySpell() const;
    bool IsSelfCast() const;

    bool IsPassive() const;
    bool IsAutocastable() const;
    bool ComputeIsCritCapable() const;
    bool IsCritCapable() const;
    bool RequireCooldownInfo() const;
    void SetCritCapable(bool val);
    bool ComputeIsStackableWithRanks() const;
    bool IsStackableWithRanks() const;
    void SetStackableWithRanks(bool val);
    bool IsSpellValid() const;
    void SetSpellValid(bool val);
    bool IsPassiveStackableWithRanks() const;
    bool IsMultiSlotAura() const;
    bool IsCooldownStartedOnEvent() const;
    bool IsDeathPersistent() const;
    bool IsRequiringDeadTarget() const;
    bool IsAllowingDeadTarget() const;
    bool CanBeUsedInCombat() const;
    bool RequiresCombat() const; // hater
    bool IsPositive() const;
    bool IsPositiveEffect(uint8 effIndex) const;
    bool IsChanneled() const;
    [[nodiscard]] bool IsActionAllowedChannel() const;
    bool NeedsComboPoints() const;
    bool IsBreakingStealth() const;
    bool IsRangedWeaponSpell() const;
    bool IsAutoRepeatRangedSpell() const;
    bool ComputeIsDamagingOrHealingEffect() const;

    WeaponAttackType GetAttackType() const;

    bool HasInitialAggro() const;

    bool IsAffected(uint32 familyName, flag96 const& familyFlags) const;

    bool IsAffectedBySpellMods() const;
    bool IsAffectedBySpellMod(SpellModifier const* mod) const;

    bool CanPierceImmuneAura(SpellInfo const* aura) const;
    bool CanDispelAura(SpellInfo const* aura) const;

    bool IsSingleTarget() const;
    bool IsAuraExclusiveBySpecificWith(SpellInfo const* spellInfo) const;
    bool IsAuraExclusiveBySpecificPerCasterWith(SpellInfo const* spellInfo) const;

    SpellCastResult CheckShapeshift(uint32 form) const;
    SpellCastResult CheckLocation(uint32 map_id, uint32 zone_id, uint32 area_id, Player* player = nullptr, bool strict = true) const;
    SpellCastResult CheckTarget(Unit const* caster, WorldObject const* target, bool implicit = true) const;
    SpellCastResult CheckExplicitTarget(Unit const* caster, WorldObject const* target, Item const* itemTarget = nullptr) const;
    bool CheckTargetCreatureType(Unit const* target) const;

    // xinef: aura stacking
    bool IsStrongerAuraActive(Unit const* caster, Unit const* target) const;
    bool IsAuraEffectEqual(SpellInfo const* otherSpellInfo) const;
    bool ValidateAttribute6SpellDamageMods(Unit const* caster, const AuraEffect* auraEffect, bool isDot) const;

    SpellSchoolMask GetSchoolMask() const;
    uint32 GetAllEffectsMechanicMask() const;
    uint32 GetEffectMechanicMask(uint8 effIndex) const;
    uint32 GetSpellMechanicMaskByEffectMask(uint32 effectMask) const;
    Mechanics GetEffectMechanic(uint8 effIndex) const;
    bool HasAnyEffectMechanic() const;
    uint32 GetDispelMask() const;
    static uint32 GetDispelMask(DispelType type);
    uint32 GetExplicitTargetMask() const;

    AuraStateType GetAuraState() const;
    SpellSpecificType GetSpellSpecific() const;

    float GetMinRange(bool positive = false) const;
    float GetMaxRange(bool positive = false, Unit* caster = nullptr, Spell* spell = nullptr) const;

    int32 GetDuration() const;
    int32 GetMaxDuration() const;
    
    uint32 GetMaxTicks(Unit*, float&) const;
    uint32 GetMaxTicks() const;
    uint32 GetNoHasteTicks() const;
    uint32 GetMaxTicks(int32 DotDuration, Unit* caster, float& dmgRatio) const;
    uint32 CalculateTicks(uint32 ampl, int32 DotDuration, Unit* caster, float& dmgRatio) const;

    uint32 CalcCastTime(Unit* caster = nullptr, Spell* spell = nullptr) const;
    uint32 GetRecoveryTime() const;

    int32 CalcPowerCost(Unit const* caster, SpellSchoolMask schoolMask, Spell* spell = nullptr) const;

    bool IsRanked() const;
    uint8 GetRank() const;
    SpellInfo const* GetFirstRankSpell() const;
    SpellInfo const* GetLastRankSpell() const;
    SpellInfo const* GetNextRankSpell() const;
    SpellInfo const* GetPrevRankSpell() const;
    SpellInfo const* GetAuraRankForLevel(uint8 level) const;
    bool IsRankOf(SpellInfo const* spellInfo) const;
    bool IsDifferentRankOf(SpellInfo const* spellInfo) const;
    bool IsHighRankOf(SpellInfo const* spellInfo) const;

    std::array<SpellEffectInfo, MAX_SPELL_EFFECTS> const& GetEffects() const { return Effects; }
    SpellEffectInfo const& GetEffect(SpellEffIndex index) const { ASSERT(index < Effects.size()); return Effects[index]; }

    // loading helpers
    void _InitializeExplicitTargetMask();
    bool _IsPositiveEffect(uint8 effIndex, bool deep) const;
    bool _IsPositiveSpell() const;
    static bool _IsPositiveTarget(uint32 targetA, uint32 targetB);

    AuraStateType LoadAuraState() const;
    SpellSpecificType LoadSpellSpecific() const;

    // unloading helpers
    void _UnloadImplicitTargetConditionLists();

    bool CheckElixirStacking(Unit const* caster) const;

    bool IsHealingSpell() const;

private:
    std::array<SpellEffectInfo, MAX_SPELL_EFFECTS>& _GetEffects() { return Effects; }
    SpellEffectInfo& _GetEffect(SpellEffIndex index) { ASSERT(index < Effects.size()); return Effects[index]; }
};

#endif // _SPELLINFO_H
