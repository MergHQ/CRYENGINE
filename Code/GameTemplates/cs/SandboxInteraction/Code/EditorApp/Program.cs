// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using CryEngine.Resources;
using CryEngine.Components;
using CryEngine.FlowSystem;
using System.Reflection;

namespace CryEngine.Editor
{
	public class Program : ICryEngineAddIn
	{
		Application _app;

		public void Initialize(InterDomainHandler handler)
		{
		}

		public void StartGame()
		{
			_app = Application.Instantiate<Editor> ();
		}

		public void OnFlowNodeSignal(FlowNode node, PropertyInfo signal)
		{
		}

		public void Shutdown()
		{
		}

		public void EndGame()
		{
			_app.Shutdown (false);
		}
	}
}
