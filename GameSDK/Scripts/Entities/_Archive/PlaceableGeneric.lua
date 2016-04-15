


PlaceableGeneric = {
	type = "Trigger",

	Properties = {
		DimX = 5,
		DimY = 5,
		DimZ = 5,
		ScriptCommand="",
		PlaySequence="",
		dummyModel="Objects/Pickups/explosive/explosive_dummy.cgf",		--
		fileModel="objects/pickups/explosive/explosive.cgf",			--
		object_ModelDestroyed="",										--
		
		bInitiallyVisible=1,
		bAutomaticPlaceable=1,
		TextInstruction=noinstruction,
		PlaceableObject="",
		bActive=1,
	},

	Editor={
		Model="Objects/Editor/T.cgf",
		ShowBounds = 1,
	},	
}


-------------------------------------------------------------------------------
function PlaceableGeneric:LoadGeometry()
	if(self.Properties.dummyModel~="")then
		self:LoadObject(self.Properties.dummyModel,0,1);
	end
	if(self.Properties.fileModel~="")then
		self:LoadObject(self.Properties.fileModel,1,1);
	end
	if(self.Properties.object_ModelDestroyed~="")then
		self:LoadObject(self.Properties.object_ModelDestroyed,2,1);
	end
end

-------------------------------------------------------------------------------
function PlaceableGeneric:RegisterStates()
	self:RegisterState("placeable");		-- show dummy
	self:RegisterState("notarmed");			-- show explosives not ticking, armable
	self:RegisterState("armed");			-- show explosives ticking, disarmable
	self:RegisterState("detonated");		-- detonated no visual
end


-------------------------------------------------------------------------------
function PlaceableGeneric:OnReset()
	self:Activate(0);
	self:TrackColliders(1);
	
	local Min = { x=-self.Properties.DimX/2, y=-self.Properties.DimY/2, z=-self.Properties.DimZ/2 };
	local Max = { x=self.Properties.DimX/2, y=self.Properties.DimY/2, z=self.Properties.DimZ/2 };
	self:SetBBox( Min, Max );
				
	if(self.Properties.bInitiallyVisible==1)then
		self:DrawObject(0,1);
	else
		self:DrawObject(0,0);
	end

	self:GotoState("placeable");

	self.bActive=self.Properties.bActive;
end


-------------------------------------------------------------------------------
function PlaceableGeneric:OnMultiplayerReset()
	self:OnReset();
end


-------------------------------------------------------------------------------
function PlaceableGeneric:OnPropertyChange()
	self:OnReset();
end

-------------------------------------------------------------------------------
function PlaceableGeneric:PrintMessage(id,msg)
	if(id)then
		local slot=Server:GetServerSlotByEntityId(id)
		if(slot)then
			slot:SendText(msg,.9)
		end
	end
end


----------------------------------------------------
--SERVER
----------------------------------------------------
PlaceableGeneric.Server={
	OnInit=function(self)
		self:LoadGeometry()
		self:RegisterStates();
		self:OnReset();
		self:NetPresent(nil);
	end,
-------------------------------------
	placeable={
		OnBeginState=function(self)
			self:DestroyPhysics();
		end,
		OnEnterArea = function(self,collider)
			if(GameRules:IsInteractionPossible(collider,self))then
				if(collider.type=="Player" and (not collider:IsDead()))then
					self.Who = collider;
					self:SetTimer(1);
				end
			end		
		end,
		OnLeaveArea = function(self,collider)
			if (collider == self.Who) then
				self.Who = nil;
			end
		end,
		--------------------------------
		OnTimer = function(self,dt)
			if (self.Who) then
				self:Collide( self.Who );
				self:SetTimer(1);
			end
		end,
	},
-------------------------------------
	notarmed={
		OnBeginState=function(self)
			self:DestroyPhysics();
		end,
	},
-------------------------------------
	armed={
		OnBeginState=function(self)
			self:DestroyPhysics();
		end,
		OnTimer=function(self)
			--if (self.countdown_step==0) then
				self:GotoState("detonated");
				BroadcastEvent( self,"Explode");
			--end
		end,
	},
-------------------------------------
	detonated={
		OnBeginState=function(self)
			self:CreateStaticEntity( 10,100 );
		end,
	},
}

