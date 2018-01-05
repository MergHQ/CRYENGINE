// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

using System.Collections.Generic;
using CryEngine.Common;
using CryEngine.Rendering;

namespace CryEngine
{
	/// <summary>
	/// Central point for mouse access. Listens to CryEngine sided mouse callbacks and generates events from them.
	/// </summary>
	public class Mouse
	{
		private struct MouseEvent
		{
			public int X;
			public int Y;
			public EHARDWAREMOUSEEVENT HardwareEvent;
			public int WheelDelta;

			public MouseEvent(int x, int y, EHARDWAREMOUSEEVENT hardwareEvent, int wheelDelta)
			{
				X = x;
				Y = y;
				HardwareEvent = hardwareEvent;
				WheelDelta = wheelDelta;
			}
		}

		private class DeferredMouseListener : IHardwareMouseEventListener, IGameUpdateReceiver
		{
			private Mouse _mouse;
			private readonly Queue<MouseEvent> _eventQueue = new Queue<MouseEvent>();

			public DeferredMouseListener(Mouse mouse)
			{
				_mouse = mouse;
				GameFramework.RegisterForUpdate(this);
				Global.gEnv.pHardwareMouse.AddListener(this);
			}

			/// <summary>
			/// Disposes this instance.
			/// </summary>
			public override void Dispose()
			{
				GameFramework.UnregisterFromUpdate(this);
				Global.gEnv.pHardwareMouse.RemoveListener(this);
				_mouse = null;

				base.Dispose();
			}

			public override void OnHardwareMouseEvent(int iX, int iY, EHARDWAREMOUSEEVENT eHardwareMouseEvent, int wheelDelta)
			{
				MouseEvent mouseEvent = new MouseEvent(iX, iY, eHardwareMouseEvent, wheelDelta);
				_eventQueue.Enqueue(mouseEvent);
			}

			public override void OnHardwareMouseEvent(int iX, int iY, EHARDWAREMOUSEEVENT eHardwareMouseEvent)
			{
				OnHardwareMouseEvent(iX, iY, eHardwareMouseEvent, 0);
			}

			public void OnUpdate()
			{
				var queue = new Queue<MouseEvent>(_eventQueue);
				_eventQueue.Clear();
				foreach(var mouseEvent in queue)
				{
					// If a mouse event triggered an engine unload the mouse can be null.
					if(_mouse == null)
					{
						break;
					}
					_mouse.HandleMouseEvent(mouseEvent);
				}

				_mouse?.Update();
			}
		}

		/// <summary>
		/// Interface for overriding mouse input.
		/// </summary>
		public interface IMouseOverride
		{
			event MouseEventHandler LeftButtonDown;
			event MouseEventHandler LeftButtonUp;
			event MouseEventHandler RightButtonDown;
			event MouseEventHandler RightButtonUp;
			event MouseEventHandler Move;
		}

		/// <summary>
		/// Used by all Mouse events.
		/// </summary>
		public delegate void MouseEventHandler(int x, int y);
		/// <summary>
		/// Invoked when the left mouse button is pressed down.
		/// </summary>
		public static event MouseEventHandler OnLeftButtonDown;
		/// <summary>
		/// Invoked when the left mouse button is released after being pressed down.
		/// </summary>
		public static event MouseEventHandler OnLeftButtonUp;
		/// <summary>
		/// Invoked when the right mouse button is pressed down.
		/// </summary>
		public static event MouseEventHandler OnRightButtonDown;
		/// <summary>
		/// Invoked when the right mouse button is released after being pressed down.
		/// </summary>
		public static event MouseEventHandler OnRightButtonUp;
		/// <summary>
		/// Invoked when the mouse has moved.
		/// </summary>
		public static event MouseEventHandler OnMove;
		/// <summary>
		/// Invoked when the mouse moves outside of a window.
		/// </summary>
		public static event MouseEventHandler OnWindowLeave;
		/// <summary>
		/// Invoked when the mouse enters a window.
		/// </summary>
		public static event MouseEventHandler OnWindowEnter;

		internal static Mouse Instance { get; set; }

