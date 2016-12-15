// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine.Common;

namespace CryEngine
{
	public delegate void EventHandler();
	public delegate void EventHandler<T>(T arg);
	public delegate void EventHandler<T1, T2>(T1 arg1, T2 arg2);
	
	/// <summary>
	/// Processes CryEngine's system callback and generates events from it.
	/// </summary>
	public class SystemEventHandler : ISystemEventListener
	{
		public static EventHandler FocusChanged;
		public static EventHandler PrecacheEnded;
		public static EventHandler EditorGameStart; ///< Raised if Game-Mode was initiated in Editor 
		public static EventHandler EditorGameEnded; ///< Raised if Game-Mode was exited in Editor 

		internal static SystemEventHandler Instance { get; set; }

		/// <summary>
		/// Called by framework. Do not call directly.
		/// </summary>
		internal SystemEventHandler()
		{
			AddListener();

			Engine.EndReload += AddListener;
		}

		void AddListener()
		{
			Engine.System.GetISystemEventDispatcher().RegisterListener(this);
		}

		public override void Dispose()
		{
			Engine.System.GetISystemEventDispatcher().RemoveListener(this);

			base.Dispose();
		}

		/// <summary>
		/// Called by CryEngine. Do not call directly.
		/// </summary>
#if WIN64
		public override void OnSystemEvent(ESystemEvent evt, ulong wparam, ulong lparam)
#elif WIN32
		public override void OnSystemEvent (ESystemEvent evt, uint wparam, uint lparam)
#endif
		{
			switch (evt)
			{
				case ESystemEvent.ESYSTEM_EVENT_CHANGE_FOCUS:
					if (FocusChanged != null)
						FocusChanged();
					break;
				case ESystemEvent.ESYSTEM_EVENT_LEVEL_PRECACHE_END:
					if (PrecacheEnded != null)
						PrecacheEnded();
					break;

				case ESystemEvent.ESYSTEM_EVENT_EDITOR_GAME_MODE_CHANGED:
					if (wparam == 1 && EditorGameStart != null)
						EditorGameStart();
					if (wparam == 0 && EditorGameEnded != null)
						EditorGameEnded();
					break;
			}
		}
	}
}
