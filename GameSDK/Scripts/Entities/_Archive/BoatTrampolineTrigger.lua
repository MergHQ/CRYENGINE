----------------------------------------------------------------------------
--
-- Description :		Boat Trampoline trigger
--
-- Created by Luciano :	19 april 2003
--
----------------------------------------------------------------------------
BoatTrampolineTrigger = {
	type = "Trigger",

	Properties = {
		DimX = 5,
		DimY = 5,
		DimZ = 5,
		bEnabled=1,
		--EnterDelay=0,
		--ExitDelay=0,
		bKillOnTrigger=0,
		bTriggerOnce=0,
		--aianchorAIAction = "",
		ImpulseStrength = 200,
		ImpulseFadeInTime = 1,
		ImpulseDuration = 1,
		MinSpeed = 15,
		MaxAngleOfImpact = 15,
		ScriptCommand="",
		PlaySequence="",
	},

	boat = nil,
	imp = {x=0,y=0,z=0},
	MaxImpulse = 0, 
	InitImpulseTime = 0,
	TotalImpulseDuration = 0,
	MinSpeed2 =0,
	Editor={
		Model="Objects/Editor/T.cgf",
	},
}

function BoatTrampolineTrigger:OnPropertyChange()
	self:OnReset();
end

function BoatTrampolineTrigger:OnInit()
	self.Who = nil;
	self.Entered = 0;

	self:RegisterState("Inactive");
	self:RegisterState("Empty");
	self:RegisterState("Occupied");
	self:OnReset();
	self:Activate(1);
end

function BoatTrampolineTrigger:OnShutDown()
end

function BoatTrampolineTrigger:OnSave(stm)
	--WriteToStream(stm,self.Properties);
end


function BoatTrampolineTrigger:OnLoad(stm)
	--self.Properties=ReadFromStream(stm);
	--self:OnReset();
end

function BoatTrampolineTrigger:OnReset()

	self.imp = new(self:GetDirectionVector());
 
	self:Activate(1);
	local Min = { x=-self.Properties.DimX/2, y=-self.Properties.DimY/2, z=-self.Properties.DimZ/2 };
	local Max = { x=self.Properties.DimX/2, y=self.Properties.DimY/2, z=self.Properties.DimZ/2 };
	self:SetBBox( Min, Max );
	--System.LogToConsole( "BBox:"..Min.x..","..Min.y..","..Min.z.."  "..Max.x..","..Max.y..","..Max.z );
	self.Who = nil;
	self.UpdateCounter = 0;
	self.Entered = 0;
	self.MinSpeed2 = self.Properties.MinSpeed * self.Properties.MinSpeed;
	self.TotalImpulseDuration = self.Properties.ImpulseFadeInTime + self.Properties.ImpulseDuration;
	
	if(self.Properties.bEnabled==1)then
		self:GotoState( "Empty" );
	else
		self:GotoState( "Inactive" );
	end

--	AI:RegisterWithAI(self.id, AIAnchor[self.Properties.aianchorAIAction]);
end

function BoatTrampolineTrigger:Event_Enter( sender )
--	self.imp = self:GetDirectionVector();
	System.LogToConsole("Something entered ");
	
	if ((self.Entered ~= 0)) then
		return
	end
