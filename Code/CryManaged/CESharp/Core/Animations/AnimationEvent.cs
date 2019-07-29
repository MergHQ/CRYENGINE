// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

namespace CryEngine.Animations
{
	/// <summary>
	/// Contains data about an animation event.
	/// </summary>
	public struct AnimationEvent
	{
		/// <summary>
		/// The time of the event.
		/// </summary>
		public float Time { get; set; }

		/// <summary>
		/// The time the event ends.
		/// </summary>
		public float EndTime { get; set; }

		/// <summary>
		/// The CRC32 of the <see cref="EventName"/> in lower-case.
		/// </summary>
		public uint EventNameLowerCaseCRC32 { get; set; }

		/// <summary>
		/// The name of the event.
		/// </summary>
		public string EventName { get; set; }

		/// <summary>
		/// The meaning of <see cref="CustomParameter"/> changes depending on the event.
		/// For example for a sound event it will be the sound path, for an effect event it will be the effect name.
		/// </summary>
		public string CustomParameter { get; set; }

		/// <summary>
		/// The path of the bone the event happens on.
		/// </summary>
		public string BonePathName { get; set; }

		/// <summary>
		/// The offset of the AnimationEvent as set in the Character Tool.
		/// </summary>
		public Vector3 Offset { get; set; }

		/// <summary>
		/// The direction of the AnimationEvent as set in the Character Tool.
		/// </summary>
		public Vector3 Direction { get; set; }

		internal AnimationEvent(Common.AnimEventInstance nativeInstance)
		{
			Time = nativeInstance.m_time;
			EndTime = nativeInstance.m_endTime;
			EventNameLowerCaseCRC32 = nativeInstance.m_EventNameLowercaseCRC32;
			EventName = nativeInstance.m_EventName;
			CustomParameter = nativeInstance.m_CustomParameter;
			BonePathName = nativeInstance.m_BonePathName;
			Offset = nativeInstance.m_vOffset;
			Direction = nativeInstance.m_vDir;
		}
	}
}
