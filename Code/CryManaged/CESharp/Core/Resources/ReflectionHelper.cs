using System;
using System.Diagnostics;
using System.Reflection;
using CryEngine.Common;

namespace CryEngine
{
	public static class ReflectionHelper
	{
		public static string FindPluginInstance(string assemblyName)
		{
			Type pluginType = typeof(ICryEnginePlugin);
			
			foreach (Assembly assembly in AppDomain.CurrentDomain.GetAssemblies())
			{
				if (!assembly.GetName().Name.Equals (assemblyName, StringComparison.InvariantCultureIgnoreCase))
				{
					continue;
				}

				foreach (Type t in assembly.GetTypes())
				{
					if (pluginType.IsAssignableFrom (t))
					{
						return t.FullName;
					}
				}
			}
			return null;
		}
	}
}