// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine;
using System.Reflection;
using CryEngine.Resources;
using CryEngine.Common;

namespace CryEngine.DomainHandler
{
	/// <summary>
	/// Stores CRYENGINE state for restoration after an application run.
	/// </summary>
	class EngineState
	{
		private float _fov;

		public void Save()
		{
			_fov = Components.Camera.FieldOfView;
		}

		public void Load()
		{
			Components.Camera.FieldOfView = _fov;
		}
	}

	/// <summary>
	/// Entry Point to Mono world, called from CryMonoBridge on CryEngine startup.
	/// </summary>
    public class DomainHandler
    {
		private static EngineState m_EngineState = new EngineState();
		private static InterDomainHandler m_InterDomainHandler = new InterDomainHandler();

		public static InterDomainHandler GetDomainHandler()
		{
			return m_InterDomainHandler;
		}

		public static void Initialize()
		{
			Env.Initialize (m_InterDomainHandler);
		}

		private static void OnGameStart()
		{
			m_EngineState.Save ();
		}

		private static void OnGameStop()
		{
			m_EngineState.Load ();
		}

		public static void Shutdown()
		{
		}
    }
}
