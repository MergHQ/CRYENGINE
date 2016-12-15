// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine.Common;
using CryEngine.UI;
using CryEngine.UI.Components;
using System;

namespace CryEngine.SampleApp
{
	/// <summary>
	/// SampleApp (Hello World). Basic sample of running a Mono Application.
	/// </summary>
	public class SampleApp : IGameUpdateReceiver, IDisposable
    {
        public static SampleApp Instance { get; set; }

        private DateTime _updateFPSTime = DateTime.MinValue;
		private int _frameCount = 0;
		private Text _fpsText;

        public SampleApp()
        {
			Instance = this;
			CreateUI ();

			GameFramework.RegisterForUpdate(this);
			Input.OnKey += OnKey;
		}

		private void CreateUI()
		{
			// Canvas to contain the FPS label.
			var canvas = SceneObject.Instantiate<Canvas>(SceneManager.RootObject);

			// Create FPS Label.
			_fpsText = canvas.AddComponent<Text>();
			_fpsText.Alignment = Alignment.TopLeft;
			_fpsText.Height = 28;
			_fpsText.Offset = new Point (10, 10);
			_fpsText.Color = Color.Yellow.WithAlpha(0.5f);
		}

        public void Dispose()
        {
			Input.OnKey -= OnKey;
            GameFramework.UnregisterFromUpdate(this);
        }

        public virtual void OnUpdate()
		{
			// Update FPS Label.
			if (DateTime.Now > _updateFPSTime)
			{				
				_fpsText.Content = _frameCount + " fps";
				_frameCount = 0;
				_updateFPSTime = DateTime.Now.AddSeconds(1);
			}
			_frameCount++;
		}

		void OnKey(SInputEvent e)
		{
			if (e.KeyPressed(EKeyId.eKI_Escape) && !Engine.IsSandbox)
				Engine.Console.ExecuteString("quit");

			// Show/Hide FPS Label on F5.
			if (e.KeyPressed(EKeyId.eKI_F5))
				_fpsText.Active = !_fpsText.Active;
		}
	}
}
