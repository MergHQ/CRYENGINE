----------------------------------------------------------------------------
--
-- Description :		Delayed proxymity trigger
--
-- Create by Alberto :	03 March 2002
--
----------------------------------------------------------------------------
ProximityKeyTrigger = {
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
		bOnlyAI = 0,
		bOnlySpecialAI = 0,
		bKillOnTrigger=0,
		bTriggerOnce=0,
		ScriptCommand="",
		PlaySequence="",		
		aianchorAIAction = "",
		TextInstruction= "",
		bActivateWithUseButton=0,
		bInVehicleOnly=0,		
		iNeededKey=0,
		bKeepKeycardAfterUse=0,
	},
	
	Editor={
		Model="Objects/Editor/T.cgf",
		ShowBounds = 1,
	},	

	bLocked=false,
}

function ProximityKeyTrigger:OnPropertyChange()
	self:OnReset();
end

function ProximityKeyTrigger:OnInit()
	self:Activate(0);
	self:SetUpdateType( eUT_Physics );
	self:TrackColliders(1);

	self.Who = nil;
	self.Entered = 0;
	self.bLocked = 0;
	self.bTriggered = 0;

	self:RegisterState("Inactive");
	self:RegisterState("Empty");
	self:RegisterState("Occupied");
	self:RegisterState("OccupiedUse");
	self:OnReset();
end

function ProximityKeyTrigger:OnShutDown()
end

function ProximityKeyTrigger:OnSave(stm)
	--WriteToStream(stm,self.Properties);	
	stm:WriteInt(self.bTriggered);
	stm:WriteInt(self.bLocked);
	if (self.Who) then 
		if (self.Who == _localplayer) then 
			stm:WriteInt(0);
		else
			stm:WriteInt(self.Who.id);
		end
	else
		stm:WriteInt(-1);
	end

end


function ProximityKeyTrigger:OnLoad(stm)
	--self.Properties=ReadFromStream(stm);
	--self:OnReset();	
	self.bTriggered=stm:ReadInt();
	self.bLocked = stm:ReadInt();

	-- this complication is there to support loading.saving
	self.WhoID = stm:ReadInt();
	if (self.WhoID<0) then 
		self.WhoID = nil;
	elseif (self.WhoID==0) then 
		self.WhoID = 0;
	end
end

function ProximityKeyTrigger:OnLoadRELEASE(stm)
	--self.Properties=ReadFromStream(stm);
	--self:OnReset();	
	self.bTriggered=stm:ReadInt();
	self.bLocked = stm:ReadInt();
end


function ProximityKeyTrigger:OnReset()
	self:KillTimer();
	self.bTriggered = 0;

	local Min = { x=-self.Properties.DimX/2, y=-self.Properties.DimY/2, z=-self.Properties.DimZ/2 };
	local Max = { x=self.Properties.DimX/2, y=self.Properties.DimY/2, z=self.Properties.DimZ/2 };
	self:SetBBox( Min, Max );
	--self:Log( "BBox:"..Min.x..","..Min.y..","..Min.z.."  "..Max.x..","..Max.y..","..Max.z );
	self.Who = nil;
	self.UpdateCounter = 0;
	self.Entered = 0;
	if(self.Properties.bEnabled==1)then
		self:GotoState( "Empty" );
	else
		self:GotoState( "Inactive" );
	end

	if (self.Properties.iNeededKey~=-1) then
		--System.LogToConsole("Need the key!");
		self.bLocked=1;
	else
		self.bLocked=0;
	end

end

