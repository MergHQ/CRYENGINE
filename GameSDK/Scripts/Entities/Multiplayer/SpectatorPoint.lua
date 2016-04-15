--------------------------------------------------------------------------
--	Crytek Source File.
-- 	Copyright (C), Crytek Studios, 2001-2007.
--------------------------------------------------------------------------
--	$Id$
--	$DateTime$
--	Description: Spectator Point
--  
--------------------------------------------------------------------------
--  History:
--  - 6:2:2007   12:00 : Created by Márcio Martins
--
--------------------------------------------------------------------------
SpectatorPoint =
{
	Client = {},
	Server = {},

	Editor={
		--Model="Editor/Objects/spawnpointhelper.cgf",
		Icon="spectator.bmp",
		DisplayArrow=1,
	},
	
	Properties=
	{
		bEnabled=1,
	},
}

--------------------------------------------------------------------------
function SpectatorPoint.Server:OnInit()
	self:Enable(tonumber(self.Properties.bEnabled)~=0);	
	self:SetFlags(ENTITY_FLAG_CLIENT_ONLY,0);
end

--------------------------------------------------------------------------
function SpectatorPoint.Client:OnInit()
	if (not CryAction.IsServer()) then
		self:Enable(tonumber(self.Properties.bEnabled)~=0);	
	end
	self:SetFlags(ENTITY_FLAG_CLIENT_ONLY,0);
end

----------------------------------------------------------------------------------------------------
function SpectatorPoint:Enable(enable)
	if (enable) then
		g_gameRules.game:AddSpectatorLocation(self.id);
	else
		g_gameRules.game:RemoveSpectatorLocation(self.id);
	end
	self.enabled=enable;
end

--------------------------------------------------------------------------
function SpectatorPoint.Server:OnShutDown()
	if (g_gameRules) then
		g_gameRules.game:RemoveSpectatorLocation(self.id);
	end
end

--------------------------------------------------------------------------
function SpectatorPoint.Client:OnShutDown()
	if (g_gameRules) and (not CryAction.IsServer()) then
		g_gameRules.game:RemoveSpectatorLocation(self.id);
	end
end

--------------------------------------------------------------------------
function SpectatorPoint:IsEnabled()
	return self.enabled;
end
