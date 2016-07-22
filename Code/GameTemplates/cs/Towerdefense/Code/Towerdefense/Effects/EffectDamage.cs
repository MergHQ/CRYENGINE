using System;
using CryEngine.Common;
using CryEngine.Towerdefense.Components;

namespace CryEngine.Towerdefense
{
    public class EffectDamage : Effect
    {
        int damage;

        public int Damage
        {
            set
            {
                damage = value;
            }
        }

        protected override void OnApplyEffect(GameObject target)
        {
            var healthComp = target.GetComponent<Health>();
            if (healthComp != null)
            {
                Log.Info("Applying Damage");

                if (Data != null)
                {
                    var damageData = Data as EffectDamageData;
                    if (damageData != null)
                    {
                        if (damageData.UseFalloff)
                        {
                            var falloff = GetFalloff(damageData.AreaOfEffect, target.Position);

                            // Calculate the damage using the falloff value. The further this target is from the main target, the less damage that will be dealt.
                            damage = (int)(damageData.Damage * falloff);

                            Log.Info("Apply {0} damage based on falloff of {1}", damage, falloff);
                        }
                        else
                        {
                            damage = damageData.Damage;
                        }

                        healthComp.ApplyDamage(damage);
                    }
                }
            }
        }
    }
}

