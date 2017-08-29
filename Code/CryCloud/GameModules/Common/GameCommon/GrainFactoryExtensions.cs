using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using CryBackend.Protocol;
using GameCommon.Platform;
using Orleans;
using Orleans.Runtime;

namespace GameCommon
{
	/// <summary>
	/// Extensions to GrainFactory
	/// </summary>
	public static class GrainFactoryExtensions
	{
		private const string DefaultGameModuleName = "GameCommon";

		/// <summary>
		/// Helper extension to fetch class name prefix from session context automatically.
		/// It will try to find an implementation class with a full name starts with the game name (e.g. Hunt.Meta.Profile),
		/// if that failed it will fall back to GameCommon
		/// This overriding mechanism depends on the fact that top level namespace being the game name (e.g. Hunt)
		/// so please keep the naming convention followed if you do not want to get into trouble
		/// </summary>
		public static T GetGameGrain<T>(this IGrainFactory factory, long key)
			where T : IGrainWithIntegerKey
		{
			string classNamePrefix = RequestContextHelper.Game;
			if (string.IsNullOrEmpty(classNamePrefix))
				return factory.GetGrain<T>(key);

			try
			{
				return factory.GetGrain<T>(key, classNamePrefix);
			}
			catch (ArgumentException)
			{
				// game module didn't override this class, then try the default one (in GameCommon)
			}

			return factory.GetGrain<T>(key, DefaultGameModuleName);
		}

		public static IUser GetUserGrain(this IGrainFactory factory, long userId)
		{
			return factory.GetGameGrain<IUser>(userId);
		}

		public static long GetCurrentProfileId(this IGrainFactory factory)
		{
			// TODO: get profile id directly from RequestContext
			long? mayPid = RequestContextHelper.ProfileId;
			if (!mayPid.HasValue || mayPid.Value == 0)
				throw new NoChosenProfileException(RequestContext.Export());

			return mayPid.Value;
		}

		public static long GetCurrentProfileIdNoThrow(this IGrainFactory factory)
		{
			long? mayPid = RequestContextHelper.ProfileId;
			return mayPid ?? 0;
		}

		public static IProfileBase GetCurrentProfile(this IGrainFactory factory)
		{
			var pid = factory.GetCurrentProfileId();
			return factory.GetGameGrain<IProfileBase>(pid);
		}

		public static IProfileBase GetCurrentProfileNoThrow(this IGrainFactory factory)
		{
			var pid = factory.GetCurrentProfileIdNoThrow();
			if (pid == 0)
				return null;

			return factory.GetGameGrain<IProfileBase>(pid);
		}

		public static T GetCurrentProfileAndCast<T>(this IGrainFactory factory)
			where T : IGrainWithIntegerKey
		{
			var profile = factory.GetCurrentProfile();
			return profile.Cast<T>();
		}

		public static T GetCurrentProfileAndCastNoThrow<T>(this IGrainFactory factory)
			where T : IGrainWithIntegerKey
		{
			var profile = factory.GetCurrentProfileNoThrow();
			if (profile != null)
				return profile.Cast<T>();
			return (T)(object)null;
		}
	}
}
