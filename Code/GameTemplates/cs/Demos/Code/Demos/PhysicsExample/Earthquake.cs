using System;
using System.Linq;
using CryEngine.Common;
using CryEngine.EntitySystem;

namespace CryEngine.SampleApp
{
    [EntityClass("Earthquake", "Environment", null, "TagPoint.bmp")]
    public class Earthquake : BaseEntity
    {
        [EntityProperty]
        public bool StartActive { get; set; }

        [EntityProperty("How often the earthquake should occur.", EEditTypes.Float, 0.01f, 999f, "1")]
        public float Frequency { get; set; }

        [EntityProperty]
        public float Radius { get; set; }

        [EntityProperty("", EEditTypes.Float, 0.1f, 999f, "1")]
        public float MinAmplitude { get; set; }

        [EntityProperty("", EEditTypes.Float, 0.1f, 999f, "2")]
        public float MaxAmplitude { get; set; }

        [EntityProperty]
        public float RandomOffset { get; set; }

        [EntityProperty]
        public bool Visualize { get; set; }

        public bool ApplyInAir { get; set; }

        private Timer _timer;
        private Vec3 _position;

        public override void OnInitialize()
        {
            base.OnInitialize();
            _position = Vec3.Zero;
        }

        public override void OnStart()
        {
            base.OnStart();

            if (StartActive)
                _timer = new Timer(Frequency, true, () => Shake());
        }

        public override void OnEnd()
        {
            base.OnEnd();
            _timer?.Destroy();
        }

        public override void OnUpdate(float frameTime, PauseMode pauseMode)
        {
            base.OnUpdate(frameTime, pauseMode);

            // Display a visualizer.
            if (Visualize)
                Env.AuxRenderer.DrawCylinder(_position, Vec3.Up, Radius, 0.25f, new ColorB(0, 255, 0, 100));
        }

        public void Shake(Action onShake = null)
        {
            // Get a random position and amplitude.
            _position = GetRandomPosition(Position, RandomOffset);
            var amplitude = GetAmplitude(MinAmplitude, MaxAmplitude);

            // Get all affectors within our radius.
            var affectors = EntityFramework.GetEntities<Ball>()
                .Where(x => x.Position.GetDistance(_position) < Radius)
                .ToList();

            affectors.ForEach(entity =>
            {
                bool apply = false;

                if (!ApplyInAir)
                {
                    // Raycast from each affector to test if it's grounded.
                    var rayOut = RaycastHelper.Raycast(entity.Position, Vec3.Down, entity.Scale.y);

                    // If the ray hit something, it must close enough to the ground to be considered grounded.
                    apply = rayOut.Intersected && !rayOut.HasHitEntity(entity);
                }
                else
                {
                    apply = true;
                }

                // Add an impulse to each object using our position and amplitude.
                if (apply)
                {
                    entity.Physics.Action<pe_action_impulse>((p) =>
                    {
                        p.point = _position;
                        p.impulse = Vec3.Up * amplitude;
                    });
                }
            });

            if (onShake != null)
                onShake();
        }

        Vec3 GetRandomPosition(Vec3 origin, float offset)
        {
            if (offset != 0)
            {
                var rndX = new Random((int)offset).Next(-1000, 1000);
                var rndY = new Random((int)offset + 1).Next(-1000, 1000);
                var x = origin.x + (rndX / 1000f) * offset;
                var y = origin.y + (rndY / 1000f) * offset;
                var z = origin.z;
                return new Vec3(x, y, z);
            }
            return origin;
        }

        float GetAmplitude(float min, float max)
        {
            min = MathExtensions.Clamp(min, min, max);
            max = MathExtensions.Clamp(max, min, max);
            return new Random().Next((int)min, (int)max);
        }
    }
}