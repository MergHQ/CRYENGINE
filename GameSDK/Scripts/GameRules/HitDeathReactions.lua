-------------------------------------------------------------------
-- Hit death reaction lua script
-- self.entity contains the entity table of the associated actor
-- self.binds contains the methods bound from C++
--
-- Validation Functions:
--   You can specify them using the validationFunc attribute on the 
-- hit reaction params. This is its "signature":
-- function HitDeathReactions:CustomValidationFunc(reactionsParams, hitInfo, (optional) causedDamage)
-- Params:
-- * reactionParams is the lua table of the params for this reaction
-- * hitInfo is the table containing the last hit information
-- * causedDamage is the damage actually caused by the hit 
--
-- Reaction Functions:
--   You can specify custom reaction functions using the reactionFunc on the
-- hit reaction params. This is its "signature":
-- function HitDeathReactions:CustomReactionFunc(reactionsParams)
-------------------------------------------------------------------

HitDeathReactions = {}

-------------------------------------------------------------------
-- Loads a XML reaction file and returns it as a script table
-------------------------------------------------------------------
function HitDeathReactions:LoadXMLData(fileHitDeathReactionsParamsDataFile)
	return CryAction.LoadXML("Scripts/GameRules/HitDeathReactions_Defs.xml", fileHitDeathReactionsParamsDataFile);
end

-------------------------------------------------------------------
-- BEGIN DEFAULT FUNCTIONS
-- Don't remove these, and be careful changing them
-------------------------------------------------------------------

-------------------------------------------------------------------
-- Default behavior for kill reactions
-------------------------------------------------------------------
function HitDeathReactions:DefaultKillReaction(reactionParams)
	self.entity:IgnoreNextDeathImpulse();

	-- Call the C++ version of the default reaction behavior	
	-- Current implementation sets the signal animation graph 
	-- input to the value on the "inputValue" property in the 
	-- parameters (Libs/HitDeathReactionsData/)	
	self.binds:ExecuteDeathReaction(reactionParams);
end

-------------------------------------------------------------------
-- Default behavior for hit reactions
-------------------------------------------------------------------
function HitDeathReactions:DefaultHitReaction(reactionParams)
	-- Call the C++ version of the default reaction behavior	
	-- Current implementation sets the signal animation graph 
	-- input to the value on the "inputValue" property in the 
	-- parameters (Libs/HitDeathReactionsData/)	
	self.binds:ExecuteHitReaction(reactionParams);
end

-------------------------------------------------------------------
-- Default validation for all reactions
-------------------------------------------------------------------
function HitDeathReactions:DefaultValidation(reactionParams, hitInfo, causedDamage)
	-- For now it just calls the C++ code for the validation
	local bDefaultValidation = self.binds:IsValidReaction(reactionParams, hitInfo, causedDamage);
	
	return bDefaultValidation;
end

-------------------------------------------------------------------
-- END DEFAULT FUNCTIONS
-------------------------------------------------------------------



-------------------------------------------------------------------
-- BEGIN CUSTOM FUNCTIONS
-- Add your customized reaction and validation functions from here
-------------------------------------------------------------------

-------------------------------------------------------------------
-- Useful if we just want the ragdoll to activate
-------------------------------------------------------------------
function HitDeathReactions:ReactionDontApplyDeathImpulse(reactionParams)
	self.entity:IgnoreNextDeathImpulse();

	self.binds:EndCurrentReaction();
end

-------------------------------------------------------------------
--[[
-- Moved to C++, uncomment to override the C++ implementation
function HitDeathReactions:ReactionDoNothing(reactionParams)
	self.binds:EndCurrentReaction();
end
--]]

-------------------------------------------------------------------
function HitDeathReactions:FallAndPlay_Validation(reactionParams, hitInfo)
	local bValid = hitInfo.knocksDownLeg and self.binds:IsValidReaction(reactionParams, hitInfo); 
	
	return bValid;
end

-------------------------------------------------------------------
--[[
-- Moved to C++, uncomment to override the C++ implementation
function HitDeathReactions:FallAndPlay_Reaction(reactionParams)
  self.entity.actor:Fall({x=0,y=0,z=0}, true);
	Script.SetTimer(2500, self.binds.EndCurrentReaction, self.binds);
end
--]]

-------------------------------------------------------------------
function HitDeathReactions:DeathImpulse()
	self.binds:EndCurrentReaction();
	local configuration = self.entity.hitDeathReactionsParams.HitDeathReactionsConfig;
	BasicActor.ApplyDeathImpulse(self.entity, (configuration and configuration.maxRagdolImpulse) or -1.0);
end

--[[
-- Moved to C++, uncomment to override the C++ implementation
-------------------------------------------------------------------
function HitDeathReactions:DeathImpulse_Reaction(reactionParams)
	self:DeathImpulse();
end
--]]

-------------------------------------------------------------------
-- HACK: This reaction will be invoked until the AI leave cover in 
-- these situations
-------------------------------------------------------------------
-- ToDo: Move to C++
function HitDeathReactions:BackHitInCover_Reaction(reactionParams)
  self.binds:ExecuteHitReaction(reactionParams);
  
	-- We are assuming that back hits in cover make the entity to leave cover immediately, 
	-- so let's make sure that's true. Wait some time to allow the Anim Graph to select the proper
	-- states
	Script.SetTimer(500, function() self.entity:LeaveCover(); AI.SetStance(self.entity.id, STANCE_STAND); end);
end


-------------------------------------------------------------------
-- TESTS - You can use them as references
-------------------------------------------------------------------
function HitDeathReactions:TestValidation(reactionParams, hitInfo)
	local bDefaultValidation = self.binds:IsValidReaction(reactionParams, hitInfo);
	
	return bDefaultValidation;
end

-------------------------------------------------------------------
-- validate body destructibility driven reactions
-------------------------------------------------------------------
function HitDeathReactions:Destructible_Validation(reactionParams, hitInfo)
	-- if System.GetCVar("g_hitDeathReactions_debug") == 1 then
		-- Log("Destructible hit reaction!!!");
		-- Log("self.entity.lastHitEvent: "..(self.entity.lastHitEvent or "none"));
	-- end
	
	if self.entity.lastHitEvent ~= nil then
		local bDefaultValidation = self.binds:IsValidReaction(reactionParams, hitInfo);
		local bHasReactionAnimation = self.entity.lastHitEvent == reactionParams.destructibleEvent;
		
		-- if System.GetCVar("g_hitDeathReactions_debug") == 1 then
			-- Log("reactionParams.destructibleEvent: "..(reactionParams.destructibleEvent or "none"));
			-- if bDefaultValidation and bHasReactionAnimation then Log("VALID! VALID! VALID!") end
		-- end
		
		return bDefaultValidation and bHasReactionAnimation;
	else
		return false;
	end
end

-------------------------------------------------------------------
function HitDeathReactions:TestKillReaction(reactionParams)
	self.entity:IgnoreNextDeathImpulse();

	if (not self.binds:StartReactionAnim("deathReact_spiral_all_3p_01", true, 0.1)) then
		Script.SetTimer(700, BasicActor.TurnRagdoll, self.entity, 1);
	end
end

-------------------------------------------------------------------
function HitDeathReactions:PlayerDeathTest(reactionParams)
	self.binds:ExecuteDeathReaction(reactionParams);

	if (not self.binds:StartInteractiveAction(reactionParams.interactiveAction, 1000)) then
		Script.SetTimer(3900, self.binds.EndCurrentReaction, self.binds);
	end
end
