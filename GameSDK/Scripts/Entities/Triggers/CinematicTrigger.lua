----------------------------------------------------------------------------------------------------
--  Crytek Source File.
--  Copyright (C), Crytek Studios, 2001-2004.
----------------------------------------------------------------------------------------------------
--  $Id$
--  $DateTime$
--  Description: Cinematic Trigger
--
----------------------------------------------------------------------------------------------------
--  History:
--  - 30:5:2005   19:16 : Created by Márcio Martins
--
----------------------------------------------------------------------------------------------------
CinematicTrigger =
{
	Properties =
	{
		fDimX							= 1,
		fDimY							= 1,
		fDimZ							= 1,
		bEnabled					= 1,
		ScriptCommand			= "",
		Sequence					= "",
		bTriggerOnce			= 0,
		fMaxDistance			= 200,
		fMinDistance			= 0,
		fMinVisibleTime		= 0.5,
		fDelay						= 0,
		fCheckTimer				= 0.25,
		fZoomMinimum			= 1,
		bInVehicleOnly		= 0,
		
		MultiplayerOptions=
		{
			bNetworked		= 0,
			bPerPlayer		= 0,
			MinPlayers		= 1,
		},		
	},
	
	VISIBILITY_TIMER_ID = 0,
	
	Client={},
	Server={},
	
	Editor={
		Model="Editor/Objects/T.cgf",
		Icon="Trigger.bmp",
		ShowBounds = 1,
	},
}



Net.Expose{
	Class = CinematicTrigger,
	ClientMethods = {
		ClEnable		= { RELIABLE_ORDERED, PRE_ATTACH, BOOL },
		ClVisible		= { RELIABLE_ORDERED, PRE_ATTACH, ENTITYID },
		ClInvisible	= { RELIABLE_ORDERED, PRE_ATTACH, ENTITYID },
		ClTrigger		= { RELIABLE_ORDERED, PRE_ATTACH, ENTITYID },
	},

	ServerMethods = {
		SvRequestVisible		= { RELIABLE_ORDERED, PRE_ATTACH, ENTITYID },
		SvRequestInvisible	= { RELIABLE_ORDERED, PRE_ATTACH, ENTITYID },
	},
	
	ServerProperties = {
	}
}


function CinematicTrigger:OnSpawn()
	self:OnReset();
end


function CinematicTrigger:OnPropertyChange()
	self:OnReset();
end


function CinematicTrigger:OnReset()
	if (self.timers) then
		for i,v in pairs(self.timers) do
			self:KillTimer(i);
		end
	end

	local bbmin = g_Vectors.temp_v1;
	local bbmax = g_Vectors.temp_v2;
	
	local dx = self.Properties.fDimX*0.5;
	local dy = self.Properties.fDimY*0.5;
	local dz = self.Properties.fDimZ*0.5;
	
	bbmin.x = -dx; bbmin.y = -dy; bbmin.z = -dz;
	bbmax.x = dx; bbmax.y = dy; bbmax.z = dz;
	
	self:SetTriggerBBox(bbmin, bbmax);
	self:Activate(0);
	
	self.timerId = 1;
	self.isvisible = nil;
	
	self.enabled = nil;
	self.triggerOnce = tonumber(self.Properties.bTriggerOnce)~=0;
	self.localOnly = self.Properties.MultiplayerOptions.bNetworked==0;
	self.perPlayer = tonumber(self.Properties.MultiplayerOptions.bPerPlayer)~=0;
	self.minPlayers = tonumber(self.Properties.MultiplayerOptions.MinPlayers);
	if (self.minPlayers<1) then self.minPlayers=1; end;
	
	self.isServer=CryAction.IsServer();
	self.isClient=CryAction.IsClient();
	
	self.visible={};
	self.timers={};
	
	if (not self.localOnly) then
		self.triggeredPP={};
		self.triggeredOncePP={};
	end
	
	self.triggeredOnce=nil;
	self.triggered=nil;
	self.visibleCount=0;
	
	self:Enable(tonumber(self.Properties.bEnabled)~=0);
	self.enableChanged=false;
end


function CinematicTrigger:Enable(enable)
	if (enable) then
		if (self.isClient) then
			self:SetTimer(self.VISIBILITY_TIMER_ID, self.Properties.fCheckTimer*1000);
			self.last_visible_time = nil;
		end
	else
		if (self.isClient) then
			self:KillTimer(self.VISIBILITY_TIMER_ID);
		end
	end
	
	if (not self.localOnly and self.isServer and enable~=self.enabled) then
		if (self.otherClients) then -- during Init this doesn't exist
			self.otherClients:ClEnable(g_localChannelId, enable);
		end
	end
	
	self.enabled=enable;
	self.enableChanged=true;
end


function CinematicTrigger:OnLoad(props)
		self:OnReset();
		self:Enable(props.enabled);
		self.triggered =	props.triggered;
		self.triggeredOnce =	props.triggeredOnce;
end


function CinematicTrigger:OnSave(props)
		props.enabled = self.enabled;
		props.triggered =	self.triggered;
		props.triggeredOnce =	self.triggeredOnce;
