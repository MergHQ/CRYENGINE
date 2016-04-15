AI.LogEvent("LOADING AI BEHAVIORS");

AIBehavior = 
{
}

function CreateAIBehavior(name, parent, def)
	if ((not def) and ((type(parent) == "table") or (not parent))) then
		def = parent
		parent = nil
	end

	if (not def) then
		def = {}
	end

	assert(not def.parent)
	assert(not def.name)

	local behavior = {}
	mergef(behavior, def, true)

	if (parent) then
		behavior = def
		mergef(def, AIBehavior[parent], true)
		behavior.parent = AIBehavior[parent]
	else
		behavior = def
		behavior.parent = nil
	end
	
	AIBehavior[name] = behavior
	behavior.name = name

	return behavior
end

AI.LoadBehaviors("Scripts/AI/Behaviors", "*.lua");