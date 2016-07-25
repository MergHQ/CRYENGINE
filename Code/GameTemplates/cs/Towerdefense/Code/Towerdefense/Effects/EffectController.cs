using System;
using System.Collections.Generic;
using System.Linq;
using CryEngine.Common;


namespace CryEngine.Towerdefense
{
    /// <summary>
    /// Controls and applies a Combat Effect, destroying itself once the effect has been fully applied.
    /// </summary>
    public class EffectController : IGameUpdateReceiver
    {
        Effect effect;
        GameObject target;
        IEffectControllerParams parameters;
        float timeAlive;
        float timeStamp;
        bool isApplying;
        bool hasApplied;

        /// <summary>
        /// Helper method which creates and returns an EffectController.
        /// </summary>
        public static EffectController Create<T>(string identity, GameObject target, Action<T> onAddEffect = null) where T : Effect
        {
            var instance = new EffectController();
            instance.Setup(identity, target, onAddEffect);
            return instance;
        }

        /// <summary>
        /// Helper method which creates and returns an EffectController.
        /// </summary>
        public static EffectController Create<T>(GameObject target, Action<T> onAddEffect = null) where T : Effect
        {
            return Create("", target, onAddEffect);
        }

        void Setup<T>(string identity, GameObject target, Action<T> onAddEffect = null) where T : Effect
        {
            if (target == null)
            {
                Destroy();
                return;
            }

            this.target = target;

            // Assign default parameters
            parameters = new EffectControllerParams();

            GameFramework.RegisterForUpdate(this);

            // Add the Effect
            effect = target.AddComponent<T>();
            onAddEffect?.Invoke(effect as T);
            effect.Initialize(identity);
        }

        void Setup<T>(GameObject target, Action<T> onAddEffect = null) where T : Effect
        {
            Setup("", target, onAddEffect);
        }

        public virtual void SetParams(Action<IEffectControllerParams> onAdd)
        {
            var parameters = new EffectControllerParams();
            onAdd(parameters);
            this.parameters = parameters;
            CheckForExistingEffects();
        }

        public virtual void SetParams(IEffectControllerParams parameters)
        {
            this.parameters = parameters;
            CheckForExistingEffects();
        }

        public virtual void SetSource(Vec3 source)
        {
            if (effect != null)
                effect.Source = source;
        }

        public virtual void ApplyEffect()
        {
            if (!CanApplyEffect())
                return;

            if (parameters != null && parameters.TimeToLive.HasValue)
            {
                isApplying = true;
            }
            else
            {
                Apply(true);
            }
        }

        public virtual void OnUpdate()
        {
            if (target == null)
                Destroy();

            if (isApplying)
            {
                // End the effect if has reached it's lifetime
                if (timeAlive > parameters.TimeToLive)
                {
                    isApplying = false;
                    effect.EndEffect(target);
                    Destroy();
                }

                // Apply the effect once every elapsed tick
                if (Global.gEnv.pTimer.GetCurrTime() > timeStamp + parameters.TickRate)
                {
                    if (hasApplied && parameters.ApplyEveryTick)
                    {
                        Apply();
                    }
                    else if (!hasApplied)
                    {
                        Apply();

                        // Prevents the effect from being applied on every tick.
                        hasApplied = true;
                    }

                    timeStamp = Global.gEnv.pTimer.GetCurrTime();
                }

                timeAlive += FrameTime.Delta;
            }
        }

        public void Destroy()
        {
            GameFramework.UnregisterFromUpdate(this);

            effect?.Remove();

            effect = null;
            target = null;
        }

        public virtual void SetEffectData(EffectData effectData)
        {
            effect?.SetData(effectData);
        }

        protected void CheckForExistingEffects()
        {
            // Prevent an effect of the same type being added if a previously attached effect of the same type is set to prevent stacking.
            if (target != null && effect.Identity != string.Empty)
            {
                var existingEffects = target.Components.OfType<Effect>().ToList();

                if (parameters.PreventStacking && existingEffects.Where(x => x != this.effect).Any(x => x.Identity == this.effect.Identity))
                {
                    Log.Warning("Preventing stacking of effect '" + this.effect.Identity + '"');
                    Destroy();
                }
            }
        }

        /// <summary>
        /// Internally applies the effect.
        /// </summary>
        /// <param name="destroy">If set to <c>true</c>, this controller will be destroyed.</param>
        void Apply(bool destroy = false)
        {
            if (target != null)
                effect.ApplyEffect(target);

            if (destroy)
                Destroy();
        }

        bool CanApplyEffect()
        {
            if (isApplying)
            {
                Log.Warning("Cannot apply effect as effect is already being applied.");
                return false;
            }

            if (target == null)
            {
                Log.Warning("Cannot apply effect as the target is null.");
                return false;
            }

            return true;
        }
    }
}

