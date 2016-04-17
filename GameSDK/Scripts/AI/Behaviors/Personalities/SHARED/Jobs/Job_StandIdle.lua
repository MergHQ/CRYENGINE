local Behavior = CreateAIBehavior("Job_StandIdle",
{
	Constructor = function(self,entity )
		g_StringTemp1 = tostring(entity.Properties.aicharacter_character).."Idle";
		local defaultBehavior = AIBehavior[g_StringTemp1];
		if(defaultBehavior and defaultBehavior ~= AIBehavior.Job_StandIdle and defaultBehavior.Constructor) then 
			defaultBehavior:Constructor(entity);
		else			
			if ( entity.AI and entity.AI.needsAlerted ) then
				AI.Signal(SIGNALFILTER_SENDER, 1, "INCOMING_FIRE",entity.id);
				entity.AI.needsAlerted = nil;
			end	
		end
		
		entity:InitAIRelaxed();
		entity:SelectPipe(0, "DoNothing");
	end,

	Destructor = function(self, entity)
	end,

	OnJobContinue = function(self,entity,sender )	
		self:Constructor(entity);
	end,

	OnBored = function (self, entity)
	end,	
})