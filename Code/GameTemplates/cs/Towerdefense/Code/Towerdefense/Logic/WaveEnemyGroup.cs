using System;
using System.Linq;
using System.Collections.Generic;
using CryEngine.Framework;

namespace CryEngine.Towerdefense
{
    public class WaveEnemyGroup
    {
        public event EventHandler<WaveEnemyGroup> AllEnemiesKilled;
        public event EventHandler<WaveEnemyGroup> AllSpawned;
        public event EventHandler<Enemy> EnemyKilled;
        public event EventHandler<Enemy> EnemyReachedBase;

        List<Enemy> enemies;
        List<Timer> spawnDelayTimers;
        Timer postSpawnDelay;

        const float delayBetweenSpawns = 1f;

        public bool AllEnemiesDead { get; private set; }
        public WaveEnemyGroupData Data { get; private set; }
        public int Alive { get { return enemies.Where(x => x.Health.CurrentHealth != 0).ToList().Count; } }

        public WaveEnemyGroup(WaveEnemyGroupData data)
        {
            enemies = new List<Enemy>();
            spawnDelayTimers = new List<Timer>();
            Data = data;
        }

        public void Spawn(List<SpawnPoint> spawnPoints)
        {
            if (spawnPoints.Count == 0)
            {
                Log.Error("Cannot spawn enemy as no spawn points exist.");
                return;   
            }
            
            var rnd = new Random().Next(0, spawnPoints.Count);
            var spawnPoint = spawnPoints[rnd];

            float time = 0f;
            for (int i = 0; i < Data.Amount; i++)
            {
                spawnDelayTimers.Add(new Timer(time, () => AddEnemy(spawnPoint)));
                time += delayBetweenSpawns;
            }

            if (Data.PostDelay.HasValue)
            {
                Log.Info("Waiting " + Data.PostDelay.Value + " seconds before spawning next group...");
                postSpawnDelay = new Timer(Data.PostDelay.Value, () =>AllSpawned?.Invoke(this));
            }
            else
            {
                AllSpawned?.Invoke(this);
            }
        }

        public void Destroy()
        {
            foreach (var enemy in enemies)
            {
                enemy.OnDeath -= OnEnemyDeath;   
                enemy.OnReachedPathEnd -= OnEnemyReachedPathEnd;
            }

            spawnDelayTimers.ForEach(x => x.Destroy());
            spawnDelayTimers.Clear();
            enemies.Clear();
            postSpawnDelay?.Destroy();
        }

        void AddEnemy(SpawnPoint spawnPoint)
        {
            // Spawn an enemy at a random spawn point
            UnitData unitData = null;
            switch (Data.Type)
            {
                case EnemyType.Easy:
                    unitData = AssetLibrary.Data.Enemies.Easy;
                    break;
                case EnemyType.Medium:
                    unitData = AssetLibrary.Data.Enemies.Medium;
                    break;
                case EnemyType.Hard:
                    unitData = AssetLibrary.Data.Enemies.Hard;
                    break;
                default:
                    break;
            }

            var enemy = spawnPoint.Spawn(unitData);

            enemy.OnReachedPathEnd += OnEnemyReachedPathEnd;
            enemy.OnDeath += OnEnemyDeath;
            enemies.Add(enemy);
        }

        void RemoveEnemy(Enemy enemy)
        {
            enemy.OnReachedPathEnd -= OnEnemyReachedPathEnd;
            enemy.OnDeath -= OnEnemyDeath;
            enemy.Destroy();
            enemies.Remove(enemy);

            if (enemies.Count == 0)
            {
                AllEnemiesDead = true;
                AllEnemiesKilled?.Invoke(this);
            }
        }

        void OnEnemyReachedPathEnd (Enemy enemy)
        {
            EnemyReachedBase?.Invoke(enemy);
            RemoveEnemy(enemy);
        }

        void OnEnemyDeath (Unit unit)
        {
            var enemy = unit as Enemy;
            EnemyKilled?.Invoke(enemy);
            RemoveEnemy(enemy);
        }
    }
}

