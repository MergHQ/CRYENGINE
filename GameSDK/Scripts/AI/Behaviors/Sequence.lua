CreateAIBehavior("ExecuteSequence",
{
	Constructor = function (behavior, entity)
		entity:StartOrRestartPipe("Empty")
		AI.SequenceBehaviorReady(entity.id)
	end,

	Destructor = function (behavior, entity)
		entity:StartOrRestartPipe("Empty")
		AI.SequenceNonInterruptibleBehaviorLeft(entity.id)
	end,
})

CreateAIBehavior("ExecuteInterruptibleSequence",
{
	Constructor = function (behavior, entity)
		entity:StartOrRestartPipe("Empty")
		AI.SequenceBehaviorReady(entity.id)
	end,

	Destructor = function (behavior, entity)
		entity:StartOrRestartPipe("Empty")
		AI.SequenceInterruptibleBehaviorLeft(entity.id)
	end,
})
