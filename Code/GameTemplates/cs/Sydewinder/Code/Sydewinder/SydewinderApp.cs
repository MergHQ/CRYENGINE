// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Collections.Generic;
using System.Runtime.Serialization;
using System.Linq;
using CryEngine.Common;
using CryEngine.Components;
using CryEngine.Sydewinder.Types;
using CryEngine.Sydewinder.UI;
using CryEngine.UI;
using CryEngine.UI.Components;
using CryEngine.EntitySystem;

namespace CryEngine.Sydewinder
{
	public enum GameState
	{
		Running,
		Paused,
		Finished
	}

	public class SydewinderApp : Application
	{
		public event EventHandler<bool> GameOver;

		public GameState State { get; private set; }

		// Create random enemies every x s.
		private const int SPAWN_EVERY_SECONDS = 3;

		private GameData _gameData;

		/// <summary>
		/// The player spaceship (controlled by the player).
		/// </summary>
		private Player _player = null;

		/// <summary>
		/// The spawn timer will count up to SPAWN_EVERY_SECONDS and will be reset after spawning enemy.
		/// </summary>
		private float _spawnTimer = 0;

		// Total game time can be used for various purposes.
		// For now we are using this to increase the degree of difficulty.
		private float _totalGameTime;

		/// <summary>
		/// When the player has been destroyed, marks the time when the game was over.
		/// </summary>
		private DateTime _gameOverTime = DateTime.MinValue;

		private Vec3 _cameraPositionBeforePause = null;
		private Vec3 _cameraForwardDirectionBeforePause = null;

		/// <summary>
		/// Internally handles the level geometry (tunnels, doors, ...).
		/// </summary>
		private LevelGeometry _levelElements = null;

		public void ResumeGame()
		{
			State = GameState.Running;
			Camera.Current.Position = _cameraPositionBeforePause;
			Camera.Current.ForwardDirection = _cameraForwardDirectionBeforePause;
		}

		public virtual void OnAwake()
		{
			AudioManager.PlayTrigger ("game_start");

			// Initialize highscore functionality.
			_gameData = new GameData();

			// _Cry3DEngine_cs_SWIGTYPE_p_ITimeOfDay.SetTime won't work - therefore use console to set time to 7 am.
			Env.Console.ExecuteString("e_TimeOfDay 24");

			// We are in space, so use (almost) zero gravity (zero gravity would disable physics)
			Env.Console.ExecuteString("p_gravity_z 0.001");

			// Hook on to Key input.
			Input.OnKey += Input_OnKey;

			Camera.Current = Root.AddComponent<Camera>();
			Camera.Current.OnPlayerEntityAssigned += c => UI.MainMenu.SetupMainMenuPerspective();

			_totalGameTime = 0;
			State = GameState.Finished;
		}

		public void Reset()
		{
			Camera.FieldOfView = 20f;

			// Init Head Up Display
			InitializeHud();
			Hud.CurrentHud.Show();

			_gameData.Score = 0;
			_gameOverTime = DateTime.MinValue;

			// Searches for inital tunnel in level and initializes movement of other tunnel elements.
			_levelElements = new LevelGeometry();

			_player = Player.Create(new Vec3(65, 633, 69));

			// Set camera forth/back based on FOV.
			Camera.Current.Position = new Vec3(145f, 635f, 70f);
			Camera.Current.ForwardDirection = new Vec3(-1f, 0f, 0f);
			State = GameState.Running;
		}

		private void InitializeHud()
		{
			Hud.InitializeMainHud(Root, Color.SkyBlue.WithAlpha(0.5f), Color.White);
			Hud.CurrentHud.ResumeGameClicked += () => 
			{ 
				Hud.CurrentHud.HideGamePauseDialog();
				State = GameState.Running;
			};
			Hud.CurrentHud.GameExited += ExitRunningGame;
		}

		public void OnDestroy()
		{
			AudioManager.PlayTrigger("game_stop");

			// Un-Hook Key input event.
			Input.OnKey -= Input_OnKey;

			GamePool.Clear();

			// Store highscore.
			//Highscore.CurrentScore.StoreToFile();
		}

