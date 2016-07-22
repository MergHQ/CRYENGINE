using System;
using System.Linq;
using System.Collections.Generic;
using CryEngine.Common;
using CryEngine.EntitySystem;

namespace CryEngine.Towerdefense
{
    public class SpawnPoint : BaseEntity
    {
        public EventHandler<Enemy> OnEnemySpawned;
        public EventHandler<Enemy> OnEnemyDespawned;

        List<Enemy> enemies;
        Path pathToFollow;

        public override void OnInitialize()
        {
            base.OnInitialize();
        }

        public SpawnPoint(Path pathToFollow)
        {
            this.pathToFollow = pathToFollow;
            enemies = new List<Enemy>();
        }

        /// <summary>
        /// Spawns an enemy at this spawn point.
        /// </summary>
        public Enemy Spawn(UnitData enemyData)
        {
            var enemy = Factory.CreateUnit(enemyData) as Enemy;

            if (pathToFollow != null && pathToFollow.Points.Count != 0)
            {
                enemy.Position = pathToFollow.Points[0];
            }
            else
            {
                Log.Warning("Spawn Point has no assigned Path...");
            }

            enemy.MoveAlongPath(pathToFollow);
            enemies.Add(enemy);

            // Remove the enemy once killed
            enemy.OnDeath += OnEnemyDeath;

            OnEnemySpawned?.Invoke(enemy);

            return enemy;
        }

        void OnEnemyDeath (Unit unit)
        {
            var enemy = unit as Enemy;
            enemy.OnDeath -= OnEnemyDeath;
            enemies.Remove(enemy);
            OnEnemyDespawned?.Invoke(enemy);
        }

        /// <summary>
        /// Removes any enemies associated with this spawn that are still alive.
        /// </summary>
        public void Cleanup()
        {
            enemies.ToList().ForEach(x =>
            {
                OnEnemyDespawned?.Invoke(x);
                x.Destroy();
            });
            enemies.Clear();
        }
    }
}

