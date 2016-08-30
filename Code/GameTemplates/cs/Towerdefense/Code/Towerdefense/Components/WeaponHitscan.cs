using CryEngine.Common;
using CryEngine.Towerdefense;

namespace CryEngine.Towerdefense
{
    /// <summary>
    /// Instantly applies damage to the target once every tick.
    /// </summary>
    public class WeaponHitscan : Weapon
    {
        protected override void OnInitialize()
        {
            FireRate = 0.1f;
            DamageOutput = 1;
        }

        protected override void OnFire()
        {
            var damageEffect = EffectController.Create<EffectDamage>(Target, (effect) => effect.Damage = DamageOutput);
        }
    }
}

