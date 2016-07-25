using CryEngine.Common;
using CryEngine.EntitySystem;
using CryEngine.Components;

namespace CryEngine.SampleApp
{
    public class ProjectileSpawner : BaseEntity
    {
        [EntityProperty("How fast the projectile should travel.", EEditTypes.Float, 0.1f, 999f, "50")]
        public float Velocity { get; set; }

        [EntityProperty("How large the impact radius should be.", EEditTypes.Float, 0.1f, 999f, "2")]
        public float ImpactRadius { get; set; }

        [EntityProperty("The force of the impact.", EEditTypes.Float, 0.1f, 999f, "20")]
        public float ImpactForce { get; set; }

        public override void OnStart()
        {
            base.OnStart();
            Mouse.OnLeftButtonDown += Mouse_OnLeftButtonDown;
            Mouse.ShowCursor();

            if (Camera.Current == null)
                Camera.Current = SceneObject.Instantiate(null).AddComponent<Camera>();
        }

        public override void OnEnd()
        {
            base.OnEnd();
            Mouse.OnLeftButtonDown -= Mouse_OnLeftButtonDown;
        }

        private void Mouse_OnLeftButtonDown(int x, int y)
        {
            // Get the direction of the mouse relative to the world.
            var mouseDir = Camera.Unproject(x, y);

            // Spawn a ball projectile slightly in front of the current mouse position.
            var projectile = EntityFramework.Spawn<Ball>();
            projectile.Position = Camera.Current.Position + (mouseDir * 4f);

            // Physicalize the ball.
            projectile.Physics.Physicalize(5, EPhysicalizationType.ePT_Rigid);

            // Apply a physics action to set the ball's velocity.
            projectile.Physics.Action<pe_action_set_velocity>((action) =>
            {
                action.v = mouseDir * Velocity;
            });

            // Get (or add) a collision listener component then hook into the OnCollide event.
            projectile.GetOrAddComponent<EntityCollisionListener>().OnCollide += Projectile_OnCollide;
        }

        private void Projectile_OnCollide(EntityCollisionListener.CollisionEvent arg)
        {
            var health = arg.Entity != null ? arg.Entity.GetComponent<Health>() : null;
            if (health != null)
            {
                health.ApplyDamage(60f);
            }
            else
            {
                Log.Warning("Target has no Health component");
            }

            // Remove the OnCollide callback.
            arg.Sender.OnCollide -= Projectile_OnCollide;

            // Remove the entity that sent this event. This will be the projectile that collided.
            arg.Sender.Entity.Remove();
        }

        //private void Projectile_OnCollide(EntityCollisionListener.CollisionEvent arg)
        //{
        //    // Create an impact effect.
        //    var impactEffect = EntityFramework.Spawn<Earthquake>();
        //    impactEffect.MinAmplitude = ImpactForce;
        //    impactEffect.MaxAmplitude = ImpactForce;
        //    impactEffect.Radius = ImpactRadius;
        //    impactEffect.Position = arg.Point;
        //    impactEffect.ApplyInAir = true;

        //    // Shake, then remove the earthquake upon completion.
        //    impactEffect.Shake(() => impactEffect.Remove());

        //    // Remove the OnCollide callback.
        //    arg.Sender.OnCollide -= Projectile_OnCollide;

        //    // Remove the entity that sent this event. This will be the projectile that collided.
        //    arg.Sender.Entity.Remove();
        //}
    }
}
