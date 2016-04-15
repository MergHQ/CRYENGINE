Script.ReloadScript("scripts/Utils/EntityUtils.lua")

BreakableObject =
{
	Properties =
	{
		objModel = "objects/default/primitive_box.cgf",
		fDensity = 5000,
		fMass = -1,
		bResting = 0, -- If rigid body is originally in resting state.
		bRigidBody = 0,
		bPickable = 0,
		bUsable = 0,
		nBreakableType = 0,

		PhysicsBuoyancy=
		{
			water_density = 1,
			water_damping = 1.5,
			water_resistance = 0,	
		},
		
		PhysicsSimulation=
		{
			max_time_step = 0.01,
			sleep_speed = 0.04,
			damping = 0,
		},
		

		PhysicsBreakable=
		{
			max_push_force = 0.01,
			max_pull_force = 0.01,
			max_shift_force = 0.01,
			max_twist_torque = 0.01,
			max_bend_torque = 0.01,
			crack_weaken = 0.5,

			GroundPlanes=
			{
				positiveX = 0,
				negativeX = 0,
				positiveY = 0,
				negativeY = 0,
				positiveZ = 0.05,
				negativeZ = 0,
			},
		},
	},
	
	Editor=
	{
		Icon = "physicsobject.bmp",
		IconOnTop=1,
	},
}


-------------------------------------------------------
function BreakableObject:OnSave(table)
  table.first_break = self.first_break
end

-------------------------------------------------------
function BreakableObject:OnLoad(table)
  self.first_break = table.first_break;
end

----------------------------------------------------------------------------------------------------
function BreakableObject:OnSpawn()
	self:OnReset();
end

----------------------------------------------------------------------------------------------------
function BreakableObject:OnReset()
	self:ResetOnUsed()
	
	self:LoadObject(0, self.Properties.objModel);
	self:DrawSlot(0, 1);
	
	local physType;
	if (tonumber(self.Properties.bRigidBody) ~= 0) then
		physType = PE_RIGID;
	else
		physType = PE_STATIC;
	end

	local props = self.Properties;
	
	self:Physicalize(0, physType, { mass = props.fMass, density = props.fDensity,});
	self:LoadObjectLattice(0);
	self:SetPhysicParams(PHYSICPARAM_SUPPORT_LATTICE, props.PhysicsBreakable);
	self:SetPhysicParams(PHYSICPARAM_SIMULATION, props.PhysicsSimulation);
	self:SetPhysicParams(PHYSICPARAM_BUOYANCY, props.PhysicsBuoyancy);
	self:SetPhysicParams(PHYSICPARAM_PART_FLAGS, {partId = 0, mat_breakable = props.nBreakableType});
	
	local planes = props.PhysicsBreakable.GroundPlanes;
	local plane = 0;
	
	if (planes.positiveX ~= 0) then
		self:SetGroundPlane(plane, 1, {x=1, y=0, z=0}, planes.positiveX);
		plane = plane+1;
	end
	if (planes.negativeX ~= 0) then
		self:SetGroundPlane(plane, 1, {x=-1, y=0, z=0}, planes.negativeX);
		plane = plane+1;
	end
	if (planes.positiveY ~= 0) then
		self:SetGroundPlane(plane, 2, {x=0, y=1, z=0}, planes.positiveY);
		plane = plane+1;
	end
	if (planes.negativeY ~= 0) then
		self:SetGroundPlane(plane, 2, {x=0, y=-1, z=0}, planes.negativeY);
		plane = plane+1;
	end
	if (planes.positiveZ ~= 0) then
		self:SetGroundPlane(plane, 3, {x=0, y=0, z=1}, planes.positiveZ);
		plane = plane+1;
	end
	if (planes.negativeZ ~= 0) then
		self:SetGroundPlane(plane, 3, {x=0, y=0, z=-1}, planes.negativeZ);
		plane = plane+1;
	end
	
	if (tonumber(self.Properties.bResting) ~= 0) then
		self:AwakePhysics(0);
	else
		self:AwakePhysics(1);
	end
	
	local wmin, wmax = self:GetWorldBBox(g_Vectors.temp_v1, g_Vectors.temp_v2)
	self.radius = 0.15 + math.max(wmax.x - wmin.x, math.max(wmax.y - wmin.y, wmax.z - wmin.z))
	self.center = { x = (wmax.x + wmin.x) * 0.5, y = (wmax.y + wmin.y) * 0.5, z = (wmax.z + wmin.z) * 0.5 };
	
	self.first_break = true;
end

----------------------------------------------------------------------------------------------------
function BreakableObject:SetGroundPlane(index, axis, n, distance)
	local wmin, wmax = self:GetLocalBBox();
	local size = { x=wmax.x-wmin.x, y=wmax.y-wmin.y, z=wmax.z-wmin.z };
	local c = { x=(wmax.x+wmin.x)*0.5, y=(wmax.y+wmin.y)*0.5, z=(wmax.z+wmin.z)*0.5 };
	local org = { x=c.x+n.x*size.x*(distance-0.5), y=c.y+n.y*size.y*(distance-0.5), z=c.z+n.z*size.z*(distance-0.5) };
	
	self:SetPhysicParams(PHYSICPARAM_GROUND_PLANE, { origin = org, normal = n, plane_index = index});
end

----------------------------------------------------------------------------------------------------
function BreakableObject:OnPropertyChange()
	self:OnReset();
end

----------------------------------------------------------------------------------------------------
function BreakableObject:OnShutDown()
end

----------------------------------------------------------------------------------------------------
function BreakableObject:OnPhysicsBreak()
	self:Event_OnBreak();
end


----------------------------------------------------------------------------------------------------
function BreakableObject:OnPhysicsBreak()
	AI.BreakEvent(self.id, self.center, self.radis);
	
	self:Event_OnBreak();
	
	if (self.first_break) then
		self:Event_OnFirstBreak();
		self.first_break = false;
	end
end

----------------------------------------------------------------------------------------------------
function BreakableObject:Event_OnBreak()
	AI.BreakEvent(self.id, self.center, self.radis);
	
	BroadcastEvent( self,"OnBreak" );
end

----------------------------------------------------------------------------------------------------
function BreakableObject:Event_OnFirstBreak()
	BroadcastEvent( self,"OnFirstBreak" );
end

BreakableObject.FlowEvents =
{
	Inputs =
	{
		OnBreak = { BreakableObject.Event_OnBreak, "bool" },
		OnFirstBreak = { BreakableObject.Event_OnFirstBreak, "bool" },
	},
	Outputs =
	{
		OnBreak = "bool",
		OnFirstBreak = "bool",
	},
}

MakeUsable(BreakableObject);
MakePickable(BreakableObject);
MakeTargetableByAI(BreakableObject);
MakeKillable(BreakableObject);
