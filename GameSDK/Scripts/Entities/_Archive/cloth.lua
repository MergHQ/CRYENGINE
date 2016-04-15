TestCloth = 
{
	Properties=
	{
		mass = 5,
		density = 200,

		fileModel="Objects/testcloth.cgf",
		gravity={x=0,y=0,z=-9.81},
		damping = 0,
		max_time_step = 0.025,
		sleep_speed = 0.1,
		thickness = 0.06,
		friction = 0,
		max_safe_step = 0.4,
		stiffness = 500,
		damping_ratio = 0.9,
		air_resistance = 0,
		wind = {x=0,y=0,z=0},
		max_iters = 40,
		accuracy = 0.01,
		water_resistance = 600,	
		impulse_scale = 0.02,
		explosion_scale = 0.002,
		collision_impulse_scale = 1.0,
		max_collision_impulse = 160,
		bCollideWithTerrain = 0,
		bCollideWithStatics = 1,
		bCollideWithPhysical = 1,
		bCollideWithPlayers = 1,
	},
	id_attach_to = -1,
	id_part_attach_to = -1,

	Editor=
	{
		Model="Objects/Editor/LightSphear.cgf",
	},
}

function TestCloth:OnReset()
	self.lightId=nil;
	self:NetPresent(nil);
	self:Activate(1);
	
	self:LoadObject(self.Properties.fileModel,0,1,"cloth");
	self:CreateSoftEntity(self.Properties.mass,self.Properties.density, 1, self.id_attach_to,self.id_part_attach_to);
  self:SetPhysicParams(PHYSICPARAM_SIMULATION, self.Properties );
	self:SetPhysicParams(PHYSICPARAM_BUOYANCY, self.Properties );
	self:SetPhysicParams(PHYSICPARAM_SOFTBODY, self.Properties );
	local CollParams = { collision_mask = 0 };
	if self.Properties.bCollideWithTerrain==1 then CollParams.collision_mask = CollParams.collision_mask+ent_terrain; end
	if self.Properties.bCollideWithStatics==1 then CollParams.collision_mask = CollParams.collision_mask+ent_static; end
	if self.Properties.bCollideWithPhysical==1 then CollParams.collision_mask = CollParams.collision_mask+ent_rigid+ent_sleeping_rigid; end
	if self.Properties.bCollideWithPlayers==1 then CollParams.collision_mask = CollParams.collision_mask+ent_living; end
	self:SetPhysicParams(PHYSICPARAM_SOFTBODY, CollParams );
	self:DrawObject(0, 1);
	self:AwakePhysics(0);
end

function TestCloth:OnPropertyChange()
	self:OnReset();
end

function TestCloth:OnDamage( hit )

--	System.LogToConsole("dir="..hit.dir.x..","..hit.dir.y..","..hit.dir.z);
--	System.LogToConsole("pos="..hit.pos.x..","..hit.pos.y..","..hit.pos.z);
--	System.LogToConsole("impact_force="..hit.impact_force_mul);

	if( hit.ipart ) then
		self:AddImpulse( hit.ipart, hit.pos, hit.dir, hit.impact_force_mul );
	else	
		self:AddImpulse( -1, hit.pos, hit.dir, hit.impact_force_mul );
	end	
end

function TestCloth:OnInit()
	self:SetUpdateType( eUT_Visible );
	self:OnReset();
end


function TestCloth:OnShutDown()
end
