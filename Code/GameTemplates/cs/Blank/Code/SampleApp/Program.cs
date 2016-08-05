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

	public override void Initialize()
	{
		_app = Application.Instantiate<SampleApp>();
	}

	public override void Shutdown()
	{
		_app.Shutdown(false);
		_app.Destroy();
		_app = null;
	}
}
}
