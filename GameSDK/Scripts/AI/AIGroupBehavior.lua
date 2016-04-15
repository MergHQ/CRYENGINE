AI.LogEvent("LOADING AI GROUP BEHAVIORS");

AIGroupBehavior = 
{
}

function CreateAIGroupBehavior(name, parent, def)
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
		mergef(def, AIGroupBehavior[parent], true)
		behavior.parent = AIGroupBehavior[parent]
	else
		behavior = def
		behavior.parent = nil
	end
	
	AIGroupBehavior[name] = behavior
	behavior.name = name

	return behavior
end
