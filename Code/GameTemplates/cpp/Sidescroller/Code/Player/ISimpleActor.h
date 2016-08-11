#pragma once

#include <IActorSystem.h>

// Helper struct to avoid having to implement all IGameObjectExtension functions every time
struct ISimpleActor : public IActor
{
	// IGameObjectExtension
	virtual bool Init(IGameObject* pGameObject) override {SetGameObject(pGameObject); return true;}
	virtual void PostInit(IGameObject* pGameObject) override {}
	virtual void HandleEvent(const SGameObjectEvent& event) override {}
	virtual void ProcessEvent(SEntityEvent& event) override {}
	virtual void InitClient(int channelId) override {}
	virtual void PostInitClient(int channelId) override {}
	virtual bool ReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) override {return true;}
	virtual void PostReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) override {}
	virtual bool GetEntityPoolSignature(TSerialize signature) override {return false;}
	virtual void Release() override {delete this;}
	virtual void FullSerialize(TSerialize ser) override {}
	virtual bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int pflags) override {return true;}
	virtual void PostSerialize() override {}
	virtual void SerializeSpawnInfo(TSerialize ser) override {}
	virtual ISerializableInfoPtr GetSpawnInfo() override {return nullptr;}
	virtual void Update(SEntityUpdateContext& ctx, int updateSlot) override {}
	virtual void SetChannelId(uint16 id) override {}
	virtual void SetAuthority(bool auth) override {}
	virtual void PostUpdate(float frameTime) override {}
	virtual void PostRemoteSpawn() override {}
	virtual void GetMemoryUsage(ICrySizer* pSizer) const override {}
	virtual ComponentEventPriority GetEventPriority(const int eventID) const override {return EEntityEventPriority_GameObject;}
	// ~IGameObjectExtension

	// IActor
	virtual void  SetHealth( float health ) override {}
	virtual float GetHealth() const override { return GetMaxHealth(); }
	virtual int   GetHealthAsRoundedPercentage() const override { return int(floor(GetHealth() / GetMaxHealth()) * 100.f); }
	virtual void  SetMaxHealth( float maxHealth ) override {}
	virtual float GetMaxHealth() const override  { return 100; }
	virtual int   GetArmor() const override  { return 0; }
	virtual int   GetMaxArmor() const override  { return 0; }

	virtual bool IsFallen() const override { return false; }
	virtual bool IsDead() const override { return GetHealth() <= 0; }
	virtual int  IsGod() override { return 0; }
	virtual void Fall(Vec3 hitPos = Vec3(0,0,0)) override {}
	virtual bool AllowLandingBob() override { return false; }

	virtual void                  PlayAction(const char* action,const char* extension, bool looping = false) override {}
	virtual IAnimationGraphState* GetAnimationGraphState() override { return nullptr; }
	virtual void                  ResetAnimationState() override {}

	virtual void CreateScriptEvent(const char* event,float value,const char* str = NULL) override {}
	virtual bool BecomeAggressiveToAgent(EntityId entityID) override { return false; }

	virtual void SetFacialAlertnessLevel(int alertness) override {}
	virtual void RequestFacialExpression(const char* pExpressionName = NULL, f32* sequenceLength = NULL) override {}
	virtual void PrecacheFacialExpression(const char* pExpressionName) override {}

	virtual EntityId GetGrabbedEntityId() const override { return INVALID_ENTITYID; }

	virtual void HideAllAttachments(bool isHiding) override {}

	virtual void SetIKPos(const char* pLimbName, const Vec3 &goalPos, int priority) override {}

	virtual void SetViewInVehicle(Quat viewRotation) override {}
	virtual void SetViewRotation( const Quat &rotation )  override {}
	virtual Quat GetViewRotation() const override { return IDENTITY; }

	virtual bool IsFriendlyEntity(EntityId entityId, bool bUsingAIIgnorePlayer = true) const override { return false; }

	virtual Vec3 GetLocalEyePos() const override { return ZERO; }

	virtual void CameraShake(float angle,float shift,float duration,float frequency,Vec3 pos,int ID,const char* source = "") override {}

	virtual IItem* GetHolsteredItem() const override { return nullptr; }
	virtual void   HolsterItem(bool holster, bool playSelect = true, float selectSpeedBias = 1.0f, bool hideLeftHandObject = true) override {}
	virtual IItem*      GetCurrentItem(bool includeVehicle = false) const override { return nullptr; }
	virtual bool        DropItem(EntityId itemId, float impulseScale = 1.0f, bool selectNext = true, bool byDeath = false) override { return false; }
	virtual IInventory* GetInventory() const override { return nullptr; }
	virtual void        NotifyCurrentItemChanged(IItem* newItem) override {}

	virtual IMovementController* GetMovementController() const override { return nullptr; }

	virtual IEntity* LinkToVehicle(EntityId vehicleId) override { return nullptr; }
	virtual IEntity* GetLinkedEntity() const override { return nullptr; }

	virtual uint8 GetSpectatorMode() const override { return 0; }

	virtual bool IsThirdPerson() const override { return false; }
	virtual void ToggleThirdPerson() override {}

	virtual bool IsPlayer() const override { return false; };
	virtual bool IsClient() const override { return false; }
	virtual bool IsMigrating() const override { return false; }
	virtual void SetMigrating(bool isMigrating) override {}

	virtual void InitLocalPlayer() override {}

	virtual const char* GetActorClassName() const override { return GetEntityClassName(); }
	virtual ActorClass  GetActorClass() const override { return 0; }

	virtual const char* GetEntityClassName() const override 
	{ 
		return GetEntity()->GetClass()->GetName();
	}

	virtual void SerializeXML( XmlNodeRef &node, bool bLoading ) override {}
	virtual void SerializeLevelToLevel( TSerialize &ser ) override {}
	
	virtual IAnimatedCharacter*       GetAnimatedCharacter() override { return nullptr; }
	virtual const IAnimatedCharacter* GetAnimatedCharacter() const override { return nullptr; }
	virtual void                      PlayExactPositioningAnimation( const char* sAnimationName, bool bSignal, const Vec3 &vPosition, const Vec3 &vDirection, float startWidth, float startArcAngle, float directionTolerance ) override {}
	virtual void                      CancelExactPositioningAnimation() override {}
	virtual void                      PlayAnimation(const char* sAnimationName, bool bSignal) override {}

	virtual void EnableTimeDemo( bool bTimeDemo ) override {}

	virtual void SwitchDemoModeSpectator(bool activate) override {}

	virtual IVehicle* GetLinkedVehicle() const override { return nullptr; }

	virtual void OnAIProxyEnabled(bool enabled) override {}
	virtual void OnReturnedToPool() override {}
	virtual void OnPreparedFromPool() override {}
	
	virtual void OnReused(IEntity* pEntity, SEntitySpawnParams &params) override {}

	// Begin unreferenced IActor mentions
	virtual int   GetTeamId() const override { return 0; }
	virtual bool ShouldMuteWeaponSoundStimulus() const override { return false; }
	// ~IActor

	ISimpleActor() {}
	virtual ~ISimpleActor() {}
};