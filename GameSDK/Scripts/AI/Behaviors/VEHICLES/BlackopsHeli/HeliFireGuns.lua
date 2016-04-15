local Behavior = CreateAIBehavior("HeliFireGuns", "HeliIdle",
{
	Constructor = function (self, entity)
		self:AnalyzeSituation(entity)
	end,
	
	ShouldRelocate = function(self, entity)
		return AI.GetTargetType(entity.id) ~= AITARGET_ENEMY
	end,
	
	AnalyzeSituation = function(self, entity, sender, data)
		--AI.SetRefPointPosition( entity.id, data.point )
		--entity:InsertSubpipe(AIGOALPIPE_HIGHPRIORITY + AIGOALPIPE_KEEP_ON_TOP, "HeliShootAt", nil, nil)
	end,
})