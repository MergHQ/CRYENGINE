using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace CryEngine
{
	/// <summary>
	/// Represents the entry point of a C# CRYENGINE plug-in
	/// Only one type per assembly may implement this
	/// An instance will be created shortly after the assembly has been loaded.
	/// </summary>
	public interface ICryEnginePlugin
	{
		void Initialize();
		/// <summary>
		/// Called when engine is being shut down
		/// </summary>
		void Shutdown();

        void OnLevelLoaded();

        void OnGameStart();

        void OnGameStop();
	}
}
