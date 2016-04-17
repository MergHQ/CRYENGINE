// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using CryEngine.Common;

namespace CryEngine
{
	/// <summary>
	/// Static wrapper around ILog (gEnv->pLog)
	/// </summary>
	public static class Log
	{
		public static string FileName
		{
			get
			{
				if (Global.gEnv == null || Global.gEnv.pLog == null)
					return null;
				return Global.gEnv.pLog.GetFileName ();
			}
			set
			{
				if (Global.gEnv == null || Global.gEnv.pLog == null)
					return;

				Global.gEnv.pLog.SetFileName (value);
			}
		}
		
		public static void Info(string msg)
		{
			if (Global.gEnv == null || Global.gEnv.pLog == null || String.IsNullOrEmpty(msg))
				return;

			Global.gEnv.pLog.Log (msg);
		}

		public static void Info(string format, params object[] args)
		{
			if (args == null || args.Length < 1)
				Info (format);
			else
				Info(String.Format(format, args));
		}

		public static void Always(string msg)
		{
			if (Global.gEnv == null || Global.gEnv.pLog == null || String.IsNullOrEmpty(msg))
				return;

			Global.gEnv.pLog.LogAlways (msg);
		}

		public static void Always(string format, params object[] args)
		{
			if (args == null || args.Length < 1)
				Always (format);
			else
				Always (String.Format (format, args));
		}

		public static void Warning(string msg)
		{
			if (Global.gEnv == null || Global.gEnv.pLog == null || String.IsNullOrEmpty(msg))
				return;

			Global.gEnv.pLog.LogWarning (msg);
		}

		public static void Warning(string format, params object[] args)
		{
			if (args == null || args.Length < 1)
				Warning (format);
			else
				Warning (String.Format (format, args));
		}

		public static void Error(string msg)
		{
			if (Global.gEnv == null || Global.gEnv.pLog == null || String.IsNullOrEmpty(msg))
				return;

			Global.gEnv.pLog.LogError (msg);
		}

		public static void Error(string format, params object[] args)
		{
			if (args == null || args.Length < 1)
				Error (format);
			else
				Error (String.Format (format, args));
		}

		public static void Plus(string msg)
		{
			if (Global.gEnv == null || Global.gEnv.pLog == null || String.IsNullOrEmpty(msg))
				return;

			Global.gEnv.pLog.LogPlus (msg);			
		}

		public static void Plus(string format, params object[] args)
		{
			if (args == null || args.Length < 1)
				Plus (format);
			else
				Plus (String.Format (format, args));
		}

		public static void ToConsole(string msg)
		{
			if (Global.gEnv == null || Global.gEnv.pLog == null || String.IsNullOrEmpty(msg))
				return;

			Global.gEnv.pLog.LogToConsole (msg);
		}

		public static void ToConsole(string format, params object[] args)
		{
			if (args == null || args.Length < 1)
				ToConsole (format);
			else
				ToConsole (String.Format (format, args));
		}

		public static void ToConsolePlus(string msg)
		{
			if (Global.gEnv == null || Global.gEnv.pLog == null || String.IsNullOrEmpty(msg))
				return;

			Global.gEnv.pLog.LogToConsolePlus (msg);
		}

		public static void ToConsolePlus(string format, params object[] args)
		{
			if (args == null || args.Length < 1)
				ToConsolePlus (format);
			else
				ToConsolePlus(String.Format(format, args));
		}

		public static void ToFile(string msg)
		{
			if (Global.gEnv == null || Global.gEnv.pLog == null || String.IsNullOrEmpty(msg))
				return;

			Global.gEnv.pLog.LogToFile (msg);
		}

		public static void ToFile(string format, params object[] args)
		{
			if (args == null || args.Length < 1)
				ToFile (format);
			else
				ToFile(String.Format(format, args));
		}

		public static void ToFilePlus(string msg)
		{
			if (Global.gEnv == null || Global.gEnv.pLog == null || String.IsNullOrEmpty(msg))
				return;

			Global.gEnv.pLog.LogToFilePlus (msg);
		}

		public static void ToFilePlus(string format, params object[] args)
		{
			if (args == null || args.Length < 1)
				ToFilePlus (format);
			else
				ToFilePlus(String.Format(format, args));
		}
	}
}
