using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace CryEngine
{
	public enum CVarFlags
	{
		None = 0,
		Cheat,
	}

	/// <summary>
	/// Attribute indicating that an entity represents a player
	/// This is used to automatically spawn entities when clients connect
	/// </summary>
	[AttributeUsage(AttributeTargets.Field, Inherited = false)]
	public sealed class ConsoleVariableAttribute : Attribute
	{
		public ConsoleVariableAttribute(string name, string description, CVarFlags flags = CVarFlags.None)
		{
		}
	}
}
