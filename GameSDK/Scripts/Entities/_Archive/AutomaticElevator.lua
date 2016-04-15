AutomaticElevator = {
	Properties = {
			bActive=1,
  		MapVisMask = 0,
  		Direction = {
			X=0,
  		Y=0,
			Z=1,
  		},
  		OpenDelay=1.5,
  		CloseDelay=1.5,
  		RetriggerDelay=1,
  		MovingDistance = 2,
  		MovingSpeed = 0.5,
  		bAutomatic = 1,
  		fileModel = "Objects/indoor/lift/lift.cgf",
  	fileStartSound = "Sounds/lifts/liftst.wav",
  	fileLoopSound = "Sounds/lifts/liftlp.wav",
  	fileStopSound = "Sounds/lifts/liftend.wav",
  	LoopSoundStart = 1.1,
  		WarnLight=
	  	{
	  		bHasWarnLight = 1,
		  	fLightRadius=10,
				clrLightDiffuse={1, 0, 0},
				clrLightSpecular={1, 1, 1},
				LightAngles = {
		  		x=0,
		  		y=0,
		  		z=0,
		  	},
		  	fLightRotSpeed = 50,

			bProjectInAllDirs=0,
			ProjectorFov=90,
			texture_ProjectorTexture="Textures/projector.jpg",
			shader_lightShader="",
			bAffectsThisAreaOnly=1,
			bUsedInRealTime=1,
			bFakeLight=0,
			bHeatSource=0,
	  	},

		sMaterialDefault="m06_ben.Lift.reseach_lift_Default",
		sMaterialUp="m06_ben.Lift.reseach_lift_UP",
		sMaterialDown="m06_ben.Lift.reseach_lift_DOWN",
  	},
  base=nil,
  temp_vec={x=0,y=0,z=0},
  base_pos={x=0,y=0,z=0},
  Distance = 0.0,
  bOpeningDelay=nil,
  bTriggerOpenRequest=nil,
  bActive=1,

  MoveDir=1,
}

function AutomaticElevator:OnSave(stm)
	WriteToStream(stm,self.base_pos);
	stm:WriteFloat(self.Distance);
	stm:WriteInt(self.bActive);
	if (self.InUpPos) then 
		stm:WriteInt(self.InUpPos);
	else
		stm:WriteInt(0);
	end
end

function AutomaticElevator:OnLoad(stm)	
	self:OnPropertyChange();
	self.base_pos=ReadFromStream(stm);	
	self:SetPos(self.base_pos);
	self.Distance=stm:ReadFloat();
	self.bActive=stm:ReadInt();
	self.InUpPos = stm:ReadInt();

	if (self.InUpPos==0) then
		self.InUpPos=nil;
	else
		self.temp_vec.x = self.Properties.Direction.X * self.Properties.MovingDistance * self.MoveDir + self.base_pos.x;
		self.temp_vec.y = self.Properties.Direction.Y * self.Properties.MovingDistance * self.MoveDir + self.base_pos.y;
		self.temp_vec.z = self.Properties.Direction.Z * self.Properties.MovingDistance * self.MoveDir + self.base_pos.z;
		self:SetPos(self.temp_vec);
	end

end

function AutomaticElevator:OnLoadRELEASE(stm)	
	self:OnPropertyChange();
	self.base_pos=ReadFromStream(stm);	
	self:SetPos(self.base_pos);
	self.Distance=stm:ReadFloat();
	self.bActive=stm:ReadInt();

end


function AutomaticElevator:OnPropertyChange()
	--System.Log("ELEVATOR["..self:GetName()); --.."] x="..self.base.x..",y="..self.base.y..",z="..self.base.z)
	self:OnReset();
	if ( self.Properties.fileStartSound ~= self.CurrStartSound ) then
		self.CurrStartSound=self.Properties.fileStartSound;
		self.StartSound=Sound.Load3DSound(self.CurrStartSound);
		Sound.SetSoundVolume(self.StartSound, 255);
		Sound.SetMinMaxDistance(self.StartSound, 10, 30);
	end
	if ( self.Properties.fileLoopSound ~= self.CurrLoopSound ) then
		self.CurrLoopSound=self.Properties.fileLoopSound;
		self.LoopSound=Sound.Load3DSound(self.CurrLoopSound);
		Sound.SetSoundLoop(self.LoopSound, 1);
		Sound.SetSoundVolume(self.LoopSound, 255);
		Sound.SetMinMaxDistance(self.LoopSound, 10, 30);
	end
	if ( self.Properties.fileStopSound ~= self.CurrStopSound ) then
		self.CurrStopSound=self.Properties.fileStopSound;
		self.StopSound=Sound.Load3DSound(self.CurrStopSound);
		Sound.SetSoundVolume(self.StopSound, 255);
		Sound.SetMinMaxDistance(self.StopSound, 10, 30);
	end
