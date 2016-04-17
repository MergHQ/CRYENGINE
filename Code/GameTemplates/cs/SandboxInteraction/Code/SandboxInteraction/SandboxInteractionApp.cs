// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Collections.Generic;
using CryEngine.UI;
using CryEngine.UI.Components;
using CryEngine.Resources;
using CryEngine.Common;
using CryEngine.FlowSystem;
using CryEngine.Components;
using System.Reflection;

namespace CryEngine.SandboxInteraction
{
	/// <summary>
	/// SandboxInteractionApp shows how to initialize and modify the Camera position, as well as basic FlowNode interaction.
	/// </summary>
	public class SandboxInteractionApp : Application
	{
		private Canvas _canvas;
		private Text _log;
		private FlowScene<RunScene> _currentScene;

		public void OnAwake ()
		{
			_canvas = SceneObject.Instantiate<Canvas>(Root);
			_log = _canvas.AddComponent<Text> ();
			_log.Alignment = Alignment.TopLeft;

			Camera.Current = Owner.AddComponent<Camera> ();
			Camera.Current.OnPlayerEntityAssigned += InitCamera;

			SystemHandler.EditorGameStart += () => 
			{
				Debug.Log ("Editor Game Started");
				InitCamera (Camera.Current);
			};
			SystemHandler.EditorGameEnded += EditorGameEnded;

			Program.OnSignal += OnFlowNodeSignal;
		}

		void InitCamera(Camera cam)
		{
			var planeEntity = EntitySystem.Entity.ByName ("ThePlane");
			var stance = new Vec3 (0.1f, -1.5f, 4.5f) / 4;
			cam.Position = planeEntity.Position + stance;
			cam.ForwardDirection = -stance;
		}

		void EditorGameEnded()
		{
			Debug.Log ("Editor Game Ending");

			if (_currentScene != null)
				_currentScene.Cleanup ();
			_currentScene = null;
		}

		public void OnUpdate()
		{
			var underMouse = _canvas.ComponentUnderMouse;
			_log.Content = "Mouse-Focus: " + (underMouse == null ? "[None]" : underMouse.GetType().Name);
		}

		void OnFlowNodeSignal(FlowNode node, PropertyInfo signal)
		{
			if (node.GetType () != typeof(RunScene) || signal.Name != "OnStart")
				return;
			var runScene = node as RunScene;

			if (runScene.SceneName == "ChoiceDialog") 
			{
				_currentScene = new ChoiceDialogScene (runScene, _canvas);
				_currentScene.OnSignalQuit += (n) => 
				{
					_currentScene = null;
					if (n.Return == "Exit")
					{
						if (Env.IsSandbox)
							Env.Console.ExecuteString("ed_disable_game_mode");
						else 							
							Shutdown();
					}
				};
			}
			else if (runScene.SceneName == "GoCloseup")
			{
				var targetEntity = Global.gEnv.pEntitySystem.FindEntityByName ("ThePlane");
				_currentScene = new CameraGoToScene (runScene, targetEntity, new Vec3 (0.6f, -1.5f, 3.5f) / 6);
				_currentScene.OnSignalQuit += (n) => _currentScene = null;
			}
		}
	}

	/// <summary>
	/// Args-Port: Title = Window Title
	/// Output-Port: Clicked Button Action
	/// </summary>
	public class ChoiceDialogScene : FlowScene<RunScene>
	{
		Window _window;
		Button _goToButton;
		Button _exitButton;

		public ChoiceDialogScene(RunScene node, Canvas canvas) : base(node)
		{
			_window = SceneObject.Instantiate<Window>(canvas);
			_window.Caption = Node.GetValue("Title");
			_window.RectTransform.Alignment = Alignment.Center;
			_window.RectTransform.Size = new Point(240, 140);

			_goToButton = SceneObject.Instantiate<Button>(_window);
			_goToButton.Background.Color = Color.Gray;
			_goToButton.RectTransform.Alignment = Alignment.Bottom;
			_goToButton.RectTransform.Size = new Point (140, 35);
			_goToButton.RectTransform.Padding = new Padding (0, -80);
			_goToButton.Ctrl.Text.Content = "Go Closeup";
			_goToButton.Ctrl.OnPressed += OnGoToPressed;

			_exitButton = SceneObject.Instantiate<Button>(_window);
			_exitButton.Background.Color = Color.Gray;
			_exitButton.RectTransform.Alignment = Alignment.Bottom;
			_exitButton.RectTransform.Size = new Point (140, 35);
			_exitButton.RectTransform.Padding = new Padding (0, -40);
			_exitButton.Ctrl.Text.Content = "Exit";
			_exitButton.Ctrl.OnPressed += OnExitPressed;
		}

		public override void Cleanup()
		{
			_goToButton.Ctrl.OnPressed -= OnGoToPressed;
			_exitButton.Ctrl.OnPressed -= OnExitPressed;
			_window.Destroy();
		}

		public void OnGoToPressed()
		{
			Cleanup ();
			Node.Return = "Transition_GoCloseup";
			SignalQuit ();
		}

		public void OnExitPressed()
		{
			Cleanup ();
			Node.Return = "Exit";
			SignalQuit ();
		}
	}

	/// <summary>
	/// Args-Port: Pos = [Target Position], LookAt = [LookAt Position].
	/// </summary>
	public class CameraGoToScene : FlowScene<RunScene>, IGameUpdateReceiver
	{
		Vec3 _targetPos;
		Vec3 _targetForwardDir;

		public CameraGoToScene(RunScene node, IEntity targetObject, Vec3 stance) : base(node)
		{
			_targetPos = targetObject.GetPos () + stance;
			_targetForwardDir = -stance.GetNormalized();

			GameFramework.RegisterForUpdate (this);
		}

		public void OnUpdate()
		{
			var cam = Camera.Current;
			cam.Position = cam.Position * 0.8f + _targetPos * 0.2f;
			cam.ForwardDirection = cam.ForwardDirection * 0.8f + _targetForwardDir * 0.2f;

			if ((cam.Position - _targetPos).GetLength () < 0.03f) 
			{
				Debug.Log ("CameraGoToScene Done");

				cam.Position = _targetPos;
				cam.ForwardDirection = _targetForwardDir;
				Cleanup ();
				SignalQuit ();
			}
		}

		public override void Cleanup()
		{
			GameFramework.UnregisterFromUpdate (this);
		}
	}
}
