function PipeManager:InitHuman()

	--Log("Loading Human goal pipes");
	

	AI.BeginGoalPipe("FireMountedWeapon");
		AI.PushGoal("script", 1, "AI.SetUseSecondaryVehicleWeapon(entity.id, false); return false;");
		AI.PushGoal("firecmd", 1, FIREMODE_BURST);
		AI.PushGoal("timeout", 1, 0.5, 0.75);
		AI.PushGoal("script", 1, "entity:CheckMountedWeaponTarget(); return true;");
	AI.EndGoalPipe();
	
	AI.BeginGoalPipe("FireSecondaryMountedWeapon");
		AI.PushGoal("script", 0, "AI.SetUseSecondaryVehicleWeapon(entity.id, true); return true;");
		AI.PushGoal("firecmd", 0, FIREMODE_BURST);
		AI.PushGoal("timeout", 1, 3.0);
	AI.EndGoalPipe();
	
	AI.BeginGoalPipe("LookAroundInMountedWeapon");
		AI.PushGoal("lookaround", 0, 30, 2.2, 9999, 9999, AI_BREAK_ON_LIVE_TARGET);
		AI.PushLabel("Loop");
		AI.PushGoal("timeout", 1, 1.0);
		AI.PushGoal("script", 0, "entity:CheckMountedWeaponTarget(); return true;");
		AI.PushGoal("branch", 1, "Loop", BRANCH_ALWAYS);
	AI.EndGoalPipe();
	
	AI.BeginGoalPipe("FireRocket");
		AI.PushGoal("locate",0,"atttarget");
		AI.PushGoal("lookat",0,0,0,true,1);
		AI.PushGoal("firecmd", 0, FIREMODE_AIM);
		AI.PushGoal("adjustaim", 1, ADJUSTAIM_AIM, 1.5);
		AI.PushGoal("timeout", 1, 3.0);
		AI.PushGoal("firecmd", 0, FIREMODE_FORCED);		
		AI.PushGoal("timeout", 1, 1.0);
		AI.PushGoal("signal", 0, 1, "OnFireRocketDone", SIGNALFILTER_SENDER);
	AI.EndGoalPipe();
	
	AI.BeginGoalPipe("HumanRunToCombatHidespot");
		AI.PushLabel("Start");
		AI.PushGoal("firecmd", 1, FIREMODE_BURST_WHILE_MOVING);
		AI.PushGoal("tacticalpos", 1, "HumanCombatHidespotQuery", AI_REG_COVER);
		AI.PushGoal("branch", 0, "RunToCover", IF_LASTOP_SUCCEED);
		AI.PushGoal("tacticalpos", 1, "HumanDistantCombatHidespotQuery", AI_REG_REFPOINT);
		AI.PushGoal("branch", 0, "ApproachDistantCover", IF_LASTOP_SUCCEED);
		
			AI.PushGoal("signal", 0, 1, "OnCombatHidespotTPSQueryFail", SIGNALFILTER_SENDER);
			AI.PushGoal("branch", 0, "End", BRANCH_ALWAYS);
		
		AI.PushLabel("ApproachDistantCover");
		
			AI.PushGoal("firecmd", 1, FIREMODE_BURST_WHILE_MOVING);
			--AI.PushGoal("stance", 0, STANCE_CROUCH);
			AI.PushGoal("speed", 0, SPEED_WALK);
			AI.PushGoal("strafe", 0, 99, 99, 1);
			AI.PushGoal("locate", 1, "refpoint");
			AI.PushGoal("stick", 1, 3, AILASTOPRES_USE+AI_REQUEST_PARTIAL_PATH+AI_USE_TIME, STICK_BREAK);
			AI.PushGoal("look", 1, AILOOKMOTIVATION_GLANCE, 0,AILASTOPRES_USE); -- Experimetal
			AI.PushGoal("branch", 0, "Start", BRANCH_ALWAYS);
					
		AI.PushLabel("RunToCover");
		
			AI.PushGoal("stance", 0, STANCE_STAND);
			AI.PushGoal("firecmd", 1, FIREMODE_BURST_WHILE_MOVING);
			AI.PushGoal("RunToCover");
			
		AI.PushLabel("End");
		AI.PushGoal("timeout", 1, 1);
	AI.EndGoalPipe();

	AI.BeginGoalPipe("HumanNoCoverSpotCrouchAndFire");
		AI.PushGoal("stance", 0, STANCE_CROUCH);
		AI.PushGoal("firecmd", 1, FIREMODE_BURST);
		AI.PushGoal("timeout", 1, 2.0);
		AI.PushGoal("script", 1, "entity.Behavior:Relocate(entity); return true;");
	AI.EndGoalPipe();	
	
	AI.BeginGoalPipe("CallReinforcementsWave");
		AI.PushGoal("stance", 0, STANCE_STAND);
		AI.PushGoal("speed", 0, SPEED_RUN);
		AI.PushGoal("firecmd", 0, FIREMODE_OFF);
		
		-- readability here
		
		AI.PushGoal("locate", 0, "refpoint");
		AI.PushGoal("animtarget", 1, 1, "callReinforcementsWave", 0.5, 5, 0.5);
		AI.PushGoal("locate", 0, "animtarget" );
		AI.PushGoal("stick", 1, 0, AILASTOPRES_USE + AI_REQUEST_PARTIAL_PATH, STICK_BREAK, 0, 0);

		AI.PushGoal("signal", 1, 1, "calling_for_help", SIGNALID_READIBILITY, 120);
		AI.PushGoal("timeout", 1, 1.25, 2);

		AI.PushGoal("signal", 1, 1, "calling_for_help", SIGNALID_READIBILITY, 120);
		AI.PushGoal("timeout", 1, 1.25, 2);

		AI.PushGoal("signal", 1, 1, "CallReinforcementsDone", 0);
	AI.EndGoalPipe();

	AI.BeginGoalPipe("CallReinforcementsRadio");
		AI.PushGoal("stance", 0, STANCE_STAND);
		AI.PushGoal("firecmd", 0, FIREMODE_OFF);

		AI.PushGoal("script", 0, "AI.PlayCommunication(entity.id, \"comm_radio_call_reinforcements\", \"Radio\", 5); return true;");
		AI.PushGoal("timeout", 1, 1.25, 2);

		AI.PushGoal("signal", 1, 1, "CallReinforcementsDone", 0);
	AI.EndGoalPipe();

	AI.BeginGoalPipe("CallReinforcementsFlare");
		AI.PushGoal("firecmd", 1, FIREMODE_BURST);
		AI.PushGoal("stance", 1, STANCE_STAND);
		AI.PushGoal("speed", 1, 1, 0, 3);
		AI.PushGoal("strafe", 1, 0);
		AI.PushGoal("locate", 1, "refpoint");
		AI.PushGoal("stick", 1, 0, AILASTOPRES_USE + AI_REQUEST_PARTIAL_PATH, STICK_BREAK, 0, 0);
		AI.PushGoal("firecmd", 1, FIREMODE_OFF);
		AI.PushGoal("animation", 1, AIANIM_SIGNAL, "fireFlare");
		AI.PushGoal("signal", 1, 1, "CallReinforcementsDone", 0);
	AI.EndGoalPipe();	
	
	AI.BeginGoalPipe("CallReinforcementsSmokeGrenade");
		--AI.PushGoal("ignoreall", 0, 1);
		AI.PushGoal("firecmd", 0, FIREMODE_OFF);
		AI.PushGoal("stance", 0, STANCE_STAND, 1);
		AI.PushGoal("firecmd", 0, FIREMODE_SECONDARY);
		AI.PushGoal("script", 0, "AI.ThrowGrenade(entity.id, AI_RGT_FRAG_GRENADE, AI_REG_REFPOINT); return true;");
		AI.PushGoal("timeout", 1, 3);
		AI.PushGoal("firecmd", 0, FIREMODE_OFF);
		--AI.PushGoal("ignoreall", 0, 0);
		AI.PushGoal("signal", 1, 1, "CallReinforcementsDone", 0);
	AI.EndGoalPipe();

	AI.BeginGoalPipe("ThrowGrenade");
		--AI.PushGoal("ignoreall", 0, 1);
		AI.PushGoal("firecmd", 0, FIREMODE_OFF);
		AI.PushGoal("stance", 0, STANCE_STAND, 1);
		AI.PushGoal("firecmd", 0, FIREMODE_SECONDARY);
		AI.PushGoal("script", 0, "AI.ThrowGrenade(entity.id, AI_RGT_FRAG_GRENADE, AI_REG_ATTENTIONTARGET); return true;");
		AI.PushGoal("timeout", 1, 3);
		AI.PushGoal("firecmd", 0, FIREMODE_OFF);
		--AI.PushGoal("ignoreall", 0, 0);
		AI.PushGoal("signal", 1, 1, "ThrowGrenadeDone", 0);
	AI.EndGoalPipe();

	AI.BeginGoalPipe("Flank");
		AI.PushGoal("firecmd", 1, FIREMODE_BURST_WHILE_MOVING);
		AI.PushGoal("speed", 1, SPEED_RUN);
		AI.PushGoal("stance", 1, STANCE_STAND);
		AI.PushGoal("tacticalpos", 1, "FlankQuery", AI_REG_COVER);
		AI.PushGoal("hide", 1, AI_REG_COVER);
	AI.EndGoalPipe();

	AI.BeginGoalPipe("BraveFlanker");
		AI.PushLabel("GetCloseToTarget");
		AI.PushGoal("firecmd", 1, FIREMODE_OFF);
		AI.PushGoal("stance", 1, STANCE_STAND);
		AI.PushGoal("speed", 1, SPEED_RUN);
		AI.PushGoal("locate", 1, "atttarget");
		AI.PushGoal("stickminimumdistance", 1, 7.0, AILASTOPRES_USE+AI_REQUEST_PARTIAL_PATH, STICK_BREAK);
		AI.PushGoal("firecmd", 1, FIREMODE_BURST_WHILE_MOVING);
		AI.PushLabel("Loop");
		AI.PushGoal("signal", 1, 1, "VerifyFlankPositionIsGood", 0);
		AI.PushGoal("stance", 1, STANCE_CROUCH);
		AI.PushGoal("branch", 1, "GetCloseToTarget", IF_TARGET_DIST_GREATER, 7.0);
		AI.PushGoal("timeout", 1, 0.3);
		AI.PushGoal("branch", 1, "Loop", BRANCH_ALWAYS);

	AI.EndGoalPipe();

	AI.BeginGoalPipe("FlankerInCover");
		AI.PushGoal("timeout", 1, 2.0);
		AI.PushGoal("signal", 1, 1, "FlankerAdvance", SIGNALFILTER_SENDER);
	AI.EndGoalPipe();
	
	AI.BeginGoalPipe("SniperEngageTarget");
		AI.PushGoal("bodypos", 1, BODYPOS_CROUCH);
		AI.PushGoal("firecmd",1, FIREMODE_BURST);
		AI.PushGoal("adjustaim", 1, ADJUSTAIM_AIM, 20.0);
		AI.PushGoal("timeout", 1, 1.0);
	AI.EndGoalPipe();
	
	AI.BeginGoalPipe("SniperEngageTargetAtCloseRange");
		AI.PushGoal("ReactToPressure"); -- For now it reacts as if in pressure (goes to cover and fires)
	AI.EndGoalPipe();
	
	AI.BeginGoalPipe("SniperLookForTarget");
		AI.PushGoal("bodypos", 1, BODYPOS_STAND);
		AI.PushGoal("speed", 0, SPEED_WALK);
		AI.PushGoal("tacticalpos", 1, "SniperSpotQueryForceVisible");
		AI.PushGoal("lookat", 1, 0, 0, false, AI_LOOKAT_USE_BODYDIR);
		AI.PushGoal("timeout", 1, 3.0);
	AI.EndGoalPipe();

	AI.BeginGoalPipe("AtSearchSpot");
		-- Look at search spot
		AI.PushGoal("locate", 1, "refpoint");
		AI.PushGoal("lookat", 0, 0, 0, true, AI_LOOKAT_CONTINUOUS);
		AI.PushGoal("timeout", 1, 1.5);
		AI.PushGoal("clear", 1, 0); -- Jonas: The last argument is basically 'resetAttentionTarget' and defaults to 1 *smacks forehead*
		-- Look around
		AI.PushGoal("lookaround", 0, 360, 5, 60, 60, AI_BREAK_ON_LIVE_TARGET, 0);
		AI.PushGoal("timeout", 1, 3.0);
		AI.PushGoal("script", 1, "entity.Behavior:OnAtSearchSpotDone(entity); return true;");
	AI.EndGoalPipe();

	AI.BeginGoalPipe("PauseAndReportIn");
		AI.PushGoal("timeout", 1, 1.0);
		AI.PushGoal("script", 1, "entity.Behavior:PlayAreaClearReadability(entity); return true;");
		AI.PushGoal("timeout", 1, 1.0);
		AI.PushGoal("script", 1, "entity.Behavior:GoToNextSearchSpot(entity); return true;");
	AI.EndGoalPipe();
end
