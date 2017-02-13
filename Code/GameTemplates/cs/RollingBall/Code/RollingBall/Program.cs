// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

namespace CryEngine.RollingBall
{
	/// <summary>
	/// Add-in entry point will be re-instantiated in runtime, whenever the assembly is updated (e.g. Re-compiled)
	/// </summary>
	public class MyPlugin : ICryEnginePlugin
	{
		public void Initialize()
		{
			new RollingBall();
		}

		public void OnLevelLoaded()
		{			
		}

		public void OnGameStart()
		{			
		}

		public void OnGameStop()
		{
		}

		public void Shutdown()
		{
			RollingBall.Instance.Dispose();
		}
	}
}
