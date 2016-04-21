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
			_app = Application.Instantiate<Editor> ();
		}

		public void OnFlowNodeSignal(FlowNode node, PropertyInfo signal)
		{
		}

		public void Shutdown()
		{
			_app.Shutdown (false);
		}
	}
}