		private static IMouseOverride s_override = null;
		private static float _lmx = 0;
		private static float _lmy = 0;
		private static bool _updateLeftDown = false;
		private static bool _updateLeftUp = false;
		private static bool _updateRightDown = false;
		private static bool _updateRightUp = false;
		private static uint _hitEntityId = 0;
		private static Vector2 _hitEntityUV = new Vector2();
		private static bool _cursorVisible = false;

		private DeferredMouseListener _mouseListener;

		/// <summary>
		/// Current Mouse Cursor Position, refreshed before update loop.
		/// </summary>
		public static Point CursorPosition { get { return new Point(_lmx, _lmy); } }

		/// <summary>
		/// Indicates whether left mouse button is Down during one update phase.
		/// </summary>
		public static bool LeftDown { get; private set; }

		/// <summary>
		/// Indicates whether left mouse button is Released during one update phase.
		/// </summary>
		public static bool LeftUp { get; private set; }

		/// <summary>
		/// Indicates whether right mouse button is Down during one update phase.
		/// </summary>
		public static bool RightDown { get; private set; }

		/// <summary>
		/// Indicates whether right mouse button is Released during one update phase.
		/// </summary>
		public static bool RightUp { get; private set; }

		/// <summary>
		/// ID of the Entity currently under the cursor position.
		/// </summary>
		public static uint HitEntityId
		{
			get { return _hitEntityId; }
			set { _hitEntityId = value; }
		}

		/// <summary>
		/// UV-coordinates where the mouse-cursor is hitting an Entity.
		/// </summary>
		public static Vector2 HitEntityUV
		{
			get { return _hitEntityUV; }
			set { _hitEntityUV = value; }
		}

		/// <summary>
		/// The Entity currently under the cursor position
		/// </summary>
		public static Entity HitEntity
		{
			get
			{
				return Entity.Get(HitEntityId);
			}
		}

		/// <summary>
		/// Show the mouse-cursor
		/// </summary>
		public static void ShowCursor()
		{
			if(!_cursorVisible)
			{
				Global.gEnv.pHardwareMouse.IncrementCounter();
			}
			_cursorVisible = true;
		}

		/// <summary>
		/// Hide the mouse-cursor
		/// </summary>
		public static void HideCursor()
		{
			if(_cursorVisible)
			{
				Global.gEnv.pHardwareMouse.DecrementCounter();
			}
			_cursorVisible = false;
		}

		private void HandleMouseEvent(MouseEvent mouseEvent)
		{
			int x = mouseEvent.X;
			int y = mouseEvent.Y;

			switch (mouseEvent.HardwareEvent)
			{
			case EHARDWAREMOUSEEVENT.HARDWAREMOUSEEVENT_LBUTTONDOWN:
				_updateLeftDown = true;
				HitScenes(x, y);
				if (OnLeftButtonDown != null)
				{
						OnLeftButtonDown(x, y);
				}
				break;

			case EHARDWAREMOUSEEVENT.HARDWAREMOUSEEVENT_LBUTTONUP:
				_updateLeftUp = true;
				HitScenes(x, y);
				if (OnLeftButtonUp != null)
				{
							OnLeftButtonUp(x, y);
				}
				break;
			case EHARDWAREMOUSEEVENT.HARDWAREMOUSEEVENT_RBUTTONDOWN:
				_updateRightDown = true;
				HitScenes(x, y);
				if (OnRightButtonDown != null)
				{
					OnRightButtonDown(x, y);
				}
				break;
			case EHARDWAREMOUSEEVENT.HARDWAREMOUSEEVENT_RBUTTONUP:
				_updateRightUp = true;
				HitScenes(x, y);
				if (OnRightButtonUp != null)
				{
					OnRightButtonUp(x, y);
				}
				break;
			}
		}

		private static void OnOverrideLeftButtonDown(int x, int y)
		{
			_updateLeftDown = true;
			if (OnLeftButtonDown != null)
				OnLeftButtonDown(x, y);
		}

		private static void OnOverrideLeftButtonUp(int x, int y)
		{
			_updateLeftUp = true;
			if (OnLeftButtonUp != null)
				OnLeftButtonUp(x, y);
		}

