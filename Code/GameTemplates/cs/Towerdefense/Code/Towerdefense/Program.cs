// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.IO;
using System.Reflection;
using System.Xml;
using System.Xml.Serialization;
using CryEngine.Resources;
using CryEngine.Components;
using CryEngine.FlowSystem;
using CryEngine.Common;

namespace CryEngine.Towerdefense
{
	class EyeTrackerControl : Mouse.IMouseOverride, IGameUpdateReceiver
	{
		private uint _hitEntityID;
		private Vec2 _hitEntityUV = Vec2.Zero;
		private int _eyeX;
		private int _eyeY;
		private bool _leftPressed;

		public uint HitEntityID { get { return _hitEntityID; } }
		public Vec2 HitEntityUV { get { return _hitEntityUV; } }
		public event Mouse.MouseEventHandler LeftButtonDown;
		public event Mouse.MouseEventHandler LeftButtonUp;
		public event Mouse.MouseEventHandler RightButtonDown;
		public event Mouse.MouseEventHandler RightButtonUp;
		public event Mouse.MouseEventHandler Move;

		public EyeTrackerControl()
		{
			GameFramework.RegisterForUpdate(this);
			Mouse.SetOverride (this);
			Input.OnKey += (e) => 
			{
				if (e.KeyDown(EKeyId.eKI_LCtrl) && !_leftPressed)
				{
					LeftButtonDown(_eyeX, _eyeY);
					_leftPressed = true;
				}
				if (e.KeyUp(EKeyId.eKI_LCtrl) && _leftPressed)
				{
					LeftButtonUp(_eyeX, _eyeY);
					_leftPressed = false;
				}
			};
		}

		public void OnUpdate()
		{
			var eyePos = Input.GetAxis ("EyeTracker");
			if (eyePos != null) 
			{				
				_eyeX = (int)(eyePos.x * Screen.Width);
				_eyeY = (int)(eyePos.y * Screen.Height);
				Mouse.HitScenes (_eyeX, _eyeY);
				Move (_eyeX, _eyeY);
			}
		}
	}

	/// <summary>
	/// Add-in entry point will be re-instantiated in runtime, whenever the assembly is updated (e.g. Re-compiled)
	/// </summary>
	public class Program : ICryEngineAddIn
	{
        Game app;
        UiFrontend frontEnd;

        public void Initialize(InterDomainHandler handler)
		{
			app = Application.Instantiate<Game>();
			new EyeTrackerControl ();
		}

        public void StartGame()
        {                
            // Override default draw distance ratio
            Env.Console.ExecuteString("e_ViewDistRatio 1000");

            // Add debug UI
            SceneObject.Instantiate<UiDebug>(null);

            // Create the Frontend UI
            frontEnd = new UiFrontend();
            frontEnd.StartGamePressed += () =>
            {
                app.LoadLevel(AssetLibrary.Levels.GridTest);
                frontEnd.Hide();
            };
            frontEnd.DebugPressed += () => 
            {
                app.LoadLevel(AssetLibrary.Levels.Empty);
                frontEnd.Hide();
            };
            frontEnd.QuitGamePressed += Shutdown;

            // Assign game over callback
            app.GameOver += () => 
            {
                frontEnd.Show();
            };

            Mouse.ShowCursor();
        }

        public void EndGame()
        {
            
        }

		/// <summary>
		/// Will be called if a RunScene FlowNode was triggered insode game.
		/// </summary>
		/// <param name="scene">A wrapper object containing the FlowNode's data.</param>
		public void OnFlowNodeSignal(FlowNode node, PropertyInfo signal)
		{
		}

		public void Shutdown()
		{
			app.Shutdown(false);
		}
	}
}
