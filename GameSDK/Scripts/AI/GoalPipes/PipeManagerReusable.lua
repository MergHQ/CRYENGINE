
function PipeManager:InitReusable()





	AI.CreateGoalPipe("do_nothing");
	AI.PushGoal("do_nothing","firecmd",0,0);	

	AI.CreateGoalPipe("clear_all");
	AI.PushGoal("clear_all", "clear", 0,1);



	AI.CreateGoalPipe("devalue_target");
	AI.PushGoal("devalue_target", "devalue", 0,1);



	AI.CreateGoalPipe("acquire_target");
	AI.PushGoal("acquire_target","acqtarget",0,""); 




	-------------------------------------------------------
	AI.BeginGoalPipe("throw_grenade_execute");
		AI.PushGoal("ignoreall", 0, 1);
		AI.PushGoal("firecmd", 0, 0);
		AI.PushGoal("timeout", 1, .6);
		AI.PushGoal("firecmd", 0, FIREMODE_SECONDARY);
		AI.PushGoal("timeout", 1, 3);
		AI.PushGoal("firecmd", 0, FIREMODE_OFF);
		AI.PushGoal("ignoreall", 0, 0);
		AI.PushGoal("signal", 1, 1, "THROW_GRENADE_DONE", 0);
	AI.EndGoalPipe();

	AI.BeginGoalPipe("fire_mounted_weapon");
		AI.PushGoal("signal",0,1,"SET_MOUNTED_WEAPON_PERCEPTION",0);
		AI.PushLabel("LOOP");
		AI.PushGoal("firecmd", 0, FIREMODE_BURST_DRAWFIRE);
		AI.PushGoal("timeout", 1, 2,3);
		AI.PushGoal("signal", 1, 1, "CheckTargetInRange", 0);
		AI.PushGoal("timeout", 1, 2,3);
		AI.PushGoal("signal", 1, 1, "CheckTargetInRange", 0);
		AI.PushGoal("timeout", 1, 2,3);
		AI.PushGoal("signal", 1, 1, "CheckTargetInRange", 0);
		AI.PushGoal("timeout", 1, 2,3);
		AI.PushGoal("signal", 1, 1, "CheckTargetInRange", 0);
		AI.PushGoal("firecmd", 0, 0);
		AI.PushGoal("timeout", 1, 0.3, 1);
		AI.PushGoal("signal", 1, 1, "CheckTargetInRange", 0);
		AI.PushGoal("branch",1,"LOOP",BRANCH_ALWAYS);
	AI.EndGoalPipe();	


	AI.BeginGoalPipe("mounted_weapon_look_around");
		AI.PushGoal("signal",0,1,"SET_MOUNTED_WEAPON_PERCEPTION",0);
		AI.PushGoal("locate",1,"refpoint");	
		AI.PushGoal("lookaround",1,30,2.2,10005,10010,AI_BREAK_ON_LIVE_TARGET + AILASTOPRES_USE);
		--AI.PushGoal("lookat",1,30,2.2,5,10,1);
	AI.EndGoalPipe();		
	

	


	-------------------
	----
	---- COVER
	----
	--------------------






	---------------------------------------
	---
	--- orders goalpipes
	---
	--------------------------------------


	AI.CreateGoalPipe("order_timeout");
	AI.PushGoal("order_timeout", "signal", 0, 10, "order_timeout_begin", SIGNALFILTER_SENDER);
	AI.PushGoal("order_timeout", "timeout", 1, 0.3, 1.0);
	AI.PushGoal("order_timeout", "signal", 0, 10, "ORD_DONE", SIGNALFILTER_LEADER);
	AI.PushGoal("order_timeout", "signal", 0, 10, "order_timeout_end", SIGNALFILTER_SENDER);

	

	AI.CreateGoalPipe("action_lookatpoint");
	AI.PushGoal("action_lookatpoint","locate",0,"refpoint");
	AI.PushGoal("action_lookatpoint","+lookat",1,0,0,true);
	AI.PushGoal("action_lookatpoint","timeout",1,0.2);
--	AI.PushGoal("action_lookatpoint","signal",0,10,"ACTION_DONE",SIGNALFILTER_SENDER);
	
	AI.CreateGoalPipe("action_resetlookat");
	AI.PushGoal("action_resetlookat","lookat",1,0,0);