		public virtual void OnUpdate()
		{
			// Ensures physics won't kick in while paused.
			if (State == GameState.Paused)
				GamePool.UpdatePoolForPausedGame();

			if (State != GameState.Running)
				return;

			// Increase level every 30 seconds.
			_totalGameTime += FrameTime.Delta;
			Hud.CurrentHud.SetStage((int)_totalGameTime / 30 + 1);

			// Go Game Over if player destroyed.
			if (_player.IsAlive == false) 
			{
				// Avoid counting scores.
				State = GameState.Paused;

				if (_gameOverTime == DateTime.MinValue) 
				{
					_gameOverTime = DateTime.Now;

					Mouse.ShowCursor ();
					Hud.CurrentHud.ShowGameOverDialog();
				} 
			}

			// Handle dynamic spawning of tunnels, doors and lights (does not handle tunnel movement).
			_levelElements.UpdateGeometry();

			// Check Key input and update player speed.
			if (_player.Exists) 
			{
				_player.CheckCoolDown();
				_player.UpdateSpeed();

				// Rotation while moving makes the game more dynamic.
				// Might be explained in an advanced tutorial.
				_player.UpdateRotation(); 
			}

			// Update position of game objects including player.
			GamePool.UpdatePool();

			Hud.CurrentHud.SetScore(_gameData.Score);

			// Add frame time and check if greater than or equal timer.
			_spawnTimer += FrameTime.Delta;
			if(_spawnTimer >= SPAWN_EVERY_SECONDS)
			{				
				// Reset spawn timer.
				_spawnTimer = 0;

				int waveType = Rand.NextInt(3);
				int enemyType = Rand.NextInt(3);

				// Increase speed for each difficulty level.
				Vec3 waveSpeed = new Vec3 (0f, -11f - (Hud.CurrentHud.Stage * 2), 0f);

				if ((Enemy.WaveType)waveType == Enemy.WaveType.VerticalLine) 
				{ 
					Enemy.SpawnWave (new Vec3 (65, 660, 79), enemyType, Enemy.WaveType.VerticalLine, waveSpeed);
				}
				else if((Enemy.WaveType)waveType == Enemy.WaveType.HorizontalLine)
				{
					var waveOffset = Rand.NextInt(3);
					Enemy.SpawnWave(new Vec3(65, 664, 72 + waveOffset), enemyType, Enemy.WaveType.HorizontalLine, waveSpeed);
					Enemy.SpawnWave(new Vec3(65, 664, 64 + waveOffset), enemyType, Enemy.WaveType.HorizontalLine, waveSpeed);
				}
				else if((Enemy.WaveType)waveType == Enemy.WaveType.Diagonal)
					Enemy.SpawnWave (new Vec3 (65, 660, 79), enemyType, Enemy.WaveType.Diagonal, waveSpeed);
			}
		}

		/// <summary>
		/// Makes the score.
		/// </summary>
		/// <returns>The new score value.</returns>
		/// <param name="scoreIncrement">Score increment.</param>
		public double MakeScore(double scoreIncrement)
		{
			if(State == GameState.Running)
				_gameData.Score += scoreIncrement;
			return _gameData.Score;
		}

		private void Input_OnKey(SInputEvent e)
		{
			// Commands which are only allowed when game is currently running and not paused.
			if (State == GameState.Running)
			{
				// Fire primary player weapon.
				if ((e.KeyPressed (EKeyId.eKI_Space) || e.KeyPressed (EKeyId.eKI_XI_A)) && _player.Exists) 
					_player.Fire();
			}

			if ((State != GameState.Finished) && (e.KeyPressed(EKeyId.eKI_Escape) || e.KeyPressed(EKeyId.eKI_XI_Start))) 
			{
				if (_gameOverTime == DateTime.MinValue)
				{
					State = State == GameState.Paused ? GameState.Running : GameState.Paused;

					if (State == GameState.Paused)
						Hud.CurrentHud.ShowGamePauseDialog();
					else
						Hud.CurrentHud.HideGamePauseDialog();
				}
			}
		}

		private void ExitRunningGame(string name, double score, bool saveScore)
		{
			if (saveScore)
				Highscore.CurrentScore.TryAddScore(new GameData() { Score = score, Name = name });

			_totalGameTime = 0;
			Hud.CurrentHud.Hide();
			GamePool.Clear();

			State = GameState.Finished;
			if (GameOver != null)
				GameOver(saveScore);
		}
	}
}
