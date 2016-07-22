// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Reflection;
using CryEngine.Resources;
using CryEngine.Components;
using CryEngine.FlowSystem;
using CryEngine.Common;

namespace CryEngine.SampleApp
{
	/// <summary>
	/// Add-in entry point will be re-instantiated in runtime, whenever the assembly is updated (e.g. Re-compiled)
	/// </summary>
	public class MyPlugin : ICryEnginePlugin
	{
		static Application _app;

		public void Initialize()
		{
			Env.Initialize (null);

			_app = Application.Instantiate<SampleApp>();
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
			_app.Shutdown(false);
			_app.Destroy ();
			_app = null;

			Env.Shutdown (null);
		}
	}
}
