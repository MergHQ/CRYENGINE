AI.TacticalPositionManager.HumanGrunt = AI.TacticalPositionManager.HumanGrunt or {};

function AI.TacticalPositionManager.HumanGrunt:OnInit()

	AI.RegisterTacticalPointQuery({
		Name = "PointVisibleFromTarget",
		{
			Parameters =
			{
				density = 8.0,
			},
			Generation =
			{
				grid_around_puppet = 25.0,
			},
			Conditions =
			{
				max_changeInDistance_to_attentionTarget = 0.0,
				visible_from_attentionTarget = true,
			},
			Weights =
			{
				distance_from_puppet = -0.5,
				distance_from_attentionTarget = 0.3,
				towards_the_attentionTarget = 1.0,
			},
		},
	});
	
	AI.RegisterTacticalPointQuery({
		Name = "LeapfrogCoverTowardsTarget",
		{
			Generation =
			{
				cover_from_attentionTarget_around_puppet = 25,
			},
			Conditions =
			{
				min_distance_to_attentionTarget = 8,
				max_changeInDistance_to_attentionTarget = -2.0,
			},
			Weights =
			{
				distance_to_puppet = -0.5,
				distance_to_attentionTarget = -0.5,
				directness_to_attentionTarget = 1.0,
			},
		},
	});

	AI.RegisterTacticalPointQuery({
		Name = "FindCoverImmediately",
		{
			Generation =
			{
				cover_from_attentionTarget_around_puppet = 25,
			},
			Conditions =
			{
				min_distance_to_attentionTarget = 5,
			},
			Weights =
			{
				distance_to_puppet = -0.9,
				distance_to_attentionTarget = -0.1,
				changeInDistance_to_attentionTarget = -0.5,
				otherSide_of_attentionTarget = -3.0,
			},
		},
	});
	
	AI.RegisterTacticalPointQuery({
		Name = "FindCoverImmediatelyForHuman",
		{
			Generation =
			{
				cover_from_attentionTarget_around_puppet = 20,
			},
			Conditions =
			{
				min_distance_to_attentionTarget = 10,
			},
			Weights =
			{
				distance_to_puppet = -0.9,
				distance_to_attentionTarget = -0.1,
				otherSide_of_attentionTarget = -5.0,
			},
			Generation =
			{
				cover_from_attentionTarget_around_puppet = 60,
			},
			Conditions =
			{
				min_distance_to_attentionTarget = 10,
			},
			Weights =
			{
				distance_to_puppet = -0.9,
				distance_to_attentionTarget = -0.1,
				otherSide_of_attentionTarget = -5.0,
			},
		},
	});
	
	AI.RegisterTacticalPointQuery({
		Name = "FindAdvanceCoverForHuman",
		{
			Generation =
			{
				cover_from_attentionTarget_around_puppet = 30,
			},
			Conditions =
			{
				min_distance_to_attentionTarget = 15.0,
				max_changeInDistance_to_attentionTarget = -15.0,
				otherSide_of_attentionTarget = false,
			},
			Weights =
			{
				distance_to_puppet = -0.8,
				distance_to_attentionTarget = 0.2,
			},
		},
		{
			Generation =
			{
				cover_from_attentionTarget_around_puppet = 60,
			},
			Conditions =
			{
				min_distance_to_attentionTarget = 15.0,
				max_changeInDistance_to_attentionTarget = -15.0,
				otherSide_of_attentionTarget = false,
			},
			Weights =
			{
				distance_to_puppet = -0.8,
				distance_to_attentionTarget = 0.2,
			},
		},
	});

	AI.RegisterTacticalPointQuery({
		Name = "FindCloseCoverWithShootingPostureForHuman",
		{
			Generation =
			{
				cover_from_attentionTarget_around_puppet = 6,
			},
			Conditions =
			{
				otherSide_of_attentionTarget = false,
				min_distance_from_attentionTarget = 4.0,
				hasShootingPosture_to_attentionTarget = true,
			},
			Weights =
			{
				distance_to_puppet = -1.0,
			},
		},
	});

	AI.RegisterTacticalPointQuery({
		Name = "CoverWithShootingPosture",
		{
			Generation =
			{
				cover_from_attentionTarget_around_puppet = 5,
			},
			Conditions =
			{
				hasShootingPosture_to_attentionTarget = true,
			},
			Weights =
			{
				distance_to_puppet = -1.0,
			},
		},
		{
			Generation =
			{
				cover_from_attentionTarget_around_puppet = 15,
			},
			Conditions =
			{
				min_distance_to_attentionTarget = 8,
				hasShootingPosture_to_attentionTarget = true,
			},
			Weights =
			{
				distance_to_puppet = -1.0,
			},
		},
	});

	AI.RegisterTacticalPointQuery({
		Name = "HumanCoverWithShootingPostureToTarget",
		{
			Generation =
			{
				cover_from_attentionTarget_around_puppet = 25,
			},
			Conditions =
			{
				hasShootingPosture_to_attentionTarget = true,
				min_effectiveCoverHeight_from_attentionTarget = 1.75,
			},
			Weights =
			{
				distance_to_puppet = -0.9,
				distance_to_attentionTarget = -0.1,
			},
		},
	});

	AI.RegisterTacticalPointQuery({
		Name = "SniperCombatCover",
		{
			Generation =
			{
				cover_from_attentionTarget_around_puppet = 10,
			},
			Conditions =
			{
			},
			Weights =
			{
				distance_to_puppet = -1.0,
			},
		},
	});

	AI.RegisterTacticalPointQuery({
		Name = "EngageCloseCover",
		{
			Generation =
			{
				cover_from_attentionTarget_around_puppet = 5,
			},
			Conditions =
			{
				hasShootingPosture_to_attentionTarget = true,
			},
			Weights =
			{
				distance_to_puppet = -1.0,
			},
		},
	});

	AI.RegisterTacticalPointQuery({
		Name = "HumanCombatHidespotQuery",
		{
			Generation =
			{
				cover_from_attentionTarget_around_puppet = 25,
			},
			Conditions =
			{
				min_distance_to_attentionTarget = 8.0,
				canReachBefore_the_attentionTarget = true,
				max_distance_from_battlefront = 15.0,
				hasShootingPosture_to_attentionTarget = true,
				max_changeInDistance_to_attentionTarget = 10.0,
			},
			Weights =
			{
				distance_to_puppet = -0.05,
				distance_to_attentionTarget = -0.05,
			},
		},
		{
			Generation =
			{
				cover_from_attentionTarget_around_puppet = 20,
			},
			Conditions =
			{
				min_distance_to_attentionTarget = 8.0,
				hasShootingPosture_to_attentionTarget = true,
				max_changeInDistance_to_attentionTarget = 10.0,
			},
			Weights =
			{
				distance_to_puppet = -0.1,
				distance_to_attentionTarget = -0.2,
				distance_to_battlefront = -0.7,
			},
		},
	});

	AI.RegisterTacticalPointQuery({
		Name = "HumanDistantCombatHidespotQuery",
		{
			Generation =
			{
				cover_from_attentionTarget_around_puppet = 30,
			},
			Conditions =
			{
				min_distance_to_attentionTarget = 8.0,
				canReachBefore_the_attentionTarget = true,
			},
			Weights =
			{
				distance_to_puppet = -0.05,
				distance_to_attentionTarget = -0.05,
				distance_to_battlefront = -0.7,
			},
		},
	});

	AI.RegisterTacticalPointQuery({
		Name = "HumanUnderPressureHidespotQuery",
		{
			Generation = {
				cover_from_attentionTarget_around_puppet = 30,
			},
			Conditions = {
				min_distance_to_attentionTarget = 8.0,
				canReachBefore_the_attentionTarget = true,
			},
			Weights = {
				distance_to_puppet = -0.1,
				distance_to_attentionTarget = -0.01,
			},
		},
	});

	AI.RegisterTacticalPointQuery({
		Name = "HideSpotFromRefPointQuery",
		{
			Generation = {
				cover_from_referencePoint_around_puppet = 15,
			},
			Conditions = {
				towards_the_attentionTarget = false,
			},
			Weights = {
				distance_to_puppet = -0.1,
			},
		},
	});

	AI.RegisterTacticalPointQuery({
		Name = "HumanBaseOfFireQuery",
		{
			Generation = {
				-- This distance is directly linked to the behavior property "baseOfFireRange",
				-- so it needs to be at least equal or less, or else the agent can get stuck
				-- between behaviors. /Mario
				cover_from_attentionTarget_around_attentionTarget = 20,
			},
			Conditions = {
				towards_the_attentionTarget = true,
				hasShootingPosture_to_attentionTarget = true,
				min_distance_to_attentionTarget = 8.0,
			},
			Weights = {
				distance_to_puppet = -0.1,
			},
		},
	});

	AI.RegisterTacticalPointQuery({
		Name = "SniperSpotQuery",
		{
			Parameters =
			{
				objectsType = AIAnchorTable.SNIPER_SPOT,
			},
			Generation =
			{
				objects_around_puppet = 15,
			},
			Conditions =
			{
				min_distance_to_attentionTarget = 10.0,
				visible_from_attentionTarget = true,
			},
			Weights =
			{
				distance_from_puppet = -1.0,
			},
		},
		{
			Parameters =
			{
				objectsType = AIAnchorTable.SNIPER_SPOT,
			},
			Generation =
			{
				objects_around_puppet = 15,
			},
			Conditions =
			{
				min_distance_to_attentionTarget = 10.0,
			},
			Weights =
			{
				distance_from_puppet = -1.0,
			},
		},
	});

	AI.RegisterTacticalPointQuery({
		Name = "SniperSpotQueryForceVisible",
		{
			Parameters =
			{
				objectsType = AIAnchorTable.SNIPER_SPOT,
			},
			Generation =
			{
				objects_around_puppet = 30,
			},
			Conditions =
			{
				min_distance_to_attentionTarget = 10.0,
				visible_from_attentionTarget = true,
			},
			Weights =
			{
				distance_from_puppet = -1.0,
			},
		},
	});

