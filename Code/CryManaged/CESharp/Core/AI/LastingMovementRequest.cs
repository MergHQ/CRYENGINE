using System;
using System.Runtime.InteropServices;
using CryEngine.Common;

namespace CryEngine
{
	public class ManagedMovementRequest : CLastingMovementRequest
	{
		private GCHandle _handle;
		private Action<MovementRequestResult> _callback;

		public ManagedMovementRequest(Action<MovementRequestResult> callback)
		{
			_handle = GCHandle.Alloc(this, GCHandleType.Pinned);
			_callback = callback;
		}

		public override void OnMovementRequestCallback(MovementRequestResult requestResult)
		{
			if(_callback != null)
			{
				_callback(requestResult);
			}
			_handle.Free();
		}
	}
}