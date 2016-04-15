local Behavior = CreateAIBehavior("Dummy",
{
	Constructor = function(behavior, entity)
		entity:SelectPipe(0, "Empty")
	end,
})