end

function AutomaticElevator:IsMovingClient(dt)

	--System.Log("Update");

	--System.Log("ELEVATOR["..self:GetName()..", IS MOVING CLIENT");
	
	if (self.Properties.WarnLight.bHasWarnLight>0) then

		--System.Log("Light");

		local Pos=self:GetHelperPos("warnlight");
		--local Pos=self:GetPos();
		if (Pos) then
			if (dt>5) then
				dt=0;
			end
			self.Properties.WarnLight.LightAngles.z=self.Properties.WarnLight.LightAngles.z+self.Properties.WarnLight.fLightRotSpeed*dt;
			if (self.Properties.WarnLight.LightAngles.z>=360) then
				self.Properties.WarnLight.LightAngles.z=self.Properties.WarnLight.LightAngles.z-360;
			end
			if (self.Properties.WarnLight.LightAngles.z<0) then
				self.Properties.WarnLight.LightAngles.z=self.Properties.WarnLight.LightAngles.z+360;
			end
			--System.LogToConsole("LightPos("..Pos.x..", "..Pos.y..", "..Pos.z..")");
			--System.LogToConsole("LightAng("..self.Properties.WarnLight.LightAngles.x..", "..self.Properties.WarnLight.LightAngles.y..", "..self.Properties.WarnLight.LightAngles.z..")");

			self:AddDynamicLight(	Pos,
						self.Properties.WarnLight.fLightRadius,
						self.Properties.WarnLight.clrLightDiffuse[1],
						self.Properties.WarnLight.clrLightDiffuse[2],
						self.Properties.WarnLight.clrLightDiffuse[3],
						1,
						self.Properties.WarnLight.clrLightSpecular[1],
						self.Properties.WarnLight.clrLightSpecular[2],
						self.Properties.WarnLight.clrLightSpecular[3],
						1,
						0,
						0, -- not used
						self.Properties.WarnLight.LightAngles,
						self.Properties.WarnLight.ProjectorFov,
						self.proj_tex_id,
						self.Properties.WarnLight.bAffectsThisAreaOnly,
						self.Properties.WarnLight.bUsedInRealTime,
						self.Properties.WarnLight.bHeatSource, 
						self.Properties.WarnLight.bFakeLight );

		else
			--System.Log("AutomaticElevator: warnlight helper is missing");
		end
	end
end

function AutomaticElevator:OnReset()


	--System.Log("ELEVATOR["..self:GetName()..", ON RESET");

	if(self.Properties.OpenDelay<=0)then
		self.Properties.OpenDelay=0.001;
	end
	if(self.Properties.RetriggerDelay<=0)then
		self.Properties.RetriggerDelay=0.001;
	end
	if (self.Properties.MovingDistance<0) then
		self.Properties.MovingDistance=0;
	end

	self.bActive=self.Properties.bActive;
	self:LoadObject( self.Properties.fileModel, 0, 1 );
	self:DrawObject( 0, 1 );
	self:CreateRigidBody( 0, 0, -1 );
	self:AwakePhysics(0);
	self:SetUpdateType( eUT_Always );
