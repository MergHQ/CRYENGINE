using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using System.IO;

using CryEngine.Common;

namespace CryEngine
{
	public static class FileSystem
	{
		static FileSystem()
		{
			EngineRootDirectory = Global.GetEnginePath().c_str();
		}

		public static string EngineRootDirectory { get; private set; }

		public static string DataDirectory
		{
			get
			{
				return Global.GetGameFolder().c_str() + "/";
			}
		} ///< Path where application data should be stored.

		private static string MonoDirectory
		{
			get
			{
				return Path.Combine(EngineRootDirectory, "Engine", "Mono");
			}
		}

		public static string GlobalAssemblyCacheDirectory
		{
			get
			{
				return Path.Combine(MonoDirectory, "lib", "mono", "gac");
			}
		}
	}
}
