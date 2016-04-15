--------------------------------------------------------------------------
--	Crytek Source File.
-- 	Copyright (C), Crytek Studios, 2001-2004.
--------------------------------------------------------------------------
--	$Id$
--	$DateTime$
--	Description: Common code for all/most of the vehicle implementations
--  
--------------------------------------------------------------------------
--  History:
--  - 30/11/2004   15:36 : Created by Mathieu Pinard
--
--------------------------------------------------------------------------

Script.ReloadScript("Scripts/Utils/Math.lua");
Script.ReloadScript("Scripts/Entities/Vehicles/VehicleSeat.lua");


VehicleBase =
{  
	State =
	{
		pos = {},
		aiDriver = nil,
	},
	
	Seats = {},
	AI = {},

	Client = {},
	Server = {},
	
	Hit = {},	
}

--------------------------------------------------------------------------
function VehicleBase:HasDriver()
	for i,seat in pairs(self.Seats) do
		if (seat.isDriver) then
			if (seat.passengerId) then
				return true;
			end
		end
	end

	return false;
end

--------------------------------------------------------------------------
function VehicleBase:GetDriverId()
	if (self.Seats and self.Seats[1]) then
		return self.Seats[1].seat:GetPassengerId();
	end
	
	return nil;
end

--------------------------------------------------------------------------
function VehicleBase.Server:OnSpawnComplete()
	self.vehicle:OnSpawnComplete();
end


--------------------------------------------------------------------------
function GetNextAvailableSeat(seats)
	for i,s in pairs(seats) do
		if (not s.seat:GetPassengerId()) then
			return i;
		end
	end
	
	return -1;
end

--------------------------------------------------------------------------
function VehicleBase:InitSeats()
  if (self.Seats) then
    for i,seat in pairs(self.Seats) do
  		mergef(seat, VehicleSeat, 1);
  		seat:Init(self, i);
  	end
  end
end

--------------------------------------------------------------------------
function VehicleBase:InitVehicleBase()
	self:OnPropertyChange();
	if(self.Properties.bSyncPhysics == 0) then
		CryAction.DontSyncPhysics(self.id);
	end
end

--------------------------------------------------------------------------
function VehicleBase:OnPropertyChange()	
	if (self.OnPropertyChangeExtra) then		
		self:OnPropertyChangeExtra();
	end
end

--------------------------------------------------------------------------
function VehicleBase:DestroyVehicleBase()	
	--Log("DestroyVehicleBase()");
	
	if (self.DestroyAI) then
	  self:DestroyAI();
	end		
end

--------------------------------------------------------------------------
function VehicleBase:GetExitPos(seatId)
	if (self.Seats[seatId] == nil) then
		Log("VehicleBase:GetExitPos(seatId) - Invalid seat id: "..tostring(seatId))
		return;
	end 

	local exitPos;
	local seat = self.Seats[seatId];
	
	if (seat.exitHelper) then
		exitPos = self.vehicle:MultiplyWithWorldTM(self:GetVehicleHelperPos(self.Seats[seatId].exitHelper));
	else
		exitPos = self.vehicle:MultiplyWithWorldTM(self:GetVehicleHelperPos(self.Seats[seatId].enterHelper));
	end
	
	--exitPos.z = exitPos.z + 1.0;
	return exitPos;
end

--------------------------------------------------------------------------
function VehicleBase:GetSeatPos(seatId)
	if (seatId == -1) then
		Log("Error: VehicleBase:GetSeatPos(seatId) - seatId -1 is invalid");
		return {x=0, y=0, z=0,};
	else
		
		local helper = self.Seats[seatId].enterHelper;
		local pos;
		if (self.vehicle:HasHelper(helper)) then		  
		  pos = self.vehicle:GetHelperWorldPos(helper);
		else		  
		  pos = self:GetHelperPos(helper, HELPER_WORLD);		  
		end
		
		--Log("GetSeatPos: "..helper..": "..Vec2Str(pos));
	  
		return pos;
	end
