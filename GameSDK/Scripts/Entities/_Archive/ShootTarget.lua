ShootTarget = {
	Properties = {
		objectModel="Objects/Natural/Bushes/cliff_bush_a1.cgf",
		bPhysicalize = 1,
		sName = "shootTarget",
		fDensity = -1,
		fMass = -1
	},
	nShootCount = 0,
	nContactCount = 0,
	nLog = 0,
	fDamage = 0,
	fCollisionDamage = 0,

	-- This is the size of the damage list/queue that keeps the history
	-- of damages to calculate average time
	nDamageQueueSize = 100,
};

function ShootTarget:Event_StartAnimation(sender)
	self:StartAnimation(0,self.Properties.Animation);
end

function ShootTarget:Event_StopAnimtion(sender)
	self:ResetAnimation(0, -1);
end


function ShootTarget:OnReset()
	if (self.Properties.objectModel ~= "") then
		self:LoadObject(self.Properties.objectModel,0,1);
		self:DrawObject(0,1);
	end

	-- Not rigid body, not character.
	-- Make simple static physics.
	if (self.Properties.bPhysicalize) then
		self:CreateStaticEntity( self.Properties.fMass,-1 );
	end
	self.nShootCount = 0;
	self.nContactCount = 0;
	self.fDamage = 0;
	self.fCollisionDamage = 0;
end

function ShootTarget:OnContact(Hit)
	self.nContactCount = self.nContactCount+1;
	local dmg=Hit.CollisionDmg;
	if dmg then
		self.fCollisionDamage = self.fCollisionDamage + dmg;
	end
	local dam=format("%.2f",self.fCollisionDamage);
	if Hud then
		Hud:AddMessage("Collided with " .. self.Properties.sName .. " "  .. self.nContactCount .. " times, total damage ".. dam);
	end
		
	if (self.nLog == 1) then
		System.Log ("Dumping the hit structure:");
		for name,value in pairs(Hit) do
			System.Log( " " .. name);
		end
		self.nLog = 0
	end
end 

function ShootTarget:OnDamage(Hit)
	self.nShootCount = self.nShootCount+1;
	self.fDamage = self.fDamage + Hit.damage;
	local dam=format("%.2f",Hit.damage);
	local totaldam=format("%.2f",self.fDamage);
	if Hud then
		Hud:AddMessage("Shoot " .. self.nShootCount .. " times damage=" .. dam .. " totaldamage=" .. totaldam);
	end
	if (self.nLog == 1) then
		System.Log ("Dumping the hit structure from OnDamage:");
		for name,value in pairs(Hit) do
			System.Log(" " .. name .. " = ");
		end
		self.nLog = 0
	end
end

function ShootTarget:OnPropertyChange()
	self:OnReset();
end

function ShootTarget:OnInit()
	self:OnReset();
end

function ShootTarget:OnShutDown()
end

function ShootTarget:OnSave(stm)
end

function ShootTarget:OnLoad(stm)
end

ShootTarget.FlowEvents =
{
	Inputs =
	{
		StartAnimation = { ShootTarget.Event_StartAnimation, "bool" },
		StopAnimation = { ShootTarget.Event_StopAnimation, "bool" },
	},
	Outputs =
	{
		StartAnimation = "bool",
		StopAnimation = "bool",
	},
}