function ProximityKeyTrigger:Event_Enter( sender )

	-- to make it not trigger when event sent to inactive tringger
	if (self:GetState( ) == "Inactive") then return end

	if ((self.Entered ~= 0)) then
		return
	end
	if (self.Properties.bTriggerOnce == 1 and self.bTriggered == 1) then
		return
	end
	self.bTriggered = 1;
	self.Entered = 1;

	-- Trigger script command on enter.
	if(self.Properties.ScriptCommand and self.Properties.ScriptCommand~="")then
		--self:Log( "Executing: "..self.Properties.ScriptCommand);
		dostring(self.Properties.ScriptCommand);
	end
	if(self.Properties.PlaySequence~="")then
		Movie.PlaySequence( self.Properties.PlaySequence );
	end

	BroadcastEvent( self,"Enter" );
	AI:RegisterWithAI(self.id, 0);

	--self:Log( "Player "..sender:GetName().." Enter ProximityKeyTrigger "..self:GetName() );
end


function ProximityKeyTrigger:Event_Leave( sender )
	if (self.Entered == 0) then
		return
	end
	self.Entered = 0;
	BroadcastEvent( self,"Leave" );

	--self:Log( "Player "..sender:GetName().." Leave ProximityKeyTrigger "..self:GetName() );
	
	if(self.Properties.bTriggerOnce==1)then
		self:GotoState("Inactive");
	end
end

function ProximityKeyTrigger:Event_Enable( sender )
	self:GotoState("Empty")
	BroadcastEvent( self,"Enable" );
end

function ProximityKeyTrigger:Event_Disable( sender )
	self:GotoState( "Inactive" );
	AI:RegisterWithAI(self.id, 0);
	BroadcastEvent( self,"Disable" );
end

----------------------------------------------------------
function ProximityKeyTrigger:Event_Unlocked(sender)
	self.bLocked=0;
	BroadcastEvent(self, "Unlocked");	
	--self:PlaySound(self.UnlockSound);
end

--function ProximityKeyTrigger:Log( msg )
--	System.LogToConsole( msg );
--end

-- Check if source entity is valid for triggering.
function ProximityKeyTrigger:IsValidSource( entity )
	if (self.Properties.bOnlyPlayer ~= 0 and entity.type ~= "Player") then
		return 0;
	end

	if (self.Properties.bOnlySpecialAI ~= 0 and entity.ai ~= nil and entity.Properties.special==0) then 
		return 0;
	end

	-- if Only for AI, then check
	if (self.Properties.bOnlyAI ~=0 and entity.ai == nil) then
		return 0;
	end

		-- Ignore if not my player.
	if (self.Properties.bOnlyMyPlayer ~= 0 and entity ~= _localplayer) then
		return 0;
	end

	-- if only in vehicle - check if collider is in vehicle
	if (self.Properties.bInVehicleOnly ~= 0 and not entity.theVehicle) then
		return 0;
	end

	if (entity:IsDead()) then 
		return 0;
	end


	return 1;
end


-------------------------------------------------------------------------------
-- Inactive State -------------------------------------------------------------
-------------------------------------------------------------------------------
ProximityKeyTrigger.Inactive =
{
	OnBeginState = function( self )
		AI:RegisterWithAI(self.id, 0);
	end,
	OnEndState = function( self )
	end,
}
-------------------------------------------------------------------------------
-- Empty State ----------------------------------------------------------------
-------------------------------------------------------------------------------
ProximityKeyTrigger.Empty =
{
	-------------------------------------------------------------------------------
	OnBeginState = function( self )
		self.Who = nil;
		self.UpdateCounter = 0;
		self.Entered = 0;
		if (self.Properties.aianchorAIAction~="") then
			AI:RegisterWithAI(self.id, AIAnchor[self.Properties.aianchorAIAction]);
		end
	end,

	OnTimer = function( self )
		self:GotoState( "Occupied" );
	end,

	-------------------------------------------------------------------------------
	OnEnterArea = function( self,entity,areaId )


		if (self:IsValidSource(entity) == 0) then
			return
		end
				
		if (entity.ai==nil) then
			if (self.Properties.bActivateWithUseButton~=0) then
				self.Who = entity;
				self:GotoState( "OccupiedUse" );
				do return end;
			end
		end
		
		if (self.Properties.EnterDelay > 0) then
			if (self.Who == nil) then
				-- Not yet triggered.
				self.Who = entity;
				self:SetTimer( self.Properties.EnterDelay*1000 );
			end
		else
			self.Who = entity;
			self:GotoState( "Occupied" );
		end
	end,


}