end

--------------------------------------------------------------------------
function VehicleBase:OnUsed(user, idx)
	if (not CryAction.IsClient()) then
		return;
	end
	
	self.vehicle:OnUsed(user.id, idx);
end

--------------------------------------------------------------------------
function VehicleBase:IsUsable(user)	  
	if (user:IsOnVehicle()) then
		return 0;
	end
  
  local ret = self.vehicle:IsUsable(user.id);
  if (ret ~= 0) then
  	return ret;
  end
  
  if(self.Properties.bInteractLargeObject ~= 0) then
		return 1;
  end
  
  return 0;
end


----------------------------------------------------------------------------------------------------
function VehicleBase:CanEnter(userId)
	if (g_gameRules and g_gameRules.CanEnterVehicle) then
		return g_gameRules:CanEnterVehicle(self, userId);
	end
end

--------------------------------------------------------------------------
function VehicleBase:GetSeat(userId)
	local seatId = self.vehicle:GetSeatForPassenger(userId);
	if(seatId) then
		return self.Seats[seatId];
	end
	
	return nil;
end

--------------------------------------------------------------------------
function VehicleBase:GetSeatByIndex(i)
	return self.Seats[i];		
end

--------------------------------------------------------------------------
function VehicleBase:GetSeatId(userId)	
	return self.vehicle:GetSeatForPassenger(userId);
end


--------------------------------------------------------------------------
function VehicleBase:ResetVehicleBase()
	self.State.pos = self:GetWorldPos(self.State.pos);
	
	-- disable the AI driver.
	if (self.AIDriver) then
		self:AIDriver( 0 );
	end
	
	self.State.aiDriver = nil;
	 
  self:OnPropertyChange();
  
  if (self.Seats) then
  	for i,seat in pairs(self.Seats) do
  		if (seat and seat.OnReset) then
  			seat:OnReset();
  		end
  	end
  end
  
  if (self.ResetAI) then
    self:ResetAI();
  end
end
	

--------------------------------------------------------------------------
function VehicleBase:OnShutDown()
	Game.RemoveTacticalEntity(self.id, eTacticalEntity_Vehicle);
end

--------------------------------------------------------------------------
function VehicleBase:OnDestroy()	
	self:DestroyVehicleBase();	
end

----------------------------------------------------------------------------------------------------
function VehicleBase:UseCustomFiring(weaponId)
	return false;
end

----------------------------------------------------------------------------------------------------
function VehicleBase:GetFiringPos(weaponId)
	return g_Vectors.v000;
end

--------------------------------------------------------------------------
function VehicleBase:GetFiringVelocity()
	return g_Vectors.v000;
end

--------------------------------------------------------------------------
function VehicleBase:OnAIShoot()	
	local weaponId = self.Seats[1]:GetWeaponId(1);
	
	if (weaponId) then
		local weapon = System.GetEntity(weaponId);

		weapon:StartFire(self.id);
		weapon:StopFire();
	end
end


--------------------------------------------------------------------------
function VehicleBase:PrepareSeatMountedWeapon(user)
	local seat = self:GetSeat(user.id);
	if (seat and user.PrepareForMountedWeaponUse) then
		local weaponId = seat.seat:GetWeaponId(1);
		if (weaponId) then
			user:PrepareForMountedWeaponUse(weaponId);
		end
	end
end

--------------------------------------------------------------------------
function VehicleBase:IsGunner(userId)
	local seat = self:GetSeat(userId);
	
	if (seat) then
		if (seat.Weapons) then
			return true;
		end
	end
	
	return false;
end

--------------------------------------------------------------------------
function VehicleBase:IsDriver(userId)
	local seat = self:GetSeat(userId);
	
	if (seat) then
		if (seat.isDriver) then
			return true;
		end
	end

	return false;
end


