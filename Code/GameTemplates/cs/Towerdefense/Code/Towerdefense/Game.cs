// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Collections.Generic;
using CryEngine.UI;
using CryEngine.UI.Components;
using CryEngine.Resources;
using CryEngine.Common;
using CryEngine.FlowSystem;
using CryEngine.Components;
using CryEngine.Framework;

namespace CryEngine.Towerdefense
{
	/// <summary>
	/// Towerdefense
	/// </summary>
	public class Game : Application
	{
        public event EventHandler GameOver;
       
        GameObject host;
        Resources resources;
        UiHud hud;
        RoundManager roundManager;

        public void OnAwake()
        {
            // Assign callback that will setup the game once level loading has completed
            LevelSystem.LoadingComplete += OnLevelLoadComplete;

            // Add the class that provides mouse interaction with the world
            new WorldMouse();

            // DEBUG
            Input.OnKey += (arg) => 
            {
                if (arg.KeyPressed(EKeyId.eKI_F))
                {
                    EndGame();
                }
            };
        }

        /// <summary>
        /// Loads the specified level and begins the game.
        /// </summary>
        /// <param name="mapName">Map name.</param>
        public void LoadLevel(string mapName = "main")
        {
            Env.Console.ExecuteString("map " + mapName);
        }

        void OnLevelLoadComplete (EventArgs<ILevelInfo> arg)
        {
            // Create objects and inject dependencies
            host = GameObject.Instantiate<GameObject>();

            resources = new Resources();
            hud = SceneObject.Instantiate<UiHud>(Root);

            var inventory = host.AddComponent<Inventory>();
            inventory.Setup(resources);

            roundManager = host.AddComponent<RoundManager>();
            roundManager.Setup(hud, resources);
            roundManager.RoundOver += EndGame;

            new ManagedEntityTest();

            StartGame();
        }

        void StartGame()
        {
            roundManager.StartRound();
        }

        void EndGame()
        {
            Global.gEnv.pLog.LogToConsole("Game Over!");

            roundManager.RoundOver -= EndGame;

            host.Destroy();
            hud.Destroy();

            roundManager = null;
            resources = null;
            hud = null;

            GameOver?.Invoke();
        }
    }
}
