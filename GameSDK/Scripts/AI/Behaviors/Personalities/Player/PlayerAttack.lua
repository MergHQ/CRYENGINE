local Behavior = CreateAIBehavior("PlayerAttack", "PlayerIdle",{
	---------------------------------------------
	Constructor = function (self, entity,data)
		-- called when the behaviour is selected
		-- the extra data is from the signal that caused the behavior transition
	end,
	---------------------------------------------
	Destructor = function (self, entity,data)
		-- called when the behaviour is de-selected
		-- the extra data is from the signal that is causing the behavior transition
	end,

	---------------------------------------------
	OnSeenByEnemy = function ( self, entity, sender)
		-- use it as signal in order to execute it once even if squadmates send it
	end,
	
	---------------------------------------------
	OnThreateningSoundHeardByEnemy = function ( self, entity, sender)
	end,
	
	---------------------------------------------
	START_ATTACK = function ( self, entity, sender)
		local i=1;
	end,
	
	---------------------------------------------
	OnLeaderActionCompleted = function(self,entity,sender,data)
		-- data.iValue = Leader action type
		-- data.iValue2 = Leader action subtype
		-- data.id = group's live attention target 
		-- data.ObjectName = group's attention target name
		-- data.fValue = target distance
		-- data.point = enemy average position
		
		local avgPos = g_Vectors.temp;
		AI.GetGroupAveragePosition(entity.id,UPR_COMBAT_GROUND,avgPos);
		local dist = DistanceVectors(avgPos,entity:GetPos());

		if(dist<100) then 
			self.parent:Follow(entity);
		end
	end,
	
	---------------------------------------------
	OnLeaderActionFailed  = function(self,entity,sender,data)
		self:OnLeaderActionCompleted(entity,sender,data);
	end,
	
	---------------------------------------------
	OnNavTypeChanged  = function(self,entity,sender,data)
		self.parent:StartAttack(entity,data.iValue);
	end,	
	
	---------------------------------------------
	CAPTURE_ME = function(self,entity,sender,data)
		AI.Signal(SIGNALFILTER_SENDER,1,"ORDER_FOLLOW",data.id);
	end,
})