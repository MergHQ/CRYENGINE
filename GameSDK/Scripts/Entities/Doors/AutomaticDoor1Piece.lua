AutomaticDoor1Piece = {
	Properties = {
		soclasses_SmartObjectClass = "Door,AutomaticDoor",

  		MapVisMask = 0,
  		Direction = {
				X=0,
  			Y=0,
				Z=1,
  		},
  		CloseDelay=1.5,
  		BBOX_Size={
				X=3,
  			Y=3,
				Z=2,
  		},
  		MovingDistance = 2.40,
  		MovingSpeed = 4,
  		bPlayerBounce = 1,
  		bPlayerOnly = 0,
  		fileModel = "Objects/Indoor/doors/default_200w250h.cgf",
  		soundOpenSound = "Sounds/doors/open.wav",
  		soundCloseSound = "Sounds/doors/close.wav",
  		bAutomatic = 1,
  		bCloseTimer = 1,
			bEnabled = 1,
			nNeededKey=-1,
  	},
  CurrModel = "Objects/lift/lift.cgf",
  temp_vec={x=0,y=0,z=0},
  Distance = 0,
  EndReached=nil,
  OpeningTime=0,
  bLocked=false,
  bInitialized=nil
}

function AutomaticDoor1Piece:OnPropertyChange()
	self:OnReset();
	if ( self.Properties.fileOpenSound ~= self.CurrOpenSound ) then
		self.CurrOpenSound=self.Properties.fileOpenSound;
		--REINST 
		--self.OpenSound=Sound.Load3DSound(self.CurrOpenSound);
	end
	if ( self.Properties.fileCloseSound ~= self.CurrCloseSound ) then
		self.CurrCloseSound=self.Properties.fileCloseSound;
		--REINST
		--self.CloseSound=Sound.Load3DSound(self.CurrCloseSound);
	end
end

function AutomaticDoor1Piece:OnReset()
	self:LoadObject( self.Properties.fileModel, 0, 0 );
	--System.Log("AutomaticDoor1Piece LOADING"..self.Properties.fileModel);
	self:DrawObject( 0, 1 );
	self:CreateStaticEntity( 100, 0 );
	self.CurrOpenSound=self.Properties.fileOpenSound;
	--REINST
	--self.OpenSound=Sound.Load3DSound(self.CurrOpenSound);
	self.CurrCloseSound=self.Properties.fileCloseSound;
	--REINST
	--self.CloseSound=Sound.Load3DSound(self.CurrCloseSound);
	self:SetBBox({x=-(self.Properties.BBOX_Size.X*0.5),y=-(self.Properties.BBOX_Size.Y*0.5),z=-(self.Properties.BBOX_Size.Z*0.5)},
	{x=(self.Properties.BBOX_Size.X*0.5),y=(self.Properties.BBOX_Size.Y*0.5),z=(self.Properties.BBOX_Size.Z*0.5)});
	self:RegisterState("Opened");
	self:RegisterState("Closed");
	
	if (self.Properties.nNeededKey~=-1) then
		--System.LogToConsole("Door locked !");
		self.bLocked=1;
	else
		self.bLocked=0;
	end
	
	self:GotoState("Closed");
	if(self.MovingSpeed==0)then
		self.MovingSpeed=0.01;
	end
	--calculate how long the door to be opened
	self.OpeningTime=self.Properties.MovingDistance/self.Properties.MovingSpeed;
	self.Timer=0;
	--self:Activate(1);
	System.ActivatePortal(self:GetPos(),0,self.id);
end

function AutomaticDoor1Piece:OnInit()
	self:Activate(0);
	self:TrackColliders(1);
	
	if(self.bInitialized==nil)then
		self:OnReset();
		self:NetPresent(nil);
		self.bInitialized=1;
	end
end

function AutomaticDoor1Piece:Event_Open(sender)
	if (self.Properties.bEnabled == 0) then
		do return end;
	end
	self:GotoState( "Opened" );
	BroadcastEvent(self, "Open");
	System.ActivatePortal(self:GetPos(),1,self.id);
--	System.LogToConsole("Portal active");
end

function AutomaticDoor1Piece:Event_Close(sender)
	if (self.Properties.bEnabled == 0) then
		do return end;
	end
	self:GotoState( "Closed" );
	BroadcastEvent(self, "Close");
	--System.ActivatePortal(self:GetPos(),0,self.id);
	--System.LogToConsole("Portal not active");
end

function AutomaticDoor1Piece:Event_Opened(sender)
	if (self.Properties.bEnabled == 0) then
		do return end;
	end
	self:Activate(0);
	BroadcastEvent(self, "Opened");
	--System.ActivatePortal(self:GetPos(),1,self.id);
	--System.LogToConsole("Portal active");