--	self.CurrOpenSound=self.Properties.fileOpenSound;
--	self.OpenSound=Sound.Load3DSound(self.CurrOpenSound);
--	self.CurrCloseSound=self.Properties.fileCloseSound;
--	self.CloseSound=Sound.Load3DSound(self.CurrCloseSound);
--	System.Log("$4SETTING DURINGLOADING 1 !!!");
	
	self.DuringLoading=1;
	self:RegisterState("Idle");
	self:RegisterState("Opening");
	self:RegisterState("Closing");
	self:GotoState("Idle");
	self.DuringLoading=nil;

	if (self.Properties.MovingSpeed<0) then
		self.MoveDir=-1;
	else
		self.MoveDir=1;
	end
	self:SetVelocity(0);
	self.Distance=0;

	self.proj_tex_id = System.LoadTexture(self.Properties.WarnLight.texture_ProjectorTexture,1);
	--System.LogToConsole("ProjId="..self.proj_tex_id);
	
	self:InitDynamicLight(self.Properties.WarnLight.texture_ProjectorTexture, self.Properties.WarnLight.shader_lightShader, 
		self.Properties.WarnLight.bProjectInAllDirs,0);

	-- increase the bbox to match the elevator travel distance
	--local bbox=self:GetBBox();
	--local min=bbox.min;local max=bbox.max;
	--max.x=max.x+self.Properties.Direction.X * self.Distance * self.MoveDir;
	--max.y=max.y+self.Properties.Direction.Y * self.Distance * self.MoveDir;
	--max.z=max.z+self.Properties.Direction.Z * self.Distance * self.MoveDir;
	--self:SetBBox(min,max);

--	System.Log("$4SETTING DURINGLOADING NIL !!!");
end

function AutomaticElevator:SetVelocity(Scale)
	self.Velocity={};
	self.Velocity.v={};

	self.Velocity.v.x = self.Properties.Direction.X*self.Properties.MovingSpeed*Scale;
	self.Velocity.v.y = self.Properties.Direction.Y*self.Properties.MovingSpeed*Scale;
	self.Velocity.v.z = self.Properties.Direction.Z*self.Properties.MovingSpeed*Scale;
	if (Scale==0) then
		self:SetUpdateType( eUT_Always );	
	else
		self:AwakePhysics(1);
		self:SetUpdateType( eUT_PhysicsPostStep );
	end

end

function AutomaticElevator:OnInit()
	self:OnReset();
	--System.Log("ELEVATOR["..self:GetName()..", ON INIT");
	self:OnPropertyChange();
	self:NetPresent(nil);
	self:Activate(0);
	self:TrackColliders(1);
	self.base_pos=new(self:GetPos());
end

function AutomaticElevator:Event_Open(sender)
		if (self.bActive==0) then return end
		
		--System.Log("ELEVATOR["..self:GetName()..", EVENT OPEN");
		
		self:GotoState( "Opening" );
		BroadcastEvent(self, "Open");	
end

function AutomaticElevator:Event_Close(sender)
	if (self.bActive==0) then return end
	
	--System.Log("ELEVATOR["..self:GetName()..", EVENT CLOSE; SENDER="..sender:GetName());
	
	self:GotoState( "Closing" );
	BroadcastEvent(self, "Close");
end

function AutomaticElevator:Event_Opened(sender)
	if (self.bActive==0) then return end
	
	--System.Log("ELEVATOR["..self:GetName()..", EVENT OPENED");
	
	BroadcastEvent(self, "Opened");
end

function AutomaticElevator:Event_Closed(sender)
	if (self.bActive==0) then return end
	
	--System.Log("ELEVATOR["..self:GetName()..", EVENT CLOSED");
	
	BroadcastEvent(self, "Closed");
end

function AutomaticElevator:Event_Activate(sender)
	self.bActive=1;
	--System.Log("ELEVATOR["..self:GetName()..", EVENT ACTIVATE");
end

function AutomaticElevator:Event_Deactivate(sender)
	self.bActive=0;
	--System.Log("ELEVATOR["..self:GetName()..", EVENT DEACTIVATE");
end

------------------------------------------------------------------------------------------------------
function AutomaticElevator:Event_ResetAnimation()
	if (self.bActive==0) then return end
	--System.Log("ELEVATOR["..self:GetName()..", EVENT RESET ANIMATION");
	self:ResetAnimation(0, -1);
end

------------------------------------------------------------------------------------------------------
function AutomaticElevator:Event_StartAnimation(sender)
	if (self.bActive==0) then return end
	--System.Log("ELEVATOR["..self:GetName()..", EVENT START ANIMATION");
	self:ResetAnimation(0, -1);
	self:StartAnimation( 0,"default",0,0,1,0 );	
	self:SetAnimationSpeed( 1 );
end

