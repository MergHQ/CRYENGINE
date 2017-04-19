// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryMath/Cry_Math.h>
#include <CryExtension/ICryUnknown.h>
#include <CryExtension/ClassWeaver.h>
#include <CryNetwork/SerializeFwd.h>
#include <CryAudio/IAudioSystem.h>
#include <CrySerialization/IArchiveHost.h>
#include <CrySerialization/Serializer.h>
#include <CryCore/Containers/CryArray.h>
#include <CryCore/BitMask.h>
#include <CryNetwork/ISerialize.h>
#include <CryNetwork/INetEntity.h>

struct SEntitySpawnParams;
struct SEntityEvent;
struct SEntityUpdateContext;
struct IShaderPublicParams;
struct IFlowGraph;
struct IEntityEventListener;
struct IPhysicalEntity;
struct SSGHandle;
struct a2DPoint;
struct IRenderMesh;
struct IClipVolume;
struct IBSPTree3D;
struct IMaterial;
struct IScriptTable;
struct AABB;
struct IRenderNode;

typedef uint64 EntityGUID;  //!< Same as in IEntity.h.

//! Derive from this interface to expose custom entity properties in the editor using the serialization framework.
//! Each entity component can contain one property group, each component will be separated by label in the entity property view
struct IEntityPropertyGroup
{
	virtual ~IEntityPropertyGroup() {}

	virtual const char* GetLabel() const = 0;
	virtual void        SerializeProperties(Serialization::IArchive& archive) = 0;
};

//! Entity proxies that can be hosted by the entity.
enum EEntityProxy
{
	ENTITY_PROXY_AUDIO,
	ENTITY_PROXY_AREA,
	ENTITY_PROXY_BOIDS,
	ENTITY_PROXY_BOID_OBJECT,
	ENTITY_PROXY_CAMERA,
	ENTITY_PROXY_FLOWGRAPH,
	ENTITY_PROXY_SUBSTITUTION,
	ENTITY_PROXY_TRIGGER,
	ENTITY_PROXY_ROPE,
	ENTITY_PROXY_ENTITYNODE,
	ENTITY_PROXY_CLIPVOLUME,
	ENTITY_PROXY_DYNAMICRESPONSE,
	ENTITY_PROXY_SCRIPT,

	ENTITY_PROXY_USER,

	//! Always the last entry of the enum.
	ENTITY_PROXY_LAST
};

//////////////////////////////////////////////////////////////////////////
//! Compatible to the CRYINTERFACE_DECLARE
#define CRY_ENTITY_COMPONENT_INTERFACE(iname, iidHigh, iidLow) CRYINTERFACE_DECLARE(iname, iidHigh, iidLow)

#define CRY_ENTITY_COMPONENT_CLASS(implclassname, interfaceName, cname, iidHigh, iidLow) \
  CRYGENERATE_CLASS_FROM_INTERFACE(implclassname, interfaceName, cname, iidHigh, iidLow)

#define CRY_ENTITY_COMPONENT_INTERFACE_AND_CLASS(implclassname, cname, iidHigh, iidLow) \
  CRY_ENTITY_COMPONENT_INTERFACE(implclassname, iidHigh, iidLow)                        \
  CRYGENERATE_CLASS_FROM_INTERFACE(implclassname, implclassname, cname, iidHigh, iidLow)

//! Base interface for all entity components.
struct IEntityComponent : public ICryUnknown
{
public:
	typedef int ComponentEventPriority;

public:
	//~ICryUnknown
	virtual ICryFactory* GetFactory() const { return 0; };

protected:
	virtual void* QueryInterface(const CryInterfaceID& iid) const { return 0; };
	virtual void* QueryComposite(const char* name) const          { return 0; };
	//~ICryUnknown

public:
	// Return Host entity pointer
	ILINE IEntity* GetEntity() const { return m_pEntity; };
	ILINE EntityId GetEntityId() const;

public:
	IEntityComponent() : m_pEntity(nullptr) {}
	virtual ~IEntityComponent() {}

