ForbiddenArea=
{
	Server={},
	Client={},
	
	Editor={
		--Model="Editor/Objects/ForbiddenArea.cgf",
		Icon="forbiddenarea.bmp",
	},
		
	type = "ForbiddenArea",

	Properties=
	{
		bEnabled=1,
		bReversed=1,
		DamagePerSecond=35,
		esDamageType="punish",
		Delay=5,
		bShowWarning=1,
		bInfiniteFall=0,
		teamName="",
		overrideTimerLength=0,
		bResetsObjects=1,

		MultiplayerOptions = {
			bNetworked		= 0,
		},
	},

	owner=0,
	bTimerStarted = false,
	TIMER_LENGTH = 1000,
};

Net.Expose {
	Class = ForbiddenArea,

	ClientMethods = {
		ClSetOwner = { RELIABLE_UNORDERED, NO_ATTACH, ENTITYID },
	},
	ServerMethods = {
		SvSetOwner = { RELIABLE_UNORDERED, NO_ATTACH, ENTITYID },
	},
}

ForbiddenArea.AUDIO_SIGNAL = GameAudio.GetSignal("ForbiddenArea_Loop");

-----------------------------------------------------------------------------------------------------
function ForbiddenArea:OnSpawn()
	if (self.Properties.MultiplayerOptions.bNetworked == 0) then
		self:SetFlags(ENTITY_FLAG_CLIENT_ONLY,0);
	end
end

-----------------------------------------------------------------------------------------------------
function ForbiddenArea.Server:SvSetOwner(ownerId)
	self.owner = ownerId
	--sync owner over to all clients (other than sender) too
	local user=System.GetEntity(ownerId);
	self.otherClients:ClSetOwner(user.actor:GetChannel(), ownerId or NULL_ENTITY);
end

function ForbiddenArea.Client:ClSetOwner(ownerId)
	self.owner = ownerId
end


----------------------------------------------------------------------------------------------------
function ForbiddenArea:OnPropertyChange()
	self:OnReset();
end


----------------------------------------------------------------------------------------------------
function ForbiddenArea.Server:OnInit()
	self.inside={};
	self.warning={};

	self:OnReset();
end


----------------------------------------------------------------------------------------------------
function ForbiddenArea.Client:OnInit()
	if (not CryAction.IsServer()) then
		self.inside={};
		self.warning={};

		self:OnReset();
	end
end

----------------------------------------------------------------------------------------------------
function ForbiddenArea.Client:OnTimer(timerId, msec)
	if(self.Properties.bEnabled == 0) then
		return
	end

	local timerLength = self:GetTimerLength();
	if (g_localActor.actor:GetHealth() > 0) then
		self:SetTimer(timerId, timerLength);
	end

	if (g_localActor) then
		local skip = false

		vehId = g_localActor.actor:GetLinkedVehicleId()
		if( vehId and vehId ~= 0 ) then
			if( (self.Properties.bInfiniteFall == 0) and ( self.Properties.esDamageType ~= "punish") and (self.Properties.esDamageType ~= "punishFall") ) then
				skip = true;
			end
		end

		if( not skip ) then
			if (not self.reverse) then
				if (self:IsPlayerInside(g_localActorId)) then
					if ((not self.teamId) or (self.teamId~=g_gameRules.game:GetTeam(g_localActorId))) then
						self:PunishPlayer(g_localActor, timerLength);
					end
				end
			else
				if (not self:IsPlayerInside(g_localActorId)) then
					if ((not self.teamId) or (self.teamId~=g_gameRules.game:GetTeam(g_localActorId))) then
						self:PunishPlayer(g_localActor, timerLength);
					end
				end
			end		
		end
	end
end 
----------------------------------------------------------------------------------------------------
function ForbiddenArea:PunishPlayer(player, time)
	local immuneToForbiddenAreas = player.actor and player.actor:IsImmuneToForbiddenArea()
	if ((player.actor:GetSpectatorMode()~=0) or player:IsDead() or immuneToForbiddenAreas) then
		return;
	end

	local damageTypeId = -1;

	if (self.Properties.bInfiniteFall ~= 0) then
		damageTypeId = g_gameRules.game:GetHitTypeId("punishFall");
	else
		damageTypeId = g_gameRules.game:GetHitTypeId(self.Properties.esDamageType);
		--Log("ForbiddenArea:PunishPlayer() using damageType=%s (%d)", self.Properties.esDamageType, damageTypeId);
	end

	local damage = self.dps * (time / 1000);

	-- Send the hit to the gamerules, note: we have to set the direction and normal so that the game
	-- doesn't throw away the hit as a 'backface' hit
	if( not self.owner or self.owner == 0 ) then
		g_gameRules.game:ClientSelfHarm( damage, 0, -1, damageTypeId, {x=0,y=0,z=0}, {x=0,y=0,z=1}, {x=0,y=0,z=-1} );
	else
		g_gameRules.game:ClientSelfHarmByEntity( self.owner, damage, 0, -1, damageTypeId, {x=0,y=0,z=0}, {x=0,y=0,z=1}, {x=0,y=0,z=-1} );
	end

end

