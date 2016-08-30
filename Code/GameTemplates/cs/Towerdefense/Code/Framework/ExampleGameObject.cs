using System;
using CryEngine.Common;
using CryEngine.EntitySystem;
using CryEngine.Framework;

namespace CryEngine.Towerdefense
{
    public class ExampleGameObject : GameObject
    {
        CollisionListener collisionListener;

        public override void OnInitialize()
        {
            base.OnInitialize();

            collisionListener = AddComponent<CollisionListener>();
            collisionListener.OnCollide += CollisionListener_OnCollide;

            NativeEntity.LoadGeometry(0, "objects/default/primitive_sphere.cgf");

            var physParams = new SEntityPhysicalizeParams()
            {
                density = 1,
                mass = 10,
                type = (int)EPhysicalizationType.ePT_Rigid,
            };
            NativeEntity.Physicalize(physParams);

        }

        void CollisionListener_OnCollide(CollisionListener.CollisionEvent arg)
        {
            Logger.LogInfo("Collided with " + arg.GameObject.Name);
        }
    }
}

