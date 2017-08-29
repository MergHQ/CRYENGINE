using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
using System.Reflection;
using System.Runtime.Serialization;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Xml;
using System.Xml.Linq;
using CryBackend;
using CryBackend.NodeCore;
using log4net;
using log4net.Config;
using CryBackend.Configuration;

namespace NodeLauncher
{

	[Serializable]
	public class GracefulTerminator : IGracefulTerminator
	{
		AppDomain m_primaryDomain;

		public GracefulTerminator(AppDomain primaryDomain)
		{
			m_primaryDomain = primaryDomain;
			// GracefulTerminator has special rights
			var node = Env.SingletonMgr.Get<INodeModule>();
			((NodeModule)node).SetGracefulTerminator(this);
		}

		public void ShutdownNode()
		{
			m_primaryDomain.DoCallBack(() =>
				{
					Task.Run(() => Launcher.Instance.Shutdown());
				});
		}
	}

	/// <summary>
	/// Launcher is responsible to config loading and argument parsing
	/// </summary>
	internal class Launcher
	{
		static ILog Logger = LogManager.GetLogger("Launcher");

		#region Embeded Types
		[Serializable]
		internal class AppDomainInitException : Exception
		{
			public AppDomainInitException() : base() { }

			public AppDomainInitException(SerializationInfo info, StreamingContext context)
				: base(info, context)
			{ }
		}

		struct Module
		{
			public AppDomain HostingDomain;
			public EventWaitHandle SigFinialized;
		}
		#endregion Embeded Types

		public static Launcher Instance;    //< for GracefulTerminator only
#if DEBUG
		private static readonly int PRESS_ANY_KEY_TIMEOUT_MS = -1;
#else
		private static readonly int PRESS_ANY_KEY_TIMEOUT_MS = 5000;
#endif

		private readonly Dictionary<NodeRole,NodeRole[]> m_dependency = new Dictionary<NodeRole, NodeRole[]>()
		{
			{NodeRole.UnitTestRunner, new[]{NodeRole.ComputeNode} },
			{NodeRole.StressTestRunner, new[]{NodeRole.ComputeNode, NodeRole.FEP_Tcp} },
			{NodeRole.ComputeNode, new[] { NodeRole.ConfigBootstrapper } },
			{NodeRole.FEP_Tcp, new[] { NodeRole.ConfigBootstrapper } },
			{NodeRole.FEP_Http, new[] { NodeRole.ConfigBootstrapper } },
		};

		string m_nodeName;
		NodeRole m_roleComb = NodeRole.None;
		List<Module> m_modules = new List<Module>();
		bool m_bStarted = false;
		int m_isShuttingDown = 0;
		bool m_bCancelKeyPressed;
		EventWaitHandle m_hTerminatedByModule; //< event to make "press any key" the last message on the console screen

		public Launcher()
		{
			if (Instance != null)
				throw new InvalidOperationException("Launcher can only have one instance");

			m_hTerminatedByModule = new EventWaitHandle(false, EventResetMode.ManualReset);
			Instance = this;
		}

		string MakeNodeName()
		{
			var hostName = Dns.GetHostName();

			var roleNames = NodeRoleUtils.EnumRoleNames(m_roleComb);
			if (roleNames.Count() > 1)
				return hostName + "_" + string.Join("", roleNames.Select(n => n[0]));
			else
				return hostName + "_" + m_roleComb.ToString();
		}

		void SelectRole(string sRoles)
		{
			NodeRole roleComb = NodeRole.None;
			foreach (var sFlag in sRoles.Split(','))
			{
				roleComb |= (NodeRole)Enum.Parse(typeof(NodeRole), sFlag.Replace('.', '_'), false);
			}

			if (roleComb == NodeRole.None)
			{
				Logger.FatalFormat("No valid Role found for this node. role:{0}", sRoles);
				throw new ArgumentOutOfRangeException("role");
			}

			m_roleComb = roleComb;
		}

