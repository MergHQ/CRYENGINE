using System.Collections.Generic;

namespace CryEngine.Towerdefense
{
    public class UnitTowerPoison : UnitTowerData
    {
        public override string Name { get { return "Poison Tower"; } }
        public override int MaxHealth { get { return 100; } }
        public override UnitType Type { get { return UnitType.Tower; } }
        public override string Mesh { get { return AssetLibrary.Meshes.Tower; } }

        public override List<TowerData> UpgradePath
        {
            get
            {
                return new List<TowerData>()
                {
                    new TowerData() // Level One
                    {
                        Cost = 250,
                        DamageMultiplier = 1, // This value is used by the tower's Weapon to calculate the damage output by multiplying BaseDamageOutput by this value.
                        FireRateMultiplier = 1, // This value is used by the tower's Weapon to calculate the fire rate by multiplying BaseFireRate by this value.
                        RangeMultiplier = 1, // This value is used by the tower's Sensor to calculate the range by multiplying BaseRange by this value.
                        Material = AssetLibrary.Materials.Purple,
                    },
                };
            }
        }

        #region Weapon Info
        public override int BaseDamageOutput { get { return 5; } }
        public override float BaseFireRate { get { return 4f; } }
        public override float BaseRange { get { return 10f; } }
        public override List<EffectData> OnHitEffects
        {
            get
            {
                return new List<EffectData>()
                {
                    new EffectDamageData()
                    {
                        Identity = "Poison",
                        Damage = 2,
                        TickRate = 1f,
                        TimeToLive = 4,
                        ApplyEveryTick = true,
                        AreaOfEffect = 2f,
                        PreventStacking = true,
                    },
                };
            }
        }
        #endregion
    }
}