end

function AutomaticDoor1Piece:Event_Closed(sender)
	if (self.Properties.bEnabled == 0) then
		do return end;
	end
	self:Activate(0);
	BroadcastEvent(self, "Closed");
	System.ActivatePortal(self:GetPos(),0,self.id);
--	System.LogToConsole("Portal not active");
end

function AutomaticDoor1Piece:Event_Enable(sender)
	self.Properties.bEnabled = 1;
end

function AutomaticDoor1Piece:Event_Disable(sender)
	self.Properties.bEnabled = 0;
end


------------------------------------------------------------------------------------------
------------------------------------------------------------------------------------------
-- SERVER
------------------------------------------------------------------------------------------
------------------------------------------------------------------------------------------
AutomaticDoor1Piece["Server"]={
	OnInit = AutomaticDoor1Piece.OnInit,
	------------------------------------------------------------------------------------------
	--OPENED
	------------------------------------------------------------------------------------------
	Opened = {
		-- Called when Opened State is Set.
	----------------------------------------------------------
		OnBeginState = function(self)
			--System.LogToConsole("SERVER:Opened");
			if(self.Properties.bCloseTimer==1)then
				self:SetTimer(self.Properties.CloseDelay*1000);
			end
			if(self.Timer~=0)then
				Game:KillTimer(self.Timer);
			end
			self.Timer=Game:SetTimer(self,self.OpeningTime);
			self.EventSent = nil;
		end,
	----------------------------------------------------------
		OnEndState = function(self)
			Game:KillTimer(self.Timer);
			self.Timer=0;
		end,
	----------------------------------------------------------
		OnContact = function(self,other)
			--System.LogToConsole("SERVER:OnContact");
			if(self.Properties.bCloseTimer==1)then
				if(self.Properties.bPlayerOnly==1 and (other.type~="Player"))then
					return
				end
				self:SetTimer(self.Properties.CloseDelay*1000);
			end
		end,
	----------------------------------------------------------
		OnTimer = function(self)
			--System.LogToConsole("Opened State Timer Exprired");
			-- close door.
			self:GotoState( "Closed" );
			self:Event_Close(self);
		end,
	----------------------------------------------------------
		OnEvent = function(self,eid)
			if(eid==ScriptEvent_OnTimer)then
				self.temp_vec.x = self.Properties.Direction.X * self.Properties.MovingDistance;
				self.temp_vec.y = self.Properties.Direction.Y * self.Properties.MovingDistance;
				self.temp_vec.z = self.Properties.Direction.Z * self.Properties.MovingDistance;
				self:SetObjectPos(0,self.temp_vec);
			end
		end
	----------------------------------------------------------
	},
	------------------------------------------------------------------------------------------
	--CLOSED
	------------------------------------------------------------------------------------------
	Closed = {
		-- Called when Closed State is Set.
		----------------------------------------------------------
		OnBeginState = function(self)
			--System.LogToConsole("SERVER:Closed");
			if(self.Timer~=0)then
				Game:KillTimer(self.Timer);
			end
			self.Timer=Game:SetTimer(self,self.OpeningTime);
		
			self.EventSent = nil;
			self.EndReached = nil;
		end,
		----------------------------------------------------------
		OnEndState = function(self)
			Game:KillTimer(self.Timer);
			self.Timer=0;
		end,
		----------------------------------------------------------
		OnContact = function(self, other)
			if ( self.Properties.bAutomatic==1 ) then
				-- if the door is locked we need to check for the key first
				if (self.bLocked~=0) then
					if (other and other.cnt and other.cnt.KeyCardAvailable) then
						if (other.cnt:KeyCardAvailable(self.Properties.nNeededKey)) then
							other.cnt:SetKeyCard(self.Properties.nNeededKey, 0);
							self.bLocked=0;
							Hud:AddMessage("Door Unlocked.");
--							System.LogToConsole("Door	 unlocked !");
						else
							--System.LogToConsole("Key not available !");
							if (not self.MsgShown and KeyCardInfo and KeyCardInfo[self.Properties.nNeededKey]) then
								Game:ShowIngameDialog(-1, "", "", 12, "You need the "..KeyCardInfo[self.Properties.nNeededKey].Desc.." to open this door...", 3);
								Hud:AddMessage("You need the "..KeyCardInfo[self.Properties.nNeededKey].Desc.." to open this door...");
								self.MsgShown=1;
							end
							self:SetTimer(100);
							return
						end
					else
						Hud:AddMessage("This door is locked.");
						--System.LogToConsole("Wrong entity !");
						return
					end
				end
				------
				if(self.Properties.bPlayerOnly==1 and (other.type~="Player"))then
					return
				end
				--System.LogToConsole("Closed contact");
				if (self.EndReached==nil) then
					--self.GotoState( "Opened" );
					--System.LogToConsole("Bau");
					self:Event_Open(self);
				end
			end
		end,
		OnTimer = function( self )
			--System.LogToConsole("No contact");
			self.MsgShown=nil;
		end,
		----------------------------------------------------------
--		OnTimer = function(self)
--		end,
		----------------------------------------------------------
		OnEvent = function(self,eid)
			if(eid==ScriptEvent_OnTimer)then
				self:SetObjectPos(0, g_Vectors.v000);
				end
		end
		----------------------------------------------------------
	}
}

