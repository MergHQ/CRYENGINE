-- FormationManager: defines all the formations descriptors 
-- Created by Luciano Morpurgo - 2005
-- Usage:
-- 

FormationManager = {

}

function FormationManager:OnInit()

	local UNDEFINED = UNIT_CLASS_UNDEFINED;
	local INFANTRY = UNIT_CLASS_INFANTRY;
	local ENGINEER = UNIT_CLASS_ENGINEER;
	local MEDIC = UNIT_CLASS_MEDIC;
	local LEADER = UNIT_CLASS_LEADER;
	local SCOUT = UNIT_CLASS_SCOUT;
	local CIVILIAN = UNIT_CLASS_CIVILIAN;

	AI.CreateFormation("convoy0");
	--AI.AddFormationPointFixed("convoy0",0,0,0,0,LEADER);
	AI.AddFormationPoint("convoy0",90,20,4,UNDEFINED,25,3.0);
	AI.AddFormationPoint("convoy0",-90,20,-4,ENGINEER,25,-3.0);
	AI.AddFormationPoint("convoy0",90,40,4,UNDEFINED,45,3.0);
	AI.AddFormationPoint("convoy0",-90,40,-4,UNDEFINED,45,-3.0);
	AI.AddFormationPoint("convoy0", 90,60,4,UNDEFINED,65,3.0);
	AI.AddFormationPoint("convoy0",-90,60,-4,UNDEFINED,65,-3.0);
	AI.AddFormationPoint("convoy0", 180,70,4,UNDEFINED,70,3.0);

	AI.CreateFormation("convoy1");
	AI.AddFormationPoint("convoy1",90,20,4.5,UNDEFINED);
	AI.AddFormationPoint("convoy1",-90,20,-4.5,ENGINEER);
	AI.AddFormationPoint("convoy1",90,30,4.5,UNDEFINED);
	AI.AddFormationPoint("convoy1",-90,30,-4.5,UNDEFINED);
--	AI.AddFormationPoint("convoy1",0,23,0,UNDEFINED,25,3.0);




	AI.CreateFormation("wedge_follow");
	--AI.AddFormationPointFixed("wedge_follow",0,0,0,0,LEADER);
	AI.AddFormationPoint("wedge_follow",-20,4,-2,ENGINEER+INFANTRY);
	AI.AddFormationPoint("wedge_follow",20,4,2,ENGINEER+INFANTRY);
	AI.AddFormationPoint("wedge_follow",-20,6,-4,INFANTRY);
	AI.AddFormationPoint("wedge_follow",20,6,4,INFANTRY);
	AI.AddFormationPoint("wedge_follow",0,0.1,-0.5,SPECIAL_FORMATION_POINT);
	AI.AddFormationPoint("wedge_follow",0,4.5,0,CIVILIAN);
	AI.AddFormationPoint("wedge_follow",0,6,0,CIVILIAN);
	AI.AddFormationPoint("wedge_follow",0,7.5,0,CIVILIAN);

	AI.CreateFormation("line_follow3");
	--AI.AddFormationPointFixed("line_follow",0,0,0,0,LEADER);
	AI.AddFormationPoint("line_follow3",-45,3,0);
	AI.AddFormationPoint("line_follow3",45,6,0);
	AI.AddFormationPoint("line_follow3",0,9,0);

	AI.CreateFormation("line_follow3_extended");
	AI.AddFormationPoint("line_follow3_extended",-45,4,1);
	AI.AddFormationPoint("line_follow3_extended",45,8,-1);
	AI.AddFormationPoint("line_follow3_extended",0,12,1);

	AI.CreateFormation("line_follow5");
	--AI.AddFormationPointFixed("line_follow",0,0,0,0,LEADER);
	AI.AddFormationPoint("line_follow5",-45,5,0);
	AI.AddFormationPoint("line_follow5",45,8,0);
	AI.AddFormationPoint("line_follow5",45,11,0);
	AI.AddFormationPoint("line_follow5",45,14,0);
	AI.AddFormationPoint("line_follow5",0,17,0);
	AI.AddFormationPoint("line_follow5",170,0.1,-0.5,SPECIAL_FORMATION_POINT);

	AI.CreateFormation("line_follow5_extended");
	AI.AddFormationPoint("line_follow5_extended",-45,6,1);
	AI.AddFormationPoint("line_follow5_extended",45,10,-1);
	AI.AddFormationPoint("line_follow5_extended",45,14,1);
	AI.AddFormationPoint("line_follow5_extended",45,18,-1);
	AI.AddFormationPoint("line_follow5_extended",0,22,1);
	AI.AddFormationPoint("line_follow5_extended",170,0.1,-0.5,SPECIAL_FORMATION_POINT);

	AI.CreateFormation("line_follow8");
	--AI.AddFormationPointFixed("line_follow",0,0,0,0,LEADER);
	AI.AddFormationPoint("line_follow8",-45,3,0);
	AI.AddFormationPoint("line_follow8",45,6,0);
	AI.AddFormationPoint("line_follow8",45,9,0);
	AI.AddFormationPoint("line_follow8",45,12,0);
	AI.AddFormationPoint("line_follow8",45,15,0);
	AI.AddFormationPoint("line_follow8",45,18,0);
	AI.AddFormationPoint("line_follow8",45,21,0);
	AI.AddFormationPoint("line_follow8",0,24,0);

	AI.CreateFormation("line_follow8_extended");
	AI.AddFormationPoint("line_follow8_extended",-45,4,1);
	AI.AddFormationPoint("line_follow8_extended",45,8,-1);
	AI.AddFormationPoint("line_follow8_extended",45,12,1);
	AI.AddFormationPoint("line_follow8_extended",45,16,-1);
	AI.AddFormationPoint("line_follow8_extended",45,20,1);
	AI.AddFormationPoint("line_follow8_extended",45,24,-1);
	AI.AddFormationPoint("line_follow8_extended",45,28,1);
	AI.AddFormationPoint("line_follow8_extended",0,32,-1);

