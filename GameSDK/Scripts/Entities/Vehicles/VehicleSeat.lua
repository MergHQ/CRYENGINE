--------------------------------------------------------------------------
--	Crytek Source File.
-- 	Copyright (C), Crytek Studios, 2001-2005.
--------------------------------------------------------------------------
--	$Id$
--	$DateTime$
--	Description: vehicle seat implementation
--  
--------------------------------------------------------------------------
--  History:
--  - 28/04/2005   : Created by Mathieu Pinard
--
--------------------------------------------------------------------------

VehicleSeat =
{
	-- Data:
	--
	-- * vehicleId
	-- * passengerId
	--
	-- * Weapons[].id
	--
	-- * isDriver
	-- * part
	-- * sitHelper
	-- * enterHelper
	-- * exitHelper
}

--------------------------------------------------------------------------
function VehicleSeat:Init(vehicle, seatId)	
	self.vehicleId = vehicle.id;	
	local vehicle = System.GetEntity(self.vehicleId);
	
	self.seatId = seatId;
	self.status = 0;
	self.playerId = 0;
	
	if vehicle.vehicle then
		vehicle.vehicle:AddSeat(self);
	end
end

--------------------------------------------------------------------------
function VehicleSeat:OnReset()

end

--------------------------------------------------------------------------
function VehicleSeat:IsFree(userId)
  return self.seat:IsFree(userId);
end


--------------------------------------------------------------------------
function VehicleSeat:GetExitPos()
	local vehicle = System.GetEntity(self.vehicleId);
	local exitPos = {x=0,y=0,z=0};
	
	if (self.exitHelper) then
		exitPos = vehicle.vehicle:MultiplyWithWorldTM(vehicle:GetVehicleHelperPos(self.exitHelper));
	elseif (self.enterHelper) then
		exitPos = vehicle.vehicle:MultiplyWithWorldTM(vehicle:GetVehicleHelperPos(self.enterHelper));
	end
	
	--FIXME:if the model dont have the right helper use the vehicle position instead
	if (LengthSqVector(exitPos) == 0) then
		CopyVector(exitPos, vehicle.State.pos);
		exitPos.z = exitPos.z + 1;
	end

	return exitPos;
end

--------------------------------------------------------------------------
function VehicleSeat:GetWeaponId(index)
	if (self.Weapons and self.Weapons[index]) then
		return self.seat:GetWeapon(i);
	else
		Log("Error: weapon index #%s invalid.", tostring(index));
	end
end

--------------------------------------------------------------------------
function VehicleSeat:GetWeaponCount()
	return count(self.Weapons);
end

--------------------------------------------------------------------------
function VehicleSeat:IsPassengerReady()
	return true;
end

--------------------------------------------------------------------------
function VehicleSeat:IsPassengerRemote()
	return self.isRemote;
end

--------------------------------------------------------------------------
function VehicleSeat:OnUpdate(deltaTime)

end

--------------------------------------------------------------------------
function VehicleSeat:OnUpdateView()

end

--------------------------------------------------------------------------
function VehicleSeat:LoadPassenger(passengerId)

end

