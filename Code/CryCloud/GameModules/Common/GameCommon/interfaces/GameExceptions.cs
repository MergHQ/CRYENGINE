using System;
using System.Runtime.Serialization;

namespace GameCommon
{
	// TODO: place more game-logic errors/exceptions under this class
	[Serializable]
	public class GameException : Exception
	{
		public GameException(string msg) : base(msg) { }
		public GameException(string msg, Exception inner) : base(msg, inner) { }
		protected GameException(SerializationInfo info, StreamingContext context) : base(info, context) { }
	}
}