	virtual EEntityProxy GetProxyType() const { return ENTITY_PROXY_LAST; };

	//! Called at the very first initialization of the component, at component creation time.
	virtual void Initialize() {}

	//! Called on all Entity components right before all of the Entity Components are destructed.
	virtual void OnShutDown() {};

	// By overriding this function component will be able to handle events sent from the host Entity.
	// Requires returning the desired event flag in GetEventMask.
	// \param event Event structure, contains event id and parameters.
	virtual void ProcessEvent(SEntityEvent& event) {}

	//! Return bit mask of the EEntityEvent flags that we want to receive in ProcessEvent (ex: BIT64(ENTITY_EVENT_HIDE)|BIT64(ENTITY_EVENT_UNHIDE))
	virtual uint64                 GetEventMask() const                      { return 0; }

	ComponentEventPriority         GetEventPriority() const                  { return (ComponentEventPriority)GetProxyType(); }
	virtual ComponentEventPriority GetEventPriority(const int eventID) const { return (ComponentEventPriority)GetProxyType(); }

	//! Optionally serialize component to/from XML.
	//! For user-facing properties, see GetProperties.
	virtual void                         LegacySerializeXML(XmlNodeRef& entityNode, XmlNodeRef& componentNode, bool bLoading) {}

	virtual struct IEntityPropertyGroup* GetPropertyGroup()                                                                   { return nullptr; }

	//! \brief Network serialization. Override to provide a mask of active network aspects
	//! used by this component. Called once during binding to network.
	virtual NetworkAspectType GetNetSerializeAspectMask() const { return 0; }

	//! \brief Network serialization. Will be called for each active aspect for both reading and writing.
	//! @param[in,out] ser Serializer for reading/writing values.
	//! @param[in] aspect The number of the aspect being serialized.
	//! @param[in] profile Can be ignored, used by CryPhysics only.
	//! @param[in] flags Can be ignored, used by CryPhysics only.
	//! \see ISerialize::Value()
	virtual bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags) { return true; };

	//! \brief Call this to trigger aspect synchronization over the network. A shortcut.
	//! \see INetEntity::MarkAspectsDirty()
	virtual void NetMarkAspectsDirty(const NetworkAspectType aspects); // The definition is in IEntity.h	

	//! SaveGame serialization. Override to specify what to serialize in a saved game.
	//! \param ser Serializing stream. Use IsReading() to decide read/write phase. Use Value() to read/write a property.
	virtual void GameSerialize(TSerialize ser) {}

	//! SaveGame serialization. Override to enable serialization for the component.
	//! \return true If component needs to be serialized to/from a saved game.
	virtual bool NeedGameSerialize()                     { return false; };

	virtual void GetMemoryUsage(ICrySizer* pSizer) const {};

protected:
	friend class CEntity;

	// Host Entity pointer
	IEntity* m_pEntity;
};

struct IEntityScript;

//! Lua Script component interface.
struct IEntityScriptComponent : public IEntityComponent
{
	CRY_ENTITY_COMPONENT_INTERFACE(IEntityScriptComponent, 0xBD6403CF3B49F39E, 0x95403FD1C6D4F755)

	virtual void          SetScriptUpdateRate(float fUpdateEveryNSeconds) = 0;
	virtual IScriptTable* GetScriptTable() = 0;
	virtual void          CallEvent(const char* sEvent) = 0;
	virtual void          CallEvent(const char* sEvent, float fValue) = 0;
	virtual void          CallEvent(const char* sEvent, bool bValue) = 0;
	virtual void          CallEvent(const char* sEvent, const char* sValue) = 0;
	virtual void          CallEvent(const char* sEvent, const Vec3& vValue) = 0;
	virtual void          CallEvent(const char* sEvent, EntityId nEntityId) = 0;

	//! Change current state of the entity script.
	//! \return If state was successfully set.
	virtual bool GotoState(const char* sStateName) = 0;

	//! Change current state of the entity script.
	//! \return If state was successfully set.
	virtual bool GotoStateId(int nStateId) = 0;

