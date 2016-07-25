using System;
using CryEngine.Common;
using CryEngine.Framework;


namespace CryEngine.Towerdefense
{
    public abstract class Effect : GameComponent
    {
        protected Vec3 source;

        public string Identity { get; private set; }
        public Vec3 Source
        {
            get
            {
                return source;
            }
            set
            {
                source = value;
            }
        }

        protected EffectData Data { get; private set; }

        public void Initialize(string identity)
        {
            Identity = identity;
        }

        public virtual void ApplyEffect(GameObject target)
        {
            if (target == null)
                return;

            OnApplyEffect(target);
        }

        public void EndEffect(GameObject target)
        {
            if (target == null)
                return;

            OnEndEffect(target);
        }

        public void SetData(EffectData effect)
        {
            Data = effect;
            OnSetData(effect);
        }

        /// <summary>
        /// Returns a normalized value from 1 to 0, with 1 being close to the source, and 0 being far from the source.
        /// </summary>
        /// <returns>The falloff.</returns>
        /// <param name="radius">Radius.</param>
        /// <param name="source">Source.</param>
        protected float GetFalloff(float radius, Vec3 target)
        {
            if (Source == null)
            {
                Log.Error("Source not set");
                return 1f;
            }
            else
            {
                var distance = target.GetDistance(Source);
                return distance > 0f ? MathEx.Clamp(radius / distance, 0f, 1f) : 1f;   
            }
        }

        protected abstract void OnApplyEffect(GameObject target);
        protected virtual void OnEndEffect(GameObject target) { }
        protected virtual void OnSetData(EffectData effect) { }
    }
}

