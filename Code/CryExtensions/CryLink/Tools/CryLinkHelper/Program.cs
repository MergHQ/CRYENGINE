using System;
using System.Linq;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace CryLinkHelper
{
	// TODO: Proper log writing.
	// TODO: Show a windows form instead of a console window.
	// TODO: Settings should be in the registry as well so we can
	//			 share them between branches etc.

	class Program
	{
		[System.Runtime.InteropServices.DllImport("user32.dll")]
		static extern bool SetForegroundWindow(IntPtr hWnd);

		static void Main(String[] args)
		{
			// DEBUG
			//args = new String[] { "-install" };
			/*args = new String[] 
			{ 
				"-link",
				//"crylink://editor/exec?cmd2=sc_rpcShow+46fddd14-721e-49bf-a30f-4f07bbfd06f3+b841f7d7-d14b-4044-880a-24d351c2fb37&cmd1=e_schematycShowNode+c62dfc71-960f-4cce-9b11-ab215350e738+d11ff37b-6b41-4b47-aac3-4bd87f776fa1"
				"crylink://editor/exec?cmd1=sc_rpcShow+46fddd14-721e-49bf-a30f-4f07bbfd06f3+b841f7d7-d14b-4044-880a-24d351c2fb37"
			};*/
			/*while(true)
			{
				Thread.Sleep(1000);
			}*/

			if (args.Length > 0)
			{
				Console.WriteLine();

				for (Int32 i = 0; i < args.Length; ++i)
				{
					if (args[i].Equals("-install", StringComparison.CurrentCultureIgnoreCase))
					{
						Console.WriteLine("Installing CryLinkHandler -----------------------------------");

						Install.Uri();
						Console.WriteLine("URI registered to the system.");
						if (Install.DefaultSettings())
						{
							Console.WriteLine("Settings file '{0}' generated.", Settings.FilePath);
						}
						Console.WriteLine("-------------------------------------------------------------");

						break;
					}

					if (args[i].Equals("-link", StringComparison.CurrentCultureIgnoreCase))
					{
						if (!Settings.Load())
						{
							Console.WriteLine("[Error] Failed to load 'settings.xml'. Please check your settings file or run 'CryLinkHelper.exe -install'.");
							break;
						}

						if (++i < args.Length && !String.IsNullOrEmpty(args[i]) && args[i][0] != '-')
						{
							ProcessLink(args[i]);
						}
					}
				}

				if (!Settings.Loaded || Settings.AutoClose == false)
				{
					Console.WriteLine("\r\nPress a key to exit.");
					Console.ReadKey();
					return;
				}
			}
		}

		/// <summary>
		/// Process the specified CryLink string.
		/// </summary>
		/// <param name="link">A CryLink string.</param>
		static void ProcessLink(String link)
		{
			Console.WriteLine("Processing link ---------------------------------------------");
			Console.WriteLine("{0}", link);
			Console.WriteLine("-------------------------------------------------------------");

			CryLink cryLink = new CryLink(link);
			if (cryLink.ConsoleCommands.Length > 0)
			{
				Settings.Service service;
				if (!Settings.Services.TryGetValue(cryLink.ServiceName, out service))
				{
					Console.WriteLine("[Error] Unknown service! Please add a service configuration for '{0}' to your settings.", cryLink.ServiceName);
					return;
				}

				// TODO: Handle multiple instances etc.
				CryLinkService cryLinkService = new CryLinkService(service.Host, service.Port);
				for (Int32 retry = 0; retry < 100; ++retry)
				{
					if (StartProcess(service.ProcessName, service.ExecutablePath))
					{
						Int64 challenge = 0;
						try
						{
							challenge = cryLinkService.Challenge();
						}
						catch
						{
							continue;
						}

						String auth = CryLinkService.GenerateAuth(challenge, service.Password);
						if (cryLinkService.Authenticate(auth))
						{
							Console.WriteLine("Executing commands ------------------------------------------");
							Console.WriteLine("");

							foreach (CryConsoleCommand cmd in cryLink.ConsoleCommands)
							{
								cryLinkService.ExecuteCommand(cmd);
								Console.WriteLine(cmd.ToString());
							}
							Console.WriteLine("-------------------------------------------------------------");
						}
					}

					if(Settings.AutoClose == false)
					{
						Console.WriteLine("\r\nPress a key to exit.");
						Console.ReadKey();
					}
					return;
				}
			}
		}

		/// <summary>
		/// Checks if a process is already running.
		/// </summary>
		/// <param name="processName">Name of the process</param>
		/// <param name="executablePath">Abolute filepath to the executable.</param>
		/// <returns>True if the process is running.</returns>
		static bool IsProcessRunning(String processName, String executablePath)
		{
			Process[] processes = Process.GetProcessesByName(processName);
			return processes.FirstOrDefault(p => p.MainModule.FileName.Equals(executablePath, StringComparison.CurrentCultureIgnoreCase)) != default(Process);
		}

		/// <summary>
		/// Sets the focus to a process's main window.
		/// </summary>
		/// <param name="processName">Name of the process</param>
		/// <param name="executablePath">Abolute filepath to the executable.</param>
		/// <returns>True if the focus was successfully set.</returns>
		static bool FocusProcessMainWindow(String processName, String executablePath)
		{
			Process[] processes = Process.GetProcessesByName(processName);
			Process process = processes.FirstOrDefault(p => p.MainModule.FileName.Equals(executablePath, StringComparison.CurrentCultureIgnoreCase));
			if(process != default(Process))
			{
				return SetForegroundWindow(process.MainWindowHandle);
			}
			return false;
		}

		/// <summary>
		/// Starts a process and focuses it's main window if isn't already running.
		/// </summary>
		/// <param name="processName">Name of the process</param>
		/// <param name="executablePath">Abolute filepath to the executable.</param>
		/// <returns>True if the process was started.</returns>
		static bool StartProcess(String processName, String executablePath)
		{
			String fullPath = Path.GetFullPath(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location) + "\\" + executablePath);
			if (!File.Exists(fullPath))
			{
				return false;
			}

			if (!IsProcessRunning(processName, fullPath))
			{
				Process process = Process.Start(fullPath);
				Console.WriteLine("Waiting for '{0}' to get ready ...", executablePath);
				
				// TODO: We should communicate with the process to determine when it is ready.
				Thread.Sleep(Settings.StartUpSleep * 1000);

				process.Refresh();
				if (process.Responding)
				{
					SetForegroundWindow(process.MainWindowHandle);
					return true;
				}
				return false;
			}

			return FocusProcessMainWindow(processName, fullPath);
		}
	}
}