	//! Check if entity is in specified state.
	//! \param sStateName Name of state table within entity script (case sensitive).
	//! \return If entity script is in specified state.
	virtual bool IsInState(const char* sStateName) = 0;

	//! Retrieves name of the currently active entity script state.
	//! \return Name of current state.
	virtual const char* GetState() = 0;

	//! Retrieves the id of the currently active entity script state.
	//! \return Index of current state.
	virtual int GetStateId() = 0;

	//! Fires an event in the entity script.
	//! This will call OnEvent(id,param) Lua function in entity script, so that script can handle this event.
	virtual void SendScriptEvent(int Event, IScriptTable* pParamters, bool* pRet = NULL) = 0;
	virtual void SendScriptEvent(int Event, const char* str, bool* pRet = NULL) = 0;
	virtual void SendScriptEvent(int Event, int nParam, bool* pRet = NULL) = 0;

	//! Change the Entity Script used by the Script Component.
	//! Caller is responsible for making sure new script is initialised and script bound as required
	//! \param pScript an entity script object that has already been loaded with the new script.
	//! \param params parameters used to set the properties table if required.
	virtual void ChangeScript(IEntityScript* pScript, SEntitySpawnParams* params) = 0;

	//! Sets physics parameters from an existing script table
	//! \param type - one of PHYSICPARAM_... values
	//! \param params script table containing the values to set
	virtual void SetPhysParams(int type, IScriptTable *params) = 0;
};

//! Proximity trigger component interface.
struct IEntityTriggerComponent : public IEntityComponent
{
	CRY_ENTITY_COMPONENT_INTERFACE(IEntityTriggerComponent, 0xDE73851B7E35419F, 0xA50951D204F555DE)

	//! Creates a trigger bounding box.
	//! When physics will detect collision with this bounding box it will send an events to the entity.
	//! If entity have script OnEnterArea and OnLeaveArea events will be called.
	//! \param bbox Axis aligned bounding box of the trigger in entity local space (Rotation and scale of the entity is ignored). Set empty bounding box to disable trigger.
	virtual void SetTriggerBounds(const AABB& bbox) = 0;

	//! Retrieve trigger bounding box in local space.
	//! \return Axis aligned bounding box of the trigger in the local space.
	virtual void GetTriggerBounds(AABB& bbox) = 0;

	//! Forward enter/leave events to this entity
	virtual void ForwardEventsTo(EntityId id) = 0;

	//! Invalidate the trigger, so it gets recalculated and catches things which are already inside when it gets enabled.
	virtual void InvalidateTrigger() = 0;
};

//! Entity Audio component interface.
struct IEntityAudioComponent : public IEntityComponent
{
	CRY_ENTITY_COMPONENT_INTERFACE(IEntityAudioComponent, 0x9824845CFE377889, 0xB1724A63F331D8A2)

