----------------------------------------------------------------------------------------------------
--  Crytek Source File.
--  Copyright (C), Crytek Studios, 2001-2004.
----------------------------------------------------------------------------------------------------
--  $Id$
--  $DateTime$
--  Description: Explosion Entity
--
----------------------------------------------------------------------------------------------------
--  History:
--  - 15:8:2005   16:46 : Created by Márcio Martins
--
----------------------------------------------------------------------------------------------------
Explosion =
{
	Properties =
	{
		bActive = 1,
		soclasses_SmartObjectClass = "",
		Explosion =
		{
			ParticleEffect = "explosions.grenade_air.explosion",
			EffectScale = 1,
			MinRadius = 5,
			Radius = 10,
			MinPhysRadius = 2.5,
			PhysRadius = 5,
			Pressure = 1000,
			Damage = 1000,
			Direction = {x=0, y=0, z=1},
		},
	},

	Editor=
	{
		Icon = "explosion.bmp",
		IconOnTop = 1,
	}
};


----------------------------------------------------------------------------------------------------
function Explosion:OnPropertyChange()
	self:OnReset();
end


----------------------------------------------------------------------------------------------------
function Explosion:OnInit()
	self:OnReset();
	self:PreLoadParticleEffect( self.Properties.Explosion.ParticleEffect );
end


----------------------------------------------------------------------------------------------------
function Explosion:OnReset()
	self:SetFlags(ENTITY_FLAG_CLIENT_ONLY,0);
	self.bActive = self.Properties.bActive;
end


----------------------------------------------------------------------------------------------------
function Explosion:OnSave(save)
	save.bActive = self.bActive;
end


----------------------------------------------------------------------------------------------------
function Explosion:OnLoad(saved)
	self.bActive = saved.bActive;
end


----------------------------------------------------------------------------------------------------
function Explosion:Explode(shooterId)
	local expl = self.Properties.Explosion;
	g_gameRules:CreateExplosion(self.id, self.id, expl.Damage, self:GetWorldPos(), expl.Direction, expl.Radius, nil, expl.Pressure, expl.HoleSize, expl.ParticleEffect, expl.EffectScale, expl.MinRadius, expl.MinPhysRadius, expl.PhysRadius, 4);
	BroadcastEvent(self, "Explode");
end


------------------------------------------------------------------------------------------------------
-- Event Handlers
------------------------------------------------------------------------------------------------------

function Explosion:Event_Explode(sender)
	if (tonumber(self.bActive) ~= 0) then
		self:Explode(NULL_ENTITY);
	end
end

------------------------------------------------------------------------------------------------------
function Explosion:Event_Activate(sender)
	self.bActive = 1;
	BroadcastEvent(self, "Activate");
end

------------------------------------------------------------------------------------------------------
function Explosion:Event_Deactivate(sender)
	self.bActive = 0;
	BroadcastEvent(self, "Deactivate");
end


Explosion.FlowEvents =
{
	Inputs =
	{
		Activate = { Explosion.Event_Activate, "bool" },
		Deactivate = { Explosion.Event_Deactivate, "bool" },
		Explode = { Explosion.Event_Explode, "bool" },
	},
	Outputs =
	{
		Activate = "bool",
		Deactivate = "bool",
		Explode = "bool",
	},
}