using System;
using System.Linq;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using CryEngine.Common;

namespace CryEngine.Framework
{
    public static class GameTokens
    {
        static readonly List<IGameToken> tokens;
        static readonly List<GameTokenCallback> callbacks;
        static readonly GameTokenListener listener;

        static IGameTokenSystem TokenSystem { get { return Env.Game.GetIGameFramework().GetIGameTokenSystem(); } }

        const string defaultPath = "Assets/libs/gametokens/";

        public static ReadOnlyCollection<IGameToken> Tokens { get { return tokens.AsReadOnly(); } }

        static GameTokens()
        {
            tokens = new List<IGameToken>();
            listener = new GameTokenListener();
            listener.GameTokenTriggered += OnGameTokenTriggered;
            TokenSystem.RegisterListener(listener);
            callbacks = new List<GameTokenCallback>();
        }

        /// <summary>
        /// Loads up game tokens from the specified file. If 'isAbsolutePath' is false, the specified file will be loaded from the default directory 'Assets/libs/gametokens'.
        /// </summary>
        /// <param name="fileName">The name of the file including the file extension.</param>
        public static void LoadTokens(string fileName, bool isAbsolutePath = false)
        {
            if (!isAbsolutePath)
                fileName = defaultPath + fileName;

            if (!File.Exists(fileName))
            {
                Log.Error("[GameTokens] Failed to load file '{0}'", fileName);
                return;
            }

            TokenSystem.LoadLibs(fileName);

            // Reload tokens
            tokens.Clear();

            var iterator = TokenSystem.GetGameTokenIterator();

            if (iterator != null)
            {
                iterator.MoveFirst();

                while (!iterator.IsEnd())
                {
                    tokens.Add(iterator.Next());
                }
            }
        }

        /// <summary>
        /// Returns the first token matching the specified name. Returns null if no token is found.
        /// </summary>
        /// <param name="tokenName">The fully-qualified named of the token.</param>
        /// <returns></returns>
        public static IGameToken GetToken(string tokenName)
        {
            return tokens.FirstOrDefault(x => x.GetName() == tokenName);
        }

        /// <summary>
        /// Returns true if the token exists.
        /// </summary>
        /// <param name="tokenName"></param>
        /// <returns></returns>
        public static bool HasToken(string tokenName)
        {
            return tokens.Any(x => x.GetName() == tokenName);
        }

        /// <summary>
        /// Registers a callback for when a Token has changed.
        /// </summary>
        public static void RegisterCallback(GameTokenCallback callback)
        {
            if (!callbacks.Contains(callback))
                callbacks.Add(callback);
        }

        /// <summary>
        /// Removes a callback if it has been registered.
        /// </summary>
        /// <param name="callback"></param>
        public static void UnregisterCallback(GameTokenCallback callback)
        {
            if (callbacks.Contains(callback))
                callbacks.Remove(callback);
        }

        static void OnGameTokenTriggered(GameTokenListener.GameTokenEvent arg)
        {
            callbacks
                .Where(x => x.Trigger == arg.Event)
                .Where(x => x.Token.GetName() == arg.Token.GetName())
                .ToList()
                .ForEach(x => x.Callback(arg.Token));
        }
    }

    public static class IGameTokenEx
    {
        public static string AsString(this IGameToken token)
        {
            return token.GetValueAsString();
        }

        public static float AsFloat(this IGameToken token)
        {
            return float.Parse(token.GetValueAsString());
        }

        public static int AsInt(this IGameToken token)
        {
            return int.Parse(token.GetValueAsString());
        }

        public static bool AsBool(this IGameToken token)
        {
            return token.GetValueAsString() == "true";
        }

        public static Vec3 AsVec3(this IGameToken token)
        {
            return Vec3Ex.FromString(token.GetValueAsString());
        }

        public static void SetValue<T>(this IGameToken token, T value) where T : struct
        {
            if ((typeof(T) == typeof(bool)))
            {
                token.SetValueAsString(Convert.ToBoolean(value) == true ? "true" : "false");
            }
            else
            {
                token.SetValueAsString(value.ToString());
            }
        }

        public static void SetValue(this IGameToken token, Vec3 value)
        {
            token.SetValueAsString(value.AsString());
        }
    }

    public class GameTokenCallback
    {
        public EGameTokenEvent Trigger { get; private set; }
        public IGameToken Token { get; private set; }
        public Action<IGameToken> Callback { get; private set; }

        public GameTokenCallback(EGameTokenEvent trigger, IGameToken token, Action<IGameToken> callback)
        {
            Trigger = trigger;
            Token = token;
            Callback = callback;
        }
    }

    public class GameTokenListener : IGameTokenEventListener
    {
        public event EventHandler<GameTokenEvent> GameTokenTriggered;

        public class GameTokenEvent
        {
            public EGameTokenEvent Event { get; private set; }
            public IGameToken Token { get; private set; }

            public GameTokenEvent(EGameTokenEvent gameTokenEvent, IGameToken token)
            {
                Event = gameTokenEvent;
                Token = token;
            }
        }

        public override void OnGameTokenEvent(EGameTokenEvent arg0, IGameToken pGameToken)
        {
            GameTokenTriggered?.Invoke(new GameTokenEvent(arg0, pGameToken));
        }
    }
}
