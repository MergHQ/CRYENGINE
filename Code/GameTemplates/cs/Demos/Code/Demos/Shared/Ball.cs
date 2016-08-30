using CryEngine.Common;
using CryEngine.EntitySystem;

namespace CryEngine.SampleApp
{
    public class Ball : BaseEntity
    {
        public override void OnInitialize()
        {
            base.OnInitialize();

            // Load default geometry.
            NativeEntity.LoadGeometry(0, "objects/default/primitive_sphere.cgf");

            // Load default material.
            NativeEntity.SetMaterial("materials/matref_chrome128.mtl");

            // Physicalize.
            Physics.Physicalize(10, EPhysicalizationType.ePT_Rigid);
        }
    }
}