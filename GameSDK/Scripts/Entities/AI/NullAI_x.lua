-- NullAI - Empty AI shell, very light-weight, used for standing in place of an empty AI entity container

Script.ReloadScript( "SCRIPTS/Entities/actor/BasicActor.lua");
Script.ReloadScript( "SCRIPTS/Entities/AI/Shared/BasicAI.lua");

NullAI_x = 
{
	IsAIControlled = function()
		return true
	end,

	PropertiesInstance = {
 		AITerritoryAndWave = {
			aiterritory_Territory = "<None>",
			aiwave_Wave           = "<None>",
		},
		soclasses_SmartObjectClass = "",
		groupid = -1,
		bAutoDisable = 0,
	}, 
  
	Properties = 
	{	
		fileModel = "",
		
		fileHitDeathReactionsParamsDataFile = "",
		
		aicharacter_character = "Player",
		soclasses_SmartObjectClass = "",
		equip_EquipmentPack="",
	},
	
	gameParams =
	{			
		physicsParams =
		{
			Living = 
			{
				inertia = 0.0,
				inertiaAccel = 0.0,
			},
		},
	},
	
	AIMovementAbility = {},
	
	hitDeathReactionsParamsDefFile = "",
	AnimationGraph = "",
	UpperBodyGraph = "",
	ActionController = "",
	
	_Parent={},
}

------------------------------------------------------------------------------------------
-- Note this is called before there's an entity is created, so mainly useful for inheritance
function NullAI_x:InitNullAI()
	self._Parent.Reset=self.Reset;
	self.Reset=self.NullAI_Reset;

	self._Parent.OnReset=self.OnReset;
	self.OnReset=self.NullAI_OnReset;
	
	self._Parent.ResetCommon=self.ResetCommon;
	self.ResetCommon=self.NullAI_ResetCommon;
	
	self._Parent.OnInit=self.OnInit;
	self.OnInit=self.NullAI_OnInit;
	
	self._Parent.SetActorModel=self.SetActorModel;
	self.SetActorModel=self.NullAI_SetActorModel;
	
	self._Parent.OnSpawn = self.OnSpawn;
	self.OnSpawn = self.NullAI_OnSpawn;	
end

------------------------------------------------------------------------------------------
function NullAI_x:NullAI_Reset(isFromInit, isReload)

	self:NullAI_ResetCommon(isFromInit, isReload);

end

------------------------------------------------------------------------------------------
function NullAI_x:NullAI_OnReset(isFromInit, isReload)

	self.UpdateTime = 0.05;

	self:NetPresent(1);
	self:SetScriptUpdateRate(self.UpdateTime);

	self:NullAI_ResetCommon(isFromInit, isReload);

end

------------------------------------------------------------------------------------------
function NullAI_x:NullAI_ResetCommon(isFromInit, isReload)
	
	self.AI = {};

end

------------------------------------------------------------------------------------------
function NullAI_x:NullAI_OnInit(isReload)
	
	self.AI = {};
	
	if (not isReload or isReload == false) then
		BasicAI.RegisterAI(self);
	end

	self:NullAI_OnReset(true, isReload);

end

------------------------------------------------------------------------------------------
function NullAI_x:NullAI_SetActorModel(isClient)

	-- Create a render proxy (if one doesn't exist) but don't load any assets in.
	self:CreateRenderProxy();
	
end

------------------------------------------------------------------------------------------
function NullAI_x:NullAI_OnSpawn()
end

function NullAI_x:RegisterAI(bForce)
	-- do nothing (null AI don't have AI objects)
end