--------------------------------------------------------------------------
function VehicleBase:GetVehicleHelperPos(helperName)
  if (not helperName) then
    helperName = "";
  end
  
  --Log("GetVehicleHelperPos <"..helperName..">");
  
	local pos;
	if (self.vehicle:HasHelper(helperName)) then
	  pos = self.vehicle:GetHelperPos(helperName, true);
	  --Log("pos from vehicle.."..Vec2Str(pos));
	else
	  pos = self:GetHelperPos(helperName, HELPER_LOCAL);
	  --Log("pos from entity.."..Vec2Str(pos));
	end

	return pos;
end

--------------------------------------------------------------------------
function VehicleBase:RequestSeatByPosition(userId)	
	local pos = System.GetEntity(userId):GetWorldPos();
	local radiusSq = 10;

	for i,seat in pairs(self.Seats) do
		-- Look if the seat is free

		if (seat.enterHelper and not seat.passengerId) then
			-- Look if there's a seat entering pos near the player
			
			if (seat.useBoundsForEntering == nil or seat.useBoundsForEntering == true) then
				if (self.vehicle:IsInsideRadius(pos, 1)) then
					return i;
				end
			else			  
			  local enterPos;
				if (self.vehicle:HasHelper(seat.enterHelper)) then
				  enterPos = self.vehicle:GetHelperWorldPos(seat.enterHelper);
				else
				  enterPos = self:GetHelperPos(seat.enterHelper, HELPER_WORLD);
				end
				
				local distanceSq = DistanceSqVectors(pos, enterPos);

				if (distanceSq <= radiusSq) then
					-- This seat is within the radius, take it!
					return i;
				end
			end
		end
	end
	
	return nil;
end

--------------------------------------------------------------------------
function VehicleBase:RequestClosestSeat(userId)	
	local pos = System.GetEntity(userId):GetWorldPos();
	
	local minDistanceSq = 100000;
	local selectedSeat;
	
	for i,seat in pairs(self.Seats) do
		-- Look if the seat is free

		if (seat.enterHelper and seat:IsFree(userId)) then
		
			local enterPos; 
			if (self.vehicle:HasHelper(seat.enterHelper)) then
	      enterPos = self.vehicle:GetHelperWorldPos(seat.enterHelper);
	    else
	      enterPos = self:GetHelperPos(seat.enterHelper, HELPER_WORLD);
	    end
			
			--Log("Helper for seat "..tostring(i)..": "..seat.enterHelper..Vec2Str(enterPos));
			
			local distanceSq = DistanceSqVectors(pos, enterPos);
			
			if (distanceSq <= minDistanceSq) then
				minDistanceSq = distanceSq;
				selectedSeat = i;
			end
		end
	end
  
	if(selectedSeat) then
		if AI then AI.LogEvent(System.GetEntity(userId):GetName().." found seat "..selectedSeat) end;
	else
		if AI then AI.LogEvent(System.GetEntity(userId):GetName().." found no seat") end;
	end
	return selectedSeat;
end

--------------------------------------------------------------------------
function VehicleBase:RequestMostPrioritarySeat(userId)	

	local pos = System.GetEntity(userId):GetWorldPos();	
	local selectedSeat;
	-- search driver seat first
	local seat = self.Seats[1];
	if(seat:IsFree(userId)) then 
		return 1;
	end

	for i,seat in pairs(self.Seats) do
		-- search for gunner seats
		if (seat.enterHelper and seat.Weapons and seat:IsFree(userId)) then
			if AI then AI.LogEvent(System.GetEntity(userId):GetName().." found seat "..i) end;
			return i;
		end
	end

	for i,seat in pairs(self.Seats) do
		-- search for remaining seats
		if (seat.enterHelper and seat:IsFree(userId)) then
			if AI then AI.LogEvent(System.GetEntity(userId):GetName().." found seat "..i) end;
			return i;
		end
	end


	return ;
end

--------------------------------------------------------------------------
function VehicleBase:RequestSeat(userId)
	local pos = System.GetEntity(userId):GetWorldPos();
	local radiusSq = 6;

	for i,seat in pairs(self.Seats) do
		if (seat:IsFree(userId)) then
			return i;
		end
	end

	return nil;
