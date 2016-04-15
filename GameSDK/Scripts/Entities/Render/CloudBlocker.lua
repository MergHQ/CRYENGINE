
CloudBlocker =
{
	Properties =
	{
		bActive = 1, --[0,1,1,"If true, cloud blocker will be enabled."]
		fDecayInfluence = 1.0, --[0,1,0.01,"Specifies the influence of fog density decay."]
		DecayEnd = 0.0, --[0,100000,1.0,"Specifies the end distance of fog density decay in world units (m)."]
		DecayStart = 0.0, --[0,100000,1.0,"Specifies the start distance of cloud density decay in world units (m)."]
		bScreenSpace = 0, --[0,1,1,"If true, cloud blocker is projected on screen space."]
	},

	Editor = 
	{
		Model = "Editor/Objects/Particles.cgf",
		Icon = "Clouds.bmp",
		ShowBounds = 1,
	},

	_CloudBlocker = { position={x=0,y=0,z=0},decayStart=0,decayEnd=0,decayInfluence=1,screenSpace=0 },
}

-------------------------------------------------------
function CloudBlocker:EnableBlock()
	self.bActive = true;
end

-------------------------------------------------------
function CloudBlocker:DisableBlock()
	self.bActive = false;
end

-------------------------------------------------------
function CloudBlocker:OnSpawn()
	self:SetFlags(ENTITY_FLAG_CLIENT_ONLY, 0);
	self:SetFlags(ENTITY_FLAG_NO_PROXIMITY, 0);
end

-------------------------------------------------------
function CloudBlocker:OnInit()
	self.bActive = false;
	if( self.Properties.bActive == 1 ) then
		self:EnableBlock();
	end
	self:Activate(1);
	-- Force object updates.
	CryAction.ForceGameObjectUpdate(self.id, true);
end

-------------------------------------------------------
function CloudBlocker:OnShutDown()
	self:DisableBlock();
	self:Activate(0);
end

-------------------------------------------------------
function CloudBlocker:OnPropertyChange()
	self:DisableBlock();
	if( self.Properties.bActive == 1 ) then
		self:EnableBlock();
	end
end

-------------------------------------------------------
function CloudBlocker:OnReset()
	self:DisableBlock();
	if( self.Properties.bActive == 1 ) then
		self:EnableBlock();
	end
end

----------------------------------------------------------------------------------------
function CloudBlocker:OnUpdate( dt )
	if( self.bActive == true and self.Properties.bActive == 1 ) then
		local vEntityPos = self:GetPos();
		self._CloudBlocker.position = vEntityPos;
		self._CloudBlocker.decayStart = self.Properties.DecayStart;
		self._CloudBlocker.decayEnd = self.Properties.DecayEnd;
		self._CloudBlocker.decayInfluence = self.Properties.fDecayInfluence;
		self._CloudBlocker.screenSpace = self.Properties.bScreenSpace;
		System.PushCloudBlocker( self._CloudBlocker );
	end
end

-------------------------------------------------------
-- Serialization
-------------------------------------------------------
function CloudBlocker:OnLoad(table)
	if(self.bActive and not table.bActive) then
		self:DisableBlock();
	elseif(not self.bActive and table.bActive) then
		self:EnableBlock();
	end
	self.bActive = table.bActive;
end

-------------------------------------------------------
function CloudBlocker:OnSave(table)
	table.bActive = self.bActive;
end

-------------------------------------------------------
-- Set Enabled Event
-------------------------------------------------------
function CloudBlocker:Event_Enabled(i, enable)
	if (enable) then
		self:EnableBlock();
		self:ActivateOutput( "Enabled", true );
	else
		self:DisableBlock();
		self:ActivateOutput( "Enabled", false );
	end
end

-------------------------------------------------------
-- Set DecayStart Event
-------------------------------------------------------
function CloudBlocker:Event_SetDecayStart(i, val)
	self.Properties.DecayStart = val;
end

-------------------------------------------------------
-- Set DecayEnd Event
-------------------------------------------------------
function CloudBlocker:Event_SetDecayEnd(i, val)
	self.Properties.DecayEnd = val;
end

-------------------------------------------------------
-- Set DecayInfluence Event
-------------------------------------------------------
function CloudBlocker:Event_SetDecayInfluence(i, val)
	self.Properties.fDecayInfluence = val;
end

-------------------------------------------------------
CloudBlocker.FlowEvents =
{
	Inputs =
	{
		EV_Enabled  = { CloudBlocker.Event_Enabled, "bool" },
		EV_DecayStart  = { CloudBlocker.Event_SetDecayStart, "float" },
		EV_DecayEnd  = { CloudBlocker.Event_SetDecayEnd, "float" },
		EV_DecayInfluence  = { CloudBlocker.Event_SetDecayInfluence, "float" },
	},
	Outputs =
	{
		Enabled = "bool"
	},
}