--	self.Entered = 1;
	

	AI:RegisterWithAI(self.id, 0);

	if (sender and sender.type=="Vehicle" ) then
		-- check if the boat horizontal speed is enough..
		-- ..and more or less directed as the impulse direction
		local vel = new(sender:GetVelocity());
		local impul = new(self.imp);
		
		impul.z = 0;
		local spd2 = 0;
		local dot = 0;
		local angleImpact = 360;
		if(vel.x ~= 0 or vel.y ~= 0) then
			if (impul.x ~= 0 or impul.y ~= 0) then
				vel.z=0;	-- consider only horizontal components
				spd2 = vel.x*vel.x + vel.y*vel.y;
				NormalizeVector(vel);
				NormalizeVector(impul);
				angleImpact = (1 - dotproduct3d(vel,impul))*180;
			else
				-- vertical impulse, don't consider horizontal speed and make it work anyway
				angleImpact = 0;
				spd2 = vel.z*vel.z;
			end
				
				
		elseif(vel.z ~= 0) then
			if (impul.z ~= 0) then
				spd2 = vel.z*vel.z;
				vel.z = 1; -- equivalent to NormalizeVector(vel);
				NormalizeVector(impul);
				angleImpact = (1 - dotproduct3d(vel,impul))*180;
			else			
				-- horizontal impulse, don't consider vertical speed and make it work anyway
				angleImpact = 0;
				spd2 = vel.x*vel.x + vel.y*vel.y;
			end
		else			
			do return end -- vel = 0 ?
		end
		
		System.LogToConsole("Boat entered with speed2="..spd2.."Angle="..angleImpact);

		if(spd2 >= self.MinSpeed2 and angleImpact <= self.Properties.MaxAngleOfImpact) then
			
			-- Trigger script command on enter.
			if(self.Properties.ScriptCommand and self.Properties.ScriptCommand~="")then
				--System.LogToConsole("Executing: "..self.Properties.ScriptCommand);
				dostring(self.Properties.ScriptCommand);
			end
		
		
			if(self.Properties.PlaySequence~="")then
				System.LogToConsole("Playing sequence: ".. self.Properties.PlaySequence);
				Movie.PlaySequence( self.Properties.PlaySequence );
			end
	
			BroadcastEvent( self,"Enter" );
	
			self.boat = sender;
			
			if(self.Entered ==0) then
				-- Correct boat's direction by giving it a side impulse
				if (impul.x ~= 0 or impul.y ~= 0) then
					local f_Mass = self.boat.boat_params.fMass;
					local vCorrectingImpulse = new(vel);
					vCorrectingImpulse.x = impul.x - vCorrectingImpulse.x ;
					vCorrectingImpulse.y = impul.y - vCorrectingImpulse.y;
					vCorrectingImpulse.z = 0;
					if(vCorrectingImpulse.x ~= 0 or vCorrectingImpulse.y ~= 0) then 
						local fNorm = f_Mass * sqrt(vCorrectingImpulse.x * vCorrectingImpulse.x + vCorrectingImpulse.y * vCorrectingImpulse.y);
						NormalizeVector(vCorrectingImpulse);
						-- the 10 value below is just a tweaking, it hasn't a particular physical meaning
						self.boat:AddImpulseObj(vCorrectingImpulse, fNorm*10); 
				--		Hud:AddMessage("Impulse! x="..vCorrectingImpulse.x.." y="..vCorrectingImpulse.y.." amount="..fNorm);
					end
				end
				
					
			end
			
		

			
			self.Entered = 1;
	
			self.InitialAngle = new(self.boat:GetAngles());
	
			self.InitImpulseTime = _time;
			self:SetTimer(1);
	--		self.boat.boat_params.Flying = 1;
			self.MaxImpulse = self.boat.boat_params.fMass/800 * self.Properties.ImpulseStrength;
	
			if (self.Who == nil) then
				self.Who = player;
				self:GotoState( "Occupied" );
			end
			
		end		
	end		

	if ( sender ) then
		System.LogToConsole( "Player "..sender:GetName().." Enter BoatTrampolineTrigger "..self:GetName() );
	end
end

function BoatTrampolineTrigger:Event_Leave( sender )
	if (self.Entered == 0) then
		return
	end
	self.Entered = 0;
	if (sender ~= nil) then
		System.LogToConsole( "Player "..sender:GetName().." Leave BoatTrampolineTrigger "..self:GetName() );
	end
	BroadcastEvent( self,"Leave" );

end

function BoatTrampolineTrigger:Event_Enable( sender )
	self:GotoState("Empty")
	BroadcastEvent( self,"Enable" );
end

function BoatTrampolineTrigger:Event_Disable( sender )
	self:GotoState( "Inactive" );
	AI:RegisterWithAI(self.id, 0);
	BroadcastEvent( self,"Disable" );