end


function CinematicTrigger.Client:ClEnable(enable)
	if (enable) then
		self:SetTimer(self.VISIBILITY_TIMER_ID, self.Properties.fCheckTimer*1000);
		self.last_visible_time = nil;
	else
		self:KillTimer(self.VISIBILITY_TIMER_ID);
	end
	
	self.enabled=enable;
end


function CinematicTrigger.Server:OnPostInitClient(channelId)
	if (self.enableChanged) then
		self.onClient:ClEnable(channelId, self.enabled);
	end
end


function CinematicTrigger.Server:SvRequestVisible(entityId)
	self:Visible(entityId);
end


function CinematicTrigger.Server:SvRequestInvisible(entityId)
	self:Invisible(entityId);
end


function CinematicTrigger:Trigger(entityId)
	if (self.triggerOnce) then
		self:KillTimer(self.VISIBILITY_TIMER_ID);
	end

	if(self.Properties.ScriptCommand and self.Properties.ScriptCommand~="")then
		local f = loadstring(self.Properties.ScriptCommand);
		if (f~=nil) then
			f();
		end
	end
	
	if(self.Properties.Sequence~="")then
		Movie.PlaySequence(self.Properties.Sequence);
	end
end


function CinematicTrigger:Visible(entityId)
	if (not self:CanTrigger(entityId)) then
		return;
	end
	
	self.visible[entityId]=true;
	self.visibleCount=self.visibleCount+1;
	
	if (not self.localOnly) then
		if (self.visibleCount<self.minPlayers) then
			return;
		end
	end

	self:Event_Visible(entityId);

	self:CreateTimer(entityId, self.Properties.fDelay*1000);
end


function CinematicTrigger:Invisible(entityId)
	if (not self:CanTrigger(entityId)) then
		return;
	end

	self.visible[entityId]=nil;
	self.visibleCount=self.visibleCount-1;
	
	self:Event_Invisible(entityId);
end


function CinematicTrigger:CanTrigger(entity)
	local Properties = self.Properties;

	if (Properties.bInVehicleOnly ~= 0 and not entity.vehicle) then
		return false;
	end

	return true;
end


function CinematicTrigger:IsVisible()
	local point0 = g_Vectors.temp_v1;
	local point1 = g_Vectors.temp_v2;
	local point2 = g_Vectors.temp_v3;
	local point3 = g_Vectors.temp_v4;
	
	self.pos = self:GetWorldPos(self.pos);
	local p = self.pos;
	
	local dx = self.Properties.fDimX*0.5;
	local dy = self.Properties.fDimY*0.5;
	local dz = self.Properties.fDimZ*0.5;
	
	point0.x = p.x+dx; point0.y = p.y+dy; point0.z = p.z+dz;
	point1.x = p.x+dx; point1.y = p.y+dy; point1.z = p.z-dz;
	point2.x = p.x+dx; point2.y = p.y-dy; point2.z = p.z+dz;
	point3.x = p.x+dx; point3.y = p.y-dy; point3.z = p.z-dz;
	
	if (System.IsPointVisible(point0) and
			System.IsPointVisible(point1) and
			System.IsPointVisible(point2) and
			System.IsPointVisible(point3)) then
		point0.x = p.x-dx; point0.y = p.y+dy; point0.z = p.z+dz;
		point1.x = p.x-dx; point1.y = p.y+dy; point1.z = p.z-dz;
		point2.x = p.x-dx; point2.y = p.y-dy; point2.z = p.z+dz;
		point3.x = p.x-dx; point3.y = p.y-dy; point3.z = p.z-dz;
		
		if (System.IsPointVisible(point0) and
				System.IsPointVisible(point1) and
				System.IsPointVisible(point2) and
				System.IsPointVisible(point3)) then
		
			return true;
		end
	end
	
	return false;
end


function CinematicTrigger:CheckZoom()
	if(self.Properties.fZoomMinimum>1.0)then
		local fov= System.GetViewCameraFov()/math.pi*180.0;
		local client=System.GetCVar("cl_fov");
		local zoom=client/fov;
		if(zoom>=self.Properties.fZoomMinimum)then
			return true;
		else
			return false;
		end;
	else
		return true
	end;
end;


function CinematicTrigger:IsInRange()
	if (not g_localActor) then
		return;
	end
	
	local pos = self:GetWorldPos(g_Vectors.temp_v1);
	local actorpos = g_localActor:GetWorldPos(g_Vectors.temp_v2);
	local distance = vecDistanceSq(pos, actorpos);
	local mind = self.Properties.fMinDistance;
	local maxd = self.Properties.fMaxDistance;
	
	if ((distance < mind*mind) or (distance > maxd*maxd)) then
		return false;
	end
	
	return true;
end


function CinematicTrigger:CreateTimer(entityId, time)
	local timerId=self.timerId;
	if (timerId>1023) then
		timerId=1;
	end
	timerId=timerId+1;
	self.timerId=timerId;
	
	self.timers[timerId]=entityId;
	self:SetTimer(timerId, time);
end