----------------------------------------------------
--CLIENT
----------------------------------------------------
PlaceableGeneric.Client={
	OnInit=function(self)
		self:LoadGeometry()
		self:RegisterStates();
		self:OnReset();
	end,
-------------------------------------
	placeable={
		OnBeginState=function(self)
			self:DrawObject(0,1);
			self:DrawObject(1,0);
			self:DrawObject(2,0);
			self:DestroyPhysics();
		end,
	},
-------------------------------------
	notarmed={
		OnBeginState=function(self)
			self:DrawObject(0,0);
			self:DrawObject(1,1);
			self:DrawObject(2,0);
			self:DestroyPhysics();
		end,
	},
-------------------------------------
	armed={
		OnBeginState=function(self)
			self:DrawObject(0,0);
			self:DrawObject(1,1);
			self:DrawObject(2,0);
			self:DestroyPhysics();
		end,
	},
-------------------------------------
	detonated={
		OnBeginState=function(self)
			self:Explode();
			self:DrawObject(0,0);
			self:DrawObject(1,0);
			self:DrawObject(2,1);
			self:CreateStaticEntity( 10,100 );
		end,
	},
}




-------------------------------------------------------------------------------
function PlaceableGeneric:Collide(player)

	if (self.bActive == 0 ) then return end

	if (player.type ~= "Player" ) then return end

	-- if the collider does have an explosive,
	-- let's place it and remove it from the player

	--System.LogToConsole("colliding, object="..self.Properties.PlaceableObject);

	if (player.objects[self.Properties.PlaceableObject]==1) then

		System.LogToConsole("object found:"..self.Properties.PlaceableObject);

		if (self.Properties.bAutomaticPlaceable~=1) then
			if (not player.cnt.use_pressed) then
				Hud.label=self.Properties.TextInstruction;
				do return end
			end
		end

		player.objects[self.Properties.PlaceableObject]=0;				
		self.target=player.id
		self:GotoState("armed");
			
		self:Event_ExplosivePlaced();

	end	
end

-------------------------------------------------------------------------------
function PlaceableGeneric:Event_Activate(sender)
	BroadcastEvent( self,"Activate");
	self.bActive=1;
end

-------------------------------------------------------------------------------
function PlaceableGeneric:Event_ExplosivePlaced(sender)
	BroadcastEvent( self,"ExplosivePlaced");
end

-------------------------------------------------------------------------------
function PlaceableGeneric:Event_Explode(sender)
	BroadcastEvent( self,"Explode" );
	self:Explode();
end

-------------------------------------------------------------------------------
function PlaceableGeneric:Event_Hide(sender)
	self:DrawObject(0,0);
end

-------------------------------------------------------------------------------
function PlaceableGeneric:Event_Show(sender)
	self:DrawObject(0,1);
end

-------------------------------------------------------------------------------
function PlaceableGeneric:Explode()

	-- Trigger script command on explode.
	if(self.Properties.ScriptCommand and self.Properties.ScriptCommand~="")then
		--System.LogToConsole("Executing: "..self.Properties.ScriptCommand);
		dostring(self.Properties.ScriptCommand);
	end

	if(self.Properties.PlaySequence~="")then
		Movie.PlaySequence( self.Properties.PlaySequence );
	end
end

PlaceableGeneric.FlowEvents =
{
	Inputs =
	{
		Activate = { PlaceableGeneric.Event_Activate, "bool" },
		Explode = { PlaceableGeneric.Event_Explode, "bool" },
		ExplosivePlaced = { PlaceableGeneric.Event_ExplosivePlaced, "bool" },
		Hide = { PlaceableGeneric.Event_Hide, "bool" },
		Show = { PlaceableGeneric.Event_Show, "bool" },
		
	},
	Outputs =
	{
		Activate = "bool",
		Explode = "bool",
		ExplosivePlaced = "bool",
		Hide = "bool",
		Show = "bool",
	},
}




