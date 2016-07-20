using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CryLinkHelper
{
	class CryConsoleCommand
	{
		public String Name;
		public List<String> Params = new List<String>();

		public override String ToString()
		{
			StringBuilder stringBuilder = new StringBuilder(1024);
			stringBuilder.Append(Name);

			foreach (String parameter in Params)
			{
				stringBuilder.AppendFormat("\n\t {0}", parameter);
			}

			return stringBuilder.ToString();
		}
	}

	class CryLink : Uri
	{
		/// <summary>
		/// Gets an array that contains the console commands that are specified in the link.
		/// </summary>
		public CryConsoleCommand[] ConsoleCommands
		{
			get; private set;
		}

		/// <summary>
		/// Gets the service name in lower case.
		/// </summary>
		public String ServiceName
		{
			get; private set;
		}

		/// <summary>
		/// Creates a new instance from the specified link string.
		/// </summary>
		/// <param name="cryLink">A CryLink string.</param>
		public CryLink(String cryLink) : base(cryLink)
		{
			if (!ExtractConsoleCommands())
			{
				ConsoleCommands = new CryConsoleCommand[0];
			}

			ServiceName = base.Authority.ToLower();
		}

		/// <summary>
		/// Extracts the console commands that are specified in the link.
		/// </summary>
		/// <returns>True if one or more commands were found.</returns>
		private bool ExtractConsoleCommands()
		{
			if (base.Segments[base.Segments.Length - 1] == "exec" && base.Query[0] == '?')
			{
				String[] queryParams = base.Query.Substring(1, base.Query.Length - 1).Split('&');
				ConsoleCommands = new CryConsoleCommand[queryParams.Length];

				foreach (String param in queryParams)
				{
					const Int32 key = 0;
					const Int32 value = 1;

					String[] query = param.Split('=');

					if (query[key].StartsWith("cmd"))
					{
						Int32 idx;
						if (Int32.TryParse(query[key].Substring(3), out idx))
						{
							CryConsoleCommand command = new CryConsoleCommand();

							String[] commandArgs = query[value].Split('+');
							for (Int32 i = 1; i < commandArgs.Length; ++i)
							{
								command.Params.Add(commandArgs[i]);
							}

							command.Name = commandArgs[0];
							ConsoleCommands[idx - 1] = command;
						}
					}
				}

				return ConsoleCommands.Length > 0;
			}

			return false;
		}
	}
}
