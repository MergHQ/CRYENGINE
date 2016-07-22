using System;
using CryEngine.Common;
using CryEngine.EntitySystem;
using CryEngine.Framework;

namespace CryEngine.Towerdefense
{
    public class Projectile : GameObject
    {
        Timer timeToLiveTimer;
        CollisionListener collisionListener;

        public event EventHandler<Projectile, CollisionListener.CollisionEvent> Collide;

        public override void OnInitialize()
        {
            base.OnInitialize();

            NativeEntity.LoadGeometry(0, "objects/default/primitive_sphere.cgf");
            Scale = Vec3.One * 0.5f;
            collisionListener = AddComponent<CollisionListener>();
            collisionListener.OnCollide += OnCollide;
        }

        public void SetImpulse(Vec3 target, float speed, float timeToLive = 10f)
        {
            Physics.Physicalize(5, EPhysicalizationType.ePT_Rigid);
            Physics.Action<pe_action_set_velocity>((impulse) =>
            {
                impulse.v = (target - Position).Normalized * speed;
            });
            timeToLiveTimer = new Timer(timeToLive, () => Destroy());
        }

        void OnCollide(CollisionListener.CollisionEvent arg)
        {
            Collide?.Invoke(this, arg);
            collisionListener.OnCollide -= OnCollide;
            timeToLiveTimer.Destroy();
            Destroy();
        }
    }
}

