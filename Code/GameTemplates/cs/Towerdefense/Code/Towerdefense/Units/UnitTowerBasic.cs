using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace CryEngine.Towerdefense
{
    public class UnitTowerBasic : UnitTowerData
    {
        public override string Name { get { return "Beep Boop"; } }
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
                        Cost = 150,
                        DamageMultiplier = 1, // This value is used by the tower's Weapon to calculate the damage output by multiplying BaseDamageOutput by this value.
                        FireRateMultiplier = 1, // This value is used by the tower's Weapon to calculate the fire rate by multiplying BaseFireRate by this value.
                        RangeMultiplier = 1, // This value is used by the tower's Sensor to calculate the range by multiplying BaseRange by this value.
                        Material = AssetLibrary.Materials.GreyMin,
                    },
                    new TowerData() // Level Two
                    {
                        Cost = 200,
                        DamageMultiplier = 1.5f,
                        FireRateMultiplier = 0.5f,
                        RangeMultiplier = 1.5f,
                        Material = AssetLibrary.Materials.Yellow,
                    },
                    new TowerData() // Level Three
                    {
                        Cost = 250,
                        DamageMultiplier = 2f,
                        FireRateMultiplier = 0.25f,
                        RangeMultiplier = 2f,
                        Material = AssetLibrary.Materials.Red,
                    },
                };
            }
        }

        #region Weapon Info
        public override int BaseDamageOutput { get { return 5; } }
        public override float BaseFireRate { get { return 0.5f; } }
        public override float BaseRange { get { return 10f; } }
        public override List<EffectData> OnHitEffects
        {
            get
            {
                return new List<EffectData>()
                {
                    new EffectDamageData()
                    {
                        Damage = 8,
                    },
                    new EffectSlowData()
                    {
                        TimeToLive = 0.5f,
                        SpeedModifier = 0.5f,
                        OverridePreviousModifiers = true,
                    },
                };
            }
        }
        #endregion
    }
}
