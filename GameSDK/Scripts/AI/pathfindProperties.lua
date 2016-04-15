function AssignPFPropertiesToPathType(...)
	AI.AssignPFPropertiesToPathType(...);
	if (System.IsEditor() and EditorAI) then
		EditorAI.AssignPFPropertiesToPathType(...);
	end
end

-- Default properties for humans
AssignPFPropertiesToPathType(
	"AIPATH_HUMAN",
	NAV_TRIANGULAR + NAV_WAYPOINT_HUMAN + NAV_SMARTOBJECT,
	0.0, 0.0, 0.0, 0.0, 0.0,
	5.0, 0.5, -10000.0, 0.0, 20.0, 3.5,
	0, 0.4, 2, 45, 1, true);

AI.AssignPathTypeToSOUser("Human", "AIPATH_HUMAN");

-- Default properties for car/vehicle agents
AssignPFPropertiesToPathType(
	"AIPATH_CAR",
	NAV_TRIANGULAR + NAV_WAYPOINT_HUMAN + NAV_ROAD, 
	18.0, 18.0, 0.0, 0.0, 0.0, 
	0.0, 1.5, -10000.0, 0.0, 0.0, 7.0,
	0, 0.3, 2.0, 0.0, 5, true);

AssignPFPropertiesToPathType(
	"AIPATH_TANK",
	NAV_TRIANGULAR + NAV_WAYPOINT_HUMAN + NAV_ROAD, 
	18.0, 18.0, 0.0, 0.0, 0.0, 
	0.0, 1.5, -10000.0, 0.0, 0.0, 7.0,
	0, 0.3, 2.0, 0.0, 6, true);

--- character that travels on the surface but has no preferences - except it prefers to walk around
--- hills rather than over them
AssignPFPropertiesToPathType(
	"AIPATH_DEFAULT",
	NAV_TRIANGULAR + NAV_WAYPOINT_HUMAN + NAV_SMARTOBJECT,
	0.0, 0.0, 0.0, 0.0, 0.0, 
	5.0, 0.5, -10000.0, 0.0, 20.0, 3.5,
	0, 0.4, 2, 45, 7, true);
