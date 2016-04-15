----------------------------------------------------------------------------------------------------
--  Crytek Source File.
--  Copyright (C), Crytek Studios, 2009.
----------------------------------------------------------------------------------------------------
--  $Id$
--  $DateTime$
--  Description: Flag for Capture the flag
--
----------------------------------------------------------------------------------------------------
--  History:
--  - 05:03:2009   17:28 : Created by Colin Gulliver
--
----------------------------------------------------------------------------------------------------
CTFFlag =
{
	Server = {},
	Client = {},
	Properties =
	{
		objModelSame = "objects/props/misc/ctf/flag_neutral_small.cgf",
		objModelOther = "objects/props/misc/ctf/flag_neutral_small.cgf",
		Team = 0,
		ActionSuffix = "flag",
		ActionSuffixAG = "_flag",
		PlayerTag = "flag",

		Physics = 
		{
			mass = 20.0,
			density = 0.0,
			water_damping = 80,
			water_resistance = 190,
			water_density = 250,
		},
	},
};

----------------------------------------------------------------------------------------------------
function CTFFlag:LoadGeometry(slot, model)
	if (string.len(model) > 0) then
		local ext = string.lower(string.sub(model, -4));

		if ((ext == ".chr") or (ext == ".cdf") or (ext == ".cga")) then
			self:LoadCharacter(slot, model);
		else
			self:LoadObject(slot, model);
		end
	end
end

----------------------------------------------------------------------------------------------------
function CTFFlag:LoadCorrectGeometry()
	local flagTeamId = g_gameRules.game:GetTeam(self.id);

	local localPlayerTeamId = 0;
	if (g_localActorId ~= nil) then
		localPlayerTeamId = g_gameRules.game:GetTeam(g_localActorId);
	end

	if (flagTeamId == localPlayerTeamId) then
		self:LoadGeometry(0, self.Properties.objModelOther);
	else
		self:LoadGeometry(0, self.Properties.objModelSame);
	end
end

----------------------------------------------------------------------------------------------------
function CTFFlag:OnSpawn()
	CryAction.CreateGameObjectForEntity(self.id);
	CryAction.BindGameObjectToNetwork(self.id);
	CryAction.ForceGameObjectUpdate(self.id, true);

	self:RedirectAnimationToLayer0(0, true);
	self:Activate(1);
	self:SetPhysicParams(PHYSICPARAM_BUOYANCY, self.Properties.Physics );

	self:OnReset();
end

----------------------------------------------------------------------------------------------------
function CTFFlag:OnReset()
	self:LoadCorrectGeometry();
	self:SetViewDistRatio(255);
end

----------------------------------------------------------------------------------------------------
function CTFFlag:OnInit()
	self:OnReset()
end

----------------------------------------------------------------------------------------------------
function CTFFlag:OnSetTeam(teamId)
	self:LoadCorrectGeometry();
end

----------------------------------------------------------------------------------------------------
function CTFFlag.Server:OnInit()
	if (not self.bInitialized) then
		self:OnReset();
		self.bInitialized = 1;
	end;
end;

----------------------------------------------------------------------------------------------------
function CTFFlag.Client:OnInit()
	if (not self.bInitialized) then
		self:OnReset();
		self.bInitialized = 1;
	end;
end;

----------------------------------------------------------------------------------------------------
function CTFFlag:OnPropertyChange()
	self:OnReset();
end

----------------------------------------------------------------------------------------------------
function CTFFlag:IsUsable(user)
	if (g_gameRules.game:GetTeam(user.id) ~= g_gameRules.game:GetTeam(self.id)) then
		return 725725; -- magic number to identify item pickups from interactor
	else
		return 0;
	end
end

