using System;
using System.Linq;
using System.Collections.Generic;
using CryEngine.Common;
using CryEngine.Framework;

namespace CryEngine.Towerdefense
{
    public class Wave
    {
        public event EventHandler WaveComplete;
        public event EventHandler<Enemy> EnemyKilled;
        public event EventHandler<Enemy> EnemyReachedBase;

        LevelManager level;
        int currentGroup;
        List<WaveEnemyGroup> enemyGroups;

        public int EnemiesAlive { get { return enemyGroups.Select(x => x.Alive).Sum(); } }
        public int EnemiesInWave { get { return enemyGroups.Select(x => x.Data.Amount).Sum(); } }
        public bool WaveCompleted { get; private set; }
        public WaveGroupData Data
        {
            set
            {
                foreach (var group in value.EnemyGroups)
                    AddGroup(new WaveEnemyGroup(group));
            }
        }

        public Wave()
        {
            enemyGroups = new List<WaveEnemyGroup>();
        }

        public Wave(WaveGroupData data) : this()
        {
            Data = data;
        }

        public void Start(LevelManager level)
        {
            this.level = level;
            currentGroup = -1;
            Log.Info("Start Wave");
            MoveToNextGroup();
        }

        public void End()
        {
            WaveComplete?.Invoke();
        }

        public void AddGroup(WaveEnemyGroup enemyGroup)
        {
            enemyGroups.Add(enemyGroup);
        }

        public void Destroy()
        {
            foreach (var enemyGroup in enemyGroups)
            {
                enemyGroup.AllEnemiesKilled -= OnAllEnemiesKilled;
                enemyGroup.AllSpawned -= OnAllEnemiesSpawned;
                enemyGroup.AllEnemiesKilled -= OnAllEnemiesKilled;
                enemyGroup.EnemyReachedBase -= OnEnemyReachedPathEnd;
                enemyGroup.EnemyKilled -= OnEnemyDeath;
                enemyGroup.Destroy();
            }

            enemyGroups.Clear();
        }

        void MoveToNextGroup()
        {
            Log.Info("Move to next group");

            currentGroup++;

            if (currentGroup < enemyGroups.Count)
            {
                var enemyGroup = enemyGroups[currentGroup];
                enemyGroup.Spawn(level.SpawnPoints.ToList());
                enemyGroup.AllSpawned += OnAllEnemiesSpawned;
                enemyGroup.AllEnemiesKilled += OnAllEnemiesKilled;
                enemyGroup.EnemyReachedBase += OnEnemyReachedPathEnd;
                enemyGroup.EnemyKilled += OnEnemyDeath;
            }
        }

        void OnAllEnemiesKilled(WaveEnemyGroup enemyGroup)
        {
            Log.Info("All enemies in current group killed");
            enemyGroup.AllEnemiesKilled -= OnAllEnemiesKilled;

            if (enemyGroups.TrueForAll(x => x.AllEnemiesDead))
            {
                WaveCompleted = true;
                WaveComplete?.Invoke();   
            }
        }

        void OnAllEnemiesSpawned(WaveEnemyGroup enemyGroup)
        {
            Log.Info("All enemies in current group spawned");
            enemyGroup.AllSpawned -= OnAllEnemiesSpawned;
            MoveToNextGroup();
        }

        void OnEnemyReachedPathEnd(Enemy enemy)
        {
            EnemyReachedBase?.Invoke(enemy);
        }

        void OnEnemyDeath (Unit unit)
        {
            var enemy = unit as Enemy;
            EnemyKilled?.Invoke(enemy);
        }

    }
}
