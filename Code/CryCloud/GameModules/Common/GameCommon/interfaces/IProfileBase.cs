using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;
using CryBackend.Protocol;
using GameCommon.Session;
using Orleans;



namespace GameCommon
{
	/// <summary>
	/// Base interface for all Profile Components
	/// </summary>
	public interface IProfileBase : IGrainWithIntegerKey
	{
		Task<string> GetNickname();
	}

	/// <summary>
	/// Core component that allows profile to be associated with a user session
	/// </summary>
	public interface IProfileCore : IProfileBase, ISessionSkeleton
	{
		Task<long> GetUserId();
		Task Login(long userId, long sessionId);
		Task Logout();
	}

	/// <summary>
	/// wrapper type for profile ID's long integer, please use this instead of POD long
	/// </summary>
	public struct SProfileID
	{
		long val; //< must be consistent with the key type of profile grains

		public static implicit operator long(SProfileID oprand)
		{
			return oprand.val;
		}

		public static implicit operator SProfileID(long oprand)
		{
			var retval = new SProfileID();
			retval.val = oprand;
			return retval;
		}

		public override string ToString()
		{
			return val.ToString();
		}

		public override int GetHashCode()
		{
			return val.GetHashCode();
		}
	}

	public static partial class ProfileExtensions
	{
		/// <summary>
		/// Get Profile ID from any profile component
		/// </summary>
		public static SProfileID GetId(this IProfileBase profileComponent)
		{
			return profileComponent.GetPrimaryKeyLong();
		}

		public static IProfileCore GetCore(this IProfileBase profileComponent, IGrainFactory factory)
		{
			return factory.GetGameGrain<IProfileCore>(profileComponent.GetId());
		}
	}

	[Serializable]
	public class NoChosenProfileException : GameException
	{
		public NoChosenProfileException(IReadOnlyDictionary<string,object> sessionContext)
			:this((long)sessionContext[RequestContextHelper.UserIdKey]
				 , (long)sessionContext[RequestContextHelper.SessionIdKey])
		{}

		public NoChosenProfileException(long userId, long sessionId)
			: base($"No profile associated with the session. UserID: {userId}, SessionID: {sessionId}")
		{ }

		protected NoChosenProfileException(SerializationInfo info, StreamingContext context) : base(info, context) { }
	}

	[Serializable]
	public class NonPlayerSessionException : GameException
	{
		public NonPlayerSessionException(IReadOnlyDictionary<string,object> sessionContext)
			:this((long)sessionContext[RequestContextHelper.SessionIdKey])
		{}

		public NonPlayerSessionException(long sessionId)
			: base($"Not a player session. SessionID: {sessionId}")
		{ }

		protected NonPlayerSessionException(SerializationInfo info, StreamingContext context) : base(info, context) { }
	}

}
