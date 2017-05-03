using System;
using System.IO;
using System.Reflection;
using CryEngine.Common;
using CryEngine.Rendering;

namespace CryEngine
{
	/// <summary>
	/// Wraps CryEngine's gEnv variable for global access to main modules. Initializes all C# sided handler and wrapper classes.
	/// Interfaces should be removed from here once a managed wrapper (such as Input) exists.
	/// </summary>
	public static class Engine
	{
		public static IConsole Console { get { return Global.gEnv.pConsole; } }
		public static ICryFont Font { get { return Global.gEnv.pCryFont; } }
		public static ISystem System { get { return Global.gEnv.pSystem; } }
		public static IFlowSystem FlowSystem { get { return Global.gEnv.pFlowSystem; } }
		public static IParticleManager ParticleManager { get { return Global.gEnv.pParticleManager; } }
		public static I3DEngine Engine3D { get { return Global.gEnv.p3DEngine; } }
		public static IGameFramework GameFramework { get { return Global.gEnv.pGameFramework; } }
		public static ITimer Timer { get { return Global.gEnv.pTimer; } }
		public static Common.CryAudio.IAudioSystem AudioSystem { get { return Global.gEnv.pAudioSystem; } }
		public static IRenderAuxGeom AuxRenderer { get { return Global.gEnv.pRenderer.GetIRenderAuxGeom(); } }
		public static IPhysicalWorld PhysicalWorld { get { return Global.gEnv.pPhysicalWorld; } }

		/// <summary>
		/// Indicates whether this <see cref="T:CryEngine.Engine"/> is running in the Sandbox.
		/// </summary>
		/// <value><c>true</c> if this is running in the Sandbox; otherwise, <c>false</c>.</value>
		public static bool IsSandbox { get { return Global.gEnv.IsEditor(); } }

		/// <summary>
		/// True if the application is currently running in the sandbox and GameMode has been started.
		/// </summary>
		/// <value><c>true</c> if in GameMode; otherwise, <c>false</c>.</value>
		public static bool IsSandboxGameMode { get { return Global.gEnv.IsEditorGameMode(); } }

		/// <summary>
		/// Root directory of the engine.
		/// </summary>
		/// <value>The engine root directory.</value>
		public static string EngineRootDirectory => Global.GetEnginePath().c_str();

        /// <summary>
        /// Path where application data should be stored.
        /// </summary>
        /// <value>The data directory.</value>
        public static string DataDirectory => Global.GetGameFolder().c_str() + "/";

        internal static string MonoDirectory => Path.Combine(EngineRootDirectory, "bin", "common", "Mono");

        internal static string GlobalAssemblyCacheDirectory => Path.Combine(MonoDirectory, "lib", "mono", "gac");

        internal static event Action StartReload;
		internal static event Action EndReload;

		/// <summary>
		/// Called by C++ runtime. Do not call directly.
		/// </summary>
		internal static void OnEngineStart()
		{
			SystemEventHandler.Instance = new SystemEventHandler();
			Input.Initialize();
			Renderer.Instance = new Renderer();
			Mouse.Instance = new Mouse();
			CryEngine.GameFramework.Instance = new GameFramework();
			LevelSystem.Instance = new LevelSystem();
		}

		/// <summary>
		/// Called by C++ runtime. Do not call directly.
		/// </summary>
		internal static void OnEngineShutdown()
		{
			// Make sure we unify shutdown behavior with unload
			OnUnloadStart();
		}

		/// <summary>
		/// Called by the C++ runtime just before unloading the domain this assembly belongs to
		/// </summary>
		internal static void OnUnloadStart()
		{
			StartReload?.Invoke();
		}

		/// <summary>
		/// Called by the C++ runtime just after reloading the domain this assembly belongs to
		/// Is *not* called on the initial load, see OnEngineStart instead!
		/// </summary>
		internal static void OnReloadDone()
		{
			if(EndReload != null)
			{
				EndReload();
			}
		}

		internal static void ScanAssembly(Assembly assembly)
		{
			foreach(Type t in assembly.GetTypes())
			{
				EntityComponent.TryRegister(t);
				BehaviorTreeNodeFactory.TryRegister(t);
			}
		}

		/// <summary>
		/// Shuts the engine down, or exits game-mode while in the Sandbox.
		/// </summary>
		public static void Shutdown()
		{
			OnUnloadStart();
			if(!IsSandbox)
			{
				Console.ExecuteString("quit", false, true);
			}
			else
			{
				Console.ExecuteString("ed_disable_game_mode", false, true);
			}
		}
	}
}
