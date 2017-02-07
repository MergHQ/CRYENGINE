// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

namespace CryEngine.Sydewinder
{
	/// <summary>
	/// Plugin Entry point will be re-instantiated in runtime, whenever the assembly is updated (e.g. Re-compiled).
	/// </summary>
	public class Program : ICryEnginePlugin
    {
		public void Initialize()
        {
            if (!Engine.IsSandbox)
                Engine.Console.ExecuteString("map Canyon");
        }

		public void OnLevelLoaded()
		{
		}

        public void OnGameStart()
        {
			SydewinderApp.Instance = new SydewinderApp();
        }

		public void OnGameStop()
		{
            //Only clean up the data if the game will be stopped while playing in editor, so the game won't crash if restarted
            if (SydewinderApp.Instance != null && SydewinderApp.Instance.State != GameState.Finished)
            {
                    SydewinderApp.Instance.State = GameState.Finished;
                    SydewinderApp.Instance.HUD.Hide();
                    GamePool.Clear();
            }

			AudioManager.PlayTrigger("game_stop");
			UI.MainMenu.DestroyMenu();
		}

		public void Shutdown()
		{
			SydewinderApp.Instance.Dispose();
			UI.MainMenu.DestroyMenu();
		}
    }
}
