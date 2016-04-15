--------------------------------------------------------------------------
--	Crytek Source File.
-- 	Copyright (C), Crytek Studios, 2011-2012.
--------------------------------------------------------------------------
--	$Id$
--	$DateTime$
--	Description: Health pack heals player on use
--  
--------------------------------------------------------------------------
--  History:
--  - 10:5:2012 : Created by Michiel Meesters
--
--------------------------------------------------------------------------

HealthPack = 
{
	Properties = 
	{
		soclasses_SmartObjectClass = "HealthPack",
		fileModel = "Objects/Weapons/equipment/medical_Kit/medical_kit_bag_tp.cgf",
		fUseDistance = 2.5,
		fRespawnTime = 5.0,
		fHealth = 20.0,
  },
  	
	PhysParams = 
	{ 
		mass = 0,
		density = 0,
	},
	
	Server = {},
	Client = {},
	
		Editor={
		Icon = "Item.bmp",
		IconOnTop=1,
	  },
	
}

Net.Expose {
	Class = HealthPack,
	ClientMethods = {
		ClHide = { RELIABLE_UNORDERED, POST_ATTACH, INT8 },
	},
	ServerMethods = {
		SvRequestHidePack = { RELIABLE_UNORDERED, POST_ATTACH, INT8 },
		SvRequestHeal = { RELIABLE_UNORDERED, POST_ATTACH, ENTITYID, FLOAT },
	},
	ServerProperties = {
	},
};

function HealthPack.Client:ClHide(hide)
	self:Hide(hide);
end

function HealthPack.Server:SvRequestHidePack(hide)
	self:Hide(hide);
	self.allClients:ClHide(hide);
end

function HealthPack.Server:SvRequestHeal(entityId, hide)
	local user=System.GetEntity(entityId);
	user.actor:SetHealth(user.actor:GetHealth() + self.Properties.fHealth);
end

function HealthPack:OnReset()
	self:Reset();
end

function HealthPack:OnSpawn()	
	CryAction.CreateGameObjectForEntity(self.id);
	CryAction.BindGameObjectToNetwork(self.id);
	CryAction.ForceGameObjectUpdate(self.id, true);	
	
	self.isServer=CryAction.IsServer();
	self.isClient=CryAction.IsClient();

	self:Reset(1);
end

function HealthPack:OnDestroy()
end

function HealthPack:DoPhysicalize()
	if (self.currModel ~= self.Properties.fileModel) then
		CryAction.ActivateExtensionForGameObject(self.id, "ScriptControlledPhysics", false);
		self:LoadObject( 0,self.Properties.fileModel );
		self:Physicalize(0,PE_RIGID,self.PhysParams);
		CryAction.ActivateExtensionForGameObject(self.id, "ScriptControlledPhysics", true);			
	end
	
		self:SetPhysicParams(PHYSICPARAM_FLAGS, {flags_mask=pef_cannot_squash_players, flags=pef_cannot_squash_players});

	self.currModel = self.Properties.fileModel;
end


function HealthPack:Reset(onSpawn)
	self:DoPhysicalize();	
end

function HealthPack:UnHide()
	self.server:SvRequestHidePack(0);
end

function HealthPack:IsUsable(user)

	if (not user) then
		return 0;
	end

	local delta = g_Vectors.temp_v1;
	local mypos,bbmax = self:GetWorldBBox();
		
	FastSumVectors(mypos,mypos,bbmax);
	ScaleVectorInPlace(mypos,0.5);
	user:GetWorldPos(delta);
			
	SubVectors(delta,delta,mypos);
	
	local useDistance = self.Properties.fUseDistance;
	if (LengthSqVector(delta)<useDistance*useDistance) then
		return 1;
	else
		return 0;
	end
end


function HealthPack:GetUsableMessage(idx)
		return "@use_HealthPack";
end


function HealthPack:OnUsed(user)	
	Script.SetTimerForFunction(self.Properties.fRespawnTime * 1000, "HealthPack.UnHide", self);
	self.server:SvRequestHidePack(1);
	self.server:SvRequestHeal(user.id, 1);
end
