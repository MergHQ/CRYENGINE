using System;
using System.Reflection;
using CryEngine.Resources;
using CryEngine.Components;
using CryEngine.FlowSystem;
using CryEngine.Common;

namespace CryEngine.App
{
	/// <summary>
	/// Plugin entry point will be re-instantiated in runtime, whenever the assembly is updated (e.g. Re-compiled)
	/// </summary>
	public class MyPlugin : ICryEnginePlugin
	{
		public void Initialize()
		{
			Env.Initialize(null);
		}

		public void Shutdown()
		{
			Env.Shutdown(null);
		}
	}
}
