--------------------------------------------------------------------------
--	Crytek Source File.
-- 	Copyright (C), Crytek Studios, 2001-2008.
--------------------------------------------------------------------------
--	$Id$
--	$DateTime$
--	Description: Spotter Scan Target
--  
--------------------------------------------------------------------------
--  History:
--  - 18:11:2008 : Created by Márcio Martins
--
--------------------------------------------------------------------------

SpotterScannable = {
	Properties = {
		bEnabled=1,
		bOnlyOnce=0,
		fEMPChance=1,
		fScanChance=1,
	},
	
	Editor={
		Icon = "switch.bmp",
		IconOnTop = 1,
	},
}


function SpotterScannable:OnStartGame()
	self.entityId=nil
	
	local parent=self:GetParent()
	if (not parent) then
		parent=self:GetChild(0)
	end
	
	if (parent) then
		self.entityId=parent.id
	end
end


function SpotterScannable:OnPropertyChange()
	self:OnReset();
end


function SpotterScannable:OnReset()
	self.onlyOnce=self.Properties.bOnlyOnce~=0
	self.empChance=math.min(math.max(0, self.Properties.fEMPChance), 1)
	self.scanChance=math.min(math.max(0, self.Properties.fScanChance), 1)
	self.scannable=self.Properties.bEnabled~=0
	self.scanned=nil
end


function SpotterScannable:IsScannable()
	return self.scannable and (not self.scanned or not self.onlyOnce)
end


function SpotterScannable:ScanStarted(spotterId)
	self:Event_ScanStarted(spotterId)
	self.scanned=true
end


function SpotterScannable:ScanEnded(spotterId)
	self:Event_ScanEnded(spotterId)
end


function SpotterScannable:EMP(spotterId)
	self:Event_EMP(spotterId)
end


function SpotterScannable:SpotterInterested(spotterId)
	self:Event_SpotterInterested(spotterId)
end


function SpotterScannable:GetEMPChance()
	return self.empChance
end


function SpotterScannable:GetScanChance()
	return self.scanChance
end


function SpotterScannable:Event_ScanStarted(spotterId)
	self:ActivateOutput("Entity", self.entityId or NULL_ENTITY)
	self:ActivateOutput("Spotter", spotterId or NULL_ENTITY)
	BroadcastEvent(self, "ScanStarted")
end


function SpotterScannable:Event_ScanEnded(spotterId)
	self:ActivateOutput("Entity", self.entityId or NULL_ENTITY)
	self:ActivateOutput("Spotter", spotterId or NULL_ENTITY)
	BroadcastEvent(self, "ScanEnded")
end


function SpotterScannable:Event_SpotterInterested(spotterId)
	self:ActivateOutput("Entity", self.entityId or NULL_ENTITY)
	self:ActivateOutput("Spotter", spotterId or NULL_ENTITY)
	BroadcastEvent(self, "SpotterInterested");
end


function SpotterScannable:Event_EMP(spotterId)
	self:ActivateOutput("Entity", self.entityId or NULL_ENTITY)
	self:ActivateOutput("Spotter", spotterId or NULL_ENTITY)
	BroadcastEvent(self, "EMP");
end


function SpotterScannable:Event_Enable(sender)
	self.scannable=true
	BroadcastEvent( self,"Enable" );
end


function SpotterScannable:Event_Disable(sender)
	self.scannable=false
	BroadcastEvent( self,"Disable" );
end


SpotterScannable.FlowEvents =
{
	Inputs =
	{
		Disable = { SpotterScannable.Event_Disable },
		Enable = { SpotterScannable.Event_Enable },
	},

	Outputs =
	{
		ScanEnded = "bool",
		ScanStarted = "bool",
		EMP = "bool",
		SpotterInterested = "bool",
		Disable = "bool",
		Enable = "bool",
		Entity = "entity",
		Spotter = "entity",
	},
}