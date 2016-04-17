----------------------------------------------------------------------------------------------------
--  Crytek Source File.
--  Copyright (C), Crytek Studios, 2009.
----------------------------------------------------------------------------------------------------
--  $Id$
--  $DateTime$
--  Description: Retrieval target point for extraction gamemode
--
----------------------------------------------------------------------------------------------------
--  History:
--  - 26:05:2009   08:54 : Created by Colin Gulliver
--
----------------------------------------------------------------------------------------------------

ExtractionPoint = {
    Client = {},
    Server = {},
    Properties = {
		fileModelOverride = "",
		Radius            = 2,
		OpenRange         = 6,
    },
    Editor = {
		Icon		= "Item.bmp",
		IconOnTop	= 1,
    },
	modelSlot = -1,
	spawnPointIds = {},
	open = false,
}

ExtractionPoint.DEFAULT_FILE_MODEL = "objects/multiplayer/props/c3mp_gamemode_props/c3mp_powercell/c3mp_powercell.cdf";

----------------------------------------------------------------------------------------------------
function ExtractionPoint.Server:OnInit()
	Log("ExtractionPoint.Server.OnInit");
	if (not self.bInitialized) then
		self:OnReset();
		self.bInitialized = 1;
		self:SetViewDistRatio(255);		
	end;
	self.inside = {};
end


----------------------------------------------------------------------------------------------------
function ExtractionPoint.Client:OnInit()
	Log("ExtractionPoint.Client.OnInit");
	if (not self.bInitialized) then
		self:OnReset();
		self.bInitialized = 1;
		self:SetViewDistRatio(255);			
	end;
	self.inside = {};
end


----------------------------------------------------------------------------------------------------
function ExtractionPoint:OnReset()
    Log("ExtractionPoint.OnReset");

	local radius_2 = self.Properties.Radius / 2;
	
 	local Min={x=-radius_2,y=-radius_2,z=-radius_2};
	local Max={x=radius_2,y=radius_2,z=radius_2};
	self:SetTriggerBBox(Min,Max);
	self:SetViewDistRatio(255);	
	if (System.IsEditor()) then
		Log("In editor, activating");
		self:ActivateSite();

		-- Look for spawn points now so they appear in the editor log, we don't do this for the main game
		-- since the extraction point can be initialised before the spawn points exist
		self:AddSpawnPoints("spawn");
		self:AddSpawnPoints("spawnpoint");
		self:AddSpawnPoints("disruptor");
		Log("Entity=%s, found %d associated spawn points", EntityName(self), #self.spawnPointIds);
	end
end


----------------------------------------------------------------------------------------------------
function ExtractionPoint:AddSpawnPoints(linkName)
	local i = 0;
	local link = self:GetLinkTarget(linkName, i);
	while (link) do
		InsertIntoTable(self.spawnPointIds, link.id);
		i = i+1;
		link = self:GetLinkTarget(linkName, i);
	end
end


----------------------------------------------------------------------------------------------------
function ExtractionPoint:ActivateSite()
	local fileModel = self.DEFAULT_FILE_MODEL;
	if (self.Properties.fileModelOverride and (self.Properties.fileModelOverride ~= "")) then
		Log("Override file model provided, using model '%s'", self.Properties.fileModelOverride);
		fileModel = self.Properties.fileModelOverride;
	end
    self.modelSlot = self:LoadObject(self.modelSlot, fileModel);
	self:Physicalize(self.modelSlot, PE_STATIC, { mass=0 });
	self:SetViewDistRatio(255);		
	self:SetFlags(ENTITY_FLAG_ON_RADAR, 0);
end


----------------------------------------------------------------------------------------------------
function ExtractionPoint:DeactivateSite()
	self:DestroyPhysics();
	self:FreeSlot(self.modelSlot);
	self.modelSlot = -1;
	self:SetViewDistRatio(255);	
	self:SetFlags(ENTITY_FLAG_ON_RADAR, 2);
end


----------------------------------------------------------------------------------------------------
function ExtractionPoint:OnSetTeam(teamId)
	Log("ExtractionPoint (%s) set to team %d", EntityName(self), teamId);
	if (g_gameRules) then
		self.spawnPointIds = {};
		self:AddSpawnPoints("spawn");
		self:AddSpawnPoints("spawnpoint");
		self:AddSpawnPoints("disruptor");
		Log("Entity=%s, found %d associated spawn points", EntityName(self), #self.spawnPointIds);

		for i,v in ipairs(self.spawnPointIds) do
			g_gameRules.game:ClientSetTeam(teamId, v);
		end
	end
end


----------------------------------------------------------------------------------------------------
function ExtractionPoint:SetOpen(open)
	if (self.open ~= open) then
		self.open = open;
		if (self.open) then
			self:StartAnimation(0,"tick_device_open", 0, 0.0, 1.0, 0);
		else
			self:StartAnimation(0,"tick_device_close", 0, 0.0, 1.0, 0);
		end		
	end
end
