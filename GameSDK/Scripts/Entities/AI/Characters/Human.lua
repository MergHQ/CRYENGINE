Script.ReloadScript( "SCRIPTS/Entities/AI/Characters/Human_x.lua")
Script.ReloadScript( "SCRIPTS/Entities/actor/BasicActor.lua")
Script.ReloadScript( "SCRIPTS/Entities/AI/Shared/BasicAI.lua")
CreateActor(Human_x)
Human=CreateAI(Human_x)

Script.ReloadScript( "SCRIPTS/AI/Assignments.lua")
InjectAssignmentFunctionality(Human)
AddDefendAreaAssignment(Human)
AddHoldPositionAssignment(Human)
AddCombatMoveAssignment(Human)
AddPsychoCombatAllowedAssignment(Human)

Human:Expose()