function CinematicTrigger.Client:OnTimer(timerId, time)
	if (timerId == self.VISIBILITY_TIMER_ID) then
		if (self:IsInRange() and self:IsVisible() and self:CheckZoom()) then
			if (not self.isvisible) then
				if (not self.last_visible_time) then
					self.last_visible_time = _time;
				end
	
				if (_time-self.last_visible_time > self.Properties.fMinVisibleTime) then
					if (self.isServer) then	-- became visible
						self:Visible(g_localActorId);
					else
						self.server:SvRequestVisible(g_localActorId);
					end
					
					self.isvisible=true;
				end
			end
		else
			if (self.last_visible_time and self.isvisible) then
				if (self.isServer) then	-- became invisible
					self:Invisible(g_localActorId);
				else
					self.server:SvRequestInvisible(g_localActorId);
				end

				self.isvisible=nil;
			end
			self.last_visible_time = nil;
		end
		self:SetTimer(self.VISIBILITY_TIMER_ID, self.Properties.fCheckTimer*1000);
	else
		if (self.localOnly and not self.isServer) then
			self:Event_Trigger(g_localActorId);
		end
	end
end


function CinematicTrigger.Server:OnTimer(timerId, time)
	if (timerId ~= self.VISIBILITY_TIMER_ID) then
		local entityId=self.timers[timerId];
		if (not entityId) then return; end;

		self:Event_Trigger(entityId);
	end
end


function CinematicTrigger:Event_Enable()
	self:Enable(true);
	
	BroadcastEvent(self, "Enable");
end


function CinematicTrigger:Event_Disable()
	self:Enable(false);
	
	BroadcastEvent(self, "Disable");
end


function CinematicTrigger:Event_Visible(entityId)
	if (not self.enabled) then return; end; -- TODO: might need a self.active here
	if (self.triggerOnce) then
		if (self.localOnly) then
			if (self.triggeredOnce) then
				return;
			end
		elseif (self.perPlayer and self.triggeredOncePP[entityId]) then	-- TODO: will need to skip this for non-player entities
			return;
		elseif (not self.perPlayer and self.triggeredOnce) then
			return;
		end
	end
	
	self.triggered=true;
	self.triggeredOnce=true;
	
	if (not self.localOnly and entityId) then
		self.triggeredPP[entityId]=true;
		self.triggeredOncePP[entityId]=true;
	end
		
	Log("%s:Event_Visible(%s)", self:GetName(), EntityName(entityId));	
	
	BroadcastEvent(self, "Visible");
	
	if (not self.localOnly and self.isServer) then
		self.otherClients:ClVisible(g_localChannelId, entityId or NULL_ENTITY);
	end
end


function CinematicTrigger.Client:ClVisible(entityId)
	Log("%s.Client:ClVisible(%s)", self:GetName(), EntityName(entityId));
	
	BroadcastEvent(self, "Visible");
end


function CinematicTrigger:Event_Invisible(entityId)
	if (not self.enabled) then return; end;
	if (self.localOnly and not self.triggered) then return; end;
	
	if (self.perPlayer) then
		if (not self.localOnly and entityId) then
			if (not self.triggeredPP[entityId]) then return; end;
		end
	else
		if (not self.triggered) then return; end;
	end
	
	self.triggered=nil;
	
	if (not self.localOnly and entityId) then
		self.triggeredPP[entityId]=nil;
	end

	Log("%s:Event_Invisible(%s)", self:GetName(), EntityName(entityId));

	BroadcastEvent(self, "Invisible");
	
	if (not self.localOnly and self.isServer) then
		self.otherClients:ClInvisible(g_localChannelId, entityId or NULL_ENTITY);
	end
end


function CinematicTrigger.Client:ClInvisible(entityId)
	Log("%s.Client:ClInvisible(%s)", self:GetName(), EntityName(entityId));
	
	BroadcastEvent(self, "Invisible");
end


function CinematicTrigger:Event_Trigger(entityId)
	Log("%s:Trigger(%s)", self:GetName(), EntityName(entityId));
	
	self:Trigger(entityId);
	
	BroadcastEvent(self, "Trigger");
	
	if (not self.localOnly and self.isServer) then
		self.otherClients:ClTrigger(g_localChannelId, entityId or NULL_ENTITY);
	end
end


function CinematicTrigger.Client:ClTrigger(entityId)
	Log("%s.Client:ClTrigger(%s)", self:GetName(), EntityName(entityId));
	
	self:Trigger(entityId);
	
	BroadcastEvent(self, "Trigger");
end


CinematicTrigger.FlowEvents =
{
	Inputs =
	{
		Disable = { CinematicTrigger.Event_Disable, "bool" },
		Enable = { CinematicTrigger.Event_Enable, "bool" },
		
		Invisible = { CinematicTrigger.Event_Invisible, "bool" },
		Visible = { CinematicTrigger.Event_Visible, "bool" },
		
		Trigger = { CinematicTrigger.Event_Trigger, "bool" },
	},

	Outputs =
	{
		Disable = "bool",
		Enable = "bool",
		
		Invisible = "bool",
		Visible = "bool",
		
		Trigger = "bool",
	},
}