end

function BoatTrampolineTrigger:OnTimeF()
	
	local coeff;
	local vec= self.InitialAngle;

	if(self.boat) then
		if(self.Properties.ImpulseFadeInTime >0) then
			coeff = (_time - self.InitImpulseTime)/self.Properties.ImpulseFadeInTime;
		else
			coeff = 1; -- assume a duration = 1 
		end

		if (coeff > 1) then
			coeff = 1;
		end
		
		
		System.LogToConsole("Applying impulse ="..self.MaxImpulse * coeff);
		
		self.boat:AddImpulseObj( self.imp, self.MaxImpulse * coeff);
		
		
		if ((_time - self.InitImpulseTime >= self.TotalImpulseDuration) and (self.TotalImpulseDuration >=0)) then
			self:KillTimer();
			self:Event_Leave( self,self.Who );
			self.boat = nil;
			if(self.Properties.bKillOnTrigger > 0)then
				Server:RemoveEntity(self.id);
			elseif(self.Properties.bTriggerOnce > 0)then
				self:GotoState("Inactive");
			else
				self:GotoState("Empty");
			end

		else
			self:SetTimer(1);
		end

	end		
end



-------------------------------------------------------------------------------
-- Inactive State -------------------------------------------------------------
-------------------------------------------------------------------------------
BoatTrampolineTrigger.Inactive =
{
	OnBeginState = function( self )
		printf("BoatTrampolineTrigger deactivating");
	end,
	OnEndState = function( self )
		printf("BoatTrampolineTrigger activating");
	end,
}
-------------------------------------------------------------------------------
-- Empty State ----------------------------------------------------------------
-------------------------------------------------------------------------------
BoatTrampolineTrigger.Empty =
{
	-------------------------------------------------------------------------------
	OnBeginState = function( self )
		self.Who = nil;
		self.UpdateCounter = 0;
		self.Entered = 0;
	end,


	-------------------------------------------------------------------------------
	OnContact = function( self,player )
		-- Ignore if disabled.
-- local vel=new(player;GetVelocity());

		-- if Only for AI, then check

		if (self.Who==nil) then

				self.Who = player;
				self:GotoState( "Occupied" );
		end
	end,

	-------------------------------------------------------------------------------
	OnEnterArea = function( self,player,areaId )
		if (self.Who == nil) then
			self.Who = player;
			self:GotoState( "Occupied" );
		end
	end,

	OnTimer = BoatTrampolineTrigger.OnTimeF,	


}

-------------------------------------------------------------------------------
-- Empty State ----------------------------------------------------------------
-------------------------------------------------------------------------------
BoatTrampolineTrigger.Occupied =
{
	-------------------------------------------------------------------------------
	OnBeginState = function( self )
		self:Event_Enter(self.Who);

		System.LogToConsole("Enter occupied");

	end,

	-------------------------------------------------------------------------------
	OnContact = function( self,player )
		-- Ignore if disabled.
	end,

	-------------------------------------------------------------------------------
	OnTimer = BoatTrampolineTrigger.OnTimeF,

	-------------------------------------------------------------------------------
--	OnLeaveArea = function( self,player,areaId )
--		-- Ignore if disabled.
--		--add a very small delay(so is immediate)
--		if(self.Properties.ExitDelay==0) then
--			self.Properties.ExitDelay=0.01;
--		end
--	end,

	-------------------------------------------------------------------------------


}



BoatTrampolineTrigger.FlowEvents =
{
	Inputs =
	{
		Disable = { BoatTrampolineTrigger.Event_Disable, "bool" },
		Enable = { BoatTrampolineTrigger.Event_Enable, "bool" },
		Enter = { BoatTrampolineTrigger.Event_Enter, "bool" },
		Leave = { BoatTrampolineTrigger.Event_Leave, "bool" },
	},
	Outputs =
	{
		Disable = "bool",
		Enable = "bool",
		Enter = "bool",
		Leave = "bool",
	},
}
