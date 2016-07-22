using System;
using System.Collections.Generic;
using CryEngine.Framework;

namespace CryEngine.Towerdefense
{
    public class WaveManager
    {
        public event EventHandler AllWavesComplete;
        public event EventHandler<Wave> WaveComplete;
        public event EventHandler<Enemy> EnemyReachedBase;
        public event EventHandler<Enemy> EnemyKilled;

        int currentWave;
        List<Wave> waves;
        LevelManager level;
        Timer delayTimer;

        const float delayBetweenWaves = 4f;

        public int CurrentWaveId { get { return currentWave; } }
        public Wave CurrentWave { get { return waves.Count != 0 ? waves[currentWave] : null; } }
        public Wave NextWave { get { return waves.Count != 0 && currentWave < waves.Count - 1 ? waves[currentWave + 1] : null; } }
        public int TotalWaves { get { return waves.Count; } }

        public WaveData Data
        {
            set
            {
                foreach (var wave in value.Waves)
                    AddWave(new Wave(wave));
            }
        }

        public WaveManager(LevelManager level)
        {
            this.level = level;
            waves = new List<Wave>();
        }

        public void Start()
        {
            Log.Info("Start Wave Controller");
            currentWave = -1;
            MoveToNextWave();
        }

        public void AddWave(Wave wave)
        {
            waves.Add(wave);
        }

        public void Destroy()
        {
            foreach (var wave in waves)
            {
                wave.WaveComplete -= OnWaveComplete;
                wave.Destroy();
            }

            delayTimer?.Destroy();
        }

        void MoveToNextWave()
        {
            currentWave++;

            if (currentWave < waves.Count)
            {
                var wave = waves[currentWave];
                wave.Start(level);
                wave.WaveComplete += OnWaveComplete;   
                wave.EnemyReachedBase += OnEnemyReachedBase;
                wave.EnemyKilled += OnEnemyKilled;
            }
            else
            {
                Log.Warning("Cannot move onto next wave as there are no more waves.");
            }
        }

        void OnEnemyKilled(Enemy enemy)
        {
            EnemyKilled?.Invoke(enemy);
        }

        void OnEnemyReachedBase (Enemy enemy)
        {
            EnemyReachedBase?.Invoke(enemy);
        }

        void OnWaveComplete ()
        {
            var completedWave = waves[currentWave];
            completedWave.WaveComplete -= OnWaveComplete;
            completedWave.EnemyReachedBase -= OnEnemyReachedBase;
            completedWave.EnemyKilled -= OnEnemyKilled;

            WaveComplete?.Invoke(completedWave);

            if (waves.TrueForAll(x => x.WaveCompleted))
            {
                Log.Info("All Waves Complete!");
                AllWavesComplete?.Invoke();
            }
            else
            {
                delayTimer = new Timer(delayBetweenWaves, () => MoveToNextWave());
            }
        }
    }
}

