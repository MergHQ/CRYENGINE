Script.ReloadScript("scripts/gamerules/GameRulesUtils.lua");

Assault = {};
GameRulesSetStandardFuncs(Assault);

----------------------------------------------------------------
Assault.OldCVars = {};
Assault.OldCVars.AllSeeingRadar = nil;
Assault.OldCVars.NoEnemiesOnRadar = nil;
--Assault.OldCVars.NameTagsThroughGeom_friendlies = nil;
--Assault.OldCVars.NameTagsThroughGeom_enemies = nil;

----------------------------------------------------------------
-- Setup team-specifics for the LOCAL player
function Assault:SetupPlayerTeamSpecifics(localActorId)

	assert(localActorId);

	local  localTeam = self.game:GetTeam(localActorId);
	local  isAttacker = (localTeam and (localTeam > 0) and (localTeam == self.game:GetPrimaryTeam()));

	Log("[tlh] @ Assault:SetupPlayerTeamSpecifics(), isAttacker=%s", tostring(isAttacker));

	if ((not self.OldCVars.AllSeeingRadar) or (self.OldCVars.AllSeeingRadar == nil)) then
		self.OldCVars.AllSeeingRadar = System.GetCVar("g_mpAllSeeingRadar");
	end
	if ((not self.OldCVars.NoEnemiesOnRadar) or (self.OldCVars.NoEnemiesOnRadar == nil)) then
		self.OldCVars.NoEnemiesOnRadar = System.GetCVar("g_mpNoEnemiesOnRadar");
	end

	if (isAttacker and (isAttacker == true)) then
		-- Remove any defender mods that may still be active (from previous round)
		System.SetCVar("g_mpNoEnemiesOnRadar", 0);

	else
		System.SetCVar("g_mpAllSeeingRadar", 0);
--		System.SetCVar("hud_tagnames_ThroughGeom_friendies", 1);
--		System.SetCVar("hud_tagnames_ThroughGeom_enemies", 0);

	end

	self:SetTeamSpecificsForGeneralPlayer(localActorId);

end

----------------------------------------------------------------
-- Setup team-specifics for a REMOTE player
function Assault:SetupRemotePlayerTeamSpecifics(playerId)
	assert(playerId);
	assert(playerId ~= g_localActorId);

	self:SetTeamSpecificsForGeneralPlayer(playerId);

end

----------------------------------------------------------------
-- Set team-specific settings for a given general player, which can be either the local player or a remote player
-- Assault mode has different models depending on team and faction, but they are handled by the loadouts
function Assault:SetTeamSpecificsForGeneralPlayer(playerId)
	Log("[tlh] @ Assault:SetTeamSpecificsForGeneralPlayer(playerId)");
	assert(playerId);

	local  player = System.GetEntity(playerId);
	assert(player and (player ~= nil));

	if (player and (player ~= nil) and player.actor) then
		Log("[tlh]     setting team-specific settings for player \"%s\"...", player:GetName());

		local  playerTeam = self.game:GetTeam(playerId);
		local  playerAttacker = (playerTeam and (playerTeam > 0) and (playerTeam == self.game:GetPrimaryTeam()));

		local  newMaxHealth = player.Properties.Damage.health;

		if ( not playerAttacker or (playerAttacker == false) ) then
			newMaxHealth = System.GetCVar("g_mp_as_DefendersMaxHealth");
		end

		if (player.actor) then
			Log("[tlh]     setting MaxHealth to %f", newMaxHealth);
			player.actor:SetMaxHealth(newMaxHealth);
		end

    end
end

----------------------------------------------------------------
function Assault:ResetPlayerTeamSpecifics()
	Log("[tlh] @ Assault:ResetTeamSpecifics()");

	if ((self.OldCVars.AllSeeingRadar) and (self.OldCVars.AllSeeingRadar ~= nil)) then
		System.SetCVar("g_mpAllSeeingRadar", self.OldCVars.AllSeeingRadar);
		self.OldCVars.AllSeeingRadar = nil;
	end
	if ((self.OldCVars.NoEnemiesOnRadar) and (self.OldCVars.NoEnemiesOnRadar ~= nil)) then
		System.SetCVar("g_mpNoEnemiesOnRadar", self.OldCVars.NoEnemiesOnRadar);
		self.OldCVars.NoEnemiesOnRadar = nil;
	end

end

----------------------------------------------------------------------------------------------------
-- function Assault:RulesUseWeaponLoadouts()
-- 	return 0;
-- end

----------------------------------------------------------------------------------------------------
-- function Assault:RulesUsePerkLoadouts()
-- 	return 0;
-- end

----------------------------------------------------------------------------------------------------
function Assault:EquipTeamSpecifics(playerId)
	local  player = System.GetEntity(playerId);
	local  playerTeam = self.game:GetTeam(playerId);
	local  isAttacker = (playerTeam and (playerTeam > 0) and (playerTeam == self.game:GetPrimaryTeam()));

	Log("[tlh] @ Assault:EquipTeamSpecifics(), isAttacker=%s", tostring(isAttacker));

	--player.inventory:Destroy();

	if (isAttacker and (isAttacker == true)) then
		ItemSystem.GiveItem("FlashBangGrenades", playerId, false);
		ItemSystem.GiveItem("Nova", playerId, false);

	else
		ItemSystem.GiveItem("FragGrenades", playerId, false);
		ItemSystem.GiveItem("K-Volt", playerId, false);
		ItemSystem.GiveItem("SCAR", playerId, false);

	end

end

