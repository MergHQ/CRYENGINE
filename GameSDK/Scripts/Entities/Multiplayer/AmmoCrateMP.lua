AmmoCrateMP = {
	type = "AmmoCrateMP",

	-- Based from the SP AmmoCrate

	Client = {},
	Server = {},

	Properties = {
		fileModelOverride = "",
		
		teamId = 0,			-- 0 means team is ignored
		bEnabled = 0,
		NumUsagesPerPlayer = 1,
		GroupId = "",
		audioSignal = -1,

		Ammo = 
		{
			GiveClips = 1,
			bRefillWeaponAmmo = 0,
			FragGrenades = 0,
			bRefillWithCurrentGrenades = 1, --Rather than just giving frags
		},
	},

	OPEN_SLOT = 0,
	
	Editor={
		Icon="item.bmp",
		IconOnTop=1,
	},
}

AmmoCrateMP.DEFAULT_FILE_MODEL = "objects/weapons/ammo/ammo_crate/ammo_crate_closed.cgf";

Net.Expose {
	Class = AmmoCrateMP,

	ClientMethods = {
		ClRefillAmmoResult = { RELIABLE_UNORDERED, NO_ATTACH, BOOL, INT8 },
	},
	ServerMethods = {
		SvRequestRefillAmmo = { RELIABLE_UNORDERED, NO_ATTACH, ENTITYID },
	},
	ServerProperties = { },
}


-----------------------------------------------------------------------------------------------------
function AmmoCrateMP.Server:SvRequestRefillAmmo(userId)
	local user=System.GetEntity(userId);
	local ammoRefilled = 0;

	if (self.playerUsages[userId]==nil) then
		self.playerUsages[userId] = 0;
	end

	local playerUsagesLeft = -1; -- -1 is for infinite
	if (self.Properties.NumUsagesPerPlayer > 0) then
		playerUsagesLeft = self.Properties.NumUsagesPerPlayer - self.playerUsages[userId];
	end

	if (playerUsagesLeft ~= 0) then

		if (self.Properties.Ammo.GiveClips ~= 0) then
			ammoRefilled = user.actor:SvGiveAmmoClips(self.Properties.Ammo.GiveClips);
		end

		if ((self.Properties.Ammo.bRefillWeaponAmmo ~= 0) or (self.Properties.Ammo.FragGrenades ~= 0)) then
			ammoRefilled = user.actor:SvRefillAllAmmo("", self.Properties.Ammo.bRefillWeaponAmmo, self.Properties.Ammo.FragGrenades, self.Properties.Ammo.bRefillWithCurrentGrenades) + ammoRefilled;
		end

		if (ammoRefilled~=0) then
			self.playerUsages[userId] = self.playerUsages[userId] + 1;
			playerUsagesLeft = playerUsagesLeft - 1;
		end

	end

	self.onClient:ClRefillAmmoResult(user.actor:GetChannel(), ammoRefilled~=0, playerUsagesLeft);
end

function AmmoCrateMP.Client:ClRefillAmmoResult(ammoRefilled, numUsagesLeft)
	--Log( "AmmoCrateMP.Client:ClRefillAmmoResult refilled:"..tostring(ammoRefilled).." usagesLeft:"..tostring(numUsagesLeft) );
	g_localActor.actor:ClRefillAmmoResult(ammoRefilled);

	if (numUsagesLeft==0) then
		self.bLocalUsesLeft = false;
		GameAudio.StopEntitySignal(self.audioSignal, self.id);
		BroadcastEvent(self, "NoUsesRemaining");
		if (HUD) then
			HUD.RemoveObjective(self.id);
		end
	end
end

-----------------------------------------------------------------------------------------------------
MakeUsable(AmmoCrateMP);

AmmoCrateMP.Properties.bUsable=1;
AmmoCrateMP.Properties.UseMessage = "@ui_prompt_interact_ammo_cache";

-----------------------------------------------------------------------------------------------------
function AmmoCrateMP.Server:OnInit()
	if (not CryAction.IsClient()) then
		self:OnInit();
	end
end

function AmmoCrateMP.Client:OnInit()
	self:OnInit();
end

function AmmoCrateMP:OnInit()
	--Log( "AmmoCrateMP:OnInit" );	
	self.ammoRechargesModels = {};
	self:OnReset();
	Game.AddTacticalEntity(self.id, eTacticalEntity_Ammo);
	
	local givesFragGrenades = self.Properties.Ammo.FragGrenades > 0;
	Game.OnAmmoCrateSpawned(givesFragGrenades); 
end

function AmmoCrateMP:OnPropertyChange()
	self:OnReset();
end

function AmmoCrateMP:OnReset()
	self:ResetMainBoxModel();

	self.bUsable = self.Properties.bUsable;

	self.audioSignal = GameAudio.GetSignal("AmmoCrate");

	self.bLocalUsesLeft = true;
	self.playerUsages={};

	self:Enabled(self.Properties.bEnabled);

	BroadcastEvent(self, "Disabled");
end

function AmmoCrateMP:ResetMainBoxModel()
	local fileModel = self.DEFAULT_FILE_MODEL;
	if (self.Properties.fileModelOverride and (self.Properties.fileModelOverride ~= "")) then
		fileModel = self.Properties.fileModelOverride;
	end

	self:LoadObject(self.OPEN_SLOT, fileModel);
	self:Physicalize( self.OPEN_SLOT, PE_STATIC,{mass = 0, density = 0} );
end

function AmmoCrateMP:OnShutDown()
	Game.RemoveTacticalEntity(self.id, eTacticalEntity_Ammo);
end

-----------------------------------------------------------------------------------------------------
function AmmoCrateMP:IsUsable(user)
	local bValidTeam = ((not self.Properties.teamId) or (self.Properties.teamId==g_gameRules.game:GetTeam(user.id)))
	local isUsable = self.bUsable and bValidTeam and user.actor:IsLocalClient() and self.Properties.bEnabled~=0 and self.bLocalUsesLeft;
	if (isUsable) then
		return 1;
	end
	
	return 0;
end

function AmmoCrateMP:OnUsed(user, idx)
	--Log( "AmmoCrateMP:OnUsed " );
	self.server:SvRequestRefillAmmo(user.id);
end;

function AmmoCrateMP:StopUse(user, idx)
end;

-----------------------------------------------------------------------------------------------------
function AmmoCrateMP:Enabled(bEnabled)
	self.Properties.bEnabled = bEnabled;

	self.bLocalUsesLeft = true;
	self.playerUsages={};

	if (not System.IsEditor()) then
		if (bEnabled~=0) then
			self:Hide(0);
			BroadcastEvent(self, "Enabled");
			GameAudio.PlayEntitySignal(self.audioSignal, self.id);
		else
			self:Hide(1);
			BroadcastEvent(self, "Disabled");
			GameAudio.StopEntitySignal(self.audioSignal, self.id);
		end
	end
end;

----------------------------------------------------------------------------------------------------
AmmoCrateMP.FlowEvents =
{
	Inputs =
	{
	},
	Outputs =
	{
		Enabled = "bool",
		Disabled = "bool",
		NoUsesRemaining = "bool",
	},
}
