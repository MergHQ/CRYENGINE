using System;
using CryEngine.Common;
using CryEngine.Framework;


namespace CryEngine.Towerdefense
{
    /// <summary>
    /// Controls the logic of the game, such as rewarding resources, and handling attacks made to the player's base.
    /// </summary>
    class RoundManager : GameComponent
    {
        public event EventHandler RoundOver;

        Base playerBase;
        UiHud hud;
        Resources resources;
        LevelManager levelManager;
        WaveManager waveController;

        float roundTime;
        Timer waveTimer;

        public void Setup(UiHud hud, Resources resources)
        {
            this.hud = hud;
            this.resources = resources;

            // Update the HUD when the number of resources is changed
            resources.OnResourcesChanged += (amount) => hud.Resources.Value = resources.Value;

            // Create the level manager that references the player's base, enemies, and player-placed units
            levelManager = GameObject.AddComponent<LevelManager>();

            // Create the wave controller that controls the spawning on enemies
            waveController = new WaveManager(levelManager);
            waveController.EnemyReachedBase += OnEnemyReachedBase;
            waveController.EnemyKilled += OnEnemyKilled;
            waveController.WaveComplete += OnWaveComplete;

            // Assign default waves
            waveController.Data = AssetLibrary.Data.DefaultWaveData;

            // DEBUG
            Input.OnKey += (arg) =>
            {
                if (arg.KeyDown(EKeyId.eKI_Left))
                    playerBase.Health.ApplyDamage(10);

                if (arg.KeyDown(EKeyId.eKI_Right))
                    playerBase.Health.ApplyDamage(-10);
            };
        }

        public void StartRound()
        {
            levelManager.Setup();

            if (levelManager.PlayerBase != null)
            {
                // Setup player Base and callbacks
                playerBase = levelManager.PlayerBase;
                playerBase.Health.OnDamageReceived += OnPlayerBaseReceivedDamage;

                // Update HUD when health changes
                playerBase.Health.OnHealthChanged += () => hud.Base.Health = playerBase.Health;

                // Set initial base health. Prevent the health bar from flashing when it is being modified.
                hud.Base.EnableHealthBarFlash = false;
                playerBase.Setup();
                hud.Base.EnableHealthBarFlash = true;
            }

            // Set initial resources
            resources.Set(GameSettings.InitialResources);

            // Create new wave 10 seconds after the round has started
            waveController.Start();

            // Timestamp the start time
            roundTime = Global.gEnv.pTimer.GetCurrTime();
        }

        void EndRound()
        {
            Remove();
        }

        protected override void OnDestroy()
        {
            waveController.EnemyReachedBase -= OnEnemyReachedBase;
            waveController.EnemyKilled -= OnEnemyKilled;
            waveController.WaveComplete -= OnWaveComplete;
            waveController.Destroy();
            levelManager.Remove();
            waveTimer?.Destroy();
        }

        public override void OnUpdate()
        {
            roundTime += FrameTime.Delta;

            hud.Wave.Time = roundTime;
            hud.Wave.CurrentWave = waveController.CurrentWaveId + 1;
            hud.Wave.TotalWaves = waveController.TotalWaves;

            if (waveController.CurrentWave != null)
            {
                hud.Enemies.EnemiesAlive = waveController.CurrentWave.EnemiesAlive;
                hud.Enemies.EnemiesTotal = waveController.CurrentWave.EnemiesInWave;
            }
        }

        #region Callback Handlers

        void OnWaveComplete (Wave wave)
        {
            var nextWave = waveController.NextWave;

            Log.Info("Wave completed!");
            Log.Info("That wave had " + wave.EnemiesInWave + " enemies");

            if (nextWave != null)
                Log.Info("The next wave has " + nextWave.EnemiesInWave  + " enemies");
        }

        void OnEnemyKilled (Enemy enemy)
        {
            resources.Add(10);
        }

        void OnEnemyReachedBase (Enemy enemy)
        {
            if (playerBase != null)
            {
                Log.Info("Enemy reached the base!");
                playerBase.Health.ApplyDamage(10);
            }
        }

        void OnPlayerBaseReceivedDamage(int damage)
        {
            // End the round if the base is destroyed
            if (playerBase.Health.CurrentHealth == 0)
                EndRound();
        }

        #endregion
    }
}
