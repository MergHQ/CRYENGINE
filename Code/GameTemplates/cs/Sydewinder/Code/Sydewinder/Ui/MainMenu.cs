// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.IO;
using CryEngine.Common;
using CryEngine.Components;
using CryEngine.UI;
using CryEngine;
using CryEngine.UI.Components;
using CryEngine.Resources;
using CryEngine.EntitySystem;

namespace CryEngine.Sydewinder.UI
{
public sealed class MainMenu : SceneObject
{
	public event EventHandler StartClicked;
	public event EventHandler ExitClicked;

	private static Window     _mainMenuWindow;
	private static Canvas     _mainMenuPage;
	private static Button     _startButton;
	private static Button     _highscoreButton;
	private static Button     _exitButton;

	private static Canvas     _highscorePage;
	private static Table      _highscoreTable;

	#region Variables to have a smooth camera movement while switching between main menu and highscore
	private static Vec3  _positionChangeOnEachUpdate = null;
	private static Vec3  _fwdDirectionChangeOnEachUpdate = null;
	private static float _numberOfRemainingFrames = 0;
	#endregion

	private const int           TEXTURE_RESOLUTION = 768;

	private static List<Canvas> _menuPages = new List<Canvas>();

	private static MainMenu     _instance;

	public static MainMenu GetMainMenu(SceneObject root)
	{
		if (_instance == null)
			_instance = SceneObject.Instantiate<UI.MainMenu>(root);
		return _instance;
	}

	public static void SelectedMenuPage(int index)
	{
		SetInactive();
		_menuPages[index].Active = true;
	}

	public static void SetInactive()
	{
		foreach(var page in _menuPages)
		{
			page.CurrentFocus = null;
			page.Active = false;
		}
	}

	public void OnAwake()
	{
		CreateMainMenu();
		CreateHighscoreMenu();

		_menuPages.Add(_mainMenuPage);
		_menuPages.Add(_highscorePage);

		// Apply common properties (style) to main menu and highscore menu.
		ApplyMenuStyle();

		Input.OnKey += OnKey;
	}

	private void OnKey(SInputEvent arg)
	{
		if (_highscorePage.Active)
		{
			if (arg.deviceType == EInputDeviceType.eIDT_Gamepad && arg.state == EInputState.eIS_Pressed && arg.keyId == EKeyId.eKI_XI_B)
			{
				SetInactive();
				SetupMainMenuPerspective(true);
			}
		}
	}

	private void CreateMainMenu()
	{
		_mainMenuPage = SceneObject.Instantiate<Canvas>(Root);

		// Get Entity placed in level.
		Entity ent = Entity.ByName("UIPlane");
		_mainMenuPage.SetupTargetEntity(ent, TEXTURE_RESOLUTION);

		_mainMenuWindow = SceneObject.Instantiate<Window>(_mainMenuPage);

		_startButton = SceneObject.Instantiate<Button>(_mainMenuWindow);
		_startButton.RectTransform.Padding = new Padding(0, -140);
		_startButton.Ctrl.Text.Content = "Start";
		_startButton.Ctrl.OnPressed += () =>
		{
			SetInactive();
			if (StartClicked != null)
				StartClicked();
		};

		_highscoreButton = SceneObject.Instantiate<Button>(_mainMenuWindow);
		_highscoreButton.RectTransform.Padding = new Padding(0, 10);
		_highscoreButton.Ctrl.Text.Content = "Highscore";
		_highscoreButton.Ctrl.OnPressed += () =>
		{
			SetInactive();
			SetupHighscoreMenuPerspective(true);
		};

		_exitButton = SceneObject.Instantiate<Button>(_mainMenuWindow);
		_exitButton.RectTransform.Padding = new Padding(0, 160);
		_exitButton.Ctrl.Text.Content = "Exit";
		_exitButton.Ctrl.OnPressed += () =>
		{
			SetInactive();
			if (ExitClicked != null)
				ExitClicked();
		};

		_mainMenuWindow.ForEach<Button>(x => x.RectTransform.Alignment = Alignment.Center);
	}

	private void CreateHighscoreMenu()
	{
		_highscorePage = SceneObject.Instantiate<Canvas>(Root);

		// Get plane entity for highscore menu item.
		_highscorePage.SetupTargetEntity(Entity.ByName("UIPlane_Highscore"), TEXTURE_RESOLUTION);

		Window highscoreWindow = SceneObject.Instantiate<Window>(_highscorePage);

		_highscoreTable = SceneObject.Instantiate<Table>(highscoreWindow);
		_highscoreTable.RightAlignLastColumn = true;
		_highscoreTable.RectTransform.Alignment = Alignment.Stretch;
		_highscoreTable.RectTransform.Padding = new Padding(70, 70, 70, 0);

		Button backButton = SceneObject.Instantiate<Button>(highscoreWindow);
		backButton.RectTransform.Padding = new Padding(0, -115);
		backButton.Ctrl.Text.Content = "Back";
		backButton.RectTransform.Alignment = Alignment.Bottom;
		backButton.Ctrl.OnPressed += () =>
		{
			SetInactive();
			SetupMainMenuPerspective(true);
		};
	}