----------------------------------------------------------------------------------------------------
function ForbiddenArea:DamageVehicle(vehicle, time)
	if( (self.Properties.bInfiniteFall ~= 0) or ( self.Properties.esDamageType == "punish") or (self.Properties.esDamageType == "punishFall") )then
		return;
	end

	local damageTypeId = g_gameRules.game:GetHitTypeId(self.Properties.esDamageType);

	local damage = self.dps * time * 10;	--bit of a fudge, but need lots of damage to kill vehicles

	-- Send the hit to the gamerules, note: we have to set the direction and normal so that the game
	-- doesn't throw away the hit as a 'backface' hit
	g_gameRules.game:ServerHarmVehicle( vehicle, damage, 0, damageTypeId, {x=0,y=0,z=0}, {x=0,y=0,z=1}, {x=0,y=0,z=-1} );

end


----------------------------------------------------------------------------------------------------
function ForbiddenArea:ActorEnteredArea(entity)
	local inside=false;
	for i,v in ipairs(self.inside) do
		if (v==entity.id) then
			inside=true;
			break;
		end
	end	
	
	if (inside) then
		return;
	end
	
	table.insert(self.inside, entity.id);
end


----------------------------------------------------------------------------------------------------
function ForbiddenArea.Client:OnEnterArea_Internal(entity)
	if ((not self.teamId) or (self.teamId~=g_gameRules.game:GetTeam(entity.id))) then
		if (not self.reverse) then
			self.warning[entity.id]=self.delay;
				
				if(g_localActorId) then
					if(g_localActorId == entity.id) then
						if ((entity.actor:GetSpectatorMode()==0) and (not entity:IsDead())) then
							if(entity.actor:IsImmuneToForbiddenArea()) then
								self:SetTimer(0, self.delay * 1000); --Same time as normal on enter, by design.
							else
								if (self.showWarning) then
									g_gameRules.game:ForbiddenAreaWarning(true, self.delay, entity.id);
									GameAudio.PlaySignal(self.AUDIO_SIGNAL);
									GameAudio.Announce("LeavingMap", 0); -- 0 context is CAnnouncer::eAC_inGame
								end
								self:SetTimer(0, self.delay * 1000);
							end
						end
					end
				end
		else
			self.warning[entity.id]=nil;
				
				if(g_localActorId) then
					if(g_localActorId == entity.id) then
						if (self.showWarning) then
							g_gameRules.game:ForbiddenAreaWarning(false, 0, entity.id);
							GameAudio.StopSignal(self.AUDIO_SIGNAL);
						end
						self:KillTimer(0);
					end
				end
		end
	end
end



----------------------------------------------------------------------------------------------------
function ForbiddenArea.Client:OnEnterArea(entity, areaId)
	if (entity.actor) then
		self:ActorEnteredArea(entity);
		self.Client.OnEnterArea_Internal(self, entity);
	end
end


----------------------------------------------------------------------------------------------------
function ForbiddenArea:ActorLeftArea(entity)
	local inside = false;
	for i,v in ipairs(self.inside) do
		if (v==entity.id) then
			inside = true;
			table.remove(self.inside, i);
			break;
		end
	end
	return inside;
end

----------------------------------------------------------------------------------------------------
function ForbiddenArea.Client:OnLeaveArea_Internal(entity, inside)
	if ((not self.teamId) or (self.teamId~=g_gameRules.game:GetTeam(entity.id))) then
		if (self.reverse) then
			if (inside) then
				self.warning[entity.id]=self.delay;
				
					if(g_localActorId) then
						if(g_localActorId == entity.id) then
							if ((entity.actor:GetSpectatorMode()==0) and (not entity:IsDead())) then
								if(entity.actor:IsImmuneToForbiddenArea()) then
									self:SetTimer(0, self.delay * 1000); --Same time as normal on enter, by design.
								else
									if (self.showWarning) then
										g_gameRules.game:ForbiddenAreaWarning(true, self.delay, entity.id);
										GameAudio.PlaySignal(self.AUDIO_SIGNAL);
										GameAudio.Announce("LeavingMap", 0); -- 0 context is CAnnouncer::eAC_inGame
									end
									self:SetTimer(0, self.delay * 1000);
								end
							end
						end
					end
			end
		else
			self.warning[entity.id]=nil;
				
				if(g_localActorId) then
					if(g_localActorId == entity.id) then
						if (self.showWarning) then
							g_gameRules.game:ForbiddenAreaWarning(false, 0, entity.id);
							GameAudio.StopSignal(self.AUDIO_SIGNAL);
						end
						self:KillTimer(0);
					end
				end
		end
	end
end


----------------------------------------------------------------------------------------------------
function ForbiddenArea.Client:OnLeaveArea(entity, areaId)
	if (entity.actor) then
		local inside = self:ActorLeftArea(entity);
		self.Client.OnLeaveArea_Internal(self, entity, inside);
	end
end


----------------------------------------------------------------------------------------------------
function ForbiddenArea:OnSave(svTbl)
	svTbl.inside = self.inside;
end

----------------------------------------------------------------------------------------------------
function ForbiddenArea:OnLoad(svTbl)
	self:OnReset();
	self.inside = svTbl.inside;
