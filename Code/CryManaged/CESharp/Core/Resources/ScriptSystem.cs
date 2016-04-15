// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Linq;
using CryEngine.Resources;
using CryEngine.Components;
using CryEngine.FlowSystem;

namespace CryEngine.Resources
{
	/// <summary>
	/// Interface which must be implemented to allow for an assembly to be automatically instanciated by the framework on runtime. If an assembly holds a class which implements this interface, ScriptSystem will watch the assembly for modifications. If the file is changed, the plugin will be newly instanciated.
	/// </summary>
	public interface ICryEngineAddIn
	{
		/// <summary>
		/// Supposed to initialize the assembly.
		/// </summary>
		void Initialize();

		/// <summary>
		/// Raises the run scene function.
		/// </summary>
		/// <param name="scene">Scene object which originated the event.</param>
		void OnFlowNodeSignal(FlowNode node, PropertyInfo signal);

		/// <summary>
		/// Supposed to cleanup all resources used by the assembly.
		/// </summary>
		void Shutdown();
	}
}
