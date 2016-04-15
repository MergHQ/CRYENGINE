local Behavior = CreateAIBehavior("Dumb",
{
	Constructor = function(self, entity)
		if ( AI.GetTypeOf( entity.id ) == AIOBJECT_VEHICLE ) then
		else

			AI.Signal(SIGNALFILTER_LEADER,0,"OnDisableFire",entity.id);
			AI.Signal(SIGNALFILTER_SENDER, 0,"just_constructed",entity.id);	
			entity:SelectPipe(0,"do_nothing");
			entity.bBombPlanted = nil;
			local orientationTarget = System.GetEntityByName(entity:GetName().."_dir");
			if(orientationTarget) then
				entity.actor:SetMovementTarget(entity:GetWorldPos(),orientationTarget:GetWorldPos(),{x=0,y=0,z=0},1);
			end
			entity.AI.FollowStance = BODYPOS_STAND;
			entity:InitAIRelaxed();
		
		end

	end,
	
	Destructor = function( self, entity )	
		AI.SetIgnorant(entity.id,0);
	end,
	
 	OnQueryUseObject = function ( self, entity, sender, extraData )
 	end,
	OnCloseCollision = function( self, entity, sender, data )
	end,
	OnExposedToExplosion = function(self, entity, sender, data)
	end,

 	---------------------------------------------
 	
	START_VEHICLE = function(self,entity,sender)
	end,
	---------------------------------------------
--	VEHICLE_REFPOINT_REACHED = function( self,entity, sender )
--		-- called by vehicle when it reaches the reference Point 
--		--entity.AI.theVehicle:SignalCrew("exited_vehicle");
--		AI.Signal(SIGNALFILTER_SENDER,1,"STOP_AND_EXIT",entity.AI.theVehicle.id);
--	end,

	---------------------------------------------
	OnSelected = function( self, entity )	
	end,
	---------------------------------------------
	OnActivate = function( self, entity )
		-- called when enemy receives an activate event (from a trigger, for example)
	end,
	---------------------------------------------
	OnNoTarget = function( self, entity )
	
		--AI.Signal(SIGNALFILTER_GROUPONLY, 1, "GunnerLostTarget",entity.id);
		
		--AI.LogEvent("\001 gunner in vehicle lost target ");
		-- caLled when the enemy stops having an attention target
	end,
	---------------------------------------------
	OnEnemySeen = function( self, entity, fDistance )
	
		-- called when the enemy sees a living player
	end,
	
	---------------------------------------------
	OnSeenByEnemy = function( self, entity )
		
	end,
	
	---------------------------------------------
	OnEnemyMemory = function( self, entity )
		-- called when the enemy can no longer see its foe, but remembers where it saw it last
	end,
	---------------------------------------------
	OnEnemyDamage = function ( self, entity, sender)
		-- called when the enemy is damaged
	end,

	---------------------------------------------
	OnFriendlyDamage= function ( self, entity, sender)
		-- called when the enemy is damaged
	end,
	---------------------------------------------
	OnInterestingSoundHeard = function( self, entity )
		-- called when the enemy hears an interesting sound
	end,
	---------------------------------------------
	OnThreateningSoundHeard = function( self, entity )
		-- called when the enemy hears a scary sound
	end,
	---------------------------------------------
	OnReload = function( self, entity )
		-- called when the enemy goes into automatic reload after its clip is empty
	end,
	---------------------------------------------
	OnGroupMemberDied = function( self, entity )
		-- called when a member of the group dies
	end,
	OnGroupMemberDiedNearest = function ( self, entity, sender)
	end,
	---------------------------------------------
	OnNoHidingPlace = function( self, entity, sender )
		-- called when no hiding place can be found with the specified parameters
	end,	
	--------------------------------------------------
	OnNoFormationPoint = function ( self, entity, sender)
		-- called when the enemy found no formation point
	end,
	---------------------------------------------
	OnReceivingDamage = function ( self, entity, sender)
		-- called when the enemy is damaged
	end,
	---------------------------------------------
	OnCoverRequested = function ( self, entity, sender)
		-- called when the enemy is damaged
	end,
	--------------------------------------------------
	OnDeath = function( self,entity )
	end,
	--------------------------------------------------
	OnBulletRain = function ( self, entity, sender)
	end,
	--------------------------------------------------
	OnGroupChanged = function ( self, entity, sender)
	end,

	--------------------------------------------------
	OnBulletHit = function( self, entity, sender,data )
	end,

 	---------------------------------------------
	OnTargetCloaked = function(self, entity)
	end,

	---------------------------------------------
	---------------------------------------------	--------------------------------------------------
	-- CUSTOM
	--------------------------------------------
	
	--------------------------------------------------
	SHARED_ENTER_ME_VEHICLE = function( self,entity, sender )
	
	-- in vehicle already - don't do anything

	end,

	--------------------------------------------------

	--------------------------------------------------
	SHARED_LEAVE_ME_VEHICLE = function( self,entity, sender )

	end,

	exited_vehicle = function( self,entity, sender )
--		AI.Signal(0, 1, "DRIVER_OUT",sender.id);
	end,

	
	
	---------------------------------------------
	---------------------------------------------	--------------------------------------------------
	-- old FC stuff - to be revised	
	---------------------------------------------	--------------------------------------------------
	

	exited_vehicle_investigate = function( self,entity, sender )

	end,

	--------------------------------------------
	do_exit_vehicle = function( self,entity, sender )
	end,


	-- no need to run away from cars
	OnVehicleDanger = function(self,entity,sender)
	end,

	EXIT_VEHICLE_STAND = function(self,entity,sender)
	end,
	
	
	ORDER_EXIT_VEHICLE = function(self,entity,sender)
	end,

 	---------------------------------------------
	-- ignore this orders when in vehicle
	ORDER_FOLLOW = function(self,entity,sender)
	end,
	ORDER_HIDE = function(self,entity,sender)
	end,
	ORDER_FIRE = function(self,entity,sender)
	end,

--	ACT_FOLLOW = function(self,entity,sender)
--	end,

	OnDamage = function(self,entity,sender)
	end,

	OnCloseContact = 	function(self,entity,sender)
	end,

	OnGrenadeSeen = 	function(self,entity,sender)
	end,
	
	OnSomebodyDied = 	function(self,entity,sender)
	end,
	
	OnSomethingSeen = 	function(self,entity,sender)
	end,

	---------------------------------------------
	OnThreateningSeen = function( self, entity )
	end,

	---------------------------------------------	
	OnCollision	= function(self,entity,sender)
	end,
	
	---------------------------------------------	
	OnLeaderActionCompleted	= function(self,entity,sender)
	end,
	
	---------------------------------------------	
	OnPlayerSticking = function(self,entity,sender,data)
	end,

	---------------------------------------------
	OnPlayerLooking = function(self,entity,sender,data)
	end,

	---------------------------------------------
	OnPlayerLookingAway = function(self,entity,sender,data)
	end,
	
	---------------------------------------------
	OnPlayerGoingAway = function(self,entity,sender,data)
	end,
	
	---------------------------------------------
	OnNoTargetVisible = function(self,entity,sender,data)
	end,

	---------------------------------------------
	OnNoTargetAwareness = function(self,entity,sender,data)
	end,
	
	---------------------------------------------
	HEADS_UP_GUYS = function (self, entity, sender)
	end,
	---------------------------------------------
	INCOMING_FIRE = function (self, entity, sender)
	end,
	---------------------------------------------
	KEEP_FORMATION = function (self, entity, sender)
	end,
	---------------------------------------------	
	MOVE_IN_FORMATION = function (self, entity, sender)
		-- the team leader wants everyone to move in formation
	end,
	---------------------------------------------	
	BREAK_FORMATION = function (self, entity, sender)
		-- the team can split
	end,
	---------------------------------------------	
	SINGLE_GO = function (self, entity, sender)
		-- the team leader has instructed this group member to approach the enemy
	end,
	---------------------------------------------	
	GROUP_COVER = function (self, entity, sender)
		-- the team leader has instructed this group member to cover his friends
	end,
	---------------------------------------------	
	IN_POSITION = function (self, entity, sender)
		-- some member of the group is safely in position
	end,
	
	---------------------------------------------	
	PHASE_RED_ATTACK = function (self, entity, sender)
		-- team leader instructs red team to attack
	end,
	---------------------------------------------	
	PHASE_BLACK_ATTACK = function (self, entity, sender)
		-- team leader instructs black team to attack
	end,
	---------------------------------------------	
	GROUP_MERGE = function (self, entity, sender)
		-- team leader instructs groups to merge into a team again
	end,
	---------------------------------------------	
	CLOSE_IN_PHASE = function (self, entity, sender)
		-- team leader instructs groups to initiate part one of assault fire maneuver
	end,
	---------------------------------------------	
	ASSAULT_PHASE = function (self, entity, sender)
		-- team leader instructs groups to initiate part one of assault fire maneuver
	end,
	---------------------------------------------	
	GROUP_NEUTRALISED = function (self, entity, sender)
		-- team leader instructs groups to initiate part one of assault fire maneuver
	end,
	
	---------------------------------------------	
	LOOK_CLOSER = function(self,entity,sender)
	end,
	---------------------------------------------	
	GO_THREATEN = function(self,entity,sender)
	end,
	
	---------------------------------------------	
	TO_INTERESTED= function(self,entity,sender)
	end,

	---------------------------------------------	
	GOTO_SEARCH = function(self,entity,sender)
	end,
	
	---------------------------------------------	
	ENEMYSEEN_DURING_COMBAT = function(self,entity,sender)
	end,
	
	---------------------------------------------	
	ENEMYSEEN_FIRST_CONTACT = function(self,entity,sender)
	end,

	-------------------------------------------------
	OnGrenadeDanger = function ( self, entity, sender,data)
	end,
	
	-------------------------------------------------
	OnTargetApproaching = function ( self, entity, sender)
	end,
	
	-------------------------------------------------
	OnGroupMemberMutilated = function ( self, entity, sender)
	end,
	
	-------------------------------------------------
	OnBodyFallSound = function ( self, entity, sender)
	end,
	
	-------------------------------------------------
	OnTankSeen = function ( self, entity, sender)
	end,

	---------------------------------------------
	OnHeliSeen = function( self, entity, fDistance )
	end,

	---------------------------------------------
	OnBoatSeen = function( self, entity, fDistance )
	end,

	-------------------------------------------------
	OnExplosionDanger = function ( self, entity, sender)
	end,
})