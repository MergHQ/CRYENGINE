// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Reflection;
using System.Collections.Generic;

namespace CryEngine.DomainHandler
{
	public class InterDomainHandler : MarshalByRefObject
	{
		#region Fields
		private Dictionary<uint, Tuple<string, Dictionary<string, string>>> _entityCache = new Dictionary<uint, Tuple<string, Dictionary<string, string>>>();
		#endregion

		#region Properties
		public Dictionary<uint, Tuple<string, Dictionary<string, string>>> EntityCache { get { return _entityCache; } }
		#endregion

		#region Events
		public event EventHandler RequestQuit;
		#endregion

		#region Methods
		public void RaiseRequestQuit()
		{
			if (RequestQuit != null)
				RequestQuit (this, EventArgs.Empty);
		}
		#endregion
	}

	/// <summary>
	/// Entry Point to Mono world, called from CryMonoBridge on CryEngine startup.
	/// </summary>
    public class DomainHandler
    {
		private static InterDomainHandler m_InterDomainHandler = new InterDomainHandler();

		public static InterDomainHandler GetDomainHandler()
		{
			return m_InterDomainHandler;
		}
    }
}
