using System;
using System.Collections.Generic;
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

        private Timer _spawnTimer;
        private EntityPicker _picker;
        private UserInterface _ui;

        /// <summary>
        /// This is called when the game is started or ended in Sandbox, as well as when the game is started Standalone.
        /// </summary>
        public override void OnStart()
        {
            // Cleanup existing instances.
            if (_picker != null)
                _picker.Picked -= Picker_Picked;
            
            _picker?.Destroy();
            _spawnTimer?.Destroy();
            _ui?.Destroy();

            // Create a new instance of EntityPicker.
            _picker = new EntityPicker();
            _picker.Picked += Picker_Picked;

            // Create a new timer that will loop and spawn an object every time the timer ticks.
            _spawnTimer = new Timer(SpawnInterval, true, SpawnObject);

            // Create the UI.
            _ui = SceneObject.Instantiate<UserInterface>(null);
        }

        public void SpawnObject()
        {
            // Spawn the object.
            var ball = EntityFramework.Spawn<Ball>("SpawnedEntity");

            // Get a random position using the position of this ObjectSpawner as the origin.
            ball.Position = GetRandomPosition(Position, Radius, 40);

            // Get a random scale.
            ball.Scale = GetRandomScale(0.5f, 2f);

            Log.Info<BallSpawner>("Name: {0}", ball.Name);
        }

        private void Picker_Picked(BaseEntity entity)
        {
            _ui.EntityInfo = entity;
            Env.EntitySystem.RemoveEntity(entity.Id);
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

        private Vec3 GetRandomScale(float min, float max)
        {
            var rnd = new Random();

            int minAsInt = (int)(min * 10);
            int maxAsInt = (int)(max * 10);

            float scalar = rnd.Next(minAsInt, maxAsInt) / 10f;

            return new Vec3(scalar, scalar, scalar);
        }
    }
}