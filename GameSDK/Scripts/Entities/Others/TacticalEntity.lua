TacticalEntity =
{
	Editor =
	{
		Icon="Comment.bmp",
	},
	Properties =
	{
		fileModel				= "",
		ModelSubObject	= "",
		bEnabled				= 1,
		bPhysicalize		= 0,
		bAutoTarget			= 0,

		Physics = {
			bRigidBody			= 1,
			bRigidBodyActive= 1,
			bResting				= 1,
			Density					= -1,
			Mass						= 300,
			Buoyancy=
			{
				water_density			= 1000,
				water_damping			= 0,
				water_resistance	= 1000,	
			},
		},
		TacticalInfo =
		{
			LookupName = "",
		},
	},
	registeredForAutoTarget = 0,
}

--------------------------------------------------------------------------
function TacticalEntity:OnInit()
	self:OnReset();
	Game.AddTacticalEntity(self.id, eTacticalEntity_Story);
end

--------------------------------------------------------------------------
function TacticalEntity:OnPropertyChange()
	self:OnReset();
end

--------------------------------------------------------------------------
function TacticalEntity:EnableAutoTarget()
	local props = self.Properties;

	if ((props.bAutoTarget ~= 0) and (self.registeredForAutoTarget == 0)) then
		local innerRadiusVolumeFactor = 0.35;
		local outerRadiusVolumeFactor = 0.6;
		local snapRadiusVolumeFactor = 1.25;
		Game.RegisterWithAutoAimManager(self.id, innerRadiusVolumeFactor, outerRadiusVolumeFactor, snapRadiusVolumeFactor);
		self.registeredForAutoTarget = 1;
	end;
	if ((props.bAutoTarget == 0) and (self.registeredForAutoTarget ~= 0)) then
		Game.UnregisterFromAutoAimManager(self.id);
		self.registeredForAutoTarget = 0;
	end;
end;

--------------------------------------------------------------------------
function TacticalEntity:DisableAutoTarget()
	if (self.registeredForAutoTarget ~= 0) then
		Game.UnregisterFromAutoAimManager(self.id);
		self.registeredForAutoTarget = 0;
	end;
end;

--------------------------------------------------------------------------
function TacticalEntity:OnReset()

	local props=self.Properties;
	if(not EmptyString(props.fileModel))then

		self:LoadSubObject(0,props.fileModel,props.ModelSubObject);

		if(props.bPhysicalize==1)then
			self:PhysicalizeThis(0);
		end;

	end;

	self:EnableAutoTarget();
end

--------------------------------------------------------------------------
function TacticalEntity:PhysicalizeThis(slot)
	local physics = self.Properties.Physics;
	EntityCommon.PhysicalizeRigid( self,slot,physics,1 );
	
	if (physics.Buoyancy) then
		self:SetPhysicParams(PHYSICPARAM_BUOYANCY, physics.Buoyancy);
	end
end;

--------------------------------------------------------------------------
function TacticalEntity:OnDestroy()
	self:DisableAutoTarget();
	Game.RemoveTacticalEntity(self.id, eTacticalEntity_Story);
end

--------------------------------------------------------------------------

function TacticalEntity:Event_Enable()
	self.Properties.bEnabled = 1;
	self:EnableAutoTarget();
	Game.AddTacticalEntity(self.id, eTacticalEntity_Story);
end

--------------------------------------------------------------------------

function TacticalEntity:Event_Disable()
	self.Properties.bEnabled = 0;
	self:DisableAutoTarget();
	Game.RemoveTacticalEntity(self.id, eTacticalEntity_Story);
end

--------------------------------------------------------------------------

function TacticalEntity:HasBeenScanned()
	self:ActivateOutput("Scanned", true);
end

--------------------------------------------------------------------------
function TacticalEntity:OnSave(table)
	table.registeredForAutoTarget = self.registeredForAutoTarget
end

--------------------------------------------------------------------------

function TacticalEntity:OnLoad(table)
	local wasRegisteredForAutoTarget = table.registeredForAutoTarget;
	if(wasRegisteredForAutoTarget == 1) then
		self:EnableAutoTarget();
	else
		self:DisableAutoTarget();
	end
end

--------------------------------------------------------------------------

TacticalEntity.FlowEvents =
{
	Inputs =
	{
		Enable = { TacticalEntity.Event_Enable, "bool" },
		Disable = { TacticalEntity.Event_Disable, "bool" },
	},
	Outputs =
	{
		Scanned        = "bool",
	},
}