-------------------------------------------------------------------------------
-- Occupied State ----------------------------------------------------------------
-------------------------------------------------------------------------------
ProximityKeyTrigger.Occupied =
{
	-------------------------------------------------------------------------------
	OnBeginState = function( self )
		self:Event_Enter(self.Who);
--		self:Do_Enter(self.Who);

		--self:Log("Enter");

		if(self.Properties.bKillOnTrigger==1)then
			Server:RemoveEntity(self.id);
		end
	end,

	-------------------------------------------------------------------------------
	OnTimer = function( self )
		--self:Log("Sending on leave");
		self:Event_Leave( self,self.Who );
		if(self.Properties.bTriggerOnce~=1)then
			self:GotoState("Empty");
		end
	end,

	-------------------------------------------------------------------------------
	OnLeaveArea = function( self,entity,areaId )
		-- Ignore if disabled.
		--add a very small delay(so is immediate)
		if (self:IsValidSource(entity) == 0) then
			return
		end
		
		if(self.Properties.ExitDelay==0) then
			self.Properties.ExitDelay=0.01;
		end
		self:SetTimer(self.Properties.ExitDelay*1000);
	end,
}

-------------------------------------------------------------------------------
-- OccupiedText State ---------------------------------------------------------
-------------------------------------------------------------------------------
ProximityKeyTrigger.OccupiedUse =
{
	-------------------------------------------------------------------------------
	OnBeginState = function( self )
		self:Activate(1);
	end,
	-------------------------------------------------------------------------------
	OnEndState = function( self )
		self:Activate(0);
	end,
	-------------------------------------------------------------------------------
	OnUpdate = function( self )

		if (self.WhoID) then 
			if (self.WhoID == 0) then 
				self.Who = _localplayer;
			else
				self.Who = System.GetEntity(self.WhoID);
			end
			self.WhoID = nil;
		end

		if (self.Who.cnt) then			
			if (not self.Who.cnt.use_pressed) then			
				if (strlen(self.Properties.TextInstruction)>0) then
					Hud.label = self.Properties.TextInstruction;
				end
				do return end;
			end
		end

		-- if the door is locked we need to check for the key first
		if (self.bLocked~=0) then

			if (self.Who.keycards and (self.Who.keycards[self.Properties.iNeededKey]==1)) then
				if (self.Properties.bKeepKeycardAfterUse==0) then
					self.Who.keycards[self.Properties.iNeededKey]=0;
				end
				self:Event_Unlocked(self);				

				System.LogToConsole("Trigger unlocked !");
			else
				--System.LogToConsole("Key not available !");												

				return 0
			end

		end

		if (self.Properties.EnterDelay > 0) then
			self:SetTimer( self.Properties.EnterDelay*1000 );
		else
			self:GotoState( "Occupied" );
		end
	end,
	
	-------------------------------------------------------------------------------
	OnTimer = function( self )
		self:GotoState( "Occupied" );
	end,

	-------------------------------------------------------------------------------
	OnLeaveArea = function( self,entity,areaId )
		if (self.Who == entity) then
			self:GotoState( "Empty" );
		end
	end,
}

ProximityKeyTrigger.FlowEvents =
{
	Inputs =
	{
		Disable = { ProximityKeyTrigger.Event_Disable, "bool" },
		Enable = { ProximityKeyTrigger.Event_Enable, "bool" },
		Enter = { ProximityKeyTrigger.Event_Enter, "bool" },
		Leave = { ProximityKeyTrigger.Event_Leave, "bool" },
		Leave = { ProximityKeyTrigger.Event_Unlocked, "bool" },
	},
	Outputs =
	{
		Disable = "bool",
		Enable = "bool",
		Enter = "bool",
		Leave = "bool",
		Leave = "bool",
	},
}