		public void Launch(string nodeName, string sRoles, string configPath, string overridesStr)
		{
			// the Launch function should not do much about arguments, args should be passed on to
			// ModuleDomainInit and be handled over there, because the latter is in it's own domain.

			SelectRole(sRoles);

			if (string.IsNullOrEmpty(nodeName))
				nodeName = MakeNodeName();
			m_nodeName = nodeName;

			// Set console title
			{
				string appTitle;
				string appVersion;
				GetApplicationInfo(out appTitle, out appVersion);
				Console.Title = appTitle + " : " + m_nodeName + " : " + appVersion;
			}

			var defAppConfigPath = AppDomain.CurrentDomain.SetupInformation.ConfigurationFile;
			var modules = NodeRoleUtils.EnumRolePairs(m_dependency, m_roleComb)
				.Select(e => Tuple.Create(e.Item1, e.Item2, defAppConfigPath));
			var modCount = modules.Count();
			if (modCount <= 0)
				throw new ArgumentOutOfRangeException("You must pick at least one role to launch the node");

			// create one app.config per each module that overrides log4net settings to avoid inter-process log file locking
			modules = modules.Select(e =>
				Tuple.Create(e.Item1, e.Item2, DuplicateAppConfig(defAppConfigPath, e.Item2)))
				.ToArray();

			string failedModuleName = "N/A";
			try
			{
				foreach (var m in modules)
				{
					failedModuleName = "N/A";
					var role = m.Item1;
					string sRole = m.Item2;
					failedModuleName = role.ToString();

					// make sure processes of the same module type won't share same kernel object
					var sigName = "Local\\CryNode.sig_final_" + sRole + "_" + Guid.NewGuid().ToString();
					var sigFinal = new EventWaitHandle(false, EventResetMode.ManualReset, sigName);

					Logger.InfoFormat("Creating AppDomain for module {0}...", sRole);

					var primaryDomain = AppDomain.CurrentDomain;
					AppDomain moduleDomain;
					moduleDomain = AppDomain.CreateDomain(role.ToString(), null, new AppDomainSetup
					{
						AppDomainInitializer = ModuleDomainInit,
						AppDomainInitializerArguments = new string[] { m_nodeName, sRole, configPath, overridesStr, sigName },
						ConfigurationFile = m.Item3, //< pass the .config file for log4net
					});

					if (moduleDomain == null)
					{
						failedModuleName = role.ToString();
						break;
					}

					moduleDomain.CreateInstance(
						typeof(GracefulTerminator).Assembly.FullName,
						typeof(GracefulTerminator).FullName,
						false, BindingFlags.Default, null,
						new object[] { AppDomain.CurrentDomain }, null, null);

					Module module;
					module.HostingDomain = moduleDomain;
					module.SigFinialized = sigFinal;
					m_modules.Add(module);
				}
				failedModuleName = null;
			}
			catch (AppDomainInitException)
			{
				// already logged
			}
			catch (Exception ex)
			{
				Logger.ErrorFormat("Exception happened around AppDomain creation. module: {0}\n{1}"
					, failedModuleName, ex.ToString());
			}

			if (failedModuleName != null)
			{
				Logger.InfoFormat("Failed to launch node, due to module {0}", failedModuleName);
				DoShutdown();
			}
			else
			{
				m_bStarted = true;
				Logger.InfoFormat("Modules started: {0}", string.Join(",", m_modules.Select(m => m.HostingDomain.FriendlyName).ToArray()));
			}

			var arrFinalized = m_modules.Select(mod => mod.SigFinialized).ToArray();
			if (arrFinalized.Length > 0)
				WaitHandle.WaitAll(arrFinalized);

			if (!m_bCancelKeyPressed)
			{
				m_hTerminatedByModule.WaitOne();
				Console.WriteLine("Press any key to quit...");
				var stdIn = Console.OpenStandardInput();
				var buffer = new byte[1];
				var tskAnyKey = stdIn.ReadAsync(buffer, 0, 1);
				var tskDelay = Task.Delay(PRESS_ANY_KEY_TIMEOUT_MS);
				Task.WaitAny(tskAnyKey, tskDelay);
			}
		}



		static void ModuleDomainInit(string[] args)
		{
			var logger = LogManager.GetLogger("Launcher");

			if (args.Length != 5)
				throw new ArgumentException("ModuleDomainInit need 5 args");

			string nodeName = args[0];
			string role = args[1];
			string configPath = args[2];
			string overridesStr = args[3];
			string sigName = args[4];

			// must catch exceptions here, some exceptions are not serializable (e.g. KeeperException+NoNodeException)
			// so they can't be propagated across domain border
			try
			{
				DoModuleDomainInit(nodeName, role, configPath, overridesStr, sigName);
			}
			catch (NodeModuleInitFailure)
			{
				// must throw another exception to notify CreateDomain()
				throw new AppDomainInitException();
			}
			catch (Exception ex)
			{
				if (ex is AggregateException)
				{
					ex = (ex as AggregateException).Flatten().InnerException;
				}
				if (ex.GetType().FullName == "org.apache.zookeeper.KeeperException+NoNodeException")
				{
					logger.ErrorFormat("Can't find ZooKeeper node to initialize module, did you forget to run ConfigBootstrapper first?");
				}
				else if(ex is ConfigLoadException)
				{
					Logger.ErrorFormat("{0}", ex);
				}
				else
				{
					logger.ErrorFormat("Exception happened while initializing NodeModule {0}:\n{1}", role, ex);
				}

#if DEBUG
				if (Debugger.IsAttached)
					Debugger.Break();
				else
					Debugger.Launch();
#endif
				// must throw another exception to notify CreateDomain()
				throw new AppDomainInitException();
			}
		}

