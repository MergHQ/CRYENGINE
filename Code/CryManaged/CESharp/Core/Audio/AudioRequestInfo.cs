// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using CryEngine.Common.CryAudio;

namespace CryEngine
{
	using NativeAudioSystem = NativeInternals.IAudioSystem;

	/// <summary>
	/// The result of an audio request.
	/// </summary>
	public enum RequestResult
	{
		/// <summary>
		/// The default state of a result.
		/// </summary>
		None,
		/// <summary>
		/// State of the result if the request is successful.
		/// </summary>
		Success,
		/// <summary>
		/// State of the result if the request failed.
		/// </summary>
		Failure
	}

	/// <summary>
	/// Flags to define which events to receive on a RequestListener.
	/// </summary>
	[Flags]
	public enum SystemEvents : uint
	{
		/// <summary>
		/// The default value of SystemEvents. This means no events will be received.
		/// </summary>
		None = 0,
		/// <summary>
		/// Receive events when the audio implementation is set.
		/// </summary>
		ImplSet = ESystemEvents.ImplSet,
		/// <summary>
		/// Receive events when a trigger is executed.
		/// </summary>
		TriggerExecuted = ESystemEvents.TriggerExecuted,
		/// <summary>
		/// Receive events when a trigger has finished.
		/// </summary>
		TriggerFinished = ESystemEvents.TriggerFinished,
	}

	/// <summary>
	/// Handles the data of an audio request.
	/// </summary>
	public sealed class AudioRequestInfo : IDisposable
	{
		private System.Runtime.InteropServices.HandleRef _cPtr;

		private bool _isCopy;
		private uint _requestResult;
		private uint _flagsType;
		private uint _controlId;
		private uint _entityId;
		private bool _isDisposed;

		/// <summary>
		/// Constructor for an AudioRequestInfo object.
		/// </summary>
		/// <param name="eRequestResult"></param>
		/// <param name="audioSystemEvent"></param>
		/// <param name="CtrlId"></param>
		/// <param name="entityId"></param>
		public AudioRequestInfo(uint eRequestResult, uint audioSystemEvent, uint CtrlId, uint entityId) : this(NativeAudioSystem.CreateSRequestInfo(eRequestResult, audioSystemEvent, CtrlId, entityId))
		{
		}

		internal AudioRequestInfo(IntPtr nativePtr, bool isCopy = false)
		{
			_isCopy = isCopy;
			_cPtr = new System.Runtime.InteropServices.HandleRef(this, nativePtr);
			if(_isCopy)
			{
				_requestResult = NativeAudioSystem.SRIGetRequestResult(nativePtr);
				_flagsType = NativeAudioSystem.SRIGetEnumFlagsType(nativePtr);
				_controlId = NativeAudioSystem.SRIGetControlId(nativePtr);
                _entityId = NativeAudioSystem.SRIGetEntityId(nativePtr);
			}
		}

		/// <summary>
		/// Destructor for the AudioRequestInfo which ensures that Dispose is called.
		/// </summary>
		~AudioRequestInfo()
		{
			Dispose(false);
		}

		private void Dispose(bool isDisposing)
		{
			if(_isDisposed)
				return;

			if(_cPtr.Handle != IntPtr.Zero && !_isCopy)
			{
				NativeAudioSystem.ReleaseSRequestInfo(_cPtr.Handle);
				_cPtr = new System.Runtime.InteropServices.HandleRef(null, IntPtr.Zero);
			}
			_isDisposed = true;
		}

		/// <summary>
		/// Disposes of this AudioRequestInfo.
		/// </summary>
		public void Dispose()
		{
			Dispose(true);
			GC.SuppressFinalize(this);
		}

		/// <summary>
		/// Returns the result of the request.
		/// </summary>
		public RequestResult RequestResult
		{
			get
			{
				return (RequestResult)(_isCopy ? _requestResult : NativeAudioSystem.SRIGetRequestResult(_cPtr.Handle));
			}
		}

		/// <summary>
		/// Returns the eventmask of events this request is listening to.
		/// </summary>
		[Obsolete("Use the SystemEvents property instead.")]
		public SystemEvents FlagsType
		{
			get { return SystemEvents; }
		}

		/// <summary>
		/// Returns the eventmask of events this request is listening to.
		/// </summary>
		public SystemEvents SystemEvents
		{
			get
			{
				return (SystemEvents)(_isCopy ? _flagsType : NativeAudioSystem.SRIGetEnumFlagsType(_cPtr.Handle));
			}
		}

		/// <summary>
		/// The ID of the control.
		/// </summary>
		public uint ControlId
		{
			get
			{
				return _isCopy ? _controlId : NativeAudioSystem.SRIGetControlId(_cPtr.Handle);
			}
		}

		internal uint EntityId
		{
			get
			{
				return _isCopy ? _entityId : NativeAudioSystem.SRIGetEntityId(_cPtr.Handle);
			}
		}
	}
}