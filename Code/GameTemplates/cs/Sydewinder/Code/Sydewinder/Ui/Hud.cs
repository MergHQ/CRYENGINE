// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using CryEngine.Components;
using CryEngine.UI.Components;
using CryEngine.UI;
using System.Globalization;

namespace CryEngine.Sydewinder
{
	public delegate void ExitGameEventHandler(string username, double score, bool saveScore);

	/// <summary>
	/// The Head Up Display class shows in game stats while playing
	/// </summary>
	public class Hud : Component
	{
		public event EventHandler ResumeGameClicked;
		public event ExitGameEventHandler GameExited;

		public const string FontName = "Consolas";
		public const byte HudFontHeight = 35;

		// The stats can be changed from outside without any knowledge how UI elements are represented.
		public double Score { get; private set; }
		public int Energy { get; private set; }
		public int Stage { get; private set; }
		public WeaponBase Weapon { get; private set; }

		private Canvas _canvas = null;
		private Color _backgroundColor = null;
		private Color _foregroundColor = null;

		public static Hud CurrentHud { get; private set; }

		// The stats are currently simple Text elements but might be changed
		// into more interactive elements in the future.
		private Text _scoreText;
		private Text _eneryText;
		private Text _stageText;
		private Text _weaponText;

		private Panel _gameOverDialogPanel;
		private Panel _gamePausedDialogPanel;
		private TextInput _nameTextInput;

		public static void InitializeMainHud(SceneObject root, Color backgroundColor, Color foregroundColor)
		{
			CurrentHud = new Hud(root, backgroundColor, foregroundColor);
		}

		private Hud(SceneObject root, Color backgroundColor, Color foregroundColor)
		{	
			_canvas = SceneObject.Instantiate<Canvas>(root);

			_backgroundColor = backgroundColor;
			_foregroundColor = foregroundColor;

			// Init Top Panel
			Panel panelTop = SceneObject.Instantiate<Panel>(_canvas);
			panelTop.Background.Color = _backgroundColor; 
			panelTop.RectTransform.Alignment = Alignment.TopHStretch;
			panelTop.RectTransform.Size = new Point(Screen.Width, 120);	

			_scoreText = panelTop.AddComponent<Text>();
			_scoreText.Alignment = Alignment.BottomLeft;
			_scoreText.Offset = new Point(10, -10);
			SetScore(0);

			_stageText = panelTop.AddComponent<Text>();
			_stageText.Alignment = Alignment.BottomRight;
			_stageText.Offset = new Point(-10, -10);
			SetStage(0);

			// Init Bottom Panel
			Panel panelBottom = SceneObject.Instantiate<Panel>(_canvas);
			panelBottom.Background.Color = _backgroundColor;
			panelBottom.RectTransform.Alignment = Alignment.BottomHStretch;
			panelBottom.RectTransform.Size = new Point(Screen.Width, 120);

			_eneryText = panelBottom.AddComponent<Text>();
			_eneryText.Alignment = Alignment.TopLeft;
			_eneryText.Offset = new Point(10, 10);
			SetEnergy(100);

			_weaponText = panelBottom.AddComponent<Text>();
			_weaponText.Alignment = Alignment.TopRight;
			_weaponText.Offset = new Point(-10, 10);
			SetWeapon(Weapon);

			_gamePausedDialogPanel = CreateDialogPanel();
			_gameOverDialogPanel = CreateDialogPanel();

			Text gameOverText = _gameOverDialogPanel.AddComponent<Text>();
			gameOverText.FontName = FontName;
			gameOverText.Alignment = Alignment.Center;
			gameOverText.Offset = new Point(0, -Screen.Height*0.15f);
			gameOverText.Content = "Game Over";

			Text nameText = _gameOverDialogPanel.AddComponent<Text>();
			nameText.FontName = FontName;
			nameText.Alignment = Alignment.Left;
			nameText.Offset = new Point(30, Screen.Height*0.15f);
			nameText.Content = "Your name:";

			_nameTextInput = SceneObject.Instantiate<TextInput>(_gameOverDialogPanel);
			_nameTextInput.RectTransform.Size = new Point(Screen.Width * 0.35f, 50);
			_nameTextInput.RectTransform.Alignment = Alignment.Center;
			_nameTextInput.RectTransform.Padding = new Padding(0, Screen.Height*0.15f);
			_nameTextInput.Text.Content = Environment.UserName;
			_nameTextInput.Ctrl.OnSubmit += (s, e) => 
			{ 
				// To ensure the text is not cleared and shown again next time
				e.Handled = true;
				ConfirmNameEntry();
			};

			Button confirmNameButton = SceneObject.Instantiate<Button>(_gameOverDialogPanel);
			confirmNameButton.RectTransform.Padding = new Padding(-90, Screen.Height*0.15f);
			confirmNameButton.Ctrl.Text.Content = "OK";
			confirmNameButton.Ctrl.OnPressed += () => { ConfirmNameEntry(); };

			Text gamePausedText = _gamePausedDialogPanel.AddComponent<Text>();
			gamePausedText.FontName = FontName;
			gamePausedText.Alignment = Alignment.Center;
			gamePausedText.Offset = new Point(0, -Screen.Height*0.15f);
			gamePausedText.Content = "Game Paused";

			Button resumeGameButton = SceneObject.Instantiate<Button>(_gamePausedDialogPanel);
			resumeGameButton.RectTransform.Padding = new Padding(-Screen.Width*0.25f, Screen.Height*0.15f);
			resumeGameButton.Ctrl.Text.Content = "Resume Game";
			resumeGameButton.Ctrl.OnPressed += () => 
			{ 
				if (ResumeGameClicked != null) ResumeGameClicked (); 
			};

			Button exitGameButton = SceneObject.Instantiate<Button>(_gamePausedDialogPanel);
			exitGameButton.RectTransform.Padding = new Padding(Screen.Width*0.25f, Screen.Height*0.15f);
			exitGameButton.Ctrl.Text.Content = "Exit Game";
			exitGameButton.Ctrl.OnPressed += () => 
			{ 
				if (GameExited != null)
					GameExited (_nameTextInput.Text.Content, Score, false); 
			};

			_canvas.ForEachComponent<Text>(c =>
			{
				c.Color = _foregroundColor;
				c.FontName = FontName;
				c.Height = HudFontHeight;
			});

			// Overwrite previously set Text values
			gameOverText.Height = 128;
			gamePausedText.Height = 128;


			// Apply shared style between the 'Resume Game' and 'Exit Game' buttons
			_canvas.ForEach<Button>(x => 
			{
				x.RectTransform.Size = new Point(Screen.Width * 0.26f, Screen.Width * 0.13f);
				x.Background.Color = Color.FromRGB(0,0,0.5f).WithAlpha(0.5f);
				x.RectTransform.Alignment = Alignment.Center;
				x.Ctrl.Text.Height = 32;
			});

			// Overwrite previously set Button values
			confirmNameButton.RectTransform.Alignment = Alignment.Right;
			confirmNameButton.RectTransform.Size = new Point(120, 60);

			Hide();
		}

