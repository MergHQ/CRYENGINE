VisibilityTrigger = {
	type = "Trigger",

	Properties = {
		DimX = 1,
		DimY = 1,
		DimZ = 1,
		Distance = 100,
		bEnabled=1,
		EnterDelay=0,
		ExitDelay=0,
		bTriggerOnce=0,
		ScriptCommand="",
		PlaySequence="",		
		TextInstruction= "",
		bUseKey=0,
	},
	
	Editor={
		Model="Editor/Objects/Vis.cgf",
		ShowBounds = 1,
	},	
}

function VisibilityTrigger:OnPropertyChange()
	self:OnReset();
end

function VisibilityTrigger:OnInit()
	self:Activate(0);
	self:SetUpdateType( eUT_Visible );
	self:SetRegisterInSectors(1);

	self:RegisterState("Disabled");
	self:RegisterState("Visible");
	self:RegisterState("Invisible");
	self:OnReset();
end

function VisibilityTrigger:OnShutDown()
end

function VisibilityTrigger:OnSave(stm)
	stm:WriteInt(self.bTriggered);
end


function VisibilityTrigger:OnLoad(stm)
	self.bTriggered=stm:ReadInt();
end

function VisibilityTrigger:OnReset()
	self:KillTimer();
	self.bTriggered = 0;
	self.bVisible = 0;

	local Min = { x=-self.Properties.DimX/2, y=-self.Properties.DimY/2, z=-self.Properties.DimZ/2 };
	local Max = { x=self.Properties.DimX/2, y=self.Properties.DimY/2, z=self.Properties.DimZ/2 };
	self:SetBBox( Min, Max );
	
	self:SetUpdateRadius( self.Properties.Distance );
		
	if(self.Properties.bEnabled==1)then
		self:GotoState( "Invisible" );
	else
		self:GotoState( "Disabled" );
	end
end

function VisibilityTrigger:Event_Visible( sender )
	if ((self.bVisible ~= 0)) then
		return
	end
	if (self.Properties.bTriggerOnce == 1 and self.bTriggered == 1) then
		return
	end
	self.bTriggered = 1;
	self.bVisible = 1;
	-- Trigger script command on enter.
	if(self.Properties.ScriptCommand and self.Properties.ScriptCommand~="")then
		dostring(self.Properties.ScriptCommand);
	end
	if(self.Properties.PlaySequence~="")then
		Movie.PlaySequence( self.Properties.PlaySequence );
	end

	BroadcastEvent( self,"Visible" );
	
	self:GotoState("Visible");
	
	System.Log("* VISIBLE");
end

function VisibilityTrigger:Event_Invisible( sender )
	if (self.bVisible == 0) then
		return
	end
	self.bVisible = 0;
	BroadcastEvent( self,"Invisible" );
	
	if(self.Properties.bTriggerOnce==1)then
		self:GotoState("Disabled");
	else
		self:GotoState("Invisible");
	end
	
	System.Log("* INVISIBLE");
end

function VisibilityTrigger:Event_Enable( sender )
	self:GotoState("Invisible")
	BroadcastEvent( self,"Enable" );
end

function VisibilityTrigger:Event_Disable( sender )
	self:GotoState( "Disabled" );
	BroadcastEvent( self,"Disable" );
end

-------------------------------------------------------------------------------
-- Inactive State -------------------------------------------------------------
-------------------------------------------------------------------------------
VisibilityTrigger.Disabled =
{
}
-------------------------------------------------------------------------------
-- Invisible State ----------------------------------------------------------------
-------------------------------------------------------------------------------
VisibilityTrigger.Invisible =
{
	-------------------------------------------------------------------------------
	OnBeginState = function( self )
		self.bVisible = 0;
	end,

	OnTimer = function( self )
		self.OnUpdateCalled = nil;
		self:GotoState( "Visible" );
	end,

	-------------------------------------------------------------------------------
	OnUpdate = function( self )
		if (self.OnUpdateCalled == nil) then
			self.OnUpdateCalled = 1;
			if (self.Properties.bUseKey~=0) then
				self.OnUpdateCalled = nil;
				self:GotoState( "VisibleUse" );
				do return end;
			end
			
			if (self.Properties.EnterDelay ~= 0) then
				self:SetTimer( self.Properties.EnterDelay*1000 );
			else
				self.OnUpdateCalled = nil;
				self:GotoState( "Visible" );
			end
		end
	end,


}

-------------------------------------------------------------------------------
-- Occupied State ----------------------------------------------------------------
-------------------------------------------------------------------------------
VisibilityTrigger.Visible =
{
	-------------------------------------------------------------------------------
	OnBeginState = function( self )
		self.ExitDelay = self.Properties.ExitDelay;
		self:Event_Visible(self);
	end,

	-------------------------------------------------------------------------------
	OnTimer = function( self )
		-- If timer ever reached updates are stoped.
		-- Update not called, so it is not visible anymore.
		if (self.ExitDelay ~= 0) then
			self:SetTimer( self.ExitDelay*1000 );
			self.ExitDelay = 0;
		else
			self:Event_Invisible( self );
		end
		self.ExitDelay = self.Properties.ExitDelay;
	end,
	
	OnUpdate = function( self )
		self:SetTimer( 200 ); -- Enable 200ms timer.
	end
}

-------------------------------------------------------------------------------
-- OccupiedText State ---------------------------------------------------------
-------------------------------------------------------------------------------
VisibilityTrigger.VisibleUse =
{
	-------------------------------------------------------------------------------
	OnBeginState = function( self )
	end,

	-------------------------------------------------------------------------------
	OnTimer = function( self )
		-- Update not called, so it is not visible anymore.
		self:GotoState( "Invisible" );
	end,
	
	OnUpdate = function( self )
		self:SetTimer( 200 ); -- Enable 200ms timer.
		
		if (_localplayer and _localplayer.cnt and _localplayer.cnt.use_pressed) then
			self:GotoState( "Visible" );
		else
			if (Hud) then
				Hud.label = self.Properties.TextInstruction;
			end
		end
	end
}

VisibilityTrigger.FlowEvents =
{
	Inputs =
	{
		Disable = { VisibilityTrigger.Event_Disable, "bool" },
		Enable = { VisibilityTrigger.Event_Enable, "bool" },
		Invisible = { VisibilityTrigger.Event_Invisible, "bool" },
		Visible = { VisibilityTrigger.Event_Visible, "bool" },
	},
	Outputs =
	{
		Disable = "bool",
		Enable = "bool",
		Invisible = "bool",
		Visible = "bool",
	},
}
