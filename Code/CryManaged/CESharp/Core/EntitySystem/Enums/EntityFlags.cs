// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

namespace CryEngine
{
	/// <summary>
	/// Each <see cref="Entity"/> instance holds flags that indicate special behavior within the system.
	/// These can be modified by using <see cref="Entity.Flags"/>.
	/// Managed version of <see cref="Common.EEntityFlags"/>.
	/// </summary>
	[System.Flags]
	public enum EntityFlags : uint
	{
		#region Persistent flags

		/// <summary>
		/// Persistent flag that can be set from the editor. Indicates that the <see cref="Entity"/> can cast shadows.
		/// </summary>
		CastShadow = Common.EEntityFlags.ENTITY_FLAG_CASTSHADOW,

		/// <summary>
		/// Persistent flag that can be set from the editor. Entities with this flag cannot be removed using <see cref="Entity.Remove()"/> until this flag is cleared.
		/// </summary>
		Unremovable = Common.EEntityFlags.ENTITY_FLAG_UNREMOVABLE,

		/// <summary>
		/// Persistent flag that can be set from the editor. Indicates that the <see cref="Entity"/> can be very effective as an occluder.
		/// </summary>
		GoodOccluder = Common.EEntityFlags.ENTITY_FLAG_GOOD_OCCLUDER,

		/// <summary>
		/// Persistent flag that can be set from the editor. Indicates that the <see cref="Entity"/> can't receive static decal projections.
		/// </summary>
		NoDecalnodeDecals = Common.EEntityFlags.ENTITY_FLAG_NO_DECALNODE_DECALS,

		#endregion

		/// <summary>
		/// Indicates that the <see cref="Entity"/> should only be present on the client, and not server.
		/// </summary>
		ClientOnly = Common.EEntityFlags.ENTITY_FLAG_CLIENT_ONLY,

		/// <summary>
		/// Indicates that the <see cref="Entity"/> should only be present on the server, and not clients.
		/// </summary>
		ServerOnly = Common.EEntityFlags.ENTITY_FLAG_SERVER_ONLY,

		/// <summary>
		/// This <see cref="Entity"/> has a special custom view distance ratio (AI/Vehicles require it).
		/// </summary>
		CustomViewdistRatio = Common.EEntityFlags.ENTITY_FLAG_CUSTOM_VIEWDIST_RATIO,

		/// <summary>
		/// Use character and objects in BBox calculations.
		/// </summary>
		CalcbboxUseall = Common.EEntityFlags.ENTITY_FLAG_CALCBBOX_USEALL,

		/// <summary>
		/// The <see cref="Entity"/> is a volume sound (will get moved around by the sound proxy).
		/// </summary>
		VolumeSound = Common.EEntityFlags.ENTITY_FLAG_VOLUME_SOUND,

		/// <summary>
		/// The <see cref="Entity"/> has an AI object.
		/// </summary>
		HasAi = Common.EEntityFlags.ENTITY_FLAG_HAS_AI,

		/// <summary>
		/// This <see cref="Entity"/> will trigger areas when it enters them.
		/// </summary>
		TriggerAreas = Common.EEntityFlags.ENTITY_FLAG_TRIGGER_AREAS,

		/// <summary>
		/// This <see cref="Entity"/> will not be saved.
		/// </summary>
		NoSave = Common.EEntityFlags.ENTITY_FLAG_NO_SAVE,

		/// <summary>
		/// This <see cref="Entity"/> is a camera source.
		/// </summary>
		CameraSource = Common.EEntityFlags.ENTITY_FLAG_CAMERA_SOURCE,

		/// <summary>
		/// Prevents error when state changes on the client and does not sync state changes to the client.
		/// </summary>
		ClientsideState = Common.EEntityFlags.ENTITY_FLAG_CLIENTSIDE_STATE,

		/// <summary>
		/// When set the <see cref="Entity"/> will send ENTITY_EVENT_RENDER_VISIBILITY_CHANGE when it starts or stops actual rendering.
		/// </summary>
		SendRenderEvent = Common.EEntityFlags.ENTITY_FLAG_SEND_RENDER_EVENT,

		/// <summary>
		/// The <see cref="Entity"/> will not be registered in the partition grid and can not be found by proximity queries.
		/// </summary>
		NoProximity = Common.EEntityFlags.ENTITY_FLAG_NO_PROXIMITY,

		/// <summary>
		/// The <see cref="Entity"/> has been generated at runtime.
		/// </summary>
		Procedural = Common.EEntityFlags.ENTITY_FLAG_PROCEDURAL,

		/// <summary>
		/// Whether update of game logic should be skipped when the <see cref="Entity"/> is hidden. This does *not* disable update of components unless they specifically request it.
		/// </summary>
		UpdateHidden = Common.EEntityFlags.ENTITY_FLAG_UPDATE_HIDDEN,
        
		/// <summary>
		/// Used by the Editor only, don't set.
		/// </summary>
		IgnorePhysicsUpdate = Common.EEntityFlags.ENTITY_FLAG_IGNORE_PHYSICS_UPDATE,

		/// <summary>
		/// The <see cref="Entity"/> was spawned dynamically without a class.
		/// </summary>
		Spawned = Common.EEntityFlags.ENTITY_FLAG_SPAWNED,

		/// <summary>
		/// The <see cref="Entity"/>'s slots were changed dynamically.
		/// </summary>
		SlotsChanged = Common.EEntityFlags.ENTITY_FLAG_SLOTS_CHANGED,

		/// <summary>
		/// The <see cref="Entity"/> was procedurally modified by physics.
		/// </summary>
		ModifiedByPhysics = Common.EEntityFlags.ENTITY_FLAG_MODIFIED_BY_PHYSICS,

		/// <summary>
		/// Same as Brush->Outdoor only.
		/// </summary>
		OutdoorOnly = Common.EEntityFlags.ENTITY_FLAG_OUTDOORONLY,

		/// <summary>
		/// Flag to indicate that the <see cref="Entity"/> can receive wind.
		/// </summary>
		ReceiveWind = Common.EEntityFlags.ENTITY_FLAG_RECVWIND,
		
		/// <summary>
		/// Flag to indicate the <see cref="Entity"/> is a local player.
		/// </summary>
		LocalPlayer = Common.EEntityFlags.ENTITY_FLAG_LOCAL_PLAYER,

		/// <summary>
		/// AI can use the object to calculate automatic hide points.
		/// </summary>
		AiHideable = Common.EEntityFlags.ENTITY_FLAG_AI_HIDEABLE,

		/// <summary>
		/// Components known at load time were allocated in one contiguous chunk of memory along with the entity.
		/// </summary>
		DynamicDistanceShadows = Common.EEntityFlags.ENTITY_FLAG_DYNAMIC_DISTANCE_SHADOWS
	}
}
