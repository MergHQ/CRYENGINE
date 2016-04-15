----------------------------------------------------------------------------
-- 
-- Description :		Delayed proxymity trigger
--
-- Create by Alberto :	03 March 2002
-- 
----------------------------------------------------------------------------
SmartProximityTrigger = {
	type = "Trigger",
	
	Properties = {
		DimX = 5,
		DimY = 5,
		DimZ = 5,
		bEnabled=1,
		EnterDelay=0,
		ExitDelay=0,
		bOnlyPlayer=1,
		bOnlyMyPlayer=0,
		},
	

	Editor={
		Model="Objects/Editor/T.cgf",
		ShowBounds = 1,
	},
		
	-- Who triggered me.
	Who=nil,
	Enabled=1,
	Entered=0,
}

function SmartProximityTrigger:OnPropertyChange()
	self:OnReset();
end

function SmartProximityTrigger:OnInit()
	self:OnReset();
end

function SmartProximityTrigger:OnShutDown()
end

function SmartProximityTrigger:OnReset()
	local Min = { x=-self.Properties.DimX/2, y=-self.Properties.DimY/2, z=-self.Properties.DimZ/2 };
	local Max = { x=self.Properties.DimX/2, y=self.Properties.DimY/2, z=self.Properties.DimZ/2 };
	self:SetBBox( Min, Max );
	--System.LogToConsole( "BBox:"..Min.x..","..Min.y..","..Min.z.."  "..Max.x..","..Max.y..","..Max.z );
	self.Enabled = self.Properties.bEnabled;
	self.Who = nil;
	self.UpdateCounter = 0;
	self.Entered = 0;
	self:GotoState( "Empty" );
end

function SmartProximityTrigger:Event_Enter( sender )
	if ((self.Enabled ~= 1) or (self.Entered ~= 0)) then
		return
	end
	self.Entered = 1;
	if ( sender ) then
		System.LogToConsole( "Player "..sender:GetName().." Enter SmartProximityTrigger "..self:GetName() );
	end
	BroadcastEvent( self,"Enter" );
end

function SmartProximityTrigger:Event_Leave( sender )
	if (self.Enabled ~= 1 or self.Entered == 0) then
		return
	end
	System.LogToConsole( "Player "..sender:GetName().." Leave SmartProximityTrigger "..self:GetName() );
	BroadcastEvent( self,"Leave" );
end

function SmartProximityTrigger:Event_Enable( sender )
	self.Enabled = 1;
	BroadcastEvent( self,"Enable" );
end

function SmartProximityTrigger:Event_Disable( sender )
	self.Enabled = 0;
	self:GotoState( "Empty" );
	BroadcastEvent( self,"Disable" );
end

-------------------------------------------------------------------------------
-- Empty State ----------------------------------------------------------------
-------------------------------------------------------------------------------
SmartProximityTrigger.Empty = 
{
	-------------------------------------------------------------------------------
	OnBeginState = function( self )
		self.Who = nil;
		self.UpdateCounter = 0;
		self.Entered = 0;
		self["StartEnterDelay"]=nil;
	end,

	-------------------------------------------------------------------------------
	OnEndState = function( self )
	end,
	OnTimer = function( self )
		self["StartEnterDelay"]=nil;
	end,
	OnContact = function( self,player )
		-- Ignore if disabled.
		if (self.Enabled ~= 1) then
			return
		end
		
		if (self.Properties.bOnlyPlayer ~= 0 and player.type ~= "Player") 	then
			return
		end
		-- Ignore if not my player.		
		if (self.Properties.bOnlyMyPlayer ~= 0 and player ~= _localplayer) then
			return
		end
		
		if (self.Who==nil) then
			
			if (self.Properties.EnterDelay>0) then
			
				if(self["StartEnterDelay"]==nil) then
				
					self:SetTimer(500);
					System.LogToConsole("tada!");
					self["StartEnterDelay"]=_time*1000;
					
				else
					if(((_time*1000)-self["StartEnterDelay"])>=(self.Properties.EnterDelay*1000))then
					
						self.Who = player;
						self["StartEnterDelay"]=nil;
						self:KillTimer();
						self:GotoState( "Occupied" );
						
					else
					
						self:SetTimer(500);
											
					end
				end
			else
				self.Who = player;
				self:GotoState( "Occupied" );
			end
		end
	end,
}

-------------------------------------------------------------------------------
-- Empty State ----------------------------------------------------------------
-------------------------------------------------------------------------------
SmartProximityTrigger.Occupied = 
{
	-------------------------------------------------------------------------------
	OnBeginState = function( self )
			self:Event_Enter(self.Who);
	end,
	-------------------------------------------------------------------------------
	OnEndState = function( self )
	end,

	OnTimer = function( self )
		self:Event_Enter(self.Who);

	end,
	
	OnContact = function( self,player )
		-- Ignore if disabled.
		if (self.Enabled ~= 1) then
			return
		end
		
		if (self.Properties.bOnlyPlayer ~= 0 and player.type ~= "Player") 	then
			return
		end
		-- Ignore if not my player.		
		if (self.Properties.bOnlyMyPlayer ~= 0 and player ~= _localplayer) then
			return
		end
		
		--add a very small delay(so is immediate)
		if(self.Properties.ExitDelay==0) then
			self.Properties.ExitDelay=0.01;
		end
		
		self:SetTimer(self.Properties.ExitDelay*1000);
	end,
	
	OnTimer = function( self )
		System.LogToConsole("Sending on leave");
		self:Event_Leave( self,self.Who );
		self:GotoState("Empty");
	end,
}

SmartProximityTrigger.FlowEvents =
{
	Inputs =
	{
		Disable = { SmartProximityTrigger.Event_Disable, "bool" },
		Enable = { SmartProximityTrigger.Event_Enable, "bool" },
		Enter = { SmartProximityTrigger.Event_Enter, "bool" },
		Leave = { SmartProximityTrigger.Event_Leave, "bool" },
	},
	Outputs =
	{
		Disable = "bool",
		Enable = "bool",
		Enter = "bool",
		Leave = "bool",
	},
}
