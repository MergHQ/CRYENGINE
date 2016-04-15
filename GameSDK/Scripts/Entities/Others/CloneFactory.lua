local cloneTables = {
	"Properties",
	"PropertiesInstance",
	"Events"
}

CloneFactory =
{
	Properties =
	{
		Radius = 1.0,
		strSourceEntity = "",
		strDestEntity = "",
		bSafetyLock = 1,
		groupid = -1,
		bConnectOnDeath = 1,
		bMaterial = 0,
	},

	Editor =
	{
		Icon = "territory.bmp",
	},

	myLocation = {x=0,y=0,z=0},
	spawnLocation = {x=0,y=0,z=0},
	spawnOrientation = {x=1,y=0,z=0},
}

for _, table in pairs(cloneTables) do
	CloneFactory.Properties["bClone"..table] = 1
end

function CloneFactory:OnSpawn()
	self:OnReset();
end

function CloneFactory:OnReset()
	if not System.GetEntityByName( self.Properties.strSourceEntity ) then
		LogWarning( "Clone Factory cannot find entity called %s", self.Properties.strSourceEntity )
	end
	self.clone = nil
	self.spawnParams = nil
end

function CloneFactory:Clone()
	local spawnLocation = self.spawnLocation
	local spawnOrientation = self.spawnOrientation
	local myLocation = self.myLocation
	local props = self.Properties

	local radius = math.random() * props.Radius
	local phi = math.random() * math.pi
	local theta = math.random() * math.pi * 2

	self:GetPos(myLocation)

	if (props.bSafetyLock and props.bSafetyLock ~= 0) then
		local numEntities = table.getn( System.GetEntitiesInSphere(myLocation, props.Radius) )
		if numEntities > 1 then
			LogWarning( "There are %d entities in the spawn area... ignoring clone request", numEntities-1 )
			return
		end
	end

	spawnLocation.x = myLocation.x + radius * math.cos(theta) * math.sin(phi)
	spawnLocation.y = myLocation.y + radius * math.sin(theta) * math.sin(phi)
	spawnLocation.z = myLocation.z + radius * math.cos(theta)

	self:GetDirectionVector(1, spawnOrientation)

	if self.clone then
		assert( self.spawnParams )
		local ent = System.SpawnEntity( self.spawnParams )
		for _, tblName in pairs(cloneTables) do
			if props["bClone"..tblName] ~= 0 then
				ent[tblName] = new(self.clone[tblName])
			end
		end
		-- BEGIN AI HACK
		if self.Properties.groupid > 0 then
			ent.PropertiesInstance.groupid = self.Properties.groupid
		end
		-- END AI HACK
		-- BEGIN ONDEATH HACK
		if self.Properties.bConnectOnDeath == 1 then
			if not ent.Events then ent.Events = {} end
			if not ent.Events["OnDeath"] then ent.Events["OnDeath"] = {} end
			table.insert(ent.Events["OnDeath"], {self.id, "CloneDied"})
		end
		-- END ONDEATH HACK
		-- BEGIN MATERIAL HACK
		if self.Properties.bMaterial == 1 and self.material then
			ent.SetMaterial( ent, self.material )
		end
		-- END MATERIAL HACK
		ent:OnReset()
		Log("Name: %s", ent:GetName())
	else
		local ent = System.GetEntityByName( props.strSourceEntity )
		assert( ent )
		self.clone = {}
		for _, tblName in pairs(cloneTables) do
			if props["bClone"..tblName] ~= 0 then
				self.clone[tblName] = new(ent[tblName])
			end
		end

		if self.clone.Properties.bEnable then
			self.clone.Properties.bEnable = 1
		end

		-- BEGIN MATERIAL HACK
		if self.Properties.bMaterial == 1 then
			self.material = ent.GetMaterial(ent)
		end
		-- END MATERIAL HACK

		self.spawnParams = {
			class = ent.class,
			name = ent:GetName(),
			position = self.spawnLocation, -- tricky... cached vector gets linked in here
			scale = ent:GetScale(),
			orientation = self.spawnOrientation, -- ditto tricky
		}

		if props.strDestEntity ~= "" then
			self.spawnParams.name = props.strDestEntity
			self:Clone()
		else
			ent:OnReset()
		end
	end
end

function CloneFactory:Event_Clone()
	self:Clone()
end

function CloneFactory:Event_CloneDied()
	BroadcastEvent( self, "CloneDied" )
end

CloneFactory.FlowEvents =
{
	Inputs =
	{
		Clone = { CloneFactory.Event_Clone, "bool" },
		CloneDied = { CloneFactory.Event_CloneDied, "bool" },
	},
	Outputs =
	{
		Clone = { CloneFactory.Event_Clone, "bool" },
		CloneDied = { CloneFactory.Event_CloneDied, "bool" },
	},
}
