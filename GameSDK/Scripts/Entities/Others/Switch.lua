--------------------------------------------------------------------------
--	Crytek Source File.
-- 	Copyright (C), Crytek Studios, 2001-2006.
--------------------------------------------------------------------------
--	$Id$
--	$DateTime$
--	Description: Switch entity for both FG and Prefab use.
--
--------------------------------------------------------------------------
--  History:
--  - 19:5:2006 : Created by Sascha Gundlach
--
--------------------------------------------------------------------------

Switch = {
	Client = {},
	Server = {},
	Properties = {
		fileModel = "objects/props/industrial/electrical/lightswitch/lightswitch_local1.cgf",
		Switch = "Props_Interactive.switches.small_switch",
		ModelSubObject = "",
		fileModelDestroyed = "",
		DestroyedSubObject = "",
		fHealth = 100,
		bDestroyable = 0,
		bUsable = 1,
		UseMessage = "Use Light Switch",
		bTurnedOn = 0,
		Physics =
		{
			bRigidBody = 0,
			bRigidBodyActive = 0,
			bRigidBodyAfterDeath = 1,
			bResting = 1,
			Density = -1,
			Mass = 1,
		},
		Sound =
		{
			soundTurnOnSound = "",
			soundTurnOffSound = "",
		},
		SwitchPos =
		{
			bShowSwitch = 1,
			On = 35,
			Off = -35,
		},
		SmartSwitch =
		{
			bUseSmartSwitch=0,
			Entity = "",
			TurnedOnEvent = "",
			TurnedOffEvent = "",
		},
		Breakage =
		{
			fLifeTime = 20,
			fExplodeImpulse = 0,
			bSurfaceEffects = 1,
		},
		Destruction =
		{
			bExplode = 0,
			Effect = "",
			EffectScale = 0,
			Radius = 0,
			Pressure = 0,
			Damage = 0,
			Decal = "",
			Direction = {x=0, y=0, z=-1},
		},
	},
	Editor =
	{
		Icon = "switch.bmp",
		IconOnTop = 1,
	},
	States = {"TurnedOn","TurnedOff","Destroyed"},
	fCurrentSpeed = 0,
	fDesiredSpeed = 0,
	LastHit =
	{
		impulse = {x=0,y=0,z=0},
		pos = {x=0,y=0,z=0},
	},
	shooterId = nil,
}


Net.Expose{
	Class = Switch,
	ClientMethods = {
		OnUsed_Internal = { RELIABLE_ORDERED, PRE_ATTACH, ENTITYID },
		Destroy         = { RELIABLE_ORDERED, PRE_ATTACH },
	},
	ServerMethods = {
		SvRequestUse = { RELIABLE_ORDERED, PRE_ATTACH, ENTITYID },
	},
}


function Switch:OnPropertyChange()
	self:OnReset();
end;

function Switch:OnSave(tbl)
	tbl.switch=self.switch;
	tbl.usable=self.usable;
end;

function Switch:OnLoad(tbl)
	self.switch=tbl.switch;
	self.usable=tbl.usable;

	if(self:GetState() ~= "Destroyed") then
		self:PhysicalizeThis(0);
		self:SetCurrentSlot(0);
	else
		local tmp=self.Properties.Physics.bRigidBody;
		self.Properties.Physics.bRigidBody=self.Properties.Physics.bRigidBodyAfterDeath;
		self:PhysicalizeThis(1);
		self.Properties.Physics.bRigidBody=tmp;
		self:SetCurrentSlot(1);
	end

end;

