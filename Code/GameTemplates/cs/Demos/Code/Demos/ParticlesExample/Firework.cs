using CryEngine.Common;
using CryEngine.EntitySystem;
using System;
using System.Collections.Generic;

namespace CryEngine.SampleApp
{
    public class Firework : BaseEntity
    {
        [EntityProperty("The minimum lifetime of the firework", EEditTypes.Float, 0.1f, 999f, "1")]
        public float MinTime { get; set; }

        [EntityProperty("The maximum lifetime of the firework", EEditTypes.Float, 0.1f, 999f, "2")]
        public float MaxTime { get; set; }

        public override void OnInitialize()
        {
            base.OnInitialize();

            // Load default geometry.
            NativeEntity.LoadGeometry(0, "objects/default/primitive_sphere.cgf");

            // Load default material.
            NativeEntity.SetMaterial("materials/matref_chrome128.mtl");

            // Physicalize.
            Physics.Physicalize(5, EPhysicalizationType.ePT_Rigid);
        }

        public void Launch()
        {
            GetParticleEffect("particle_trail", (pfx) => NativeEntity.LoadParticleEmitter(1, pfx));

            // Launch the firework upwards.
            Physics.Action<pe_action_impulse>((action) =>
            {
                action.impulse = Rotation.Forward * 100f;
            });

            var rnd = new Random().Next((int)MinTime * 100, (int)MaxTime * 100) / 100f;

            // Start a timer that will trigger the explosion.
            new Timer(rnd, () =>
            {
                // Spawn the explosion effect then remove this entity.
                GetParticleEffect("particle_explosion", (pfx) => pfx.Spawn(Position));
                Remove();
            });
        }

        public IParticleEffect GetParticleEffect(string name, Action<IParticleEffect> onGet = null)
        {
            var particle = Env.ParticleManager.FindEffect(string.Format("libs/particles/{0}.pfx", name));

            if (particle == null)
            {
                Log.Error("Failed to find particle '{0}'", name);
            }
            else
            {
                if (onGet != null)
                    onGet(particle);

                return particle;
            }

            return null;
        }
    }
}