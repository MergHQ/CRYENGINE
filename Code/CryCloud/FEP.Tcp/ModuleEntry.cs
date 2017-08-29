using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
using System.Reflection;
using System.Runtime.ExceptionServices;
using System.Security.Authentication;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Xml.Linq;
using CryBackend.AuthProvider;
using CryBackend.GameTelemetryProvider;
using CryBackend.Messaging;
using CryBackend.Net;
using CryBackend.NodeCore;
using CryBackend.Telemetry;
using Orleans;
using Orleans.Runtime;
using Orleans.Runtime.Configuration;
using Orleans.Serialization;

namespace CryBackend.FEP.Tcp
{
	[Singleton]
	public class GameModuleLoader : IGameModuleLoader
	{
		public Assembly Load(string moduleName)
		{
			var asmName = Utility.GetLocalAssemblyName(moduleName);
			return GrainClient.LoadGrainAssembly(asmName);
		}
	}

	[ModuleEntry]
	public class FEPModuleEntry : NodeModule
	{
		private const int ORLEANS_CONN_RETRY_INTERVAL_MS = 1000;
		ConnectionAcceptor m_listener;
		IAuthProviderManager m_authProviderManager;
		IGameTelemetryProviderManager m_gameTelemetryProviderManager;

		protected override async Task Launch(XElement fepConfig)
		{
			// === 1. Initialize backend components ===

			// --- start orleans client ---

			var orleansConfig = fepConfig.GetElement("ClientConfiguration", "urn:orleans");
			var strReader = new StringReader(orleansConfig.ToString());
			var config = new ClientConfiguration();
			config.Load(strReader);

			// Add Protobuf Serializer
			var protobufTypeInfo = typeof(ProtobufSerializer).GetTypeInfo();
			if (!config.SerializationProviders.Contains(protobufTypeInfo))
				config.SerializationProviders.Add(protobufTypeInfo);

			Logger.InfoFormat("OrleansClient: Start connecting to Orleans");
			for (int attempts = 0; ; ++attempts)
			{
				try
				{
					if (attempts > 0)
						Thread.Sleep(ORLEANS_CONN_RETRY_INTERVAL_MS);
					GrainClient.Initialize(config);
					break;
				}
				catch (Exception ex)
				{
					ThrowNonLaunchOrderExceptions(ExceptionDispatchInfo.Capture(ex));
				}
			}
			Logger.InfoFormat("OrleansClient: connected to Orleans");

			// --- setup C-Node invoker ---
			var protocolMgr = Env.SingletonMgr.Get<IProtocolManager>();
			var invoker = new OrleansDelegateInvoker(GrainClient.GrainFactory, protocolMgr);

			// === 2. Initialize frontend components ===

			// --- Load FEP server config ---

			// --- initialize provider managers ---
			m_authProviderManager = Env.SingletonMgr.Get<IAuthProviderManager>();
			m_gameTelemetryProviderManager = Env.SingletonMgr.Get<IGameTelemetryProviderManager>();

			// --- create listening socket ---
			var configStream = Env.SingletonMgr.Get<IConfigStream>();

			var sessionConfig = await LoadSessionConfig(configStream, invoker);
			var sessionFactory = new SessionFactory(sessionConfig);

			var bindingEndpoint = ConfigUtils.ReadEndPointConfig(fepConfig);
			var accessAddr = ConfigUtils.ReadAccessAddrConfig(fepConfig);
			if (string.IsNullOrWhiteSpace(accessAddr))
			{
				accessAddr = NetUtils.GetIPAddress(NetUtils.LocalHostName()).ToString();
#if DEBUG
				Logger.InfoFormat("The access IP is not specified in config, using best guess: {0}", accessAddr);
#else
				Logger.ErrorFormat("The access IP is not specified in config, using best guess: {0}", accessAddr);
#endif
			}

			var netCfg = NetworkConfig.LoadFrom(fepConfig);

			X509Certificate2 certificate = null;
			var sslp = LoadSslConfig(fepConfig, out certificate);
			ICongestionControl cCtrl = null;
			if (netCfg.Congestion.Enabled != 0)
			{
				INetworkMetrics metrics = netCfg.Congestion.LogEnabled != 0 ? NetworkMetrics.Instance : null;
				cCtrl = new CongestionControl(metrics);
			}

			m_listener = new ConnectionAcceptor(sessionFactory, new AcceptorSettings
			{
				BindingEndPoint = bindingEndpoint,
				Backlog = netCfg.AcceptorBacklog,
				Capacity = netCfg.Capacity,
				SslProtocol = sslp,
				Certificate = certificate,
				// TODO: add a Create<T>() on SingletonMgr for non-singleton classes that are using singletons for convenience
				ConnectionResources = new ConnectionResources(configStream),
				Connection = new ConnectionConfig
				{
					CongestionControl = cCtrl,
					Channels = netCfg.Channels,
					Bandwidth = netCfg.Bandwidth,
					BandwidthManager = Env.SingletonMgr.Get<IBandwidthManager>(),
				}
			});

			// --- start metrics reporting ---
			var metricsReportHelper = Env.SingletonMgr.Get<IMetricsReportHelper>();
			metricsReportHelper.Setup(fepConfig);
			metricsReportHelper.StartReport();

			// --- opening service port ---
			try
			{
				m_listener.Start();
			}
			catch
			{
				Logger.ErrorFormat("FEP.Tcp failed to bind on endpoint {0}, services are inaccessible to users!!"
					, bindingEndpoint.ToString());
				throw;
			}

			Logger.InfoFormat("FEP.Tcp successfully listen on endpoint {0}"
					, bindingEndpoint.ToString());
			// === 3. Register this service ===
			var serviceReg = Env.SingletonMgr.Get<IServiceRegistry>();
			ZkAccessHelper.Register(serviceReg, "fep", accessAddr, (ushort)bindingEndpoint.Port);
		}

