AI.TacticalPositionManager = AI.TacticalPositionManager or {};

Script.ReloadScript("Scripts/AI/TPS/SDKGruntQueries.lua");
Script.ReloadScript("Scripts/AI/TPS/HumanGruntQueries.lua");
Script.ReloadScript("Scripts/AI/TPS/SharedQueries.lua");
Script.ReloadScript("Scripts/AI/TPS/HelicopterQueries.lua");

function AI.TacticalPositionManager:OnInit()
	for i,v in pairs(self) do
		if (type(v) == "table" and v.OnInit) then
			v:OnInit();
			--System.Log("[AI] Initialising TPS queries for "..i);
		end
	end
end
