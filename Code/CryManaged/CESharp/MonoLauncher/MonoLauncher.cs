// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Linq;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using System.IO;
using CryEngine;
using System.ServiceModel;
using System.Reflection;
using CryEngine.FlowSystem;

namespace CryEngine.MonoLauncher
{
	/// <summary>
	/// Entry Point to Mono world, called from CryMonoBridge on CryEngine startup.
	/// </summary>
    public class App
    {
		static AddInManager _addInManager;

		public static void Initialize()
		{
			Debug.Log ("Launcher Initializing...");
			RegisterFlowNodes ();
			_addInManager = new AddInManager ();
		}

		public static void Shutdown()
		{
			Debug.Log("Launcher Shutdown...");
			_addInManager.Deinitialize ();
		}

		private static void RegisterFlowNodes()
		{
			FlowNodeFactory<RunScene>.Register ();
		}
    }
}
