// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using CryEngine.Common;

namespace CryEngine.EntitySystem
{	
	public class EntityUpdateListener : IMonoListener
	{
		#region Events
		/// <summary>
		/// Triggered each frame. First integer contains the updateFlags for the frame. Second integer contains the current pause mode.
		/// </summary>
		public event EventHandler<int, int> Update;
		#endregion

		#region Constructors
		/// <summary>
		/// This consutrctor will automatically register the MonoListener with CRYENGINE's mono runtime.
		/// </summary>
		public EntityUpdateListener ()
		{
			Global.gEnv.pMonoRuntime.RegisterListener (this);
		}
		#endregion

		#region Methods
		/// <summary>
		/// Upon dispose the MonoListener will automatically be unregistered from CRYENGINE's mono runtime.
		/// </summary>
	 	public override void Dispose ()
		{
			Global.gEnv.pMonoRuntime.UnregisterListener (this);
			base.Dispose ();
		}

		/// <summary>
		/// Translate 'OnUpdate' callback to managed 'Update' event.
		/// </summary>
		public override void OnUpdate (int updateFlags, int nPauseMode)
		{
			if (Update != null)
				Update (updateFlags, nPauseMode);
		}
		#endregion
	}
}
