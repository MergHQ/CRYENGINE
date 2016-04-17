Script.ReloadScript("scripts/gamerules/GameRulesUtils.lua");

CaptureTheFlag = {};

GameRulesSetStandardFuncs(CaptureTheFlag);

-- TODO [tlh] would be best if we didn't need this
CaptureTheFlag.TeamInfo = 
{
	{
		DbgTeamName = "North Korea",
		Flag = nil,
		Base = nil,
	},
	{
		DbgTeamName = "USA",
		Flag = nil,
		Base = nil,
	},
}

----------------------------------------------------------------
function CaptureTheFlag:SetupPlayerTeamSpecifics(localActorId)
	assert(localActorId);

	local localTeam = self.game:GetTeam(localActorId);

	Log("[tlh] @ CaptureTheFlag:SetupPlayerTeamSpecifics(), localTeam = " .. localTeam);

	self:RecolourEntitiesOfClass("CTFFlag", localTeam, localActorId);
end

----------------------------------------------------------------
function CaptureTheFlag:RecolourEntitiesOfClass(className, playerTeamId, localActorId)
	Log ("CaptureTheFlag:RecolourEntitiesOfClass(\"" .. className .. "\")");

	local entities=System.GetEntitiesByClass(className);

	if (entities) then
		for i,v in pairs(entities) do
			if (v.id ~= localActorId) then
				self:RecolourTeamEntity(v, playerTeamId);
			end
		end
	end
end

----------------------------------------------------------------
function CaptureTheFlag:RecolourTeamEntity(entity, playerTeamId)
	local teamId = self.game:GetTeam(entity.id);
	Log ("CaptureTheFlag:RecolourTeamEntity - class=\"" .. entity.class .. "\" name=\"" .. entity:GetName() .. "\" teamId=" .. teamId .. " playerTeamId=" .. playerTeamId);

	if (entity.class == "CTFFlag") then
		entity:LoadCorrectGeometry();
	end
end
