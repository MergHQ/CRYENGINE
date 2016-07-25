using System;
using System.Collections.Generic;
using CryEngine.Common;
using CryEngine.Components;
using CryEngine.Framework;
using CryEngine.Framework.Components;
using CryEngine.Framework.Interfaces;

namespace CryEngine.SampleApp
{
    public class Ball : GameObject, ICollisionHandler, IMouseDown, IMouseEnter, IMouseLeave
    {
        string matNormalPath = "materials/white.mtl";
        string matHighlightedPath = "materials/yellow.mtl";

        public override void OnAwake()
        {
            base.OnAwake();
            AddComponent<Collider>();

            Entity.BaseEntity.SetMaterial(Utils.GetMaterial(matNormalPath));

            var physParams = new SEntityPhysicalizeParams()
            {
                mass = 100,
                density = 100,
            };

            Entity.BaseEntity.Physicalize(physParams);
        }

        public override string GetMesh()
        {
            return "objects/default/primitive_sphere.cgf";
        }

        public void OnCollide(GameObject collider)
        {

        }

        void IMouseDown.OnMouseDown(MouseButton button)
        {
            Debug.Log("OnMouseDown!");

            if (button == MouseButton.Left)
            {
                var physImpulse = new pe_action_set_velocity()
                {
                    v = new Vec3(Vec3.Up * 15f),
                };
                Physics.Action(physImpulse);
            }
            else
            {
                Destroy();
            }
        }

        void IMouseEnter.OnMouseEnter()
        {
            Debug.Log("OnMouseEnter");
            Entity.BaseEntity.SetMaterial(Utils.GetMaterial(matHighlightedPath));
        }

        void IMouseLeave.OnMouseLeave()
        {
            if (!FlaggedForDestruction)
                Entity.BaseEntity.SetMaterial(Utils.GetMaterial(matNormalPath));
        }
    }
}
