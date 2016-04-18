// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Linq;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using System.IO;
using CryEngine;
using System.ServiceModel;
using System.Reflection;
using CryEngine.FlowSystem;

namespace CryEngine.MonoLauncher
{
	/// <summary>
	/// Stores CRYENGINE state for restoration after an application run.
	/// </summary>
	class EngineState
	{
		private float _fov;

		public void Save()
		{
			_fov = Components.Camera.FieldOfView;
		}

		public void Load()
		{
			Components.Camera.FieldOfView = _fov;
		}
	}

	/// <summary>
	/// Entry Point to Mono world, called from CryMonoBridge on CryEngine startup.
	/// </summary>
    public class App
    {
		private	static AddInManager _addInManager;
		private static EngineState EngineState = new EngineState();
		private static bool _appRunningInEditor = false;

		public static void Initialize()
		{
			Debug.Log ("Launcher Initializing...");
			RegisterFlowNodes ();
			_addInManager = new AddInManager ();

			SystemHandler.Instantiate ();
			SystemHandler.EditorGameStart += StartEditorGame;
			SystemHandler.EditorGameEnded += EndEditorGame;
			AddInManager.AppUnloaded += () =>
			{
				if (_appRunningInEditor)
					Env.Console.ExecuteString("ed_disable_game_mode");
			};
		}

		private static void StartEditorGame()
		{
			EngineState.Save ();
			_appRunningInEditor = true;
			_addInManager.BootTime = DateTime.Now;
		}

		private static void EndEditorGame()
		{
			EngineState.Load ();
			_appRunningInEditor = false;
			_addInManager.UnloadApplication ();
		}

		public static void Shutdown()
		{
			SystemHandler.Destroy ();

			Debug.Log("Launcher Shutdown...");
			_addInManager.Deinitialize ();
		}

		private static void RegisterFlowNodes()
		{
			FlowNodeFactory<RunScene>.Register ();
		}
    }
}
