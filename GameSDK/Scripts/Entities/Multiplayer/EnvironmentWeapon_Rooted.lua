----------------------------------------------------------------------------------------------------
--  Crytek Source File.
--  Copyright (C), Crytek Studios, 2011
----------------------------------------------------------------------------------------------------
--  $Id$
--  $DateTime$
--  Description: Weaponised environmental object, rooted to ground until use
--
----------------------------------------------------------------------------------------------------
--  History:
--  - 25:07:2011  : Created by JonathanBunner
--
----------------------------------------------------------------------------------------------------


-- Friendly fire
FriendlyFire = 
{
	None		= 0,
	SelfNotAffected = 1,
	TeamNotAffected	= 2,
};

Classifications = 
{
	unspecified = 0,
	panel = 1,
	pole = 2,
	other = 3,
}

EnvironmentalWeapon = {
	Server = {},
	Client = {},
	Properties = {
			szUseMessage		="@pick_object",
			szMFXLibrary		="melee", -- Placeholder for now. require (art/design?) to setup data for sounds/material impacts and set library name 
			fileModelOverride 	= "",
			bPickable			= 1,
			szRootedGrabAnimTagOverride = "",
			szGrabAnimTagOverride = "",
			bInitiallyRooted			= 1,
			RagdollPostMeleeImpactSpeed = 20.0, -- Adding to lua rather than xml as could have several pole/panel weapons using SAME xml setup, but want slightly different 'impact' results from each (e.g. diff material panels / poles)
			RagdollPostThrowImpactSpeed = 17.0,
			classification = "",
			RootedAngleMin = 0,  -- Specified in degrees for half angle, from the front. 0-180
			RootedAngleMax = 180,  -- Specified in degrees for half angle, from the front. 0-180

			initialHealth				= 800,
			damageable					= 0, 
		
			-- Time after which an unused environmental weapon will de-spawn
			idleKillTime				= -1.0, -- -1 indicates do not despawn
			
			States = {
			-- empty by default
			},
			
			-- Damage protection
			shieldsFromExplosionConeAngle				   = 0, -- zero by default 

			collisionDamageScale = 0.5,		-- take 50% damage from collisions
			explosionMinDamageToDestroy = 100, --Explosions causing >= this damage will remove all remaining health
	},
	Editor = {
		Icon		= "Item.bmp",	
		IconOnTop	= 1,
		ShowBounds  = 1,
	},
	ModelSlot = -1,
	currentHealth  = 800,
	ownerID = NULL_ENTITY, 
	lowestThresholdFractionReached    = 1.1,
	IsRooted = true,
	bCurrentlyPickable = 0, 
	teamID = -1,
}


EnvironmentalWeapon.physParams =
{
	flags = 0,
	mass = 250,
};

----------------------------------------------------------------------------------------------------

EnvironmentalWeapon.DEFAULT_FILE_MODEL = "Objects/props/c3mp_strfrn/c3mp_lamp_post/c3mp_sign_post.cgf";

----------------------------------------------------------------------------------------------------

function EnvironmentalWeapon:IsUsable(user)
		return self.Properties.bPickable and self.bCurrentlyPickable;
end

----------------------------------------------------------------------------------------------------

-- at the moment we are attached to a users hand we need to ensure we are no longer
-- fixed, and will react expectedly in the physics world
function EnvironmentalWeapon:OnAttached()
	self:Physicalize(self.ModelSlot, PE_RIGID, self.physParams );
	
	-- No longer rooted!
	self.IsRooted = false; 
end

----------------------------------------------------------------------------------------------------

function EnvironmentalWeapon:GetUsableMessage()
	return self.Properties.szUseMessage;
end

----------------------------------------------------------------------------------------------------
function EnvironmentalWeapon.Server:OnInit()
	if (not self.bInitialized) then
		self:OnReset();
		self.bInitialized = 1;
		self:SetViewDistRatio(255);
	end
end

----------------------------------------------------------------------------------------------------
function EnvironmentalWeapon:SetOwnerID(ownerID)
	self.ownerID = ownerID; 

	if (ownerID == NULL_ENTITY) then
		if (self.originalViewDistRatio ~= nil) then
			self:SetViewDistRatio(self.originalViewDistRatio);
		end
		self.teamID = -1;
	else
		self.originalViewDistRatio = self:GetViewDistRatio();
		self:SetViewDistRatio(255);

		if (g_gameRules.game:IsMultiplayer()) then
			self.teamID = g_gameRules.game:GetTeam(ownerID);
		end

	end
end

----------------------------------------------------------------------------------------------------
function EnvironmentalWeapon.Client:OnInit()
	if (not self.bInitialized) then
		self:OnReset();
		self.bInitialized = 1;
	end
end