	virtual void                    SetFadeDistance(float const fadeDistance) = 0;
	virtual float                   GetFadeDistance() const = 0;
	virtual void                    SetEnvironmentFadeDistance(float const environmentFadeDistance) = 0;
	virtual float                   GetEnvironmentFadeDistance() const = 0;
	virtual float                   GetGreatestFadeDistance() const = 0;
	virtual void                    SetEnvironmentId(CryAudio::EnvironmentId const environmentId) = 0;
	virtual CryAudio::EnvironmentId GetEnvironmentId() const = 0;
	virtual CryAudio::AuxObjectId   CreateAudioAuxObject() = 0;
	virtual bool                    RemoveAudioAuxObject(CryAudio::AuxObjectId const audioAuxObjectId) = 0;
	virtual void                    SetAudioAuxObjectOffset(Matrix34 const& offset, CryAudio::AuxObjectId const audioAuxObjectId = CryAudio::DefaultAuxObjectId) = 0;
	virtual Matrix34 const&         GetAudioAuxObjectOffset(CryAudio::AuxObjectId const audioAuxObjectId = CryAudio::DefaultAuxObjectId) = 0;
	virtual bool                    PlayFile(CryAudio::SPlayFileInfo const& playbackInfo, CryAudio::AuxObjectId const audioAuxObjectId = CryAudio::DefaultAuxObjectId, CryAudio::SRequestUserData const& userData = CryAudio::SRequestUserData::GetEmptyObject()) = 0;
	virtual void                    StopFile(char const* const szFile, CryAudio::AuxObjectId const audioAuxObjectId = CryAudio::DefaultAuxObjectId) = 0;
	virtual bool                    ExecuteTrigger(CryAudio::ControlId const audioTriggerId, CryAudio::AuxObjectId const audioAuxObjectId = CryAudio::DefaultAuxObjectId, CryAudio::SRequestUserData const& userData = CryAudio::SRequestUserData::GetEmptyObject()) = 0;
	virtual void                    StopTrigger(CryAudio::ControlId const audioTriggerId, CryAudio::AuxObjectId const audioAuxObjectId = CryAudio::DefaultAuxObjectId, CryAudio::SRequestUserData const& userData = CryAudio::SRequestUserData::GetEmptyObject()) = 0;
	virtual void                    SetSwitchState(CryAudio::ControlId const audioSwitchId, CryAudio::SwitchStateId const audioStateId, CryAudio::AuxObjectId const audioAuxObjectId = CryAudio::DefaultAuxObjectId) = 0;
	virtual void                    SetParameter(CryAudio::ControlId const parameterId, float const value, CryAudio::AuxObjectId const audioAuxObjectId = CryAudio::DefaultAuxObjectId) = 0;
	virtual void                    SetObstructionCalcType(CryAudio::EOcclusionType const occlusionType, CryAudio::AuxObjectId const audioAuxObjectId = CryAudio::DefaultAuxObjectId) = 0;
	virtual void                    SetEnvironmentAmount(CryAudio::EnvironmentId const audioEnvironmentId, float const amount, CryAudio::AuxObjectId const audioAuxObjectId = CryAudio::DefaultAuxObjectId) = 0;
	virtual void                    SetCurrentEnvironments(CryAudio::AuxObjectId const audioAuxObjectId = CryAudio::DefaultAuxObjectId) = 0;
	virtual void                    AudioAuxObjectsMoveWithEntity(bool const bCanMoveWithEntity) = 0;
	virtual void                    AddAsListenerToAudioAuxObject(CryAudio::AuxObjectId const audioAuxObjectId, void (* func)(CryAudio::SRequestInfo const* const), CryAudio::ESystemEvents const eventMask) = 0;
	virtual void                    RemoveAsListenerFromAudioAuxObject(CryAudio::AuxObjectId const audioAuxObjectId, void (* func)(CryAudio::SRequestInfo const* const)) = 0;
};

//! Flags the can be set on each of the entity object slots.
enum EEntitySlotFlags
{
	ENTITY_SLOT_RENDER                      = BIT(0), //!< Draw this slot.
	ENTITY_SLOT_RENDER_NEAREST              = BIT(1), //!< Draw this slot as nearest. [Rendered in camera space].
	ENTITY_SLOT_RENDER_WITH_CUSTOM_CAMERA   = BIT(2), //!< Draw this slot using custom camera passed as a Public ShaderParameter to the entity.
	ENTITY_SLOT_IGNORE_PHYSICS              = BIT(3), //!< This slot will ignore physics events sent to it.
	ENTITY_SLOT_BREAK_AS_ENTITY             = BIT(4),
	ENTITY_SLOT_RENDER_AFTER_POSTPROCESSING = BIT(5),
	ENTITY_SLOT_BREAK_AS_ENTITY_MP          = BIT(6), //!< In MP this an entity that shouldn't fade or participate in network breakage.
};

