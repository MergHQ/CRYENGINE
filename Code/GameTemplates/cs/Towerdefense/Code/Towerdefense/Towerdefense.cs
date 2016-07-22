// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Collections.Generic;
using CryEngine.UI;
using CryEngine.UI.Components;
using CryEngine.Resources;
using CryEngine.Common;
using CryEngine.FlowSystem;
using CryEngine.Components;

namespace CryEngine.Towerdefense
{
	/// <summary>
	/// Towerdefense
	/// </summary>
	public class Towerdefense : Application
	{
        LevelManager levelManager;
        RoundManager roundManager;
        Resources resources;
        UiHud hud;

		public virtual void OnAwake()
		{
            if (!Env.IsSandbox)
                LevelSystem.LoadingComplete += LevelSystem_LoadingComplete;

            Mouse.ShowCursor();

            Env.Console.ExecuteString("map Main");

//            SceneObject.Instantiate<GridTest>(Root);
        }

        void LevelSystem_LoadingComplete (EventArgs<ILevelInfo> arg)
        {
            // Create objects and inject dependencies
            levelManager = new LevelManager();
            hud = new UiHud(Root);
            resources = new Resources();
            new ObjectPlacer(resources);

            StartGame();
        }

        void StartGame()
        {
            levelManager.Setup();
            roundManager = new RoundManager(hud, levelManager, resources);
            roundManager.OnGameOver += EndGame;
            roundManager.StartNewRound();
        }

        void EndGame ()
        {
            Global.gEnv.pLog.LogToConsole("Game Over!");
            roundManager.OnGameOver -= EndGame;
            levelManager.Cleanup();
            StartGame();
        }
    }
}