function Switch:OnReset()
	local props=self.Properties;
	self.health=props.fHealth;
	self.usable=self.Properties.bUsable;
	if(not EmptyString(props.fileModel))then
		self:LoadSubObject(0,props.fileModel,props.ModelSubObject);
	end;
	if(not EmptyString(props.fileModelDestroyed))then
		self:LoadSubObject(1, props.fileModelDestroyed,props.DestroyedSubObject);
	elseif(not EmptyString(props.DestroyedSubObject))then
		self:LoadSubObject(1,props.fileModel,props.DestroyedSubObject);
	end;
	self:SetCurrentSlot(0);
	self:PhysicalizeThis(0);
	if(not EmptyString(self.Properties.Switch))then
		self:SpawnSwitch();
	end;
	if(self.Properties.bTurnedOn==1)then
		self:GotoState("TurnedOn");
	else
		self:GotoState("TurnedOff");
	end;
end;

function Switch:PhysicalizeThis(slot)
	local physics = self.Properties.Physics;
	EntityCommon.PhysicalizeRigid( self,slot,physics,1 );
end;

function Switch.Client:OnHit(hit, remote)
	CopyVector(self.LastHit.pos, hit.pos);
	CopyVector(self.LastHit.impulse, hit.dir);
	self.LastHit.impulse.x = self.LastHit.impulse.x * hit.damage;
	self.LastHit.impulse.y = self.LastHit.impulse.y * hit.damage;
	self.LastHit.impulse.z = self.LastHit.impulse.z * hit.damage;
end

function Switch.Server:OnHit(hit)
	self.shooterId=hit.shooterId;
	BroadcastEvent( self,"Hit" );
  if (self.Properties.bDestroyable==1) then
  	self.health=self.health-hit.damage;
  	if(self.health<=0)then
  		self.allClients:Destroy();
  	end;
  end
end;

function Switch:Explode()
	local props=self.Properties;
	local hitPos = self.LastHit.pos;
	local hitImp = self.LastHit.impulse;
	self:BreakToPieces(
		0, 0,
		props.Breakage.fExplodeImpulse,
		hitPos,
		hitImp,
		props.Breakage.fLifeTime,
		props.Breakage.bSurfaceEffects
	);
	if(NumberToBool(self.Properties.Destruction.bExplode))then
		local explosion=self.Properties.Destruction;
 		g_gameRules:CreateExplosion(self.shooterId,self.id,explosion.Damage,self:GetWorldPos(),explosion.Direction,explosion.Radius,nil,explosion.Pressure,explosion.HoleSize,explosion.Effect,explosion.EffectScale);
	end;
	self:SetCurrentSlot(1);
	if(props.Physics.bRigidBodyAfterDeath==1)then
		local tmp=props.Physics.bRigidBody;
		props.Physics.bRigidBody=1;
		self:PhysicalizeThis(1);
		props.Physics.bRigidBody=tmp;
	else
		self:PhysicalizeThis(1);
	end;
	self:RemoveDecals();
	self:AwakePhysics(1);
	self:OnDestroy();
end;

function Switch:SetCurrentSlot(slot)
	if (slot == 0) then
		self:DrawSlot(0, 1);
		self:DrawSlot(1, 0);
	else
		self:DrawSlot(0, 0);
		self:DrawSlot(1, 1);
	end;
	self.currentSlot = slot;
end


function Switch:SetSwitch(state)
	if(self.switch==nil)then return;end;
	local props=self.Properties.SwitchPos;
	if(props.bShowSwitch==0)then return;end;
	local props=self.Properties.SwitchPos;
	local rot={x=0,y=0,z=0};
	if(state==1)then
		self.switch:GetAngles(rot);
		rot.y=props.On*g_Deg2Rad;
	else
		self.switch:GetAngles(rot);
		rot.y=props.Off*g_Deg2Rad;
	end;
	self.switch:SetAngles(rot);
end;

