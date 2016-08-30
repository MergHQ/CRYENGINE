using System;
using CryEngine.Common;
using CryEngine.Framework;
using CryEngine.Framework.Components;
using CryEngine.Framework.Interfaces;

namespace CryEngine.SampleApp
{
    public class KillPlane : GameObject, ICollisionHandler
    {
        float height = -8f;

        public float Height
        {
            set
            {
                Transform.Position = new Vec3(0, 0, height);
            }
        }

        public override void OnAwake()
        {
            base.OnAwake();

            Scale = new Vec3(256, 256, 1);
            Position = new Vec3(0f, 0f, height);

            var physParams = new SEntityPhysicalizeParams()
            {
                mass = -1,
                density = -1,
                type = (int)EPhysicalizationType.ePT_Rigid,
            };

            Entity.BaseEntity.Physicalize(physParams);

            AddComponent<Collider>();
        }

        public override string GetMesh()
        {
            return "objects/default/cube.cgf";
        }

        public override string GetMaterial()
        {
            return Materials.GreyMin;
        }

        public void OnCollide(GameObject gameObject)
        {
            gameObject.Destroy();
        }
    }
}
