Script.ReloadScript("scripts/gamerules/GameRulesUtils.lua");

AllOrNothing = {};
GameRulesSetStandardFuncs(AllOrNothing);

AllOrNothing.OldCVars = {};
AllOrNothing.OldCVars.AllSeeingRadarSv = nil;

function AllOrNothing:SvStartSuddenDeath()
	Log("*** AllOrNothing:SvStartSuddenDeath()");
	if (not self.OldCVars.AllSeeingRadarSv) then
		self.OldCVars.AllSeeingRadarSv = System.GetCVar("g_mpAllSeeingRadarSv");
		Log("    (remembered)");
	end
	System.SetCVar("g_mpAllSeeingRadarSv", 1);
end

function AllOrNothing:SvEndSuddenDeath()
	Log("*** AllOrNothing:SvEndSuddenDeath()");
	if (self.OldCVars.AllSeeingRadarSv) then
		System.SetCVar("g_mpAllSeeingRadarSv", self.OldCVars.AllSeeingRadarSv);
		self.OldCVars.AllSeeingRadarSv = nil;
		Log("    (reverted)");
	end
end

