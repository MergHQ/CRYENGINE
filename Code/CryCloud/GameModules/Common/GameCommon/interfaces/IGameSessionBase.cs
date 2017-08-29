using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;
using CryBackend.Protocol;
using Orleans;
using Orleans.Concurrency;


namespace GameCommon.Session
{
	/// <summary>
	/// Profile that supports session-based games
	/// </summary>
	public interface ISessionProfile : IProfileCore
	{
		Task JoinGameSession(IGameSessionBase target, IGameSessionBase from = null);
		Task<bool> TryLeaveGameSession(IGameSessionBase room);
		Task<IGameSessionBase> LeaveCurrentGameSession();
		Task<IGameSessionBase> GetGameSession();
		Task<bool> IsInGameSession();
	}

	/// <summary>
	/// What it is about the player in a session. basically the player's profile and the options he/she has chosen
	///  e.g. loadout.
	/// Notice that this is different from player's state in a session, PlayerContext is (more or less) static to
	/// a running session.
	/// </summary>
	[Serializable]
	public class SessionPlayerContext
	{
		public ISessionProfile Profile { get; set; }
		public Dictionary<int, string> Options { get; set; }

		public SessionPlayerContext(ISessionProfile profile, IDictionary<int, string> options)
		{
			Profile = profile;
			Options = options != null ? new Dictionary<int, string>(options) : new Dictionary<int, string>();
		}
	}

	/// <summary>
	/// Mutable in-lobby state of player
	/// </summary>
	[Serializable]
	public class SessionPlayerMutableState
	{
		private Dictionary<int, object> m_fields;

		public SessionPlayerMutableState()
		{
			m_fields = new Dictionary<int, object>();
		}

		public SessionPlayerMutableState(IEnumerable<KeyValuePair<int, object>> input)
			: this(input.ToDictionary(kv => kv.Key, kv => kv.Value))
		{
		}

		public SessionPlayerMutableState(IDictionary<int, object> input)
		{
			m_fields = new Dictionary<int, object>(input);
		}

		public TVal GetField<TVal>(System.Enum key)
		{
			return GetField<TVal, Enum>(key, default(TVal));
		}

		public TVal GetField<TVal, TKey>(TKey key, TVal defVal)
		{
			object retval;
			if (m_fields.TryGetValue(Convert.ToInt32(key), out retval))
				return (TVal)retval;
			return defVal;
		}

		public void SetField<TKey, TVal>(TKey key, TVal val)
		{
			m_fields[Convert.ToInt32(key)] = val;
		}
	}
	
	[Serializable]
	public class Reservation
	{
		public long ProfileId { get; set; }
		public long SquadId { get; set; }

		public Guid MissionSuiteGuid { get; set; }
		public int SlotGroupIndex { get; set; }
		public int SlotIndex => ProfileId == SquadId ? 0 : 1;
	}


	public interface IGameSessionBase : IGrainWithIntegerKey
	{
		/// <summary>
		/// Add a player to a game/lobby session
		/// </summary>
		/// <param name="playerCtx"></param>
		/// <returns>An index to locate the added player in the session. Or -1 if failed</returns>
		Task<KeyValuePair<int, bool>> AddPlayer(
			Immutable<SessionPlayerContext> pctx,
			Immutable<SessionPlayerMutableState> state = default(Immutable<SessionPlayerMutableState>));
		Task RemovePlayer(SProfileID player);
		Task<long[]> GetPlayers();
		Task<Immutable<Reservation[]>> GetReservations();

		/// <remark>
		/// Caller should not try to confirm changes to player's mutable state by the result of the call, instead
		/// read session events.
		/// </remark>
		Task SetPlayerMutableState(SessionPlayerMutableState newval);

		Task<int[]> StartMerge(Immutable<Reservation[]> reservations);
		Task EndMerge();

		// TODO: add reason
		Task CloseInternal();
	}
}
