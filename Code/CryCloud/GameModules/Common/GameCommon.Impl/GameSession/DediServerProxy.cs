using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using CryBackend.Protocol;
using Orleans;
using Orleans.Runtime;
using Orleans.Concurrency;

using fp = FP;
using pb = Google.Protobuf.WellKnownTypes;



namespace GameCommon.Session
{
	[StatelessWorker]
	class DediSrvBootstrapper : Grain, fp.IExposed_DediSrvBootstrapper
	{
		public async Task<fp.DediSessionId> Bootstrap(fp.DediInfo dsInfo)
		{
			long sessionId = (long)RequestContext.Get("SessionId");
			await GrainFactory.GetGameGrain<IDediServer>(sessionId).Enroll(dsInfo);
			return new fp.DediSessionId { Id = sessionId };
		}
	}

	[Reentrant]
	public class DediServerProxy : Grain, IDediServer
	{
		protected IMessageObserver m_dsSink;

		private ObserverSubscriptionManager<IDediSrvObserver> m_observers = new ObserverSubscriptionManager<IDediSrvObserver>();
		private fp.EDediStatus m_status = fp.EDediStatus.Invalid;
		private fp.DediInfo m_info;
		private long m_acquiredBy;
		private IMessageObserver m_unitTestCallbackSink;


		public async Task Withdraw()
		{
			await GrainFactory.GetGrain<IDediSrvManager>(0).OnDSDeactivated(this);

			System.Diagnostics.Debug.Assert(m_dsSink == null || m_dsSink.Equals(await GetCallbackSink()));
			m_dsSink = null;

			// DS disconnected while still being used
			// Close the running session
			if (m_acquiredBy != 0)
			{
				var game = GrainFactory.GetGameGrain<IGameSessionBase>(m_acquiredBy);
				await game.CloseInternal();
			}

			if (m_info != null)
				GetLogger().Info("DS went offline and be removed. {0}:{1} ({2})", m_info.Ip, m_info.Port, m_info.Name);

			DeactivateOnIdle();
		}

		public async Task Enroll(fp.DediInfo dediInfo)
		{
			DelayDeactivation(TimeSpan.MaxValue);

			if (!CheckDediInfo(dediInfo))
				throw new ArgumentOutOfRangeException("dediInfo", "the argument failed the validation");

			m_dsSink = await GetCallbackSink();
			m_info = dediInfo;
			await GrainFactory.GetGrain<IDediSrvManager>(0).OnDSActivated(this);
			m_status = fp.EDediStatus.Enlisted;

			GetLogger().Info("New DS added to DS manager: {0}:{1} ({2})", m_info.Ip, m_info.Port, m_info.Name);
		}

		public async Task OnAcquired(long gameSessionId)
		{
			if (m_acquiredBy != 0)
				throw new InvalidOperationException(string.Format("DediServer is already taken by {0}", m_acquiredBy));

			m_acquiredBy = gameSessionId;
			GetLogger().Info("Allocated a DS for game session. {0}:{1} ({2})", m_info.Ip, m_info.Port, m_info.Name);

			var msg = new fp.DediCommand
			{
				Command = (uint)fp.DediCommand.Types.ESystemDsCommands.OnAcquired,
				Args = pb.Any.Pack(new fp.OnAcquiredArgs { GameSessionId = gameSessionId })
			};

			Debug.Assert(m_dsSink != null);
			m_dsSink.OnMessage(msg);
		}


		public async Task SendMetaMsg( fp.MetaDediMsg metaMsg )
		{
			var msg = new fp.DediCommand
			{
				Command = (uint)fp.DediCommand.Types.ESystemDsCommands.MetaMsg,
				Args = pb.Any.Pack(metaMsg)
			};

			if (m_dsSink != null)
				m_dsSink.OnMessage(msg);
		}


		public async Task OnReleased(long sessionId)
		{
			if (m_acquiredBy != sessionId)
			{
				// TODO: raise to an error
				GetLogger().Info("A game session is trying to release a DS it doesn't own. session: {0}", sessionId);
				return;
			}

			m_acquiredBy = 0;
			GetLogger().Info("A DS is released from game session. {0}:{1} ({2})", m_info.Ip, m_info.Port, m_info.Name);
		}

		public Task<fp.DediInfo> GetInfo()
		{
			return Task.FromResult(m_info);
		}

		public async Task LoadLevel(string levelName, string gameRules, Google.Protobuf.IMessage customArgs = null)
		{
			var args = new fp.LoadLevelArgs()
			{
				Map = levelName,
				GameRules = gameRules,
			};

			if (customArgs != null)
				args.Custom = pb.Any.Pack(customArgs);

			var cmdMsg = new fp.DediCommand
			{
				Command = (uint)fp.DediCommand.Types.ESystemDsCommands.LoadLevel,
				Args = pb.Any.Pack(args)
			};

			Debug.Assert(m_dsSink != null);
			m_dsSink.OnMessage(cmdMsg);
		}

		public async Task OnStateChanged(fp.OnStateChangedArgs args)
		{
			GetLogger().Verbose("DediServer.OnStateChanged() GrainID:{0}, oldVal:{1}, newVal:{2}"
				, this.GetPrimaryKeyLong(), m_status, args.Status);

			// TODO: check state transitions
			m_status = args.Status;

			m_observers.Notify(ob => ob.OnStateChanged(this, m_status));
		}

		public async Task Subscribe(IDediSrvObserver observer)
		{
			GetLogger().Verbose("DediServer.Subscribe() GrainID:{0}, observer:{1}"
				, this.GetPrimaryKeyLong(), observer.GetPrimaryKeyLong());

			m_observers.Subscribe(observer);
		}

		public async Task Unsubscribe(IDediSrvObserver observer)
		{
			GetLogger().Verbose("DediServer.Unsubscribe() GrainID:{0}, observer:{1}"
				, this.GetPrimaryKeyLong(), observer.GetPrimaryKeyLong());

			m_observers.Unsubscribe(observer);
		}

		public Task _TouchDediCommand(fp.DediCommand req)
		{
			throw new NotImplementedException();
		}

		bool CheckDediInfo(fp.DediInfo dediInfo)
		{
			return (dediInfo.Port != 0);
		}

		public Task<IMessageObserver> GetCallbackSink()
		{
			if (m_unitTestCallbackSink != null)
				return Task.FromResult(m_unitTestCallbackSink);

			// DS proxy grainId is session id
			var key = this.GetPrimaryKeyLong();
			var reg = GrainFactory.GetGrain<ISessionCallbackRegistrar>(key);
			return reg.GetMessageSink();
		}

		// TODO: because this setup could only happen after DS bootstrapping in unit tests (otherwise we can't get the ID to
		// access the grain in the first place)
		// better way is making a proper user session setup for mocking and do this there
		public Task UnitTest_SetCallbackSink(IMessageObserver ob)
		{
			m_unitTestCallbackSink = ob;
			// HACK: because of the above reason
			m_dsSink = ob;
			return TaskDone.Done;
		}
	}
}
