Boid = 
{
	Properties = {},
	
	type = "Boid",

	Client = {},
	Server = {},
}

----------------------------------------------------------------------------------------------------
function Boid.Client:OnHit(hit)
	if (self.flock_entity and not self.bWasHit) then
		self.bWasHit = 1;
		Boids.OnBoidHit( self.flock_entity,self,hit );
	end
end

----------------------------------------------------------------------------------------------------
function Boid.Server:OnHit(hit)
	if (self.flock_entity and not self.bWasHit) then
		self.bWasHit = 1;
		Boids.OnBoidHit( self.flock_entity,self,hit );
	end
end

----------------------------------------------------------------------------------------------------
function Boid:OnPickup(bPickup, fThrowSpeed)
	Boids.OnPickup(self.flock_entity, self, bPickup, fThrowSpeed);
end

----------------------------------------------------------------------------------------------------
function Boid:IsUsable(user)
	if (Boids.CanPickup(self.flock_entity, self)) then
		return 725725;	-- Magic number to identify item pickups from interactor
	end
end

----------------------------------------------------------------------------------------------------
function Boid:GetUsableMessage()
	return Boids.GetUsableMessage(self.flock_entity, self);
end