using System;

namespace CryEngine.Towerdefense
{
    public interface IEffectControllerParams
    {
        float? TimeToLive { get; set; }
        float TickRate { get; set; }
        bool ApplyEveryTick { get; set; }
        bool PreventStacking { get; set; }
    }

    public class EffectControllerParams : IEffectControllerParams
    {
        /// <summary>
        /// How long the effect should exist. Leave unassigned to apply the effect instantly.
        /// </summary>
        public float? TimeToLive { get; set; }

        /// <summary>
        /// How often the effect should be applied.
        /// </summary>
        public float TickRate { get; set; } = 1f;

        /// <summary>
        /// If true, applies the effect once every tick.
        /// </summary>
        public bool ApplyEveryTick { get; set; }

        /// <summary>
        /// If true, prevents multiple effects of the same type to be applied at the same time.
        /// </summary>
        public bool PreventStacking { get; set; }
    }
}