//! Type of an area managed by IEntityAreaComponent.
enum EEntityAreaType
{
	ENTITY_AREA_TYPE_SHAPE,          //!< Area type is a closed set of points forming shape.
	ENTITY_AREA_TYPE_BOX,            //!< Area type is a oriented bounding box.
	ENTITY_AREA_TYPE_SPHERE,         //!< Area type is a sphere.
	ENTITY_AREA_TYPE_GRAVITYVOLUME,  //!< Area type is a volume around a bezier curve.
	ENTITY_AREA_TYPE_SOLID           //!< Area type is a solid which can have any geometry figure.
};

//! Area component allow for entity to host an area trigger.
//! Area can be shape, box or sphere, when marked entities cross this area border,
//! it will send ENTITY_EVENT_ENTERAREA, ENTITY_EVENT_LEAVEAREA, and ENTITY_EVENT_AREAFADE
//! events to the target entities.
struct IEntityAreaComponent : public IEntityComponent
{
	CRY_ENTITY_COMPONENT_INTERFACE(IEntityAreaComponent, 0x98EDA61FDE8BE2B1, 0xA1CA2A88E4EEDE66)

	enum EAreaComponentFlags
	{
		FLAG_NOT_UPDATE_AREA = BIT(1), //!< When set points in the area will not be updated.
		FLAG_NOT_SERIALIZE   = BIT(2)  //!< Areas with this flag will not be serialized.
	};

	//! Area flags.
	virtual void SetFlags(int nAreaComponentFlags) = 0;

	//! Area flags.
	virtual int GetFlags() = 0;

	//! Retrieve area type.
	//! \return One of EEntityAreaType enumerated types.
	virtual EEntityAreaType GetAreaType() const = 0;

	//! Sets area to be a shape, and assign points to this shape.
	//! Points are specified in local entity space, shape will always be constructed in XY plane,
	//! lowest Z of specified points will be used as a base Z plane.
	//! If fHeight parameter is 0, shape will be considered 2D shape, and during intersection Z will be ignored
	//! If fHeight is not zero shape will be considered 3D and will accept intersection within vertical range from baseZ to baseZ+fHeight.
	//! \param pPoints                   Array of 3D vectors defining shape vertices.
	//! \param pSoundObstructionSegments Array of corresponding booleans that indicate sound obstruction.
	//! \param numLocalPoints            Number of vertices in vPoints array.
	//! \param height                    Height of the shape.
	virtual void SetPoints(Vec3 const* const pPoints, bool const* const pSoundObstructionSegments, size_t const numLocalPoints, float const height) = 0;

	//! Sets area to be a Box, min and max must be in local entity space.
	//! Host entity orientation will define the actual world position and orientation of area box.
	virtual void SetBox(const Vec3& min, const Vec3& max, const bool* const pabSoundObstructionSides, size_t const nSideCount) = 0;

	//! Sets area to be a Sphere, center and radius must be specified in local entity space.
	//! Host entity world position will define the actual world position of the area sphere.
	virtual void SetSphere(const Vec3& vCenter, float fRadius) = 0;

	//! This function need to be called before setting convex hulls for a AreaSolid.
	//! Then AddConvexHullSolid() function is called as the number of convexhulls consisting of a geometry.
	//! \see AddConvexHullToSolid, EndSettingSolid
	virtual void BeginSettingSolid(const Matrix34& worldTM) = 0;

	//! Add a convex hull to a solid. This function have to be called after calling BeginSettingSolid()
	//! \see BeginSettingSolid, EndSettingSolid
	virtual void AddConvexHullToSolid(const Vec3* verticesOfConvexHull, bool bObstruction, int numberOfVertices) = 0;

	//! Finish setting a solid geometry. Generally the BSPTree based on convex hulls which is set before is created in this function.
	//!\see BeginSettingSolid, AddConvexHullToSolid
	virtual void EndSettingSolid() = 0;

	//! Retrieve number of points for shape area, return 0 if not area type is not shape.
	virtual int GetPointsCount() = 0;

	//! Retrieve array of points for shape area, will return NULL for all other area types.
	virtual const Vec3* GetPoints() = 0;

	//! Set shape area height, if height is 0 area is 2D.
	virtual void SetHeight(float const value) = 0;

