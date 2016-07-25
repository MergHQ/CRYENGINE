using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace CryEngine.Towerdefense
{
    public abstract class UnitTowerData : UnitData
    {
        public override int MaxHealth { get { return 100; } }
        public override UnitType Type { get { return UnitType.Tower; } }
        public override string Mesh { get { return AssetLibrary.Meshes.Tower; } }
        public virtual float RotateSpeed { get { return 0.1f; } }
        public abstract int BaseDamageOutput { get; }
        public abstract float BaseFireRate { get; }
        public abstract float BaseRange { get; }
        public abstract List<EffectData> OnHitEffects { get; }
        public abstract List<TowerData> UpgradePath { get; }
    }

    public class TowerData
    {
        public int Cost { get; set; }
        public int RefundCost { get { return Cost / 2; } }
        public float FireRateMultiplier { get; set; }
        public float DamageMultiplier { get; set; }
        public float RangeMultiplier { get; set; }
        public virtual string Material { get; set; }
    }
}
