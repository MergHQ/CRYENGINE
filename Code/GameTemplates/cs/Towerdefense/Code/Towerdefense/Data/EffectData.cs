using CryEngine.Towerdefense;

namespace CryEngine.Towerdefense
{
    public abstract class EffectData : IEffectControllerParams
    {
        public string Identity { get; set; }
        public virtual EffectType Type { get; }
        public float? TimeToLive { get; set; }
        public float TickRate { get; set; } = 1f;
        public bool ApplyEveryTick { get; set; }
        public bool PreventStacking { get; set; }

        /// <summary>
        /// If greater than zero, this effect will be applied across an area spanning the specified value.
        /// </summary>
        /// <value>The area of effect.</value>
        public float AreaOfEffect { get; set; }

        /// <summary>
        /// If true and the AreaOfEffect has a value, this effect will be applied using a falloff calculation.
        /// </summary>
        /// <value><c>true</c> if use falloff; otherwise, <c>false</c>.</value>
        public bool UseFalloff { get; set; }
    }

    public class EffectDamageData : EffectData
    {
        public override EffectType Type { get { return EffectType.Damage; } }
        public int Damage { get; set; }
    }

    public class EffectSlowData : EffectData
    {
        public override EffectType Type { get { return EffectType.Slow; } }
        public float SpeedModifier { get; set; }
        public bool OverridePreviousModifiers { get; set; }
    }
}