AI.RegisterTacticalPointQuery({
		Name = "GrenadierSpotQuery",
		{
			Parameters =
			{
				objectsType = AIAnchorTable.GRENADIER_SPOT,
			},
			Generation =
			{
				objects_around_puppet = 15,
			},
			Conditions =
			{
				min_distance_to_attentionTarget = 10.0,
				visible_from_attentionTarget = true,
				isAdvantageSpotOccupied_for_puppet = false,
			},
			Weights =
			{
				distance_from_puppet = -1.0,
			},
		},
		{
			Parameters =
			{
				objectsType = AIAnchorTable.GRENADIER_SPOT,
			},
			Generation =
			{
				objects_around_puppet = 15,
			},
			Conditions =
			{
				min_distance_to_attentionTarget = 10.0,
				isAdvantageSpotOccupied_for_puppet = false,
			},
			Weights =
			{
				distance_from_puppet = -1.0,
			},
		},
	});	
	
	AI.RegisterTacticalPointQuery({
		Name = "DefensiveLineLeft",
		{
			Generation =
			{
				cover_from_attentionTarget_around_battlefront = 20,
			},
			Conditions =
			{
				min_distance_to_attentionTarget = 10.0,
				min_distance_to_battlefront = 10.0,
				hasShootingPosture_to_attentionTarget = true,
				max_changeInDistance_to_attentionTarget = 5.0,
				max_distanceLeft_to_attentionTarget = 0,
				towards_the_attentionTarget = true,
				crossesLineOfFire_to_attentionTarget = false,
			},
			Weights =
			{
				distance_to_puppet = -0.25,
				distance_to_attentionTarget = -0.5,
				directness_to_attentionTarget = -0.25,
			},
		},
		{
			Generation =
			{
				cover_from_attentionTarget_around_battlefront = 20,
			},
			Conditions =
			{
				min_distance_to_attentionTarget = 10.0,
				min_distance_to_battlefront = 10.0,
				hasShootingPosture_to_attentionTarget = true,
				max_changeInDistance_to_attentionTarget = 5.0,
				towards_the_attentionTarget = true,
			},
			Weights =
			{
				distance_to_puppet = -0.15,
				distance_to_attentionTarget = -0.5,
				directness_to_attentionTarget = -0.2,
				distanceLeft_to_attentionTarget = 0.15,
			},
		},
	});

	AI.RegisterTacticalPointQuery({
		Name = "DefensiveLineCenter",
		{
			Generation =
			{
				cover_from_attentionTarget_around_puppet = 20,
			},
			Conditions =
			{
				min_distance_to_attentionTarget = 10.0,
				min_distance_to_battlefront = 10.0,
				hasShootingPosture_to_attentionTarget = true,
				max_changeInDistance_to_attentionTarget = 5.0,
			},
			Weights =
			{
				distance_to_puppet = -0.5,
				distance_to_attentionTarget = -0.5,
			},
		},
	});

	AI.RegisterTacticalPointQuery({
		Name = "DefensiveLineRight",
		{
			Generation =
			{
				cover_from_attentionTarget_around_battlefront = 20,
			},
			Conditions =
			{
				min_distance_to_attentionTarget = 10.0,
				min_distance_to_battlefront = 10.0,
				hasShootingPosture_to_attentionTarget = true,
				max_changeInDistance_to_attentionTarget = 5.0,
				min_distanceLeft_to_attentionTarget = 0,
				towards_the_attentionTarget = true,
				crossesLineOfFire_to_attentionTarget = false,
			},
			Weights =
			{
				distance_to_puppet = -0.25,
				distance_to_attentionTarget = -0.5,
				directness_to_attentionTarget = -0.25,
			},
		},
		{
			Generation =
			{
				cover_from_attentionTarget_around_battlefront = 20,
			},
			Conditions =
			{
				min_distance_to_attentionTarget = 10.0,
				min_distance_to_battlefront = 10.0,
				hasShootingPosture_to_attentionTarget = true,
				max_changeInDistance_to_attentionTarget = 5.0,
				towards_the_attentionTarget = true,
			},
			Weights =
			{
				distance_to_puppet = -0.15,
				distance_to_attentionTarget = -0.5,
				directness_to_attentionTarget = -0.2,
				distanceLeft_to_attentionTarget = -0.15,
			},
		},
	});

	AI.RegisterTacticalPointQuery({
		Name = "HumanCoverAroundDefendPos",
		{
			Generation =
			{
				cover_from_attentionTarget_around_referencePoint = 15,
			},
			Conditions =
			{
				min_distance_to_attentionTarget = 5.0,
				hasShootingPosture_to_attentionTarget = true,
			},
			Weights =
			{
				distance_to_puppet = -0.5,
				--random = 1.0,
			},
		},
		{
			Generation =
			{
				cover_from_attentionTarget_around_referencePoint = 15,
			},
			Conditions =
			{
				min_distance_to_attentionTarget = 5.0,
			},
			Weights =
			{
				distance_to_puppet = -0.5,
				--random = 1.0,
			},
		},
	});

	AI.RegisterTacticalPointQuery({
		Name = "HumanCoverTowardsDefendPos",
		{
			Generation =
			{
				cover_from_attentionTarget_around_puppet = 20,
			},
			Conditions =
			{
				max_distance_to_referencePoint = 15.0,
				min_distance_to_attentionTarget = 10.0,
				hasShootingPosture_to_attentionTarget = true,
			},
			Weights =
			{
				distance_to_referencePoint = -1.0,
			},
		},
		{
			Generation =
			{
				cover_from_attentionTarget_around_puppet = 20,
			},
			Conditions =
			{
				min_distance_to_attentionTarget = 10.0,
				hasShootingPosture_to_attentionTarget = true,
			},
			Weights =
			{
				distance_to_referencePoint = -1.0,
			},
		},
		{
			Generation =
			{
				cover_from_attentionTarget_around_referencePoint = 15,
			},
			Conditions =
			{
				min_distance_to_attentionTarget = 10.0,
				hasShootingPosture_to_attentionTarget = true,
			},
			Weights =
			{
				distance_to_puppet = -1.0,
			},
		},
		{
			Generation =
			{
				cover_from_attentionTarget_around_referencePoint = 15,
			},
			Conditions =
			{
				min_distance_to_attentionTarget = 10.0,
			},
			Weights =
			{
				distance_to_puppet = -1.0,
			},
		},
	});

	AI.RegisterTacticalPointQuery({
		Name = "HumanHoldGround",
		{
			Generation =
			{
				cover_from_attentionTarget_around_referencePoint = 5,
			},
			Conditions =
			{
				hasShootingPosture_to_attentionTarget = true,
			},
			Weights =
			{
				distance_to_puppet = -1.0,
			},
		},
	});

	AI.RegisterTacticalPointQuery(
	{
		Name = "HumanCoverShift",
		{
			Generation =
			{
				cover_from_attentionTarget_around_puppet = 5.0,
			},
			Conditions =
			{
				min_distance_from_puppet = 2.0,
				hasShootingPosture_to_attentionTarget = true,
			},
			Weights =
			{
				random = 1.0,
			},
		},
	})

	AI.RegisterTacticalPointQuery(
	{
		Name = "HumanEstablishLineOfSightQuery",
		{
			Parameters =
			{
				density = 4.0,
			},
			Generation =
			{
				grid_around_puppet = 15.0,
			},
			Conditions =
			{
				max_changeInDistance_to_referencePoint = 4.0,
				min_changeInDistance_to_referencePoint = -4.0,
				visible_from_referencePoint = true,
			},
			Weights =
			{
				distance_from_puppet = -0.8,
				distance_from_referencePoint = -0.2,
			},
		},
	})

	AI.RegisterTacticalPointQuery(
	{
		Name = "FindPositionNearSuspiciousSound",
		{
			Generation =
			{
				pointsInNavigationMesh_around_attentionTarget = 5.0, -- Width (height is hardcoded to be 20 down and 0.2 up)
			},
			Conditions =
			{
				min_distance_from_attentionTarget = 3.0,
			},
			Weights =
			{
				distance_from_attentionTarget = -1.0,
			},
		},
	})


end