	private void ApplyMenuStyle()
	{
		string backgroundImageURL = Path.Combine(Application.DataPath, "Textures/ui/scroller_window_512.png");
		string buttonBackgroundURL = Path.Combine(Application.DataPath, "Textures/ui/Ui_button.png");
		string buttonHighlightedURL = Path.Combine(Application.DataPath, "Textures/ui/Ui_button_selected.png");

		Root.ForEach<Window>(x =>
		{
				x.Caption = "Sydewinder";
				x.Background.Source = ResourceManager.ImageFromFile(backgroundImageURL);
				x.CaptionHeight = 40;
				x.RectTransform.Size = new Point(TEXTURE_RESOLUTION, TEXTURE_RESOLUTION);
				x.RectTransform.Alignment = Alignment.Center;

				// Apply common style to all buttons within the window.
				x.ForEach<Button>(b =>
				{
					b.RectTransform.Size = new Point(300, 120);
					b.Ctrl.Text.Height = 48;
					b.BackgroundImageUrl = buttonBackgroundURL;
					b.BackgroundImageInvertedUrl = buttonHighlightedURL;
					b.Ctrl.OnPressed += () => AudioManager.PlayTrigger("menu_select");
					b.Ctrl.OnFocusEnter += () => AudioManager.PlayTrigger("menu_switch");
				});
		  });
	}

	public static void SetupMainMenuPerspective(bool animated = false)
	{
		SelectedMenuPage(0);

		Vec3 finalPosition = new Vec3(86, 37, 75);
		Entity MenuPlaneEntity = Entity.ByName("UIPlane");
		if (MenuPlaneEntity != null)
		{
			Vec3 finalFwdDirection = MenuPlaneEntity.Position - finalPosition;
			if (animated)
			{
				// Animated camera move to main menu.
				StartCameraMove(1, finalPosition, finalFwdDirection);
			}
			else
			{
				Camera.Current.Position = finalPosition;
				Camera.Current.ForwardDirection = finalFwdDirection;
			}
		}
		else
		{
			Global.CryLog("[C# MainMenu] Can't find entity named 'UIPlane'");
		}
	}

	public static void SetupHighscoreMenuPerspective(bool animated = false)
	{
		SelectedMenuPage(1);

		// Set final position and view direction.
		Vec3 finalPosition = new Vec3(68, 81, 73);
		Vec3 finalFwdDirection = Entity.ByName("UIPlane_Highscore").Position - finalPosition;

		if (animated)
		{
			// Animated camera move to main menu.
			StartCameraMove(1, finalPosition, finalFwdDirection);
		}
		else
		{
			Camera.Current.Position = finalPosition;
			Camera.Current.ForwardDirection = finalFwdDirection;
		}

		// Load highscore into table and display it on the UI.
		List<string[]> contents = new List<string[]>();
		foreach(GameData gd in Highscore.CurrentScore.ScoreList)
		contents.Add(new string[] { string.IsNullOrEmpty(gd.Name) ? "[ noname ]" : gd.Name, gd.Score.ToString() });

		_highscoreTable.DataSource = new TableData(new string[] { "Name", "Score" }, contents);

		// 220 = Padding above of 50 + Padding below of 50 + Button Height of 120.
		(_highscoreTable.Parent as Window).RectTransform.Height = _highscoreTable.ContentHeight + 240;
	}

	private static void StartCameraMove(int seconds, Vec3 finalPosition, Vec3 finalFordwardDirection)
	{
		// Calculate number of frames until 2 seconds are elapsed based on the last frame time.
		_numberOfRemainingFrames = (int)((float)seconds / FrameTime.Delta);

		// Set values to be updated on each frame.
		var cam = Camera.Current;
		_positionChangeOnEachUpdate = (finalPosition - cam.Position) / _numberOfRemainingFrames;
		_fwdDirectionChangeOnEachUpdate = (finalFordwardDirection - cam.ForwardDirection) / _numberOfRemainingFrames;
	}

	public void OnUpdate()
	{
		if (_numberOfRemainingFrames > 0)
		{
			var cam = Camera.Current;
			cam.Position += _positionChangeOnEachUpdate;
			cam.ForwardDirection += _fwdDirectionChangeOnEachUpdate;
			_numberOfRemainingFrames--;
		}
	}

	public void OnDestroy()
	{
		Input.OnKey -= OnKey;

		_mainMenuPage.Destroy();
		_startButton.Destroy();
		_highscoreButton.Destroy();
		_exitButton.Destroy();

		_highscorePage.Destroy();
		_highscoreTable.Destroy();

		_mainMenuWindow.Destroy();
		_menuPages.Clear();
	}

	public static void DestroyMenu()
	{
		if (_instance != null)
		{
			_instance.Destroy();
			_instance = null;
		}
	}
}
}
