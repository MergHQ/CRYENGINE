// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine.Common;
using CryEngine.UI;
using CryEngine.UI.Components;
using System;

namespace CryEngine.RollingBall
{
	/// <summary>
	/// Example of a rolling ball physics game.
	/// </summary>
	public class RollingBall : IDisposable
	{
		public static RollingBall Instance { get; private set; }

		public RollingBall()
		{
			Instance = this;

			Input.OnKey += OnKey;

			//Only move to the map if we're not in the sandbox. The sandbox can open the map all by itself.
			if(!Engine.IsSandbox)
			{
				Engine.Console.ExecuteString("map example", false, true);
			}
		}

		public void Dispose()
		{
			Input.OnKey -= OnKey;
		}

		void OnKey(SInputEvent e)
		{
			if(e.KeyPressed(EKeyId.eKI_Escape) && !Engine.IsSandbox)
			{
				Engine.Console.ExecuteString("quit");
			}
		}
	}
}