		public void IncreaseScore(double increment)
		{
			SetScore(Score + increment);
		}

		public void SetScore(double score)
		{
			Score = score;
			_scoreText.Content = string.Format("Score: {0}", Score.ToString ("N0", CultureInfo.GetCultureInfo ("de-DE")));
		}

		public void SetStage(int stage)
		{
			Stage = stage;
			_stageText.Content = string.Format("Level: {0}", stage);
		}

		public void SetEnergy(int energy)
		{
			Energy = energy;
			_eneryText.Content = string.Format("Energy: {0}", energy);
		}

		public void SetWeapon(WeaponBase weapon)
		{
			Weapon = weapon;
			_weaponText.Content = string.Format("Weapon: {0}", Weapon != null ? weapon.ToString() : "Gun");
		}

		public void ShowGameOverDialog()
		{
			_gameOverDialogPanel.Active = true;

			// Directly allow keyboard input by focusing input field
			_canvas.CurrentFocus = _nameTextInput.Ctrl;
		}

		public void ShowGamePauseDialog()
		{
			Mouse.ShowCursor ();
			_gamePausedDialogPanel.Active = true;
		}

		public void HideGamePauseDialog()
		{
			Mouse.HideCursor ();
			_gamePausedDialogPanel.Active = false;
		}

		public void Show()
		{
			_canvas.Active = true;
		}

		public void Hide()
		{
			_canvas.Active = false;
			_gameOverDialogPanel.Active = false;
			_gamePausedDialogPanel.Active = false;
		}

		private void Reset()
		{
			SetScore(0);
			SetEnergy(0);
			SetStage(1);
			SetWeapon(null);
		}

		private Panel CreateDialogPanel()
		{
			Panel dialog = SceneObject.Instantiate<Panel>(_canvas);
			dialog.Background.Color = _backgroundColor;
			dialog.RectTransform.Alignment = Alignment.Center;
			dialog.RectTransform.Size = new Point((Screen.Width * 0.9f), (Screen.Height * 0.66f));

			return dialog;
		}

		private void ConfirmNameEntry()
		{
			if (GameExited != null)
				GameExited(_nameTextInput.Text.Content, Score, true); 
		}
	}
}