		static void DoModuleDomainInit(string nodeName, string role, string configPath, string overridesStr, string sigName)
		{
			// ---- Initialize logger ----
			XmlConfigurator.Configure();
			var logger = LogManager.GetLogger("Launcher");

			// register unobserved exception handler
            TaskScheduler.UnobservedTaskException += (sender, eventArgs) =>
            {
                eventArgs.SetObserved();
                var ex = eventArgs.Exception.GetBaseException();
				LogManager.GetLogger(role)
					.WarnFormat("Unobserved task exception: {0}\n{1}", ex.GetType(), ex.ToString());
            };

            // ---- Initialize Env ----
            // A hack for ConfigBootstrapper: for ConfigBootstrapper it should always get the local raw/unmerged config
            if (role == Enum.GetName(typeof(NodeRole), NodeRole.ConfigBootstrapper))
                NodeCoreInitializer.InitializeWithRawConfig(logger, role, configPath, true, overridesStr);
            else
                NodeCoreInitializer.Initialize(logger, role, nodeName, configPath, false, overridesStr);

			// ---- Loading node module ----
			AssemblyLoadEventHandler onAsmLoad = (_, args) =>
				((SingletonManager)Env.SingletonMgr).Rescan(args.LoadedAssembly);

			logger.InfoFormat("Loading NodeModule {0}...", role);
			AppDomain.CurrentDomain.AssemblyLoad += onAsmLoad;
			var dll = LoadRoleDll(role);

			// ---- Launching node module ----
			try
			{
				logger.InfoFormat("Initializing NodeModule {0}...", role);
				var module = NodeModule.GetModuleEntry(dll, modT =>
					{
						var inst = (NodeModule)Activator.CreateInstance(modT);
						inst.Init(nodeName, role, sigName);
						return inst;
					});

				logger.InfoFormat("Launching NodeModule {0}...", role);
				module.Launch();
			}
			catch
			{
				// raise the finalized flag from module domain
				EventWaitHandle sigFinal;
				if (EventWaitHandle.TryOpenExisting(sigName, out sigFinal))
					sigFinal.Set();

				throw;
			}

			AppDomain.CurrentDomain.AssemblyLoad -= onAsmLoad;
		}

		public void Shutdown(bool bCancelKeyPressed = false)
		{
			try
			{
				if (!m_bStarted)
					return;

				var oldVal = Interlocked.Exchange(ref m_isShuttingDown, 1);
				if (oldVal == 1)
					return;

				m_bCancelKeyPressed = bCancelKeyPressed;

				Logger.Info("Shutdown signal received, shutting down the node...");

				DoShutdown();
			}
			catch(Exception ex)
			{
				Logger.Error("Exception when shutting-down the node:\n", ex);
				m_hTerminatedByModule.Set();
			}
		}

		private void DoShutdown()
		{
			foreach (var mod in m_modules)
			{
				mod.HostingDomain.DoCallBack(ModuleDomainFinalize);
			}

			m_hTerminatedByModule.Set();
		}

		static void ModuleDomainFinalize()
		{
			var mod = Env.SingletonMgr.Get<INodeModule>();
			if (mod == null)
				return;

			var logger = LogManager.GetLogger("Launcher");
			if (mod.FinalizedSignal.WaitOne(0))
			{
				logger.InfoFormat("NodeModule {0} was finalized already.", mod.Name);
				return;
			}

			var exitcode = mod.Shutdown(true);
			logger.InfoFormat("NodeModule {0} finalized gracefully with code: {1}", mod.Name, exitcode);
		}

		static Assembly LoadRoleDll(string role)
		{
			return Utility.LoadLocalAssembly(role + ".dll");
		}

		static string GetModuleAppConfig(string oldPath, string moduleName)
		{
			var fileName =  Path.GetFileNameWithoutExtension(oldPath);
			return oldPath.Replace(fileName, fileName + "." + moduleName);
		}

		static string DuplicateAppConfig(string filePath, string moduleName)
		{
			XDocument appConfig;
			using (var file = new FileStream(filePath, FileMode.Open, FileAccess.Read))
			{
				appConfig = XDocument.Load(file);
			}

			XAttribute fileNameAttr;
			try
			{
				var fileElem = (from ap in appConfig.Root.Element("log4net").Elements("appender")
								where ap.Attribute("type").Value == "log4net.Appender.RollingFileAppender"
								select ap.Element("file"))
								.SingleOrDefault();
				fileNameAttr = fileElem.Attribute("value");
				if (fileNameAttr == null)
					throw new ArgumentException();
			}
			catch
			{
				Logger.InfoFormat("RollingFileAppender is not used skip log parallelization");
				return filePath;
			}

			if (fileNameAttr.Value.IndexOf("Node") == -1)
				throw new ArgumentException("CryCloud config file must be named as CryCloud.Node.cfg");

			fileNameAttr.Value = fileNameAttr.Value.Replace("Node", moduleName);

			var newFilePath = GetModuleAppConfig(filePath, moduleName);
			using (var file = new FileStream(newFilePath, FileMode.Create, FileAccess.ReadWrite))
			{
				using (var writer = XmlWriter.Create(file))
				{
					appConfig.WriteTo(writer);
				}
			}
			return newFilePath;
		}

		static void GetApplicationInfo(out string title, out string version)
		{
			var exe = Assembly.GetEntryAssembly();
			version = exe.GetName().Version.ToString();
			title = exe.GetCustomAttribute<AssemblyProductAttribute>().Product;
		}
	}
}
