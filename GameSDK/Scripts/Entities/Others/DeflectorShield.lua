-- ===========================================================================
-- ===========================================================================
-- ===========================================================================
--
-- Deflector shield
--
-- A shield that will deflect projectiles back, in the opposite direction
-- of impact.
--
-- The shield defaults to 'invulnerable' but it is possible to give the
-- shield some health and make it vulnerable so that it will collapse
-- after taking a certain amount of damage.
--

Script.ReloadScript("scripts/Utils/EntityUtils.lua")

DeflectorShield =
{
	Server = {},

	Client = {},

	Properties =
	{
		object_Model = "objects/default/primitive_sphere.cgf",
		ParticleEffect="",
		MinDamage = 50,
		MaxDamage = 500,
		DropMinDistance = 10,
		DropPerMeter = 10,
		EnergyRadius = 0.4,
		EnergyDelay = 0.75,
		Spread = 0.1,
		HitType = "bullet",
		AmmoType = "deflectedblast",

		Physics = {		
			Density = -1,
			Mass = -1,
		},
	},

	Editor={
		Icon="Tornado.bmp",
	},
}


function DeflectorShield:CacheResources()	
	if (self.Properties.object_Model ~= "") then		
		self:LoadObject(0, self.Properties.object_Model)
	end
	self:PreLoadParticleEffect(self.Properties.ParticleEffect)
end



function DeflectorShield.Server:OnInit()
	self:OnReset();
end




function DeflectorShield.Client:OnInit()
	if (not CryAction.IsServer()) then
		self:OnReset();
	end
end


function DeflectorShield:OnSpawn()
	self:SetFromProperties();
end



function DeflectorShield:OnReset()
	self:PhysicalizeThis();
	self:AwakePhysics(0);
	
	self:Recharged();
end



function DeflectorShield:OnPropertyChange()
	self:SetFromProperties();
end



function DeflectorShield:PhysicalizeThis()
	local physics = self.Properties.Physics;
	EntityCommon.PhysicalizeRigid(self, 0, physics, 0);
end




function DeflectorShield:SetFromProperties()
	local properties = self.Properties;
	if (properties.object_Model == "") then
		return;
	end;

	self:LoadObject(0, properties.object_Model);
	self:PhysicalizeThis();
end


-- ===========================================================================
-- 	Die.
--
--	The deflector shield has died because of a hit with an other object.
--
--	In:		The hit context information.
--
function DeflectorShield:Die(hit)	
	self:Depleted();
end


----------------------------------------------------------------------------------------------------
function DeflectorShield.Server:OnHit(hit)	

	if (self.dead ~= nil) then
		if (self.dead) then
			return true;
		end
	end

	local damage = hit.damage;	

	if (self:IsInvulnerable()) then
		return true;
	end

	self.health = self.health - damage;

	if (self.health <= 0) then
		self:Die(hit);
	end

	return true;
end


-------------------------------------------------------
function DeflectorShield:OnLoad(table)		
	self.health = table.health;	
end


-------------------------------------------------------
function DeflectorShield:OnSave(table)  	
	table.health = self.health;
end


-- ===========================================================================
--	The deflector shield has been fully recharged.
--
--  If needed, the shield will has come 'alive' again.
--
function DeflectorShield:Recharged()
	self.health = self.Properties.Health.MaxHealth;
	self.dead = false;
	self:Hide(0);
end


-- ===========================================================================
--	The deflector shield has been fully depleted.
--
--  If needed, the shield will have 'died'.
--
--	Note: this function may still be called, even though the shield might have
--	been set to be invulnerable. This can happen for example when the owner of
--	the shield lowers it voluntarily.
--
function DeflectorShield:Depleted()
	self.health = 0.0;
	self.dead = true;
	self:Hide(1); -- Note: we are just hiding the entity so that it possible
	              -- to unhide it later on when the shield becomes Recharged()
				  -- again (which might be required depending on game-play
				  -- mechanics).
end


function DeflectorShield:Event_Hide()
	self:Depleted();
end



function DeflectorShield:Event_UnHide()
	self:Recharged();
end



DeflectorShield.FlowEvents =
{
	Inputs =
	{
		Hide = { DeflectorShield.Event_Hide, "bool" },
		UnHide = { DeflectorShield.Event_UnHide, "bool" },
	}
}


MakeKillable(DeflectorShield); -- Not strictly 'killable' but by giving the deflector shield a health meter we can make it 'collapsible'.
                               -- It is up to the owner of the shield on how such events should be handled in more detail.