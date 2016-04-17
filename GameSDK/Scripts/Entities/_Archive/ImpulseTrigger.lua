----------------------------------------------------------------------------
--
-- Description :		Impulse trigger - gives a (continuous) impulse to the player
--
-- Created by Luciano :	May 03 2003
--
----------------------------------------------------------------------------
ImpulseTrigger = {
	type = "Trigger",

	Properties = {
		DimX = 5,
		DimY = 5,
		DimZ = 5,
		bEnabled=1,
		bTriggerOnce=0,
		bKillOnTrigger=0,
		bOnlyPlayer=0,
		bOnlyMyPlayer=1,
		bOnlyAI = 0,
		ImpulseStrength = 20,
		ImpulseFadeInTime = 0.0,
		ImpulseDuration = 0.1,
	},

	imp = {x=0,y=0,z=0},
	MaxImpulse = 0, 
	InitImpulseTime = 0,
	TotalImpulseDuration = 0,
	Editor={
		Model="Objects/Editor/T.cgf",
	},
}

function ImpulseTrigger:OnPropertyChange()
	self:OnReset();
end

function ImpulseTrigger:OnInit()
	self.Who = nil;
	self.Entered = 0;

	self:RegisterState("Inactive");
	self:RegisterState("Empty");
	self:RegisterState("Occupied");
	self:OnReset();
	self:Activate(0);
	self:TrackColliders(1);
end

function ImpulseTrigger:OnShutDown()
end

function ImpulseTrigger:OnSave(stm)
	--WriteToStream(stm,self.Properties);
end


function ImpulseTrigger:OnLoad(stm)
	--self.Properties=ReadFromStream(stm);
	--self:OnReset();
end

function ImpulseTrigger:OnReset()

	self.imp = new(self:GetDirectionVector());
 
	local Min = { x=-self.Properties.DimX/2, y=-self.Properties.DimY/2, z=-self.Properties.DimZ/2 };
	local Max = { x=self.Properties.DimX/2, y=self.Properties.DimY/2, z=self.Properties.DimZ/2 };
	self:SetBBox( Min, Max );
	--System.LogToConsole( "BBox:"..Min.x..","..Min.y..","..Min.z.."  "..Max.x..","..Max.y..","..Max.z );
	self.Who = nil;

	self.Entered = 0;
	self.TotalImpulseDuration = self.Properties.ImpulseFadeInTime + self.Properties.ImpulseDuration;
	
	if(self.Properties.bEnabled==1)then
		self:GotoState( "Empty" );
	else
		self:GotoState( "Inactive" );
	end

--	AI:RegisterWithAI(self.id, AIAnchor[self.Properties.aianchorAIAction]);
end

function ImpulseTrigger:Event_Enter( sender )
--	self.imp = self:GetDirectionVector();

	if ((self.Entered ~= 0)) then
		do return end
	end
--	self.Entered = 1;

	--Hud:AddMessage("Enter!");

	AI:RegisterWithAI(self.id, 0);

	BroadcastEvent( self,"Enter" );

	
	self.Entered = 1;

--		self.InitialAngle = new(self.boat:GetAngles());

	self.InitImpulseTime = _time;
	self:SetTimer(1);

	if (self.Who == nil) then
		self.Who = sender;
		self:GotoState( "Occupied" );
	end
			

	--if ( sender ) then
		--System.LogToConsole( "Player "..sender:GetName().." Enter ImpulseTrigger "..self:GetName() );
	--end
end

function ImpulseTrigger:Event_Leave( sender )
	if (self.Entered == 0) then
		do return end
	end
	self.Entered = 0;
	--Hud:AddMessage("Leave");
	--if (sender ~= nil) then
		--System.LogToConsole( "Player "..sender:GetName().." Leave ImpulseTrigger "..self:GetName() );
	--end
	BroadcastEvent( self,"Leave" );

end