	//! Retrieve shape area height, if height is 0 area is 2D.
	virtual float GetHeight() const = 0;

	//! Retrieve min and max in local space of area box.
	virtual void GetBox(Vec3& min, Vec3& max) = 0;

	//! Retrieve center and radius of the sphere area in local space.
	virtual void GetSphere(Vec3& vCenter, float& fRadius) = 0;

	virtual void SetGravityVolume(const Vec3* pPoints, int nNumPoints, float fRadius, float fGravity, bool bDontDisableInvisible, float fFalloff, float fDamping) = 0;

	//! Set area ID, this id will be provided to the script callback OnEnterArea, OnLeaveArea.
	virtual void SetID(const int id) = 0;

	//! Retrieve area ID.
	virtual int GetID() const = 0;

	//! Set area group id, areas with same group id act as an exclusive areas.
	//! If 2 areas with same group id overlap, entity will be considered in the most internal area (closest to entity).
	virtual void SetGroup(const int id) = 0;

	//! Retrieve area group id.
	virtual int GetGroup() const = 0;

	//! Set priority defines the individual priority of an area,
	//! Area with same group id will depend on which has the higher priority
	virtual void SetPriority(const int nPriority) = 0;

	//! Retrieve area priority.
	virtual int GetPriority() const = 0;

	//! Sets sound obstruction depending on area type
	virtual void SetSoundObstructionOnAreaFace(size_t const index, bool const bObstructs) = 0;

	//! Add target entity to the area.
	//! When someone enters/leaves an area, it will send ENTERAREA, LEAVEAREA, AREAFADE, events to these target entities.
	virtual void AddEntity(EntityId id) = 0;

	//! Add target entity to the area.
	//! When someone enters/leaves an area, it will send ENTERAREA, LEAVEAREA, AREAFADE, events to these target entities.
	virtual void AddEntity(EntityGUID guid) = 0;

	//! Remove target entity from the area.
	//! When someone enters/leaves an area, it will send ENTERAREA, LEAVEAREA, AREAFADE, events to these target entities.
	virtual void RemoveEntity(EntityId const id) = 0;

	//! Remove target entity from the area.
	//! When someone enters/leaves an area, it will send ENTERAREA ,LEAVEAREA, AREAFADE, events to these target entities.
	virtual void RemoveEntity(EntityGUID const guid) = 0;

	//! Removes all added target entities.
	virtual void RemoveEntities() = 0;

	//! Set area proximity region near the border.
	//! When someone is moving within this proximity region from the area outside border
	//! Area will generate ENTITY_EVENT_AREAFADE event to the target entity, with a fade ratio from 0, to 1.
	//! Where 0 will be at the area outside border, and 1 inside the area in distance fProximity from the outside area border.
	virtual void SetProximity(float fProximity) = 0;

	//! Retrieves area proximity.
	virtual float GetProximity() = 0;

	//! Compute and return squared distance to a point which is outside
	//! OnHull3d is the closest point on the hull of the area
	virtual float CalcPointNearDistSq(EntityId const nEntityID, Vec3 const& Point3d, Vec3& OnHull3d) = 0;

	//! Computes and returns squared distance from a point to the hull of the area
	//! OnHull3d is the closest point on the hull of the area
	//! This function is not sensitive of if the point is inside or outside the area
	virtual float ClosestPointOnHullDistSq(EntityId const nEntityID, Vec3 const& Point3d, Vec3& OnHull3d) = 0;

	//! Checks if a given point is inside the area.
	//! \note Ignoring the height speeds up the check.
	virtual bool CalcPointWithin(EntityId const nEntityID, Vec3 const& Point3d, bool const bIgnoreHeight = false) const = 0;

	//! get number of entities in area
	virtual size_t GetNumberOfEntitiesInArea() const = 0;

	//! get entity in area by index
	virtual EntityId GetEntityInAreaByIdx(size_t const index) const = 0;

	//! Retrieve inner fade distance of this area.
	virtual float GetInnerFadeDistance() const = 0;