end

----------------------------------------------------------------------------------------------------
function ForbiddenArea:OnReset()
	self.reverse=tonumber(self.Properties.bReversed)~=0;
	self.delay=tonumber(self.Properties.Delay);
	self.dps=tonumber(self.Properties.DamagePerSecond);
	self.showWarning=tonumber(self.Properties.bShowWarning)~=0;
	self.warning={};

	self.isServer=CryAction.IsServer();
	self.isClient=CryAction.IsClient();
	
	if (self.Properties.teamName ~= "") then
		self.teamId=g_gameRules.game:GetTeamId(self.Properties.teamName);
		if (self.teamId==0) then
			self.teamId=nil;
		end
	end

	if (self.Properties.bEnabled ~=0) then
		g_gameRules.game:AddForbiddenArea(self.id);
	end

	self:RegisterForAreaEvents(self.Properties.bEnabled and 1 or 0);
end


----------------------------------------------------------------------------------------------------
function ForbiddenArea:IsPlayerInside(playerId)
	for i,id in pairs(self.inside) do
		if (id==playerId) then
			return true;
		end
	end
	
	return false;
end


----------------------------------------------------------------------------------------------------
function ForbiddenArea:GetTimerLength()
	local timerLength = self.Properties.overrideTimerLength;
	if (timerLength == 0) then
		timerLength = self.TIMER_LENGTH;
	end
	return timerLength;
end


--------------------------------------------------------------------------
function ForbiddenArea.Server:OnShutDown()
	if (g_gameRules) then
		g_gameRules.game:RemoveForbiddenArea(self.id);
	end
end


--------------------------------------------------------------------------
function ForbiddenArea.Client:OnShutDown()
	if (g_gameRules) and (not CryAction.IsServer()) then
		g_gameRules.game:RemoveForbiddenArea(self.id);
	end
end

----------------------------------------------------------------------------------------------------
function ForbiddenArea:OnLocalPlayerRevived()			-- Called from C++
	if (self.Properties.bEnabled ~= 0) then
		local timerLength = self:GetTimerLength();
		self.bTimerStarted = true;
	else
		self:KillTimer(0);
	end
end

----------------------------------------------------------------------------------------------------
function ForbiddenArea:OnLocalPlayerImmunityOff()			-- Called from C++
	if (self.Properties.bEnabled ~= 0) then
		if(g_localActorId~=nil) then
			local entity = System.GetEntity(g_localActorId);
			if(entity and entity.actor) then
				if(self:IsPlayerInside(g_localActorId)) then
					self.Client.OnEnterArea_Internal(self, entity);
				else
					self.Client.OnLeaveArea_Internal(self, entity, true);
				end
			end
		end
	end
end

----------------------------------------------------------------------------------------------------
function ForbiddenArea:OnLocalPlayerImmunityOn()			-- Called from C++
	if (self.Properties.bEnabled ~= 0) then
		if(g_localActorId~=nil) then
			local entity = System.GetEntity(g_localActorId);
			if(entity and entity.actor) then
				if(self:IsPlayerInside(g_localActorId)) then
					self.Client.OnLeaveArea_Internal(self, entity, true);
				else
					self.Client.OnEnterArea_Internal(self, entity);
				end
			end
		end
	end
end


----------------------------------------------------------------------------------------------------
function ForbiddenArea:Event_Enable()
	if(self.Properties.bEnabled ~= 1) then
		self.Properties.bEnabled = 1;
		local timerLength = self:GetTimerLength();
		BroadcastEvent(self, "Enabled");
		g_gameRules.game:AddForbiddenArea(self.id);
		self:SetTimer(0, self.delay * 1000);
		self:RegisterForAreaEvents(1);
	end
end

----------------------------------------------------------------------------------------------------
function ForbiddenArea:Event_Disable()
	if(self.Properties.bEnabled ~= 0) then
		self.Properties.bEnabled = 0;
		BroadcastEvent(self, "Disabled");
		g_gameRules.game:RemoveForbiddenArea(self.id);
		self:RegisterForAreaEvents(0);
	end
end

----------------------------------------------------------------------------------------------------
function ForbiddenArea:Event_SetOwner( sender,newOwner )
	self.owner = newOwner.id

	if( self.bNetworked ~= 0 ) then
		if self.isServer then
			if( g_localChannelId ~= nil ) then
				self.otherClients:ClSetOwner(g_localChannelId, self.owner or NULL_ENTITY );
			else
				self.allClients:ClSetOwner(self.owner or NULL_ENTITY );
			end
		else
			self.server:SvSetOwner(self.owner or NULL_ENTITY);
		end
	end
end

----------------------------------------------------------------------------------------------------
 ForbiddenArea.FlowEvents =
{
	Inputs =
	{
		Disable = { ForbiddenArea.Event_Disable, "bool" },
		Enable = { ForbiddenArea.Event_Enable, "bool" },
		SetOwner = {ForbiddenArea.Event_SetOwner, "entity" },
	},
	Outputs =
	{
		Disabled = "bool",
		Enabled = "bool",
	},
}		
