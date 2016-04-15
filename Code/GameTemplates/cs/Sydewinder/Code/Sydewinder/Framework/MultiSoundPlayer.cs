// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.IO;
using System.Collections.Generic;
using System.Linq;

namespace CryEngine.Sydewinder
{
	public class MultiSoundPlayer : IGameUpdateReceiver
	{
		[DllImport("winmm.dll")]
		static extern Int32 mciSendString(string command, StringBuilder buffer, int bufferSize, IntPtr hwndCallback);

		private static Dictionary<string, string> _aliasByFile = new Dictionary<string, string> ();

		public string Alias { get; private set; }
		public bool IsLooping { get; private set; }
		public bool IsPlaying { get; private set; }

		public int TrackLength
		{
			get 
			{
				var sb = new StringBuilder();
				mciSendString(string.Format("status {0} length", Alias), sb, 255, IntPtr.Zero);
				return Convert.ToInt32(sb.ToString());
			}
		}

		public int TrackPosition
		{
			get 
			{
				var sb = new StringBuilder();
				mciSendString(string.Format("status {0} position", Alias), sb, 255, IntPtr.Zero);
				return Convert.ToInt32(sb.ToString());
			}
		}

		public string FreeAlias
		{
			get
			{
				var r = new Random ((int)DateTime.Now.Ticks);
				string alias;
				do 
				{
					alias = "A" + r.Next (10000).ToString ("0000");
				} 
				while (_aliasByFile.Values.SingleOrDefault (x => x == alias) != null);
				return alias;
			}
		}

		public MultiSoundPlayer(string soundLocation, bool isLooping = false)
		{
			string alias;
			if (!_aliasByFile.TryGetValue (soundLocation, out alias)) 
			{
				alias = _aliasByFile[soundLocation] = FreeAlias;
				mciSendString (string.Format ("open \"{0}\" alias {1}", soundLocation, alias), null, 0, IntPtr.Zero);
			}
			Alias = alias;
			IsLooping = isLooping;
		}

		public void Play()
		{
			mciSendString(string.Format("play {0} from 0", Alias), null, 0, IntPtr.Zero);
			if (!IsPlaying)
				GameFramework.RegisterForUpdate (this);
			IsPlaying = true;
		}

		public void Stop()
		{
			if (IsPlaying)
				GameFramework.UnregisterFromUpdate (this);
			IsPlaying = false;
			mciSendString(string.Format("stop {0}", Alias), null, 0, IntPtr.Zero);
		}

		public void OnUpdate()
		{
			if (IsPlaying && TrackLength <= TrackPosition) 
			{
				Stop ();
				if (IsLooping)
					Play ();
			}
		}
	}
}
