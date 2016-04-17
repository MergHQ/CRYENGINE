-- AUTOMATICALLY GENERATED CODE
-- use sandbox (AI/Generate Spawner Scripts) to regenerate this file
Script.ReloadScript("Scripts/Entities/AI/Grunt.lua")
SpawnGrunt =
{
	spawnedEntity = nil,
	Properties =
	{
		commrange = 30,
		SpawnedEntityName = "",
		bSpeciesHostility = 1,
		soclasses_SmartObjectClass = "Actor",
		attackrange = 70,
		special = 0,
		aicharacter_character = "Cover",
		Perception =
		{
			stanceScale = 1.8,
			sightrange = 50,
			FOVSecondary = 160,
			FOVPrimary = 80,
			audioScale = 1,
		},
		species = 1,
		bInvulnerable = 0,
		accuracy = 1.0,
		fileModel = "",
	},
	PropertiesInstance =
	{
		aibehavior_behaviour = "Job_StandIdle",
		soclasses_SmartObjectClass = "",
		groupid = 173,
	},
}
SpawnGrunt.Properties.SpawnedEntityName = ""
function SpawnGrunt:Event_Spawn(sender,params)
	local params = {
		class = "Grunt",
		position = self:GetPos(),
		orientation = self:GetDirectionVector(1),
		scale = self:GetScale(),
		properties = self.Properties,
		propertiesInstance = self.PropertiesInstance,
	}
	if self.Properties.SpawnedEntityName ~= "" then
		params.name = self.Properties.SpawnedEntityName
	else
		params.name = self:GetName()
	end
	local ent = System.SpawnEntity(params)
	if ent then
		self.spawnedEntity = ent.id
		if not ent.Events then ent.Events = {} end
		local evts = ent.Events
		if not evts.Dead then evts.Dead = {} end
		table.insert(evts.Dead, {self.id, "Dead"})
		if not evts.Spawned then evts.Spawned = {} end
		table.insert(evts.Spawned, {self.id, "Spawned"})
	end
	BroadcastEvent(self, "Spawned")
end
function SpawnGrunt:OnReset()
	if self.spawnedEntity then
		System.RemoveEntity(self.spawnedEntity)
		self.spawnedEntity = nil
	end
end
function SpawnGrunt:GetFlowgraphForwardingEntity()
	return self.spawnedEntity
end
function SpawnGrunt:Event_Spawned()
	BroadcastEvent(self, "Spawned")
end
function SpawnGrunt:Event_Dead(sender, param)
	if sender and sender.id == self.spawnedEntity then BroadcastEvent(self, "Dead") end
end
function SpawnGrunt:Event_Disable(sender, param)
	if self.spawnedEntity and ((not sender) or (self.spawnedEntity ~= sender.id)) then
		local ent = System.GetEntity(self.spawnedEntity)
		if ent and ent ~= sender then
			self.Handle_Disable(ent, sender, param)
		end
	end
end
function SpawnGrunt:Event_Enable(sender, param)
	if self.spawnedEntity and ((not sender) or (self.spawnedEntity ~= sender.id)) then
		local ent = System.GetEntity(self.spawnedEntity)
		if ent and ent ~= sender then
			self.Handle_Enable(ent, sender, param)
		end
	end
end
function SpawnGrunt:Event_Kill(sender, param)
	if self.spawnedEntity and ((not sender) or (self.spawnedEntity ~= sender.id)) then
		local ent = System.GetEntity(self.spawnedEntity)
		if ent and ent ~= sender then
			self.Handle_Kill(ent, sender, param)
		end
	end
end
function SpawnGrunt:Event_Spawn(sender, param)
	if self.spawnedEntity and ((not sender) or (self.spawnedEntity ~= sender.id)) then
		local ent = System.GetEntity(self.spawnedEntity)
		if ent and ent ~= sender then
			self.Handle_Spawn(ent, sender, param)
		end
	end
end
function SpawnGrunt:Event_Spawned(sender, param)
	if sender and sender.id == self.spawnedEntity then BroadcastEvent(self, "Spawned") end
end
SpawnGrunt.FlowEvents =
{
	Inputs = 
	{
		Spawn = { SpawnGrunt.Event_Spawn, "bool" },
		Disable = { SpawnGrunt.Event_Disable, "bool" },
		Enable = { SpawnGrunt.Event_Enable, "bool" },
		Hide = { SpawnGrunt.Event_Hide, "bool" },
		Kill = { SpawnGrunt.Event_Kill, "bool" },
		Spawn = { SpawnGrunt.Event_Spawn, "bool" },
	},
	Outputs = 
	{
		Spawned = "bool",
		Dead = "bool",
		Spawned = "bool",
	}
}
SpawnGrunt.Handle_Disable = Grunt.FlowEvents.Inputs.Disable[1]
SpawnGrunt.Handle_Enable = Grunt.FlowEvents.Inputs.Enable[1]
SpawnGrunt.Handle_Kill = Grunt.FlowEvents.Inputs.Kill[1]
SpawnGrunt.Handle_Spawn = Grunt.FlowEvents.Inputs.Spawn[1]
