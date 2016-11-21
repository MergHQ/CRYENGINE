using System;
using System.Collections.Generic;

using CryEngine.Common;

namespace CryEngine
{
	public class ConsoleCommand
	{
		/// <summary>
		/// Executes a string on the command line, for example to load a map as a user could in the console. ('map myMap')
		/// </summary>
		/// <param name="command">The command to execute</param>
		/// <param name="silent">Whether or not to output the specified command in the log</param>
		/// <param name="deferred">If false, executes the command immediately, otherwise waits until the next frame.</param>
		public static void Execute(string command, bool silent = false, bool deferred = false)
		{
			Global.gEnv.pConsole.ExecuteString(command, silent, deferred);
		}
		/*
		/// <summary>
		/// Register a new console command that the user can execute
		/// </summary>
		/// <param name="name">Command name.</param>
		/// <param name="func">Delegate to the console command function to be called when command is invoked.</param>
		/// <param name="comment">Help string, will be displayed when typing in console "command ?".</param>
		/// <param name="flags">Bitfield consist of VF_ flags (e.g. VF_CHEAT)</param>
		public static void Register(string name, ConsoleCommandDelegate func, string comment = "")
		{
			IConsole.ConsoleCommandDelegate wrappedFunc = args =>
			{
				func(new ConsoleCommandArguments(args));
			};

			Global.gEnv.pConsole.AddCommand(name, wrappedFunc, 0, comment);

			_instance._commandMap.Add(name, wrappedFunc);
		}

		static ConsoleCommand()
		{
			_instance = new ConsoleCommand();
		}
		
		internal ConsoleCommand()
		{
		}

		~ConsoleCommand()
		{
			foreach(var command in _commandMap)
			{
				Global.gEnv.pConsole.RemoveCommand(command.Key);
			}
		}*/
	}
}