------------------------------------------------------------------------------------------
------------------------------------------------------------------------------------------
-- CLIENT
------------------------------------------------------------------------------------------
------------------------------------------------------------------------------------------
AutomaticDoor1Piece["Client"]={
	OnInit = AutomaticDoor1Piece.OnInit,
	------------------------------------------------------------------------------------------
	--OPENED
	------------------------------------------------------------------------------------------
	Opened = {
		----------------------------------------------------------
		-- Called when Opened State is Set.
		OnBeginState = function(self)
			self:StartAnimation(0,"open");
			--System.LogToConsole("CLIENT:Open");
			if(self.OpenSound)then
				--REINST
				-- Sound.SetSoundPosition(self.OpenSound, self:GetPos());
				-- System.LogToConsole("Automatic Door Open("..self:GetPos().x..","..self:GetPos().y..","..self:GetPos().z..",");
				-- Sound.PlaySound(self.OpenSound);
			end
			self:Activate(1);
		end,
		----------------------------------------------------------
		OnUpdate = function(self, dt)
			self.Distance = self.Distance + dt * self.Properties.MovingSpeed;
			if ( self.Distance > self.Properties.MovingDistance ) then
				self.Distance = self.Properties.MovingDistance;
				if ( not self.EventSent ) then
					self.Event_Opened(self);
				end
				self.EventSent = 1;
			end
			
			self.temp_vec.x = self.Properties.Direction.X * self.Distance;
			self.temp_vec.y = self.Properties.Direction.Y * self.Distance;
			self.temp_vec.z = self.Properties.Direction.Z * self.Distance;
			self:SetObjectPos(0,self.temp_vec);
		end,

	},
	------------------------------------------------------------------------------------------
	--CLOSED
	------------------------------------------------------------------------------------------
	Closed = {
		-- Called when Closed State is Set.
		----------------------------------------------------------
		OnBeginState = function(self)
			self:StartAnimation(0,"close");
			--System.LogToConsole("CLIENT:Close");
			if(self.CloseSound)then
				--REINST
				--Sound.SetSoundPosition(self.CloseSound, self:GetPos());
				--System.LogToConsole("Automatic Door Close("..self:GetPos().x..","..self:GetPos().y..","..self:GetPos().z..",");
				--Sound.PlaySound(self.CloseSound);
			end
			self:Activate(1);
		end,
		----------------------------------------------------------
		OnUpdate = function(self, dt)
			self.Distance = self.Distance - dt * self.Properties.MovingSpeed;
			if ( self.Distance < 0 ) then
				self.Distance = 0;
				if ( not self.EventSent ) then
					self.Event_Closed(self);
				end
				self.EventSent = 1;
				--self.EndReached = 1;
			end
			local CurrPos = {};
			self.temp_vec.x = self.Properties.Direction.X * self.Distance;
			self.temp_vec.y = self.Properties.Direction.Y * self.Distance;
			self.temp_vec.z = self.Properties.Direction.Z * self.Distance;
			self:SetObjectPos(0,self.temp_vec);
		end,

	}
}
------------------------------------------------------------------------------------------
------------------------------------------------------------------------------------------

function AutomaticDoor1Piece:OnShutDown()
end

AutomaticDoor1Piece.FlowEvents =
{
	Inputs =
	{
		Close = { AutomaticDoor1Piece.Event_Close, "bool" },
		Closed = { AutomaticDoor1Piece.Event_Closed, "bool" },
		Disable = { AutomaticDoor1Piece.Event_Disable, "bool" },
		Enable = { AutomaticDoor1Piece.Event_Enable, "bool" },
		Open = { AutomaticDoor1Piece.Event_Open, "bool" },
		Opened = { AutomaticDoor1Piece.Event_Opened, "bool" },
	},
	Outputs =
	{
		Close = "bool",
		Closed = "bool",
		Disable = "bool",
		Enable = "bool",
		Open = "bool",
		Opened = "bool",
	},
}
