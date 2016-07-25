using System;
using System.Collections.Generic;
using System.Linq;
using CryEngine.Common;
using CryEngine.EntitySystem;

namespace CryEngine.SampleApp
{
    public class BallSpawner : BaseEntity
    {
        [EntityProperty("How often the object will spawn in seconds", EEditTypes.Float, 0.1f, 5f)]
        public float SpawnInterval { get; set; }

        [EntityProperty("The radius in which objects will be spawned", EEditTypes.Float, 1f, 16f)]
        public int Radius { get; set; }

        [EntityProperty("The scale that the objects should spawn at", EEditTypes.Integer, 0.1f, 100f, "1")]
        public float SpawnScale { get; set; }

        [EntityProperty("The maximum number of objects allowed at any one time", EEditTypes.Integer, 0, 999, "100")]
        public int Max { get; set; }

        private Timer _spawnTimer;
        private List<BaseEntity> _objects;

        public override void OnInitialize()
        {
            base.OnInitialize();
            _objects = new List<BaseEntity>();
        }

        /// <summary>
        /// This is called when the game is started or ended in Sandbox, as well as when the game is started Standalone.
        /// </summary>
        public override void OnStart()
        {
            // Create a new timer that will loop and spawn an object every time the timer ticks.
            _spawnTimer = new Timer(SpawnInterval, true, SpawnObject);
        }

        /// <summary>
        /// This is called when Sandbox playback ends, the game is stopped, or the entity is removed.
        /// </summary>
        public override void OnEnd()
        {
            base.OnEnd();
            _spawnTimer?.Destroy();
            _objects?.Clear();
        }

        public void SpawnObject()
        {
            if (_objects.Count == Max)
                return;

            // Spawn the object.
            var ball = EntityFramework.Spawn<Ball>();

            // Get a random position using the position of this ObjectSpawner as the origin.
            ball.Position = GetRandomPosition(Position, Radius, Position.z);

            // Set the scale.
            ball.Scale = Vec3.One * SpawnScale;

            // Store a reference to the ball so we can keep track of how many have been spawned.
            _objects.Add(ball);
        }

        private Vec3 GetRandomPosition(Vec3 origin, int radius, float minZ)
        {
            var rnd = new Random();

            var x = rnd.Next(-radius, radius);
            var y = rnd.Next(-radius, radius);
            var z = rnd.Next(-radius, radius);

            var newVec = origin + new Vec3(x, y, z);

            return new Vec3(newVec.x, newVec.y, MathExtensions.Clamp(z, minZ, z));
        }
    }
}