		private static void ThrowNonLaunchOrderExceptions(ExceptionDispatchInfo exInfo)
		{
			// using type name to avoid referencing to ZK libs
			Func<Exception, bool> shouldIgnore = ex =>
				(ex is OrleansException) || ex.GetType().FullName == "org.apache.zookeeper.KeeperException+NoNodeException";

			var sourceEx = exInfo.SourceException;
			var exAgg = sourceEx as AggregateException;
			if (exAgg != null)
			{
				exAgg.Handle(shouldIgnore);
			}
			else
			{
				if (!shouldIgnore(sourceEx))
					exInfo.Throw();
			}
		}

		public override int Shutdown(bool bGraceful)
		{
			m_listener.Stop();
			m_authProviderManager.Dispose();
			GrainClient.Uninitialize();
			return base.Shutdown(bGraceful);
		}

		private async Task<UserSessionConfig> LoadSessionConfig(IConfigStream cfgStream, IComputeNodeInvoker invoker)
		{
			// --- Authentication provider ---
			var authProvider = await m_authProviderManager.GetProviderFromModuleConfig(cfgStream);

			// --- Game Telemetry provider ---
			var gameTelemetryProvider = await m_gameTelemetryProviderManager.GetProviderFromModuleConfig(cfgStream);

			// --- Sequence generator ---
			var seqGenFactory = Env.SingletonMgr.Get<ISequenceGeneratorFactory>();
			var seqGen = await seqGenFactory.CreateAsync("fep");

			var sessionConfig = new UserSessionConfig
			{
				Invoker = invoker,
				SequenceGenerator = seqGen,
				AuthProvider = authProvider,
				GameTelemetryProvider = gameTelemetryProvider,
			};
			return sessionConfig;
		}

		private SslProtocols LoadSslConfig(XElement fepConfig, out X509Certificate2 certificate)
		{
			var endpointElem = fepConfig.GetElement("Endpoint");

			var sslAttr = endpointElem.Attribute("SslProtocol");
			if (sslAttr == null)
				throw new ApplicationException("Missing 'SslProtocol' attribute for Element <Endpoint>");

			var retval = SslProtocolsParse.ParseSslProtocol(sslAttr.Value);

			if (retval == SslProtocols.None)
			{
				certificate = null;
				return retval;
			}

			Logger.InfoFormat("SSL/TLS protection enabled on FEP. Protocol: {0}", retval);

			var certAttr = endpointElem.Attribute("CertificateFile");
			if (certAttr == null)
				throw new ApplicationException("Missing 'CertificateFile' attribute for Element <Endpoint>");

			var passAttr = endpointElem.Attribute("CertificatePass");
			if (passAttr == null)
				throw new ApplicationException("Missing 'CertificatePass' attribute for Element <Endpoint>");

			try
			{
				certificate = LoadSslCertificate(certAttr.Value, passAttr.Value);
			}
			catch
			{
				Logger.Warn("Error loading certificate file: " + certAttr.Value);
				throw;
			}

			return retval;
		}

		X509Certificate2 LoadSslCertificate(string path, string password)
		{
			return new X509Certificate2(path, password);
		}
	}
}