function Switch:SpawnSwitch()
	if(self.switch)then
		Entity.DetachThis(self.switch.id,0);
		System.RemoveEntity(self.switch.id);
		self.switch=nil;
	end;

	local props=self.Properties.SwitchPos;
	if(props.bShowSwitch==0)then return;end;
	if(self.switch==nil)then
		if(self.Properties.Switch=="")then
			Log("No switch found for switch object "..self:GetName());
		else
			local spawnParams = {};
			spawnParams.class = "BasicEntity";
			Log("self.Properties.Switch: "..self.Properties.Switch);
			spawnParams.archetype=self.Properties.Switch;
			spawnParams.name = self:GetName().."_switch";
			spawnParams.flags = 0;
			spawnParams.position=self:GetPos();
			self.switch=System.SpawnEntity(spawnParams);

			self:AttachChild(self.switch.id,0);
			self.switch:SetWorldPos(self:GetPos());
			self.switch:SetWorldAngles(self:GetAngles());
			self:SetSwitch(self.Properties.bTurnedOn);
		end;
	end;
end;

function Switch:OnDestroy()
	if(self.switch)then
		Entity.DetachThis(self.switch.id,0);
		System.RemoveEntity(self.switch.id);
		self.switch=nil;
	end;
end

----------------------------------------------------------------------------------------------------
function Switch.Server:OnInit()
	if(not self.bInitialized)then
		self:OnReset();
		self.bInitialized=1;
		self.usable=1;
	end;
end;

----------------------------------------------------------------------------------------------------
function Switch.Client:OnInit()
	if(not self.bInitialized)then
		self:OnReset();
		self.bInitialized=1;
	end;
end;


----------------------------------------------------------------------------------------------------
-- Entry point for OnUsed events
function Switch:OnUsed(user, idx)
		self.server:SvRequestUse(user.id);
end;


----------------------------------------------------------------------------------------------------
-- clients use this to request a use to the server
function Switch.Server:SvRequestUse(userId)
		self.allClients:OnUsed_Internal(userId);
end;


----------------------------------------------------------------------------------------------------
-- the function that really does the work when the switch is used
function Switch.Client:OnUsed_Internal(userId)
	self:ActivateOutput( "UsedBy", userId );
	if(self:GetState()=="TurnedOn")then
		self:GotoState("TurnedOff");
	elseif(self:GetState()=="TurnedOff")then
		self:GotoState("TurnedOn");
	elseif(self:GetState()=="Destroyed")then
			return
	end;
	BroadcastEvent(self, "Used");
end;



function Switch:IsUsable(user)
	if((self:GetState()~="Destroyed") and self.usable==1)then
		return 2;
	else
		return 0;
	end;

end;

function Switch:GetUsableMessage(idx)
	if(self.Properties.bUsable==1)then
		return self.Properties.UseMessage;
	else
		return "@use_object";
	end;
end;

function Switch:PlaySound(sound)
	if(sound and sound~="")then
		local snd=self.Properties.Sound["sound"..sound];
		-- REINSTANTIATE!!!
		--local sndFlags=bor(SOUND_DEFAULT_3D, 0);
		if(snd and snd~="")then
			-- REINSTANTIATE!!!
			--self.soundid=self:PlaySoundEvent(snd,g_Vectors.v000,g_Vectors.v010,sndFlags,0,SOUND_SEMANTIC_MECHANIC_ENTITY);
		else
			--System.Log("Failed to play "..sound.." sound!");
		end;
	end;
end;

function Switch:CheckSmartSwitch(switch)
	local props=self.Properties.SmartSwitch;
	if(props.bUseSmartSwitch==1)then
		local entities=System.GetEntitiesInSphere(self:GetPos(),50);
		local targets={};
		for i,v in ipairs(entities) do
			if(v:GetName()==props.Entity)then
				table.insert(targets,v);
			end;
		end
		--Get closest
		table.sort(targets, function(a, b)
				local dista=self:GetDistance(a.id);
				local distb=self:GetDistance(b.id);
				if(dista<distb)then
					return true;
				end
		end);
		local target=targets[1];
		if(target)then
			if(props[switch]~="")then
				local evtName="Event_"..props[switch];
				local evtProc=target[evtName];
				if(type(evtProc)=="function")then
				  --System.Log("Sending: "..switch.." to "..target:GetName());
				  evtProc(target);
				else
					System.Log(self:GetName().." was trying to send an invalid event! Check entity properties!");
				end;
			end;
		end;
	end;
