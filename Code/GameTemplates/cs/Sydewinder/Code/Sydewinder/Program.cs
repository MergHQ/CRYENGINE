// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Reflection;
using CryEngine.Resources;
using CryEngine.Components;
using CryEngine.FlowSystem;
using CryEngine.Common;

namespace CryEngine.Sydewinder
{
	/// <summary>
	/// Plugin Entry point will be re-instantiated in runtime, whenever the assembly is updated (e.g. Re-compiled).
	/// </summary>
	public class Program : ICryEngineAddIn
	{
		private const string HIGHSCORE_URL = "highscore.json";

		public static SydewinderApp GameApp { get; private set; }

		/// <summary>
		/// Add-In Entry point.
		/// </summary>
		public void Initialize()
		{
			if (Env.IsSandbox)
				return;
			
			Env.Console.ExecuteString ("map Canyon");
			GameApp = Application.Instantiate<SydewinderApp>();

			// Initialize Highscore with file name.
			Highscore.InitializeFromFile(HIGHSCORE_URL);

			GameApp.GameOver += showHighscore =>
			{
				// Reset field of view to ehance 3D look.
				Camera.Current.FieldOfView = 60f;

				UI.MainMenu.SetInactive();
				if (showHighscore)
					UI.MainMenu.SetupHighscoreMenuPerspective ();
				else 
					UI.MainMenu.SetupMainMenuPerspective ();
			};

			InitializeMainMenu();
		}

		private void InitializeMainMenu()
		{
			var mainMenu = UI.MainMenu.GetMainMenu(GameApp.Root);
			UI.MainMenu.SelectedMenuPage (0);

			mainMenu.StartClicked += () => 
			{
				Env.Mouse.DecrementCounter ();

				GameApp.Reset ();
				UI.MainMenu.SetInactive ();
			};
			mainMenu.ExitClicked += () => 
			{
				Env.Mouse.DecrementCounter ();

				if (!Env.IsSandbox)
					GameApp.Shutdown ();

				GameApp.ThemeSongPlayer.Stop();
				mainMenu.Destroy ();
			};

			UI.MainMenu.SetInactive ();
			UI.MainMenu.SetupMainMenuPerspective ();
		}

		/// <summary>
		/// Not used in this application. Implementation required by interface.
		/// </summary>
		public void OnFlowNodeSignal(FlowNode node, PropertyInfo signal)
		{ 
		}

		/// <summary>
		/// Called when engine is being shut down or if application is reloaded.
		/// </summary>
		public void Shutdown()
		{
			if (GameApp != null)
				GameApp.Shutdown(false);
		}
	}
}