--	AI.CreateFormation("line_follow2");
--	AI.AddFormationPoint("line_follow2",0,2.5,0,CIVILIAN);
--	AI.AddFormationPoint("line_follow2",45,4,0,CIVILIAN);
--	AI.AddFormationPoint("line_follow2",-45,5.5,0,CIVILIAN);
--	AI.AddFormationPointFixed("line_follow2",170,-0.5,2,0,SPECIAL_FORMATION_POINT);

	AI.CreateFormation("jungle_wedge");
	AI.AddFormationPoint("jungle_wedge",0,3,-3);
	AI.AddFormationPoint("jungle_wedge",0,3,3);
	AI.AddFormationPoint("jungle_wedge",0,6,-6);
	AI.AddFormationPoint("jungle_wedge",0,6,6);
	AI.AddFormationPoint("jungle_wedge",-45,9,-12);
	AI.AddFormationPoint("jungle_wedge",45,9,12);

	AI.CreateFormation("wedge");
	AI.AddFormationPoint("wedge",0,3,3,UNDEFINED);
	AI.AddFormationPoint("wedge",0,3,-3,UNDEFINED);
	AI.AddFormationPoint("wedge",0,5,5,UNDEFINED);
	AI.AddFormationPoint("wedge",0,5,-5,UNDEFINED);


	AI.CreateFormation("wedge_rev");
	AI.AddFormationPoint("wedge_rev",0,2,3,INFANTRY);
	AI.AddFormationPoint("wedge_rev",0,2,-3,INFANTRY);
	AI.AddFormationPoint("wedge_rev",0,0.1,5,UNDEFINED);
	AI.AddFormationPoint("wedge_rev",0,0.1,-5,UNDEFINED);
	AI.AddFormationPoint("wedge_rev",0,10,0,CIVILIAN);
	AI.AddFormationPoint("wedge_rev",0,3,1,SHOOTING_SPOT_POINT);
	AI.AddFormationPoint("wedge_rev",0,3,-1,SHOOTING_SPOT_POINT);
	AI.AddFormationPoint("wedge_rev",0,5,2,SHOOTING_SPOT_POINT);
	AI.AddFormationPoint("wedge_rev",0,5,-2,SHOOTING_SPOT_POINT);
	AI.AddFormationPoint("wedge_rev",0,5,3.5,SHOOTING_SPOT_POINT);
	AI.AddFormationPoint("wedge_rev",0,5,-3.5,SHOOTING_SPOT_POINT);
	AI.AddFormationPoint("wedge_rev",0,3.5,5,SHOOTING_SPOT_POINT);
	AI.AddFormationPoint("wedge_rev",0,3.5,-5,SHOOTING_SPOT_POINT);
	AI.AddFormationPoint("wedge_rev",0,3,0,SPECIAL_FORMATION_POINT);

	AI.CreateFormation("squad_indoor_combat");
	AI.AddFormationPoint("squad_indoor_combat",0,2,2,INFANTRY);
	AI.AddFormationPoint("squad_indoor_combat",0,2,-2,INFANTRY);
	AI.AddFormationPoint("squad_indoor_combat",0,0.1,1,SHOOTING_SPOT_POINT);
	AI.AddFormationPoint("squad_indoor_combat",0,0.1,-1,SHOOTING_SPOT_POINT);
	AI.AddFormationPoint("squad_indoor_combat",0,2,3,CIVILIAN);
	AI.AddFormationPoint("squad_indoor_combat",0,2,5,CIVILIAN);
	AI.AddFormationPoint("squad_indoor_combat",0,0.1,0,SPECIAL_FORMATION_POINT);

	AI.CreateFormation("squad_indoor_follow");
	AI.AddFormationPoint("squad_indoor_follow",0,4,0,INFANTRY);
	AI.AddFormationPoint("squad_indoor_follow",0,8,0,INFANTRY);
	AI.AddFormationPoint("squad_indoor_follow",0,6,0,CIVILIAN);
	AI.AddFormationPoint("squad_indoor_follow",0,0.1,0,SPECIAL_FORMATION_POINT);


		
	AI.CreateFormation("row2");
	AI.AddFormationPoint("row2",0,0.1,1.2,UNDEFINED);
	AI.AddFormationPoint("row2",0,0.1,-1.2,UNDEFINED);
	
	AI.CreateFormation("rowTest");
	AI.AddFormationPoint("rowTest",-45,0.1,-3,UNDEFINED);
	AI.AddFormationPoint("rowTest",-45,0.1,-6,UNDEFINED);	
	AI.AddFormationPoint("rowTest",-45,0.1,-9,UNDEFINED);	
	AI.AddFormationPoint("rowTest",-45,0.1,-12,UNDEFINED);	
	AI.AddFormationPoint("rowTest",-45,0.1,-15,UNDEFINED);	
	AI.AddFormationPoint("rowTest",-45,0.1,3,UNDEFINED);

	AI.CreateFormation("boxTest");
	AI.AddFormationPoint("boxTest",-45,0.1,-3,UNDEFINED);
	AI.AddFormationPoint("boxTest",-45,5,-4,UNDEFINED);	
	AI.AddFormationPoint("boxTest",-45,0.1,3,UNDEFINED);	
	AI.AddFormationPoint("boxTest",-45,5,4,UNDEFINED);	
	
	AI.CreateFormation("trooper_patrol");
	--AI.AddFormationPointFixed("convoy0",0,0,0,0,LEADER);
	AI.AddFormationPoint("trooper_patrol",90,3,4,UNDEFINED,25,3.0);
	AI.AddFormationPoint("trooper_patrol",-90,3,-4,UNDEFINED,25,-3.0);
	AI.AddFormationPoint("trooper_patrol",90,6,4,UNDEFINED,45,3.0);
	AI.AddFormationPoint("trooper_patrol",-90,6,-4,UNDEFINED,45,-3.0);
	AI.AddFormationPoint("trooper_patrol", 90,9,4,UNDEFINED,65,3.0);
	AI.AddFormationPoint("trooper_patrol",-90,9,-4,UNDEFINED,65,-3.0);
	AI.AddFormationPoint("trooper_patrol", 180,10,0,LEADER,70,3.0);

	AI.CreateFormation("alien_escort");
	AI.AddFormationPoint("alien_escort",0,0.1,-4);
	AI.AddFormationPoint("alien_escort",0,0.1, 4);
	AI.AddFormationPoint("alien_escort",0,9,-4);
	AI.AddFormationPoint("alien_escort",0,9, 4);

	AI.CreateFormation("alien_escort2");
	AI.AddFormationPoint("alien_escort2",-45,0.1,-5,UNDEFINED);
	AI.AddFormationPoint("alien_escort2",-45,10,-5,UNDEFINED);	
	AI.AddFormationPoint("alien_escort2",-45,0.1,5,UNDEFINED);	
	AI.AddFormationPoint("alien_escort2",-45,10,5,UNDEFINED);	

	AI.CreateFormation("attack_surround1");
	AI.AddFormationPoint("attack_surround1",0,5, 0,SPECIAL_FORMATION_POINT);
	AI.AddFormationPoint("attack_surround1",0,4,-8,UNDEFINED);
	AI.AddFormationPoint("attack_surround1",0,4, 8,UNDEFINED);
	AI.AddFormationPoint("attack_surround1",0,1,-6,UNDEFINED);
	AI.AddFormationPoint("attack_surround1",0,1, 6,UNDEFINED);
	AI.AddFormationPoint("attack_surround1",0,0.1,-2,UNDEFINED);
	AI.AddFormationPoint("attack_surround1",0,0.1, 2,UNDEFINED);