----------------------------------------------------------------------------------------------------
function EnvironmentalWeapon:OnReset()

	local notAlreadyPreloaded = (self.ModelSlot == -1);

	--System.Log( "EnvironmentalWeapon:OnReset()  " .. tostring(notAlreadyPreloaded) .. " " .. EntityName(self));
	if (notAlreadyPreloaded) then
	
		self.ModelSlot = -1; 

		local fileModel = self.DEFAULT_FILE_MODEL;
		if ( self.Properties.fileModelOverride and (self.Properties.fileModelOverride ~= "") ) then
			fileModel = self.Properties.fileModelOverride;
		end
			-- Load standard model (primary undamaged model)
		self.ModelSlot = self:LoadObject(self.ModelSlot, fileModel );
		--System.Log( "Loading undamaged model" ..  tostring(self.ModelSlot));

		-- load any extra models required by damage states
		if (self.Properties.States ~= nil) then
			for i, v in ipairs(self.Properties.States) do
				if (v~= nil and type(v) == "table" and v.Model~=nil) then
					-- Preload the damage model into a slot and cache slot index
					v.Slot = self:LoadObject(-1,  v.Model );
					--System.Log( "Preloading model" ..v.Model );
					--System.Log( "Slot #" ..v.Slot );
					if(v~=-1) then
						self:DrawSlot(v.Slot, 0);
					end
				end
			end
		end

	end

	self.currentHealth = self.Properties.initialHealth;
	self.ownerID = NULL_ENTITY;
	self.lowestThresholdFractionReached = 1.1;
	self.IsRooted = self.Properties.bInitiallyRooted;

	if(self.IsRooted) then
		self:Physicalize( self.ModelSlot, PE_RIGID, { mass = 0 } );
		--System.Log( "physicalizing rooted" ..  tostring(self.ModelSlot))
	else
		self:Physicalize( self.ModelSlot, PE_RIGID, self.physParams );
		--System.Log( "physicalizing non rooted" ..  tostring(self.ModelSlot))
	end
	
	self.bCurrentlyPickable = self.Properties.bPickable; 

end

----------------------------------------------------------------------------------------------------
function EnvironmentalWeapon:OnPropertyChange()
	self:OnReset();
end

----------------------------------------------------------------------------------------------------
function EnvironmentalWeapon:OnSpawn()
	self:OnReset();
end

--------------------------------------------------------------------------
function  EnvironmentalWeapon.Server:OnHit(hitInfo)
	-- disable damage when attached still.. or when on floor
	if( self.Properties.damageable ~= 0 and self.IsRooted == false and self.ownerID ~= NULL_ENTITY) then
	--if( self.Properties.damageable ~= 0 and hitInfo.damage > 0) then -- use this check for easier testing :D
		
		if ((not g_gameRules.game:IsMultiplayer()) or g_gameRules.game:GetTeam(hitInfo.shooterId) ~= self.teamID) then
			local damageToTake = hitInfo.damage;

			if (hitInfo.type == "collision") then
				damageToTake = damageToTake * self.Properties.collisionDamageScale;
			elseif (hitInfo.damage >= self.Properties.explosionMinDamageToDestroy and hitInfo.type == "frag") then
				damageToTake = self.currentHealth;
			end

			--Adjust health
			local prevHealth = self.currentHealth;
			self.currentHealth   = self.currentHealth - damageToTake;
		
			-- Lets sync this
			Game.OnEnvironmentalWeaponHealthChanged(self.id, prevHealth, self.currentHealth); 
		
			-- If we are dead... 
			self:OnClientHealthChanged();
		end
	end
	
	-- we never schedule entity removal of environmental weapons from lua.. c++ takes care of that for us. 
	return false;
end

function EnvironmentalWeapon:OnClientHealthChanged()

	-- Always check if the health change requires us to update health state
	self:UpdateHealthState();
	
end

-- Just stubbing this out for now..
function EnvironmentalWeapon:UpdateHealthState()
	
	-- work out which state we are currently in
	local modelOverride			  = nil; 
	local lowestThresholdFraction = self.lowestThresholdFractionReached;
	local slotOverride			  = nil; 
	
	-- avoid div by zero
	if(self.Properties.initialHealth == nil) then
		return;
	end
	
	--Calc [0.0, 1.0] health fraction value
	local currentHealthFraction = self.currentHealth/self.Properties.initialHealth;
	--System.Log( "current health Fraction #"..currentHealthFraction );
	
	if (self.Properties.States ~= nil) then
	
		for i, v in ipairs(self.Properties.States) do
			if (v~= nil and type(v) == "table" and v.ThresholdFraction~=nil and v.Model~=nil) then
				if(currentHealthFraction <= v.ThresholdFraction  and v.ThresholdFraction <= lowestThresholdFraction) then
					modelOverride			= v.Model; 
					lowestThresholdFraction  = v.ThresholdFraction; 
					slotOverride = v.Slot; 
				end
			end
		end
		
		--System.Log( "local lowest #"..lowestThresholdFraction );
		--System.Log( "self.lowest  #"..self.lowestThresholdFractionReached );
		
		-- If our currentState is not equal to desired we have a transition to enact
		if( self.lowestThresholdFractionReached ~= lowestThresholdFraction ) then
			
			--System.Log( "new lowest lower than current.. changing state!" );
			--System.Log( "new model is"..modelOverride ); 
			
			local v, w = self:GetVelocityEx();

			-- Hide existing && show new
			self:DrawSlot(-1, 0);
			--System.Log( "Showing Slot #" ..slotOverride );
			self:DrawSlot(slotOverride, 1);
			
			-- For now.. just physicalise self.. need to look at this though.. are we in diff phys states if held/on floor etc?
			self:Physicalize(slotOverride, PE_RIGID, self.physParams );
			self.ModelSlot = slotOverride;
			--System.Log( "self.ModelSlot #" .. self.ModelSlot );

			self:SetVelocityEx(v, w);

			self.lowestThresholdFractionReached = lowestThresholdFraction; 
			
			-- Make sure we tell pick and throw weapon about this (so it can resetup our collision mass/properties not to collide with owner etc)
			if(self.ownerID ~= NULL_ENTITY) then
				--System.Log( "Require Physics Object Refresh" );
				local player =  System.GetEntity(self.ownerID);  
				if(player) then
					player.actor:RefreshPickAndThrowObjectPhysics();
				end
			end
		end
	
	end
end


function EnvironmentalWeapon:GetClassification()
	
	id = 0;

	id = Classifications[ self.Properties.classification ]
	if( self.Properties.classification == "" ) then
		id = Classifications.Unspecified
	elseif( id == nil ) then
		id = Classifications.Other
	end

	return id
end