// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using CryEngine.Common;
using System.Collections.Generic;
using System.Linq;

namespace CryEngine.FlowSystem
{
	/// <summary>
	/// Stores SceneName and arguments for usage in a generic Scene. Arguments are split in a map for better readability.
	/// </summary>
	public class RunScene : FlowNode
	{
		public Dictionary<string, string> ArgMap { get; private set; } ///< Key-Value dictionary, determined by Args input value.

		[InputPort("The name of the Scene to run")]
		public string SceneName { get; private set; } ///< The name of the Scene to run.
		[InputPort("Optional Scene arguments (Syntax: key1=\"value1\", key2=\"value2\")")]
		public string Args ///< Optional Scene arguments (Syntax: key1="value1", key2="value2").
		{
			private set 
			{
				var argList = value.Split (',').ToList ();
				ArgMap = argList.Where(x => x.Contains('=')).ToDictionary (x => x.Split ('=') [0], x => x.Split ('=') [1].Trim('"'));
			}
			get 
			{
				return null;
			}
		}

		[OutputPort("Optional return value if Scene signals exit")]
		public string Return { get; set; } ///< Optional return value if Scene signals exit.

		/// <summary>
		/// Called by CryEngine. Do not call directly.
		/// </summary>
		public RunScene()
		{
			Description = "Stores data for running a Scene.";
		}

		public string GetValue(string arg)
		{
			string value = null;
			ArgMap.TryGetValue (arg, out value);
			return value;
		}
	}
}
