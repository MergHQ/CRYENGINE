// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Reflection;
using CryEngine.Resources;
using CryEngine.Components;
using CryEngine.FlowSystem;

namespace CryEngine.SampleApp
{
	/// <summary>
	/// Add-in entry point will be re-instantiated in runtime, whenever the assembly is updated (e.g. Re-compiled)
	/// </summary>
	public class Program : ICryEngineAddIn
	{
		Application _app;

		public void Initialize(InterDomainHandler handler)
		{
		}

		public void StartGame()
		{
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
		}

		/// <summary>
		/// Called when engine is being shut down or if application is reloaded.
		/// </summary>
		public void EndGame()
		{
			_app.Shutdown(false);
		}
	}
}
