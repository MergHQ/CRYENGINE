using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using CryEngine.Common;

namespace CryEngine.EntitySystem
{
	/// <summary>
	/// Wraps an Action that is invoked when an Entity Event of the specified Trigger occurs.
	/// </summary>
	public class EntityEventCallback
	{
		public Action<SEntityEvent> Callback { get; private set; }
		public EEntityEvent Trigger { get; private set; }

		public EntityEventCallback(Action<SEntityEvent> onCallback, EEntityEvent trigger)
		{
			Callback = onCallback;
			Trigger = trigger;
		}
	}
}
