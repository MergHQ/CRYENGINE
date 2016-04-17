// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using CryEngine.UI;
using CryEngine.Common;
using CryEngine.Components;

/// <summary>
/// General Assembly for all framework related sub-systems, wrappers and extensions.
/// </summary>
namespace CryEngine.Components
{
	/// <summary>
	/// Should be used as an entry point for each application project. Handles proper relative initiation and shutdown of framework.
	/// </summary>
	public class Application : Component
	{
		bool _requestedShutdown = false;
		bool _activeShutdown = true;
		bool _isShutdown = false;

		public static string ExecutionPath
		{
			get 
			{
				string dllDir = Global.gEnv.pMonoRuntime.GetProjectDllDir();
				if(String.IsNullOrEmpty(dllDir))
				{
					StringBuilder sb = new StringBuilder(1024);
					CryEngine.Common.Global.CryGetExecutableFolder (1024, sb);
					return sb.ToString ();
				}
				return System.IO.Path.Combine (Environment.CurrentDirectory, dllDir);
			}
		}

		public static string DataPath
		{
			get 
			{
				return Global.GetGameFolder ().c_str() + "/";
			}
		} ///< Path where application data should be stored.

		public static string UIPath { get { return DataPath + "libs/ui/"; } } ///< Path where UI System textures are stored.

		/// <summary>
		/// Creates Application of type T inside a new scene root.
		/// </summary>
		/// <typeparam name="T">The Type of application to be instantiated.</typeparam>
		public static T Instantiate<T>() where T : Application
		{
			return SceneObject.Instantiate (null, typeof(T).Name + "Root").AddComponent<T>();
		}

		/// <summary>
		/// Called by framework. Do not call directly.
		/// </summary>
		public override void Update()
		{
			base.Update ();
			if (_requestedShutdown) 
				ShutdownInternal ();
		}

		/// <summary>
		/// Initiates shutdown of application, including CryEngine.
		/// </summary>
		/// <param name="delayed">Set to <c>true</c> to ensure Shutdown is performed in proper thread. Should be false whenever not performed by user.</param>
		public void Shutdown(bool delayed = true)
		{
			if (delayed) 
			{
				// Shutdown delayed, in proper thread
				_requestedShutdown = true;
				_activeShutdown = true;
			}
			else
			{
				// Shutdown was requested by engine. Shutdown immediately
				_activeShutdown = false;
				ShutdownInternal ();
			}
		}

		void ShutdownInternal()
		{
			if (!_isShutdown) 
			{
				Root.Destroy ();
				if (_activeShutdown)
					Global.gEnv.pSystem.Quit ();
				_isShutdown = true;
			}
		}
	}
}