function ImpulseTrigger:Event_Enable( sender )
	self:GotoState("Empty")
	BroadcastEvent( self,"Enable" );
end

function ImpulseTrigger:Event_Disable( sender )
	self:GotoState( "Inactive" );
	AI:RegisterWithAI(self.id, 0);
	BroadcastEvent( self,"Disable" );
end

function ImpulseTrigger:OnTimeF()
	
	local coeff;

	if(self.Who) then
		if(self.Properties.ImpulseFadeInTime >0) then
			coeff = (_time - self.InitImpulseTime)/self.Properties.ImpulseFadeInTime;
		else
			coeff = 1; -- assume a duration = 1 
		end

		if (coeff > 1) then
			coeff = 1;
		end
		
		self.Who:AddImpulseObj( self.imp, self.Properties.ImpulseStrength * coeff);

		if ((_time - self.InitImpulseTime >= self.TotalImpulseDuration) and (self.TotalImpulseDuration >=0)) then
			-- there's no way to detect end of collision, let's automatically force it after 2 seconds
			self:SetTimer(2000); 
			self.Who = nil;
		else
			self:SetTimer(1);
		end
	else
		self:Event_Leave( self,self.Who );
		if(self.Properties.bKillOnTrigger > 0)then
			Server:RemoveEntity(self.id);
		elseif(self.Properties.bTriggerOnce > 0)then
			self:GotoState("Inactive");
		else
			self:GotoState("Empty");
		end
		self:KillTimer();
	end		
end



-------------------------------------------------------------------------------
-- Inactive State -------------------------------------------------------------
-------------------------------------------------------------------------------
ImpulseTrigger.Inactive =
{
	OnBeginState = function( self )
		printf("ImpulseTrigger deactivating");
	end,
	OnEndState = function( self )
		printf("ImpulseTrigger activating");
	end,
}
-------------------------------------------------------------------------------
-- Empty State ----------------------------------------------------------------
-------------------------------------------------------------------------------
ImpulseTrigger.Empty =
{
	-------------------------------------------------------------------------------
	OnBeginState = function( self )
		self.Who = nil;
		--self.Entered = 0;
			
	end,


	-------------------------------------------------------------------------------
	OnEnterArea = function( self,entity )
		if (self.Properties.bOnlyPlayer ~= 0 and entity.type ~= "Player") 	then
			do return end
		end

		-- if Only for AI, then check
		if (self.Properties.bOnlyAI ~=0 and entity.ai == nil) then
			do return end
		end

		-- Ignore if not my player.
		if (self.Properties.bOnlyMyPlayer ~= 0 and entity ~= _localplayer) then
			do return end
		end

		if (self.Who==nil) then
			self.Who = entity;
			self:GotoState( "Occupied" );
		end
	end,

	OnTimer = ImpulseTrigger.OnTimeF,	

}

-------------------------------------------------------------------------------
-- Empty State ----------------------------------------------------------------
-------------------------------------------------------------------------------
ImpulseTrigger.Occupied =
{
	-------------------------------------------------------------------------------
	OnBeginState = function( self )
		self:Event_Enter(self.Who);
		--Hud:AddMessage("Enter area");
	end,

	-------------------------------------------------------------------------------
	OnTimer = ImpulseTrigger.OnTimeF,

	-------------------------------------------------------------------------------
	--OnLeaveArea = function( self,player,areaId )
		--self:Event_Leave(self.Who);
	--end,

	-------------------------------------------------------------------------------


}

ImpulseTrigger.FlowEvents =
{
	Inputs =
	{
		Disable = { ImpulseTrigger.Event_Disable, "bool" },
		Enable = { ImpulseTrigger.Event_Enable, "bool" },
		Enter = { ImpulseTrigger.Event_Enter, "bool" },
		Leave = { ImpulseTrigger.Event_Leave, "bool" },
	},
	Outputs =
	{
		Disable = "bool",
		Enable = "bool",
		Enter = "bool",
		Leave = "bool",
	},
}