end

--------------------------------------------------------------------------
function VehicleBase:EnterVehicle(passengerId, seatId, isAnimationEnabled)
	--Log("VehicleBase:EnterVehicle() - playerId = %s, seatId = %s", tostring(passengerId), tostring(seatId));
	return self.vehicle:EnterVehicle(passengerId, seatId, isAnimationEnabled);
end

--------------------------------------------------------------------------
function VehicleBase:LeaveVehicle(passengerId, fastLeave)
	--Log("VehicleBase:LeaveVehicle() - "..tostring(passengerId));
	if AI then AI.Signal(SIGNALFILTER_SENDER,0,"exited_vehicle",passengerId) end;
	return self.vehicle:ExitVehicle(passengerId);
end

--------------------------------------------------------------------------
function VehicleBase:ReserveSeat(userId,seatidx)
	self.Seats[seatidx].passengerId = userId;
end

--------------------------------------------------------------------------
function VehicleBase:IsDead()
  return self.vehicle:IsDestroyed();
end

--------------------------------------------------------------------------
function VehicleBase:GetWeaponVelocity(weaponId)
	return self:GetFiringVelocity();
end

--------------------------------------------------------------------------
function VehicleBase:OnShoot(weapon, remote)
	if (weapon.userId) then
		local seat = self:GetSeat(weapon.userId);
		if (seat) then
			if (seat.Animations and seat.Animations["weaponRecoil"]) then
				local user = System.GetEntity(weapon.userId);
				
				if (user:IsDead()) then
					return;
				end

				user:StartAnimation(0, seat.Animations["weaponRecoil"], 0, 0.000000001, 1.0, false);
			end
		end
	end
	
	return true;
end


-------------------------------------------------------------------------
function VehicleBase:SpawnVehicleBase()
  
  if (self.OnPreSpawn) then
    self:OnPreSpawn();
  end
    
  if (_G[self.class.."Properties"]) then
    mergef(self, _G[self.class.."Properties"], 1);  
  end
  
  if (self.OnPreInit) then
    self:OnPreInit();
  end  

	self:InitVehicleBase();
			
	self.ProcessMovement = nil;

	if (self.OnPostSpawn) then
	    self:OnPostSpawn();
	end
	
	local aiSpeed = self.Properties.aiSpeedMult;
	local AIProps = self.AIMovementAbility;
	if (AIProps and aiSpeed and aiSpeed ~= 1.0) then	  
	  if (AIProps.walkSpeed) then AIProps.walkSpeed = AIProps.walkSpeed * aiSpeed; end;
	  if (AIProps.runSpeed) then AIProps.runSpeed = AIProps.runSpeed * aiSpeed; end;
	  if (AIProps.sprintSpeed) then AIProps.sprintSpeed = AIProps.sprintSpeed * aiSpeed; end;
	  if (AIProps.maneuverSpeed) then AIProps.maneuverSpeed = AIProps.maneuverSpeed * aiSpeed; end;
	end

  if (self.InitAI) then
    self:InitAI();
  end
	self:InitSeats();
	self:OnReset();
	ApplyCollisionFiltering(self, GetCollisionFiltering(self));
	
	Game.AddTacticalEntity(self.id, eTacticalEntity_Vehicle);
end

--------------------------------------------------------------------------
function VehicleBase.Client:OnHit(hit)

end

--------------------------------------------------------------------------
function VehicleBase:ProcessPassengerDamage(passengerId, actorHealth, damage, hitTypeId, explosion)
	return self.vehicle:ProcessPassengerDamage(passengerId, actorHealth, damage, hitTypeId, explosion);
end