		private static void OnOverrideRightButtonDown(int x, int y)
		{
			_updateRightDown = true;
			if (OnRightButtonDown != null)
				OnRightButtonDown(x, y);
		}

		private static void OnOverrideRightButtonUp(int x, int y)
		{
			_updateRightUp = true;
			if (OnRightButtonUp != null)
				OnRightButtonUp(x, y);
		}

		private static void OnOverrideMove(int x, int y)
		{
			if (OnMove != null)
				OnMove(x, y);
		}

		/// <summary>
		/// Overrides the mouse input.
		/// </summary>
		/// <param name="mouseOverride"></param>
		public static void SetOverride(IMouseOverride mouseOverride)
		{
			if (s_override != null)
			{
				s_override.LeftButtonDown -= OnOverrideLeftButtonDown;
				s_override.LeftButtonUp -= OnOverrideLeftButtonUp;
				s_override.RightButtonDown -= OnOverrideRightButtonDown;
				s_override.RightButtonUp -= OnOverrideRightButtonUp;
				s_override.Move -= OnOverrideMove;
			}
			s_override = mouseOverride;
			if (s_override != null)
			{
				s_override.LeftButtonDown += OnOverrideLeftButtonDown;
				s_override.LeftButtonUp += OnOverrideLeftButtonUp;
				s_override.RightButtonDown += OnOverrideRightButtonDown;
				s_override.RightButtonUp += OnOverrideRightButtonUp;
				s_override.Move += OnOverrideMove;
			}

		}

		/// <summary>
		/// Called by framework. Do not call directly.
		/// </summary>
		private void Update()
		{
			LeftDown = _updateLeftDown;
			LeftUp = _updateLeftUp;
			RightDown = _updateRightDown;
			RightUp = _updateRightUp;

			_updateLeftDown = false;
			_updateLeftUp = false;
			_updateRightDown = false;
			_updateRightUp = false;

			float x = 0, y = 0;
			Global.gEnv.pHardwareMouse.GetHardwareMouseClientPosition(ref x, ref y);

			var w = Renderer.ScreenWidth;
			var h = Renderer.ScreenHeight;
			bool wasInside = _lmx >= 0 && _lmy >= 0 && _lmx < w && _lmy < h;
			bool isInside = x >= 0 && y >= 0 && x < w && y < h;
			_lmx = x; _lmy = y;

			HitScenes((int)x, (int)y);
			if (wasInside && isInside)
			{
				if (OnMove != null)
					OnMove((int)x, (int)y);
			}
			else if (wasInside != isInside && isInside)
			{
				if (OnWindowEnter != null)
					OnWindowEnter((int)x, (int)y);
			}
			else if (wasInside != isInside && !isInside)
			{
				if (OnWindowLeave != null)
					OnWindowLeave((int)x, (int)y);
			}
		}

		/// <summary>
		/// Fires a raycast to check if UI is hit at the specified coordinates.
		/// </summary>
		/// <param name="x"></param>
		/// <param name="y"></param>
		public static void HitScenes(int x, int y)
		{
			if(!Global.gEnv.pGameFramework.GetILevelSystem().IsLevelLoaded())
			{
				return;
			}

			HitEntityId = 0;
			var mouseDir = Camera.Unproject(x, y);
			RaycastHit hit;
			if(Physics.Raycast(Camera.Position, mouseDir, 100, out hit))
			{
				HitEntityId = hit.EntityId;
				_hitEntityUV = hit.UvPoint;
			}
			else
			{
				_hitEntityUV.x = 0;
				_hitEntityUV.y = 0;
			}
			
		}

		/// <summary>
		/// Called by framework. Do not call directly.
		/// </summary>
		internal Mouse()
		{
			_mouseListener = new DeferredMouseListener(this);
			Engine.StartReload += OnEngineUnload;
			Engine.EndReload += OnEngineReload;


			HitEntityId = 0;
			HitEntityUV = new Vector2();
		}

		private void OnEngineUnload()
		{
			_mouseListener.Dispose();
			_mouseListener = null;
		}

		private void OnEngineReload()
		{
			_mouseListener = new DeferredMouseListener(this);
		}
	}
}
