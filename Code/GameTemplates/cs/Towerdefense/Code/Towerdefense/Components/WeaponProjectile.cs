using System;
using CryEngine.Common;
using CryEngine.EntitySystem;
using CryEngine.Towerdefense.Components;

namespace CryEngine.Towerdefense
{
    /// <summary>
    /// Fires a projectile once every tick that will apply damage upon hitting the target.
    /// </summary>
    public class WeaponProjectile : Weapon
    {
        private float projectileSpeed = 50f;

        protected override void OnInitialize()
        {
            FireRate = 0.5f;
            DamageOutput = 5;
        }

        protected override void OnFire()
        {
            var projectile = GameObject.Instantiate<Projectile>();
            projectile.Position = GameObject.Position;
            projectile.SetImpulse(Target.Position, projectileSpeed);
            projectile.Collide += OnProjectileCollision;
        }

        void OnProjectileCollision(Projectile projectile, Framework.CollisionListener.CollisionEvent collision)
        {
            if (collision.GameObject != null)
                ApplyEffect();

            projectile.Collide -= OnProjectileCollision;

            OnHit();
        }

        void ApplyEffect()
        {
            //var slowEffect = new EffectController<EffectSlow>(Target, (effect) => effect.Modifier = new UnitMovementController.MovementModifier(0.5f, true));
            //slowEffect.SetParams((param) =>
            //{
            //    param.TimeToLive = 0.5f;
            //});
            //slowEffect.ApplyEffect();

            //// AOE Damage Example. Applies damage instantly to all enemies with a 2m radius of the Target.
            //var damageEffectAoE = new EffectControllerMulti<EffectDamage>();
            //damageEffectAoE.AddEffect(TargetsInRange(2f), (effect) => effect.Damage = DamageOutput);
            //damageEffectAoE.ApplyEffects();

            // DoT Example. Applies 5 damage every 0.5 seconds to the Target over a period of 3 seconds.
            //var dotEffect = new EffectController<EffectDamage>("DoT", Target, (effect) => effect.Damage = 5);
            //dotEffect.SetParams((param) =>
            //{
            //    param.PreventStacking = true;
            //    param.TickRate = 0.5f;
            //    param.ApplyEveryTick = true;
            //    param.TimeToLive = 3f;
            //});
            //dotEffect.ApplyEffect();

            // DoT AoE example. Same as above except it applies to all enemies within a 2m radius of the Target.
            //var dotEffectAoE = new EffectControllerMulti<EffectDamage>();
            //dotEffectAoE.AddEffect(TargetsInRange(2f), (effect) => effect.Damage = DamageOutput);
            //dotEffectAoE.SetParams((param) =>
            //{
            //    param.PreventStacking = true;
            //    param.TickRate = 0.5f;
            //    param.ApplyEveryTick = true;
            //    param.TimeToLive = 3f;
            //});
            //dotEffectAoE.ApplyEffects();

            // Splash Damage AoE example
            //var splashDamageEffectAoE = new EffectControllerMulti<EffectDamage>();

            //const float radius = 2f;

            //var targets = TargetsInRange(radius);
            //targets.Add(Target);
            //targets.ForEach(target =>
            //{
            //    var falloff = GetFalloff(radius, Target.Position);

            //                    // Calculate the damage using the falloff value. The further this target is from the main target, the less damage that will be dealt.
            //                    var damage = (int)(DamageOutput * falloff);

            //    splashDamageEffectAoE.AddEffect(target, (effect) => effect.Damage = damage);
            //});
            //splashDamageEffectAoE.ApplyEffects();


        }
    }
}