end;


----------------------------------------------------------------------------------
------------------------------------Events----------------------------------------
----------------------------------------------------------------------------------
function Switch:Event_Destroyed()
	BroadcastEvent(self, "Destroyed");
	self:GotoState("Destroyed");
end;

function Switch:Event_TurnedOn()
	BroadcastEvent(self, "TurnedOn");
	self:GotoState("TurnedOn");
end;

function Switch:Event_TurnedOff()
	BroadcastEvent(self, "TurnedOff");
	self:GotoState("TurnedOff");
end;


function Switch:Event_Switch()
	if(self:GetState()~="Destroyed")then
		if(self:GetState()=="TurnedOn")then
			self:GotoState("TurnedOff");
		elseif(self:GetState()=="TurnedOff")then
			self:GotoState("TurnedOn");
		end;
	end;
end;

function Switch:Event_Hit(sender)
	BroadcastEvent( self,"Hit" );
end;

function Switch:Event_Enable(sender)
	self.usable=1;
	BroadcastEvent( self,"Enable" );
end;

function Switch:Event_Disable(sender)
	self.usable=0;
	BroadcastEvent( self,"Disable" );
end;

function Switch:Event_Hide(sender)
	self:Hide(1);
	BroadcastEvent( self,"Hide" );
end;

function Switch:Event_Unhide(sender)
	self:Hide(0);
	BroadcastEvent( self,"Unhide" );
end;



----------- This is just a bridge call. When the server decides that the switch is destroyed, in the OnHit function, it calls this on all clients to actually destroy it.
----------- Can't make GotoState() a client side callable function to just call it directly, because it is an internal C function.
function Switch.Client:Destroy()
	self:GotoState( "Destroyed" );
end;


----------------------------------------------------------------------------------
------------------------------------States----------------------------------------
----------------------------------------------------------------------------------

Switch.Client.TurnedOn =
{
	OnBeginState = function( self )
		self:PlaySound("TurnOnSound");
		--temporarily disabled
		self:SetSwitch(1);
		self:CheckSmartSwitch("TurnedOnEvent");
		BroadcastEvent(self, "TurnedOn");
	end,
	OnEndState = function( self )

	end,
}

Switch.Client.TurnedOff =
{
	OnBeginState = function( self )
		self:PlaySound("TurnOffSound");
		--temporarily disabled
		self:SetSwitch(0);
		self:CheckSmartSwitch("TurnedOffEvent");
		BroadcastEvent(self, "TurnedOff")
	end,
	OnEndState = function( self )

	end,
}

Switch.Client.Destroyed=
{
	OnBeginState = function( self )
		self:Explode();
		BroadcastEvent(self, "Destroyed")
	end,
	OnEndState = function( self )

	end,
}

----------------------------------------------------------------------------------
-------------------------------Flow-Graph Ports-----------------------------------
----------------------------------------------------------------------------------

Switch.FlowEvents =
{
	Inputs =
	{
		Switch = { Switch.Event_Switch },
		TurnedOn = { Switch.Event_TurnedOn },
		TurnedOff = { Switch.Event_TurnedOff },
		Hit = { Switch.Event_Hit, "bool" },
		Destroyed = { Switch.Event_Destroyed, "bool" },
		Disable = { Switch.Event_Disable, "bool" },
		Enable = { Switch.Event_Enable, "bool" },
		Hide = { Switch.Event_Hide, "bool" },
		Unhide = { Switch.Event_Unhide, "bool" },
	},
	Outputs =
	{
		Hit = "bool",
		TurnedOn = "bool",
		TurnedOff = "bool",
		Destroyed = "bool",
		Disable = "bool",
		Enable = "bool",
		Hide = "bool",
		Unhide = "bool",
		UsedBy = "entity"
	},
}