	//! Set this area's inner fade distance.
	virtual void SetInnerFadeDistance(float const distance) = 0;
};

struct IClipVolumeComponent : public IEntityComponent
{
	CRY_ENTITY_COMPONENT_INTERFACE(IClipVolumeComponent, 0x92BC520EAAA2B3F0, 0x9095087AEE67D9FF)

	virtual void         SetGeometryFilename(const char* sFilename) = 0;
	virtual void         UpdateRenderMesh(IRenderMesh* pRenderMesh, const DynArray<Vec3>& meshFaces) = 0;
	virtual IClipVolume* GetClipVolume() const = 0;
	virtual IBSPTree3D*  GetBspTree() const = 0;
	virtual void         SetProperties(bool bIgnoresOutdoorAO) = 0;
};

//! Flow Graph component allows entity to host reference to the flow graph.
struct IEntityFlowGraphComponent : public IEntityComponent
{
	CRY_ENTITY_COMPONENT_INTERFACE(IEntityFlowGraphComponent, 0x17E5EBA757E44662, 0xA1C21F41DE946CDA)

	virtual void        SetFlowGraph(IFlowGraph* pFlowGraph) = 0;
	virtual IFlowGraph* GetFlowGraph() = 0;

	virtual void        AddEventListener(IEntityEventListener* pListener) = 0;
	virtual void        RemoveEventListener(IEntityEventListener* pListener) = 0;
};

//! Substitution component remembers IRenderNode this entity substitutes and unhides it upon deletion
struct IEntitySubstitutionComponent : public IEntityComponent
{
	CRY_ENTITY_COMPONENT_INTERFACE(IEntitySubstitutionComponent, 0x429B0BCE294749D9, 0xA458DF6FAEE6830C)

	virtual void         SetSubstitute(IRenderNode* pSubstitute) = 0;
	virtual IRenderNode* GetSubstitute() = 0;
};

//! Represents entity camera.
struct IEntityCameraComponent : public IEntityComponent
{
	CRY_ENTITY_COMPONENT_INTERFACE(IEntityCameraComponent, 0x9DA92DF237D74D2F, 0xB64FB827FCECFDD3)

	virtual void     SetCamera(CCamera& cam) = 0;
	virtual CCamera& GetCamera() = 0;
};

//! Component for the entity rope.
struct IEntityRopeComponent : public IEntityComponent
{
	CRY_ENTITY_COMPONENT_INTERFACE(IEntityRopeComponent, 0x368E5DCD0D954101, 0xB1F9DA514945F40C)

	virtual struct IRopeRenderNode* GetRopeRenderNode() = 0;
};

namespace DRS
{
struct IResponseActor;
struct IVariableCollection;
typedef std::shared_ptr<IVariableCollection> IVariableCollectionSharedPtr;
}

//! Component for dynamic response system actors.
struct IEntityDynamicResponseComponent : public IEntityComponent
{
	CRY_ENTITY_COMPONENT_INTERFACE(IEntityDynamicResponseComponent, 0x6799464783DD41B8, 0xA098E26B4B2C95FD)

	virtual void ReInit(const char* szName, const char* szGlobalVariableCollectionToUse) = 0;
	virtual DRS::IResponseActor* GetResponseActor() const = 0;
	virtual DRS::IVariableCollection* GetLocalVariableCollection() const = 0;
};

//! Component interface for GeomEntity to work in the CreateObject panel
struct IGeometryEntityComponent : public IEntityComponent
{
	CRY_ENTITY_COMPONENT_INTERFACE(IGeometryEntityComponent, 0x54B2C1308E274E07, 0x8BF0F1FE89228D14)

	virtual void SetGeometry(const char* szFilePath) = 0;
};

//! Component interface for ParticleEntity to work in the CreateObject panel
struct IParticleEntityComponent : public IEntityComponent
{
	CRY_ENTITY_COMPONENT_INTERFACE(IParticleEntityComponent, 0x68E3655DDDD34390, 0xAAD5448264E74461)

	virtual void SetParticleEffectName(const char* szEffectName) = 0;
};