--------------------------------------------------------------------------
function VehicleBase:OnActorSitDown(seatId, passengerId)
	--Log("VehicleBase:OnActorSitDown() seatId=%s, passengerId=%s", tostring(seatId), tostring(passengerId));

	local passenger = System.GetEntity(passengerId);
	if (not passenger) then
		Log("Error: entity for player id <%s> could not be found. %s", tostring(passengerId));
		return;
	end
	
	local seat = self.Seats[seatId];
	if (not seat) then
		Log("Error: entity for player id <%s> could not be found!", tostring(passengerId));
		return;
	end
	
	if (g_gameRules.OnEnterVehicleSeat) then
		g_gameRules:OnEnterVehicleSeat(self, seat, passengerId);
	end

	-- need to generate AI sound event (vehicle engine)
	if(seat.isDriver) then 
--System.Log(">>> vehicleSoundTimer setting NOW >>>>>>");		
		self:SetTimer(AISOUND_TIMER, AISOUND_TIMEOUT);
	end
	
	seat.passengerId = passengerId;
	passenger.vehicleId = self.id;
	passenger.AI.theVehicle = self; -- fix for behaviors

	if (CryAction.HasAI(passengerId)) then
		if (seat.isDriver) then 
			self.State.aiDriver = 1;
			if (self.AIDriver) then
				if (passenger.actor and passenger.actor:GetHealth() > 0) then
					self:AIDriver(1);
				else
					self:AIDriver(0);
				end
			end
		end
	end
	
	local wc = seat.seat:GetWeaponCount();
	--Log("VehicleBase:OnActorSitDown() weapons=%s", tostring(wc));	
	
	if (not passenger.ai) then
		--AI.SetSkip(self.id);
		
		if( self.hidesUser == 1 )then
			-- Do not hide outside gunners, so that the AI may use them as targets.
			local isOutsideGunner = false;
			if (seat.Sounds) then
				isOutsideGunner = (wc > 0) and (seat.Sounds.inout == 1);
			end
			if (AI and not isOutsideGunner) then
				AI.ChangeParameter(passengerId, AIPARAM_INVISIBLE, 1);
			end
		end	
		
		if(AI and seat.isDriver) then
			-- squadmates enter vehicle only if player is the driver
			CopyVector(g_SignalData.point, g_Vectors.v000);
			CopyVector(g_SignalData.point2, g_Vectors.v000);
			g_SignalData.iValue = AIUSEOP_VEHICLE;
			g_SignalData.iValue2 = 1; -- leader has already entered
			g_SignalData.fValue = 1; -- Leader is the driver
			g_SignalData.id = seat.vehicleId;
			AI.Signal(SIGNALFILTER_LEADER, 1, "ORD_USE", passengerId, g_SignalData);
			
		end
		self:EnableMountedWeapons(false);
	end
	
	local passengerFaction = AI.GetFactionOf(passenger.id);

	-- set vehicle species to driver's 	
	if (seat.isDriver and passengerFaction and self.ChangeFaction) then 
		self:ChangeFaction(passenger, 1);
		--System.Log("Changing species to "..passenger.Properties.species);
--		AI.ChangeParameter(self.id, AIPARAM_SPECIES, passenger.Properties.species);
	else
	end

	if AI then
		if ( wc > 0) then
			AI.Signal(SIGNALFILTER_SENDER, 1, "entered_vehicle_gunner", passengerId);
		else
			AI.Signal(SIGNALFILTER_SENDER, 1, "entered_vehicle", passengerId);
		end

		-- notify the "wait" goalop
		AI.Signal(SIGNALFILTER_SENDER, 9, "ENTERING_END", passengerId); -- 9 is to skip normal processing of signal
	end
end
--------------------------------------------------------------------------
function VehicleBase:OnActorChangeSeat(passengerId, exiting)

	-- ai specific
	Log("ai changed a seat");
	local seat = self:GetSeat(passengerId);
	if (not seat) then
		Log("Error: VehicleBase:OnActorChangeSeat() could not find passenger id %s on the vehicle", tostring(passengerId));
		return;
	end

	seat.passengerId = nil;

	if (g_gameRules and g_gameRules.OnLeaveVehicleSeat) then
		g_gameRules:OnLeaveVehicleSeat(self, seat, passengerId, exiting);
	end

	if (not passenger) then
		return;
	end
	
	passenger.vehicleId = nil;
	passenger.vehicleSeatId = nil;
	
	if (passenger.ai and passenger:IsDead()) then
		return;
	end

	if (seat.isDriver) then
	 	self.State.aiDriver = nil;
		if (passenger.ai and exiting and self.AIDriver) then
			self:AIDriver(0);
	  end
  end

	BroadcastEvent(self, "PassengerExit");

