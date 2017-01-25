// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.
using System;
using System.Reflection;

using CryEngine;
using CryEngine.Common;

using System.Linq;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;

/// <summary>
/// All framework relevant Attributes.
/// </summary>
namespace CryEngine.Attributes
{
	[AttributeUsage(AttributeTargets.Method, AllowMultiple = false, Inherited = false)]
	public sealed class ConsoleCommandAttribute : Attribute
	{
		public ConsoleCommandAttribute(string commandName, uint commandFlag, string commandHelp)
		{
			Name = commandName;
			Flags = commandFlag;
			Comments = commandHelp;
		}

		public string Name
		{
			get;
			private set;
		}

		public string Comments
		{
			get;
			private set;
		}

		public uint Flags
		{
			get;
			private set;
		}
	}

	/// <summary>
	/// Thrown when Console Command Attributes are applied wrongly
	/// </summary>
	public sealed class ConsoleCommandConfigurationException :Exception
	{
		public ConsoleCommandConfigurationException(string msg):base(msg)
		{

		}
	}

	public sealed class ConsoleCommandAttributeManager
	{
		[Conditional("DEBUG")]
		private static void ValidateConsoleCommandRegisterAttributes(ref List<MethodInfo> processedlist, Assembly targetAssembly)
		{
			// detect attributes from all methods
			var totalAttributes = targetAssembly.GetTypes().SelectMany(x => x.GetMethods(BindingFlags.Static | BindingFlags.Instance | BindingFlags.NonPublic | BindingFlags.Public))
				.Where(y => y.GetCustomAttributes().OfType<ConsoleCommandAttribute>().Any())
				.ToList();
						
			//filter attributes that are detected but not processed, these are configured wrongly !
			var filteredAttributes = totalAttributes.Except(processedlist).ToList();
			if(filteredAttributes.Count >0 )
			{
				StringBuilder msg = new StringBuilder();
				foreach (var filteredAttribute in filteredAttributes)
				{
					msg.Append("Attribute on method").Append(filteredAttribute.Name).Append(" is not processed").AppendLine();
				}
				throw new ConsoleCommandConfigurationException(msg.ToString());
			}
		}

		/// <summary>
		/// Parse the specified assembly for all public, static methods marked with attribute "ConsoleCommandRegisterAttribute". Ignores all methods otherwise.
		/// </summary>
		/// <param name="targetAssembly"></param>
		public static void RegisterAttribute(Assembly targetAssembly)
		{
			//do reflection to invoke the attribute for console command function
			Assembly assembly = targetAssembly == null ? Assembly.GetExecutingAssembly() : targetAssembly;

			var processedList = targetAssembly.GetTypes().SelectMany(x => x.GetMethods(BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.Public))
				.Where(y => y.GetCustomAttributes().OfType<ConsoleCommandAttribute>().Any())
				.ToList();

			foreach(MethodInfo methodInfo in processedList)
			{
				CryEngine.ConsoleCommand.ManagedConsoleCommandFunctionDelegate managedConsoleCmdDelegate = Delegate.CreateDelegate(typeof(CryEngine.ConsoleCommand.ManagedConsoleCommandFunctionDelegate), methodInfo) as CryEngine.ConsoleCommand.ManagedConsoleCommandFunctionDelegate;
				if (managedConsoleCmdDelegate != null)
				{
					ConsoleCommandAttribute consoleAttribute = methodInfo.GetCustomAttribute<ConsoleCommandAttribute>() as ConsoleCommandAttribute;
					if(consoleAttribute != null)
					{
						CryEngine.ConsoleCommand.RegisterManagedConsoleCommandFunction(consoleAttribute.Name, consoleAttribute.Flags, consoleAttribute.Comments, managedConsoleCmdDelegate);
					}
				}
			}
			ValidateConsoleCommandRegisterAttributes(ref processedList, assembly);
		}
	}

	public sealed class ConsoleVariableAttribute : Attribute
	{
		public ConsoleVariableAttribute()
		{

		}
	}
}