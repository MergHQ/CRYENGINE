----------------------------------------------------------------------------------------------------
--  Crytek Source File.
--  Copyright (C), Crytek Studios, 2009.
----------------------------------------------------------------------------------------------------
--  $Id$
--  $DateTime$
--  Description: Intel target for assault gamemode
--
----------------------------------------------------------------------------------------------------
--  History:
--  - 02:06:2009   08:54 : Created by Colin Gulliver
--
----------------------------------------------------------------------------------------------------

AssaultIntel = {
	Client = {},
	Server = {},
	Properties = {
		fileModelOverride = "",
		ControlRadius = 3;
		ControlHeight = 3;
		ControlOffsetZ = 0;
		teamName			= "",
		DebugDraw = 0,
	},
	Editor = {
		Icon		= "Item.bmp",
		IconOnTop	= 1,
	},
	previousUploadProgress = -20.0,
	previousTeam = 0,
}

AssaultIntel.DEFAULT_FILE_MODEL = "objects/multiplayer/props/cw2_assault_mod_monitor/cw2_assault_mod_monitor.cgf";

----------------------------------------------------------------------------------------------------
Net.Expose {
	Class = AssaultIntel,

	ClientMethods = {},
	ServerMethods = {},
	ServerProperties = {
		UploadProgress = FLOAT,
		UploadInProgress = BOOL,
	},
}


----------------------------------------------------------------------------------------------------
function AssaultIntel.Server:OnInit()
	--Log("AssaultIntel.Server.OnInit");
	if (not self.bInitialized) then
		self:OnReset();
		self.bInitialized = 1;
		self:SetViewDistRatio(255);			
	end
	self.inside = {};
	self.synched.UploadProgress = 0.0;
	self.synched.UploadInProgress = false;
end


----------------------------------------------------------------------------------------------------
function AssaultIntel.Client:OnInit()
	--Log("AssaultIntel.Client.OnInit");
	self.ClAlarmActive = false;
	self.ClAlarmSoundID = nil;

	if (not self.bInitialized) then
		self:OnReset();
		self.bInitialized = 1;
		self:SetViewDistRatio(255);			
	end
	self.inside = {};
end


function AssaultIntel.Client:WarningAlarm(onOff)
    --Log("AssaultIntel.Client:WarningAlarm %s", tostring(onOff));

    if (onOff) then
	self.ClAlarmActive = true;
	if (not self.ClAlarmSoundID) then
	    self.ClAlarmSoundID = Sound.Play("Sounds/crysiswar2:interface:multiplayer/mp_alarm_ambience_loop", self:GetWorldPos(), SOUND_DEFAULT_3D, SOUND_SEMANTIC_MECHANIC_ENTITY);
	end
    else
	self.ClAlarmActive = false;
	if (self.ClAlarmSoundID) then
	    Sound.StopSound(self.ClAlarmSoundID);
	    self.ClAlarmSoundID = nil;
	end
    end
end

----------------------------------------------------------------------------------------------------
function AssaultIntel.Client:OnUpdate(frameTime)
	if (g_localActorId and g_gameRules) then
		if (self.synched.UploadInProgress==true and self.ClAlarmActive==false) then
		    self.Client.WarningAlarm(self, true)
		elseif (self.synched.UploadInProgress==false and self.ClAlarmActive==true) then
		    self.Client.WarningAlarm(self, false)
		end
	end
end


----------------------------------------------------------------------------------------------------
function AssaultIntel:OnReset()
	--Log("AssaultIntel.OnReset");
	local fileModel = self.DEFAULT_FILE_MODEL;
	if (self.Properties.fileModelOverride and (self.Properties.fileModelOverride ~= "")) then
		fileModel = self.Properties.fileModelOverride;
	end
	self:LoadObject(0, fileModel);
	self:Physicalize(0, PE_STATIC, { mass=0 });

	local radius = self.Properties.ControlRadius;
	local offsetZ = self.Properties.ControlOffsetZ;
	local height = self.Properties.ControlHeight;

	local min = { x=-radius, y=-radius, z=offsetZ };
	local max = { x=radius, y=radius, z=(height + offsetZ) };

	self:SetTriggerBBox( min, max );
	self:SetViewDistRatio(255);		
	self:SetFlags(ENTITY_FLAG_ON_RADAR, 0);

	if (self.isClient and self.ClAlarmActive==true) then
	    self.Client.WarningAlarm(self, false);
	end

	if (System.IsEditor()) then
		if (self.Properties.DebugDraw ~= 0) then
			self:Activate(1);
		else
			self:Activate(0);
		end
	end
end


----------------------------------------------------------------------------------------------------
function AssaultIntel:OnPropertyChange()
	self:OnReset();
end

----------------------------------------------------------------------------------------------------
function AssaultIntel.Client:OnEnterArea(entity, areaId)
	--Log("AssaultIntel.Client.OnEnterArea");
	local inserted = false;
	for i,id in ipairs(self.inside) do
		if (id==entity.id) then
			inserted=true;	
			--Log("Entity already inserted");
			break;
		end
	end
	if (not inserted) then
		table.insert(self.inside, entity.id);
		--Log("Entity added");
	end
end


----------------------------------------------------------------------------------------------------
function AssaultIntel.Client:OnLeaveArea(entity, areaId)
	--Log("AssaultIntel.Client.OnLeaveArea");
	for i,id in ipairs(self.inside) do
		if (id==entity.id) then
			table.remove(self.inside, i);
			--Log("Entity removed");
			break;
		end
	end
end


----------------------------------------------------------------------------------------------------
function AssaultIntel:EntityInsideArea(entityId)
	for i,id in ipairs(self.inside) do
		if (id==entityId) then
			return true;
		end
	end
	return false;
end


----------------------------------------------------------------------------------------------------
function AssaultIntel.Server:OnEnterArea(entity, areaId)
	--Log("AssaultIntel.Server.OnEnterArea");
	if (entity and entity.actor and entity.actor:IsPlayer()) then
		local inserted = false;
		for i,id in ipairs(self.inside) do
			if (id==entity.id) then
				inserted=true;	
				--Log("Entity already inserted");
				break;
			end
		end
		if (not inserted) then
			table.insert(self.inside, entity.id);
			--Log("Entity added");
		end
		
		if (g_gameRules.Server.EntityEnterIntelPoint ~= nil) then
			g_gameRules.Server.EntityEnterIntelPoint(g_gameRules, self, entity);
		end
	end
end


----------------------------------------------------------------------------------------------------
function AssaultIntel.Server:OnLeaveArea(entity, areaId)
	--Log("AssaultIntel.Server.OnLeaveArea");
	if (entity and entity.actor and entity.actor:IsPlayer()) then
		for i,id in ipairs(self.inside) do
			if (id==entity.id) then
				table.remove(self.inside, i);
				--Log("Entity removed");
				break;
			end
		end
		
		if (g_gameRules.Server.EntityLeaveIntelPoint ~= nil) then
			g_gameRules.Server.EntityLeaveIntelPoint(g_gameRules, self, entity);
		end
	end
end

----------------------------------------------------------------------------------------------------
function AssaultIntel.Server:OnUpdate(frameTime)
	if (self.Properties.DebugDraw ~= 0) then
		local pos = self:GetWorldPos();
		local radius = self.Properties.ControlRadius;
		local height = self.Properties.ControlHeight;
		local offsetZ = self.Properties.ControlOffsetZ;

		pos.z = pos.z + (height * 0.5) + offsetZ;

		Game.DebugDrawCylinder(pos.x, pos.y, pos.z, radius, height, 60, 60, 255, 100);
	end
end