------------------------------------------------------------------------------------------
------------------------------------------------------------------------------------------
-- SERVER
------------------------------------------------------------------------------------------
------------------------------------------------------------------------------------------
AutomaticElevator["Server"]={
	OnInit = function(self) self.server=1; self:OnInit() end,
	------------------------------------------------------------------------------------------
	--IDLE
	------------------------------------------------------------------------------------------
	Idle = {
		OnBeginState = function(self)
--			System.Log("ELEVATOR["..self:GetName()..", ON IDLE");
			--System.Log("Idle");
			self:Activate(0);
			self:SetVelocity(0);
			if (not self.InUpPos) then
				self.bOpeningDelay=nil;
				--self.base=new(self:GetPos());
				self.bTriggerOpenRequest=nil;
			end

			self:SetMaterial(self.Properties.sMaterialDefault);
		end,
		OnEndState = function(self)
			self:Activate(1);
		end,
		OnContact = function(self,other)
--			if (not self.InUpPos) then
			if(self.bActive==0 or (self.Properties.bAutomatic==0))then return end
			
			--System.Log("ELEVATOR["..self:GetName()..", ON CONTACT");

			if ((not self.InUpPos) and (not self.WaitForDownDelay)) then
				--System.Log("ELEVATOR["..self:GetName()..", COND 1 (IDLE)");
				if(self.bOpeningDelay==nil) then
					--System.Log("ELEVATOR["..self:GetName()..", SET TIMER (IDLE)");
					self:SetTimer(self.Properties.OpenDelay*1000);
					self.bOpeningDelay=1;
				end
			end
		end,
		OnTimer = function(self)
			if (self.bActive==0) then return end
			
			--System.Log("ELEVATOR["..self:GetName()..", ON TIMER (IDLE)");
			
			if (not self.InUpPos) then
				if (not self.WaitForDownDelay) then
--					System.LogToConsole("!InUpPos");
					self:GotoState("Opening");
				end
--			elseif (self.WaitForDownDelay) then
--				System.LogToConsole("WaitForDownDelay");
--				self.WaitForDownDelay=nil;
			else
--				System.LogToConsole("InUpPos");
				System.LogToConsole("Opened State Timer Exprired");
				self:GotoState( "Closing" );
				self:Event_Close(self);
			end
			self.InUpPos=nil;
			self.WaitForDownDelay=nil;
		end
	},
	------------------------------------------------------------------------------------------
	--OPENING
	------------------------------------------------------------------------------------------
	Opening = {
		-- Called when Opened State is Set.
	----------------------------------------------------------
		OnBeginState = function(self)			
			--System.LogToConsole("SERVER:Opened");
--			System.Log("ELEVATOR["..self:GetName()..", ON BEGIN STATE (OPENING)");
			self.EventSent = nil;
			self:SetVelocity(1);
			self:SetMaterial(self.Properties.sMaterialUp);
		end,
	----------------------------------------------------------
		OnEndState = function(self)
		end,
	----------------------------------------------------------
--		OnTimer = function(self)
--			System.LogToConsole("Opened State Timer Exprired");
--			self.GotoState( "Closing" );
--			self:Event_Close(self);
--		end,
	----------------------------------------------------------
		OnUpdate = function(self, dt)			
			self.Distance = self.Distance + abs (dt * self.Properties.MovingSpeed);

--			System.Log("ELEVATOR["..self:GetName()..", ON update (OPENING)");
			
			if ( self.Distance > self.Properties.MovingDistance ) then

				--System.Log("ELEVATOR["..self:GetName()..", MOVING DISTANCE (OPENING)");
				
				self.Distance = self.Properties.MovingDistance;

				if ( not self.EventSent ) then
					self.Event_Opened(self);
					self.InUpPos=1;
					self:GotoState( "Idle" );
					--if closedelay if <= 0 the elevator will never close by himself
					if(self.Properties.bAutomatic~=0)then
						if(self.Properties.CloseDelay>0)then
							self:SetTimer(self.Properties.CloseDelay*1000);
						end
					end
				end
				self.EventSent = 1;
			end

			self.temp_vec.x = self.Properties.Direction.X * self.Distance * self.MoveDir + self.base_pos.x;
			self.temp_vec.y = self.Properties.Direction.Y * self.Distance * self.MoveDir + self.base_pos.y;
			self.temp_vec.z = self.Properties.Direction.Z * self.Distance * self.MoveDir + self.base_pos.z;
--			System.Log("vel="..self.Velocity.v.x..","..self.Velocity.v.y..","..self.Velocity.v.z);
			self:SetPos(self.temp_vec);
			self:SetPhysicParams(PHYSICPARAM_VELOCITY, self.Velocity);
			--self:SetObjectPos(0,self.temp_vec);
		end,
	----------------------------------------------------------
	},
	------------------------------------------------------------------------------------------
	--CLOSING
	------------------------------------------------------------------------------------------
	Closing = {
		-- Called when Closed State is Set.
		----------------------------------------------------------
		OnBeginState = function(self)
			--System.LogToConsole("SERVER:Closed");
		--	System.Log("ELEVATOR["..self:GetName()..", ON BEGIN STATE (CLOSING)");
			self.EventSent = nil;
			self:SetVelocity(-1);
			self:SetMaterial(self.Properties.sMaterialDown);
		end,
		----------------------------------------------------------
		OnEndState = function(self)
		end,
		----------------------------------------------------------
--		OnTimer = function(self)
--			self.GotoState("Idle");
--		end,
		----------------------------------------------------------
		OnUpdate = function(self, dt)
			self.Distance = self.Distance - abs (dt * self.Properties.MovingSpeed);
			--System.Log("ELEVATOR["..self:GetName()..", ON UPDATE (CLOSING)");
			if ( self.Distance < 0 ) then
				self.Distance = 0;
				if ( not self.EventSent ) then
					System.Log("Server-EVENT-Closed");
					--System.Log("ELEVATOR["..self:GetName()..", ON UPDATE (closing) loop1");
					self:Event_Closed();
					self.WaitForDownDelay=1;
					self:GotoState( "Idle" );
					self.InUpPos=nil;
					if(self.Properties.bAutomatic~=0)then
						--System.Log("Server-Closed");						
						self:SetTimer(self.Properties.RetriggerDelay*1000);
					end
				end
				self.EventSent = 1;
			end
			--local CurrPos = {};
			self.temp_vec.x = self.Properties.Direction.X * self.Distance * self.MoveDir + self.base_pos.x;
			self.temp_vec.y = self.Properties.Direction.Y * self.Distance * self.MoveDir + self.base_pos.y;
			self.temp_vec.z = self.Properties.Direction.Z * self.Distance * self.MoveDir + self.base_pos.z;
			self:SetPos(self.temp_vec);
			self:SetPhysicParams(PHYSICPARAM_VELOCITY, self.Velocity);
			--self:SetObjectPos(0,self.temp_vec);
		end,
		----------------------------------------------------------
	}
}

