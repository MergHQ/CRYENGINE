-- something for AI to carry - model must have been exported with weapon bone
-- after creating lua need to place path in Editor\EntityRegistry.xml (in Artwork SS)
AICrate = {
	Name = "AICrate",
	Properties = {
		objModel = "Objects/Indoor/boxes/crates/crateCarry.cgf",
		Density = 20,
		Mass = 1,
	},

	PhysParams= {
		max_time_step = 0.01,
		sleep_speed = 0.04,
		damping = 0,
		water_damping = 0,
		water_resistance = 1000,	
		water_density = 1000,
		mass = 1,
		density = 20,
	},
}

------------------------------------------------------------------------------------------------------
function AICrate:OnPropertyChange()
	if (self.ModelName ~= self.Properties.objModel) then
		self.ModelName = self.Properties.objModel;
		self:LoadObject( self.ModelName, 0,1 );
	end
	-- see ScriptObjectEntity 
	self:CreateRigidBody( self.Properties.Density,self.Properties.Mass,0 );
	self:EnablePhysics( 1 );

	self.PhysParams.mass=self.Properties.Mass;
	self.PhysParams.density=self.Properties.Density;

	self:SetPhysicParams(PHYSICPARAM_SIMULATION, self.PhysParams );
	self:SetPhysicParams(PHYSICPARAM_BUOYANCY, self.PhysParams );

	self:AwakePhysics(0);
	self:DrawObject(0,1); --param nPos(slot number), nMode(0 = Don't draw, 1 = Draw normally, 3 = Draw near)
end

------------------------------------------------------------------------------------------------------
function AICrate:OnInit()
	self:Activate( 0 );
	self:SetUpdateType( eUT_Physics ); 

	self:OnPropertyChange();
--	self:RegisterWithAI(AIAnchor.AIOBJECT_CARRY_CRATE);
	AI:RegisterWithAI(self.id, AIAnchor.AIOBJECT_CARRY_CRATE);
end
------------------------------------------------------------------------------------------------------
function AICrate:OnEvent( id, params)
	
end
------------------------------------------------------------------------------------------------------
function AICrate:OnShutDown()
end
