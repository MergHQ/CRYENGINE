SwingingObject = 
{
	Properties=
	{
		mass = 120,

		fileModel="Objects/glm/shipwreck/stuff/chain_hook.cgf",
		--		"Objects/Indoor/lights/light6phys.cgf",
		bShootable=1,
		coll_dist=0.025,
		material="mat_default",
		gravity={x=0,y=0,z=-9.81},
		damping=0.5,
		sleep_speed=0.01,
		max_time_step=0.02,
		damage_players=1,
		damage_scale = 10,
		bResting=1,
		vector_activation_impulse = {x=1,y=2,z=3}, -- Impulse to apply at event.
	},

	Editor=
	{
		Model="Objects/Editor/LightSphear.cgf",
	},
}

function SwingingObject:OnReset()

	self.lightId=nil;
	self:NetPresent(nil);
	self:Activate(1);
	
	local RopeParams = {
		entity_id_1 = -1,
		entity_part_id_1 = 0,
		surface_idx = Game:GetMaterialIDByName(self.Properties.material),
		shootable = self.Properties.bShootable,
	};
	
	self:CreateRigidBody(0,1,0);
	self:LoadCharacter(self.Properties.fileModel,0);
	self:PhysicalizeCharacter(self.Properties.mass,0,0,0);
	self:SetCharacterPhysicParams(0,"rope", PHYSICPARAM_ROPE,self.Properties);
	self:SetCharacterPhysicParams(0,"rope", PHYSICPARAM_ROPE,RopeParams);
	self:SetCharacterPhysicParams(0,"rope", PHYSICPARAM_SIMULATION,self.Properties);
	self:AwakePhysics(1-self.Properties.bResting);
	self:DrawObject(0, 1);
end

function SwingingObject:OnPropertyChange()
	self:OnReset();
end

function SwingingObject:OnDamage( hit )

--	System.LogToConsole("dir="..hit.dir.x..","..hit.dir.y..","..hit.dir.z);
--	System.LogToConsole("pos="..hit.pos.x..","..hit.pos.y..","..hit.pos.z);
--	System.LogToConsole("impact_force="..hit.impact_force_mul);

	if( hit.ipart ) then
		self:AddImpulse( hit.ipart, hit.pos, hit.dir, hit.impact_force_mul );
	else	
		self:AddImpulse( -1, hit.pos, hit.dir, hit.impact_force_mul );
	end	
end

function SwingingObject:Event_AddImpulse(sender)
	self.temp_vec.x=self.Properties.vector_activation_impulse.x;
	self.temp_vec.y=self.Properties.vector_activation_impulse.y;
	self.temp_vec.z=self.Properties.vector_activation_impulse.z;
	self:AddImpulse(0,nil,self.temp_vec,1);
end

function SwingingObject:Event_Activate(sender)	
	self:AwakePhysics(1);
end


function SwingingObject:OnInit()
	self:OnReset();
end

function SwingingObject:OnWrite( stm )
end

function SwingingObject:OnRead( stm )
end

function SwingingObject:OnSave( stm )
end

function SwingingObject:OnLoad( stm )
end 

function SwingingObject:OnShutDown()
end

SwingingObject.FlowEvents =
{
	Inputs =
	{
		Activate = { SwingingObject.Event_Activate, "bool" },
		AddImpulse = { SwingingObject.Event_AddImpulse, "bool" },
	},
	Outputs =
	{
		Activate = "bool",
		AddImpulse = "bool",
	},
}
