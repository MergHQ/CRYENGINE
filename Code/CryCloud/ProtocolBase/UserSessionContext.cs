using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using Orleans.Runtime;

namespace CryBackend.Protocol
{
	public class UserSessionContext
	{
		public enum SessionType
		{
			Invalid,
			Client,
			DedicateServer
		};

		public FP.ESessionType Type { get; private set; }
		public bool Authenticated { get; private set; }
		public string Token { get; private set; }

		// TODO: put following fields in a RequestContext-compatible Dictionary for faster exporting
		public long SessionId { get; private set; }
		public long UserId { get; private set; }
		public string Game { get; private set; }
		public Version GameVersion { get; private set; }
		public long ProfileId { get; private set; }

		public UserSessionContext()
		{
			Authenticated = false;
		}

		public void OnAuthRequested(string game, Version version, FP.ESessionType type)
		{
			Game = game;
			GameVersion = version;
			Type = type;
		}

		public void OnAuthRequested(string game, FP.Version version, FP.ESessionType type)
		{
			var ver = (version == null)
				? new Version(1, 0, 0, 0)
				: new Version(version.Major, version.Minior, version.Build, version.Revision);
			OnAuthRequested(game, ver, type);
		}

		public void OnAuthSucceeded(long sessionId, long userId, string authToken)
		{
			SessionId = sessionId;
			UserId = userId;
			Token = authToken;
			Authenticated = true;
		}

		public void OnProfileActivated(long profileId)
		{
			ProfileId = profileId;
		}
	}

	public static class RequestContextHelper
	{
		enum EKeys
		{
			SessionId,
			UserId,
			Game,
			ProfileId,
		};

		public static readonly string SessionIdKey = Enum.GetName(typeof(EKeys), EKeys.SessionId);
		public static readonly string UserIdKey = Enum.GetName(typeof(EKeys), EKeys.UserId);
		public static readonly string GameKey = Enum.GetName(typeof(EKeys), EKeys.Game);
		public static readonly string ProfileIdKey = Enum.GetName(typeof(EKeys), EKeys.ProfileId);

		public static string Game => (string) RequestContext.Get(GameKey);
		public static long SessionId => (long)RequestContext.Get(SessionIdKey);
		public static long? UserId => RequestContext.Get(UserIdKey) as long?;
		public static long? ProfileId => RequestContext.Get(ProfileIdKey) as long?;

		static RequestContextHelper()
		{
			var ctxType = typeof(UserSessionContext);
			var props = ctxType.GetProperties(BindingFlags.Public | BindingFlags.Instance | BindingFlags.GetProperty );

			if (!Enum.GetNames(typeof(EKeys)).All(key => props.Any(e => e.Name == key)))
				throw new NotSupportedException("UserSessionContext has mismatching key(s)");
		}

		public static void Set(UserSessionContext ctx)
		{
			RequestContext.Import(ctx.GetContextDict());
		}

		public static void SetGame(string game)
		{
			RequestContext.Set(GameKey, game);
		}

		public static void SetGame(Dictionary<string, object> ctx)
		{
			RequestContext.Set(GameKey, ctx[GameKey]);
		}

		public static Dictionary<string, object> ExportSessionIndependentContext()
		{
			return new Dictionary<string, object>
			{
				{GameKey, RequestContext.Get(GameKey)},
			};
		}

		public static Dictionary<string, object> GetContextDict(string game, long sessionId, long userId, long profileId)
		{
			return new Dictionary<string, object> {
				{GameKey , game },
				{SessionIdKey, sessionId },
				{UserIdKey, userId },
				{ProfileIdKey, profileId }
			};
		}

		public static Dictionary<string, object> GetContextDict(this UserSessionContext ctx)
		{
			return EnumContext(ctx).ToDictionary(kv => kv.Key, kv => kv.Value);
		}

		public static IEnumerable<KeyValuePair<string, object>> EnumContext(this UserSessionContext ctx)
		{
			yield return new KeyValuePair<string, object>(GameKey, ctx.Game);
			if (ctx.Authenticated)
			{
				yield return new KeyValuePair<string, object>(SessionIdKey, ctx.SessionId);
				yield return new KeyValuePair<string, object>(UserIdKey, ctx.UserId);

				if (ctx.ProfileId != 0)
				{
					yield return new KeyValuePair<string, object>(ProfileIdKey, ctx.ProfileId);
				}
			}
		}
	}
}