--	AI.AddFormationPointFixed("attack_surround1",0,-5, 9,0,UNDEFINED);
--	AI.AddFormationPointFixed("attack_surround1",0, 5, 9,0,UNDEFINED);
	AI.AddFormationPoint("attack_surround1",0,2, -7,UNDEFINED);
	AI.AddFormationPoint("attack_surround1",0,2,  7,UNDEFINED);
	AI.AddFormationPoint("attack_surround1",0,7,-8,UNDEFINED);
	AI.AddFormationPoint("attack_surround1",0,7, 8,UNDEFINED);
--	AI.AddFormationPointFixed("attack_surround1",0,-5, 12,0,UNDEFINED);
--	AI.AddFormationPointFixed("attack_surround1",0, 5, 12,0,UNDEFINED);
--	AI.AddFormationPointFixed("attack_surround1",0, 8, 14,0,UNDEFINED);
--	AI.AddFormationPointFixed("attack_surround1",0, -8, 14,0,UNDEFINED);
--	AI.AddFormationPointFixed("attack_surround1",0, -1.5, 18,0,UNDEFINED);
--	AI.AddFormationPointFixed("attack_surround1",0, 1.5, 18,0,UNDEFINED);
--	AI.AddFormationPointFixed("attack_surround1",0, -7, 15,0,UNDEFINED);
--	AI.AddFormationPointFixed("attack_surround1",0, 7, 15,0,UNDEFINED);
	AI.AddFormationPoint("attack_surround1",0,13,-4,UNDEFINED);
	AI.AddFormationPoint("attack_surround1",0,13, 4,UNDEFINED);
