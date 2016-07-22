using System;
using System.Linq;
using System.Collections.Generic;
using CryEngine.Framework;

using CryEngine.Common;
using CryEngine.Towerdefense.Components;

namespace CryEngine.Towerdefense
{
    /// <summary>
    /// Base class for Weapons. A Weapon will fire once every tick.
    /// </summary>
    public abstract class Weapon : GameComponent
    {
        float timeStamp = 0f;
        GameObject target;
        Vec3 cachedPosition;
        bool hasFired;

        public float FireRate { get; set; } = 0.1f;
        public int DamageOutput { get; set; } = 5;
        public List<EffectData> OnHitEffects { get; set; }
        public List<GameObject> PotentialTargets { protected get; set; }

        public GameObject Target
        {
            get
            {
                return target != null && target.Exists ? target : null;
            }
            set
            {
                target = value;
                timeStamp = Global.gEnv.pTimer.GetCurrTime();
            }
        }

        protected override void OnInitialize()
        {
            PotentialTargets = new List<GameObject>();
        }

        public void ClearTarget()
        {
            Target = null;
        }

        public override void OnUpdate()
        {
            // Fire once every tick
            if (Target != null && (!hasFired || Global.gEnv.pTimer.GetCurrTime() > timeStamp + FireRate))
            {
                cachedPosition = GameObject.Position;
                OnFire();
                timeStamp = Global.gEnv.pTimer.GetCurrTime();
                hasFired = true;
            }
        }

        protected void OnHit()
        {
            // Loop through all the OnHitEffects that are specified in the associated Tower's Weapon data.
            OnHitEffects.ForEach(x =>
            {
                EffectController effectController = null;

                if (x.AreaOfEffect == 0f)
                {
                    switch (x.Type)
                    {
                        case EffectType.Damage: effectController = EffectController.Create<EffectDamage>(x.Identity, Target); break;
                        case EffectType.Slow: effectController = EffectController.Create<EffectSlow>(x.Identity, Target); break;
                    }
                }
                else
                {
                    switch (x.Type)
                    {
                        case EffectType.Damage: effectController = EffectControllerMulti.Create<EffectDamage>(TargetsInRange(x.AreaOfEffect)); break;
                        case EffectType.Slow: effectController = EffectControllerMulti.Create<EffectSlow>(TargetsInRange(x.AreaOfEffect)); break;
                    }
                }

                effectController.SetSource(cachedPosition);
                effectController.SetEffectData(x);
                effectController.SetParams(x);
                effectController.ApplyEffect();
            });
        }

        protected List<GameObject> TargetsInRange(float range)
        {
            if (target.Exists)
            {
                return PotentialTargets
                    .Where(x => x.Exists)
                    .Where(x => x.Position.GetDistance(target.Position) <= range).ToList();
            }
            else
            {
                return new List<GameObject>();
            }
        }

        protected virtual void OnFire() { }
    }
}