--	AI.PushGoal("action_resetlookat","signal",0,10,"ACTION_DONE",SIGNALFILTER_SENDER);
	
	AI.CreateGoalPipe("action_weaponholster");
	AI.PushGoal("action_weaponholster", "signal", 1, 10, "HOLSTERITEM_TRUE", SIGNALFILTER_SENDER);
	
	AI.CreateGoalPipe("action_weapondraw");
	AI.PushGoal("action_weapondraw", "signal", 1, 10, "HOLSTERITEM_FALSE", SIGNALFILTER_SENDER);

	-- use this goal pipe to insert it as a dummy pipe in actions which are executed inside their signal handler
	AI.CreateGoalPipe("action_dummy");
	AI.PushGoal("action_dummy","timeout",1,0.1);

	AI.CreateGoalPipe("action_enter");
	AI.PushGoal("action_enter", "waitsignal", 0, "ENTERING_END");
	AI.PushGoal("action_enter", "signal", 1, 10, "SETUP_ENTERING", SIGNALFILTER_SENDER);
	AI.PushGoal("action_enter", "locate", 0, "animtarget");	-- the anim target is set by the vehicle code
	AI.PushGoal("action_enter", "run", 0, 1);
	AI.PushGoal("action_enter", "bodypos", 0, BODYPOS_STAND);
	AI.PushGoal("action_enter", "+approach", 1, 0, AILASTOPRES_USE + AI_STOP_ON_ANIMATION_START, 2);
	AI.PushGoal("action_enter", "branch", 1, "PATH_FOUND", NOT + IF_LASTOP_FAILED );
		AI.PushGoal("action_enter", "signal", 1, 1, "CANCEL_CURRENT", 0);
	AI.PushLabel("action_enter", "PATH_FOUND" );
	AI.PushGoal("action_enter", "timeout", 1, 0.1);
	AI.PushGoal("action_enter", "branch", 1, -1, IF_ACTIVE_GOALS);
	--AI.PushGoal("action_enter", "signal", 1, 10, "CHECK_INVEHICLE", SIGNALFILTER_SENDER);

	AI.CreateGoalPipe("action_enter_fast");
	AI.PushGoal("action_enter_fast", "waitsignal", 0, "ENTERING_END" );
	AI.PushGoal("action_enter_fast", "signal", 1, 10, "SETUP_ENTERING_FAST", SIGNALFILTER_SENDER);
	AI.PushGoal("action_enter_fast", "timeout", 1, 0.1);
	AI.PushGoal("action_enter_fast", "branch", 1, -1, IF_ACTIVE_GOALS);

	AI.CreateGoalPipe("action_exit");
	AI.PushGoal("action_exit", "stance", 1, STANCE_ALERTED);
	AI.PushGoal("action_exit", "waitsignal", 1, "EXITING_END", nil, 10.0 );

	AI.CreateGoalPipe("check_driver");
	AI.PushGoal("check_driver", "signal", 1, 1, "CHECK_DRIVER", SIGNALFILTER_SENDER);

	AI.CreateGoalPipe("action_unload");
	AI.PushGoal("action_unload", "waitsignal", 0, "UNLOAD_DONE", nil, 10.0);
	AI.PushGoal("action_unload", "signal", 1, 10, "DO_UNLOAD", SIGNALFILTER_SENDER);
	AI.PushGoal("action_unload", "timeout", 1, 0.1);
	AI.PushGoal("action_unload", "branch", 1, -1, IF_ACTIVE_GOALS);

	AI.CreateGoalPipe("stay_in_formation_moving");
	AI.PushGoal("stay_in_formation_moving","locate",0,"formation",1000);
	AI.PushGoal("stay_in_formation_moving","+acqtarget",0,"");
	AI.PushGoal("stay_in_formation_moving","+locate",0,"formationsight",1000);
	AI.PushGoal("stay_in_formation_moving","+stick",1,0.0,AILASTOPRES_LOOKAT+AI_REQUEST_PARTIAL_PATH,STICK_SHORTCUTNAV);	



	AI.CreateGoalPipe("vehicle_gunner_cover_fire");
	AI.PushGoal("vehicle_gunner_cover_fire","firecmd",1,FIREMODE_BURST);
	AI.PushGoal("vehicle_gunner_cover_fire","timeout",1,2,3);
	AI.PushGoal("vehicle_gunner_cover_fire","firecmd",1,0);
	AI.PushGoal("vehicle_gunner_cover_fire", "signal",0,1,"EXIT_VEHICLE_DONE",SIGNALFILTER_SENDER);

	AI.BeginGoalPipe("vehicle_gunner_shoot");
		AI.PushGoal("firecmd",1,FIREMODE_BURST_DRAWFIRE);
		AI.PushGoal("timeout",1,1);
		AI.PushGoal("signal",1,1,"INVEHICLEGUNNER_CHECKGETOFF",SIGNALFILTER_SENDER);
		AI.PushGoal("timeout",1,1);
		AI.PushGoal("signal",1,1,"INVEHICLEGUNNER_CHECKGETOFF",SIGNALFILTER_SENDER);
		AI.PushGoal("timeout",1,1);
		AI.PushGoal("signal",1,1,"INVEHICLEGUNNER_CHECKGETOFF",SIGNALFILTER_SENDER);
		AI.PushGoal("timeout",1,1);
		AI.PushGoal("signal",1,1,"INVEHICLEGUNNER_CHECKGETOFF",SIGNALFILTER_SENDER);
		AI.PushGoal("timeout",1,1);
		AI.PushGoal("signal",1,1,"INVEHICLEGUNNER_CHECKGETOFF",SIGNALFILTER_SENDER);
		AI.PushGoal("timeout",1,1);
		AI.PushGoal("signal",1,1,"INVEHICLEGUNNER_CHECKGETOFF",SIGNALFILTER_SENDER);
		AI.PushGoal("firecmd",1,0);
		AI.PushGoal("timeout",1,0.3,0.5);
	AI.EndGoalPipe();

	AI.LogEvent("REUSABLE PIPES LOADED");
end