--	AI.AddFormationPointFixed("attack_surround1",0,-1,-8,0,UNDEFINED);
--	AI.AddFormationPointFixed("attack_surround1",0, 1,-8,0,UNDEFINED);

	AI.CreateFormation("attack_surround_chase");
	AI.AddFormationPoint("attack_surround_chase",0,6,-8, UNDEFINED);
	AI.AddFormationPoint("attack_surround_chase",0,6, 8, UNDEFINED);
	AI.AddFormationPoint("attack_surround_chase",0,3,-6, UNDEFINED);
	AI.AddFormationPoint("attack_surround_chase",0,3, 6, UNDEFINED);
	AI.AddFormationPoint("attack_surround_chase",0,0.1,-5, UNDEFINED);
	AI.AddFormationPoint("attack_surround_chase",0,0.1, 5, UNDEFINED);
	AI.AddFormationPoint("attack_surround_chase",0,4,-7, UNDEFINED);
	AI.AddFormationPoint("attack_surround_chase",0,4, 7, UNDEFINED);
	AI.AddFormationPoint("attack_surround_chase",0,9,-8, UNDEFINED);
	AI.AddFormationPoint("attack_surround_chase",0,9, 8, UNDEFINED);
	
	
	AI.LogEvent("FORMATIONS LOADED");

end