end

--------------------------------------------------------------------------
function VehicleBase:OnActorStandUp(passengerId, exiting)
	--Log("VehicleBase:OnActorStandUp() - passengerId=%s", tostring(passengerId));
	
	local seat = self:GetSeat(passengerId);
	if (not seat) then
		Log("Error: VehicleBase:OnActorStandUp() could not find passenger id %s on the vehicle", tostring(passengerId));
		return;
	end
	
	seat.passengerId = nil;
	
	if (g_gameRules and g_gameRules.OnLeaveVehicleSeat) then
		g_gameRules:OnLeaveVehicleSeat(self, seat, passengerId, exiting);
	end
	
	local passenger = System.GetEntity(passengerId);
	if (not passenger) then
		return;
	end
	
	passenger.vehicleId = nil;
	passenger.vehicleSeatId = nil;
	
	if (passenger.ai and passenger:IsDead()) then
		return;
	end

	if (seat.isDriver) then
	 	self.State.aiDriver = nil;
		if (exiting and self.AIDriver and CryAction.HasAI(passengerId)) then
			self:AIDriver(0);
	  end
  end

	if (passenger.ai ~= 1 and exiting) then
		-- player is exiting vehicle, tell squadmates
		if (self.ChangeFaction) then 
		  self:ChangeFaction();	-- restore species
		end
		if AI then
			AI.Signal(SIGNALFILTER_LEADERENTITY, 0, "ORD_LEAVE_VEHICLE", passengerId);
			AI.Signal(SIGNALFILTER_GROUPONLY, 0, "ORDER_EXIT_VEHICLE", passengerId);
			passenger.AI.theVehicle = nil;
		end
	end	
	

	if (self.AI.unloadCount) then
		self.AI.unloadCount = self.AI.unloadCount - 1;
	end;
	if (self.AI.unloadCount == 0) then
		AI.Signal(SIGNALFILTER_SENDER, 9, "UNLOAD_DONE", self.id); -- 9 is to skip normal processing of signal, only notify the waitsignal goal op.
	end

	if (exiting and passenger.AI.theVehicle~=nil) then
		if AI then AI.Signal( SIGNALFILTER_SENDER, 0, "EXIT_VEHICLE_STAND", passenger.id) end;
	end
			
	if(not passenger.ai) then
		if AI then AI.ChangeParameter(passengerId, AIPARAM_INVISIBLE, 0) end;
--		AI.SetSkip( );
		--Log( ">>>> HUMAN out " );
		self:EnableMountedWeapons(true);	
	end

	-- notify the "wait" goalop
	if AI then AI.Signal(SIGNALFILTER_SENDER, 9, "EXITING_END", passengerId) end; -- 9 is to skip normal processing of signal

	BroadcastEvent(self, "PassengerExit");

end

--------------------------------------------------------------------------
function VehicleBase:EnableMountedWeapons(enable)
 
	if not AI then return end

	if not GameAI.IsAISystemEnabled() then
		return;
	end

	-- disable/enable all the smartobject interactions for mounted weapons
	for i,seat in pairs(self.Seats) do
		local wc = seat.seat:GetWeaponCount();
		for j=1, wc do			
			local weaponid= seat.seat:GetWeaponId(j);
			if(weaponid) then 
				if(enable) then 
					AI.SetSmartObjectState(weaponid, "Idle");
				else
					AI.SetSmartObjectState(weaponid, "Busy");
				end
			end
		end
	end  
end