------------------------------------------------------------------------------------------
------------------------------------------------------------------------------------------
-- CLIENT
------------------------------------------------------------------------------------------
------------------------------------------------------------------------------------------
AutomaticElevator["Client"]={
	OnInit = AutomaticElevator.OnInit,
	Idle = {
		OnBeginState = function(self)
			self:SetVelocity(0);
			--if (self.Properties.WarnLight.bHasWarnLight>0) then
			--	local Pos=self:GetHelperPos("warnlight");
			--	if (Pos) then
			--		System.ActivateMainLight(Pos, nil);
			--	end
			--end

			if (not self.DuringLoading) then
--				System.Log("$4DURINGLOADING !!!");
				Sound.SetSoundPosition(self.StopSound, self:GetPos());
				Sound.PlaySound(self.StopSound);
				Sound.StopSound(self.LoopSound);
				Sound.StopSound(self.StartSound);
--			else
--				System.Log("$4NOT DURINGLOADING !!!");
			end
			self.LoopStarted=nil;
		end
	},
	------------------------------------------------------------------------------------------
	--OPENED
	------------------------------------------------------------------------------------------
	Opening = {
		----------------------------------------------------------
		-- Called when Opened State is Set.
		OnBeginState = function(self)
			self:SetVelocity(1);
			self:StartAnimation(0,"open");
			System.LogToConsole("CLIENT:Open");
--			Sound.SetSoundPosition(self.OpenSound, self.Base);
--			Sound.PlaySound(self.OpenSound);
			--if (self.Properties.WarnLight.bHasWarnLight>0) then
			--	local Pos=self:GetHelperPos("warnlight");
			--	if (Pos) then
			--		System.ActivateMainLight(Pos, 1);
			--	end
			--end
			Sound.SetSoundPosition(self.StartSound, self:GetPos());
			Sound.PlaySound(self.StartSound);
			self.StartMoveTime=_time;
		end,
		OnUpdate = function(self, dt)
			if(not self.server)then
				self.Distance = self.Distance + abs (dt * self.Properties.MovingSpeed);
				if ( self.Distance > self.Properties.MovingDistance ) then
					self.Distance = self.Properties.MovingDistance;
				end
				
				--local CurrPos = {};
				self.temp_vec.x = self.Properties.Direction.X * self.Distance * self.MoveDir + self.base_pos.x;
				self.temp_vec.y = self.Properties.Direction.Y * self.Distance * self.MoveDir + self.base_pos.y;
				self.temp_vec.z = self.Properties.Direction.Z * self.Distance * self.MoveDir + self.base_pos.z;
				self:SetPos(self.temp_vec);
				self:SetPhysicParams(PHYSICPARAM_VELOCITY, self.Velocity);
				--self:SetObjectPos(0,self.temp_vec);
			end
			if ( ( _time - self.StartMoveTime ) >= self.Properties.LoopSoundStart ) then
				if (not self.LoopStarted) then
					Sound.PlaySound(self.LoopSound);
					self.LoopStarted=1;
				end
			end
			Sound.SetSoundPosition(self.StartSound, self:GetPos());
			Sound.SetSoundPosition(self.LoopSound, self:GetPos());
			self:IsMovingClient(dt);
		end,
	},
	------------------------------------------------------------------------------------------
	--CLOSED
	------------------------------------------------------------------------------------------
	Closing = {
		-- Called when Closed State is Set.
		----------------------------------------------------------
		OnBeginState = function(self)
			self:SetVelocity(-1);
			self:StartAnimation(0,"close");
			System.LogToConsole("CLIENT:Close");
--			Sound.SetSoundPosition(self.CloseSound, self.GetPos());
--			Sound.PlaySound(self.CloseSound);
--			if (self.Properties.WarnLight.bHasWarnLight>0) then
--				local Pos=self:GetHelperPos("warnlight");
--				if (Pos) then
--					System.ActivateMainLight(Pos, 1);
--				end
--			end
			Sound.SetSoundPosition(self.StartSound, self:GetPos());
			Sound.PlaySound(self.StartSound);
			self.StartMoveTime=_time;
		end,
		OnUpdate = function(self, dt)
			if(not self.server)then
				self.Distance = self.Distance - abs (dt * self.Properties.MovingSpeed);
				if ( self.Distance < 0 ) then
					self.Distance = 0;
				end
				--local CurrPos = {};
				self.temp_vec.x = self.Properties.Direction.X * self.Distance * self.MoveDir + self.base_pos.x;
				self.temp_vec.y = self.Properties.Direction.Y * self.Distance * self.MoveDir + self.base_pos.y;
				self.temp_vec.z = self.Properties.Direction.Z * self.Distance * self.MoveDir + self.base_pos.z;
				self:SetPos(self.temp_vec);
				self:SetPhysicParams(PHYSICPARAM_VELOCITY, self.Velocity);
				--self:SetObjectPos(0,self.temp_vec);
			end
			if ( ( _time - self.StartMoveTime ) >= self.Properties.LoopSoundStart ) then
				if (not self.LoopStarted) then
					Sound.PlaySound(self.LoopSound);
					self.LoopStarted=1;
				end
			end
			Sound.SetSoundPosition(self.StartSound, self:GetPos());
			Sound.SetSoundPosition(self.LoopSound, self:GetPos());
			self.IsMovingClient(self, dt);
		end,
	}
}
------------------------------------------------------------------------------------------
------------------------------------------------------------------------------------------

AutomaticElevator.FlowEvents =
{
	Inputs =
	{
		Activate = { AutomaticElevator.Event_Activate, "bool" },
		Close = { AutomaticElevator.Event_Close, "bool" },
		Closed = { AutomaticElevator.Event_Closed, "bool" },
		Deactivate = { AutomaticElevator.Event_Deactivate, "bool" },
		Open = { AutomaticElevator.Event_Open, "bool" },
		Opened = { AutomaticElevator.Event_Opened, "bool" },
		ResetAnimation = { AutomaticElevator.Event_ResetAnimation, "bool" },
		StartAnimation = { AutomaticElevator.Event_StartAnimation, "bool" },
	},
	Outputs =
	{
		Activate = "bool",
		Close = "bool",
		Closed = "bool",
		Deactivate = "bool",
		Open = "bool",
		Opened = "bool",
		ResetAnimation = "bool",
		StartAnimation = "bool",
	},
}
