using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using System.Xml.Linq;
using CryBackend;
using CryBackend.Configuration;
using CryBackend.NodeCore;
using NSubstitute;
using NSubstitute.Core;

namespace TestUtils
{
	[Serializable]
	public class UnitTestBootInfo : IBootInfo
	{
		public EEnvironment Env { get; set; }
		public string Game { get; set; }
		public string Instance { get; set; }
		public string LocalConfigPath { get; }
		public bool UseCentralizedConfig { get; set; }
		public XElement ZookeeperCfg { get; set; }

		public UnitTestBootInfo(string game)
		{
			Env = EEnvironment.Dev;
			Game = game;
			Instance = "unit_test_boot";
			UseCentralizedConfig = false;
			LocalConfigPath = string.Empty;
			ZookeeperCfg = null;
		}
	}

	public class TestSuiteBase
	{
#if DEBUG
		// we should always just use one clock globally, PerfCounter<> caches clock upon first init
		private static readonly ClockMockingHelper s_clockMock = new ClockMockingHelper();
#endif

		protected ClockMockingHelper TimeController
		{
			get
			{
#if DEBUG
				return s_clockMock;
#else
				throw new NotSupportedException("Time Controller can not be used in profile build");
#endif
			}
		}

		public TestSuiteBase(AssemblyName[] asms = null, NodeRole role = NodeRole.ComputeNode)
		{
			var singletonMgr = new SingletonManager_UnitTest();
			SetupEnv(role, singletonMgr);

			AssemblyLoadEventHandler onAsmLoad = (_, args) =>
			{
				singletonMgr.Rescan(args.LoadedAssembly);
			};

			AppDomain.CurrentDomain.AssemblyLoad += onAsmLoad;

			singletonMgr.Rescan();
			if (asms != null)
				singletonMgr.Rescan(asms.Select(Utility.LoadLocalAssembly));

			AppDomain.CurrentDomain.AssemblyLoad -= onAsmLoad;
		}

		public static void SetupEnv(NodeRole role, IBootInfo bootinfo, ISingletonManager mgr, ConfigOverrides cfgOverrides = null)
		{
#if DEBUG
			((SingletonManager)mgr).SetSingleton<IClock>(s_clockMock.Clock);
#endif

			if (cfgOverrides == null)
				cfgOverrides = new ConfigOverrides();

			((SingletonManager)mgr).SetSingleton<IConfigOverrides>(cfgOverrides);

			Env.UnitTestSetup(mgr, bootinfo);

			var cwdStr = Directory.GetCurrentDirectory();
			var cfgFactory = new ConfigStreamFactory(cwdStr + "/CryCloud.cfg");
			var stream = cfgFactory.Create(null);
			stream = new MergedConfigReader(stream, "Primary", role.ToString().Replace('_', '.'));
			((SingletonManager)mgr).SetSingleton<IConfigStream>(stream);
		}

		public static void SetupEnv(NodeRole role, ISingletonManager mgr,  string game = null, ConfigOverrides cfgOverrides = null)
		{
			var bootinfo = new UnitTestBootInfo(game ?? "GameCommon");

			SetupEnv(role, bootinfo, mgr, cfgOverrides);
		}
	}

	public interface IClockMockingHelper
	{
		void Reset();
		void SetTime(Timestamp ts);
		void SetTime(Timestamp start, TimeSpan offset);
		void FastForward(TimeSpan span);
	}

	// TODO: support IClock.DateTimeNow
	public sealed class ClockMockingHelper : MarshalByRefObject, IClockMockingHelper
	{
		readonly IHiResTimeSource m_hiTimeSource;
		readonly ILoResTimeSource m_loTimeSource;
		Timestamp m_curTime;
		public IClock Clock { get; }

		public ClockMockingHelper()
		{
			m_hiTimeSource = Substitute.For<IHiResTimeSource>();
			m_loTimeSource = Substitute.For<ILoResTimeSource>();
			Clock = ClockMockingFactory.Create(m_hiTimeSource, m_loTimeSource);

			// reset time source
			var ts = Timestamp.Zero;
			m_hiTimeSource.GetTime().Returns(ts);
			m_loTimeSource.GetTime().Returns(ts);
		}


		public void Reset()
		{
			SetTime(Timestamp.Zero);
		}

		public void SetTime(Timestamp ts)
		{
			m_hiTimeSource.GetTime().Returns(ts);
			m_loTimeSource.GetTime().Returns(ts);
			m_curTime = ts;
		}

		public void SetTime(Timestamp start, TimeSpan offset)
		{
			SetTime(start + offset);
		}

		public void FastForward(TimeSpan span)
		{
			SetTime(m_curTime, span);
		}
	}


	// TODO: move to an independent file
	public static class UnitTestExtensions
	{
		public static bool CheckArg<T>(this ICall lhs, int idx, Func<T, bool> condition)
		{
			object boxedArg = lhs.GetArguments()[idx];
			return DoCheckArg(boxedArg, condition);
		}

		public static bool CheckArg<T>(this ICall lhs, Func<T, bool> condition)
		{
			return lhs.GetArguments().Any(e => DoCheckArg(e, condition));
		}

		[MethodImpl(MethodImplOptions.AggressiveInlining)]
		private static bool DoCheckArg<T>(object boxedArg, Func<T, bool> condition)
		{
			return boxedArg is T && condition((T)boxedArg);
		}

	}
}
