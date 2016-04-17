AI.TacticalPositionManager.SDKGrunt = AI.TacticalPositionManager.SDKGrunt or {};

function AI.TacticalPositionManager.SDKGrunt:OnInit()

	AI.RegisterTacticalPointQuery({
		Name = "SDKGrunt_CoverFromUnknownEnemy",
		{
			Generation =
			{
				cover_from_attentionTarget_around_puppet = 15.0,
			},
			Conditions =
			{
			},
			Weights =
			{
				distance_to_puppet = -1.0
			},
		}
	});

	AI.RegisterTacticalPointQuery({
		Name = "SDKGrunt_GetAwayFromUnknownEnemy",
		{
			Generation =
			{
				grid_around_puppet = 5.0,
			},
			Conditions =
			{
				isInNavigationMesh = true,
			},
			Weights =
			{
				distance_to_attentionTarget = 0.75,
				towards_the_attentionTarget = -1.0
			},
		}
	});

	AI.RegisterTacticalPointQuery({
		Name = "SDKGrunt_OpenCombat",
		{
			Generation =
			{
				grid_around_attentionTarget = 15,
			},
			Conditions =
			{
				canShoot_to_attentionTarget = true,
				min_distance_to_attentionTarget = 5,
				max_distance_to_attentionTarget = 15,
				isInNavigationMesh = true,
			},
			Weights =
			{
				random = 1.0
			}
		}
	});

	AI.RegisterTacticalPointQuery({
		Name = "SDKGrunt_OpenCombat_LimitedTravelDistance",
		{
			Parameters =
			{
				density = 4.0
			},
			Generation =
			{
				grid_around_attentionTarget = 15,
			},
			Conditions =
			{
				canShoot_to_attentionTarget = true,
				min_distance_to_attentionTarget = 5,
				max_distance_to_puppet = 10,
				isInNavigationMesh = true,
			},
			Weights =
			{
				random = 1.0
			}
		},
		{
			Parameters =
			{
				density = 4.0
			},
			Generation =
			{
				grid_around_attentionTarget = 15,
			},
			Conditions =
			{
				min_distance_to_attentionTarget = 5,
				max_distance_to_puppet = 10,
				isInNavigationMesh = true,
			},
			Weights =
			{
				random = 0.5,
				distance_to_attentionTarget = -0.5,
				distance_to_puppet = -1.0
			}
		},
		{
			Generation =
			{
				grid_around_puppet = 10
			},
			Conditions =
			{
				isInNavigationMesh = true,
			},
			Weights =
			{
				random = 0.5,
				distance_to_attentionTarget = -0.5
			},
		},
	});

	AI.RegisterTacticalPointQuery({
		Name = "SDKGrunt_CoverToShootFrom",
		{
			Generation =
			{
				cover_from_attentionTarget_around_puppet = 15.0,
			},
			Conditions =
			{
				hasShootingPosture_to_attentionTarget = true,
				min_distance_to_attentionTarget = 5,
			},
			Weights =
			{
				distance_to_puppet = -1.0
			},
		}
	});

	AI.RegisterTacticalPointQuery({
		Name = "SDKGrunt_CoverToShootFrom_LimitedTravelDistance",
		{
			Generation =
			{
				cover_from_attentionTarget_around_puppet = 15.0,
			},
			Conditions =
			{
				hasShootingPosture_to_attentionTarget = true,
				min_distance_to_attentionTarget = 5,
				max_distance_to_puppet = 10,
			},
			Weights =
			{
				distance_to_puppet = -1.0
			},
		}
	});

	AI.RegisterTacticalPointQuery({
		Name = "SDKGrunt_TargetPositionOnNavMesh",
		{
			Generation =
			{
				grid_around_attentionTarget = 20.0
			},
			Conditions =
			{
				isInNavigationMesh = true
			},
			Weights =
			{
				distance_to_attentionTarget = -1.0
			},
		},
	});

	AI.RegisterTacticalPointQuery({
		Name = "SDKGrunt_RandomSearchSpotAroundPuppet",
		{
			Generation =
			{
				grid_around_puppet = 30.0
			},
			Conditions =
			{
				min_distance_to_puppet = 10.0,
				visible_from_puppet = false,
				isInNavigationMesh = true
			},
			Weights =
			{
				distance_to_puppet = -0.5,
				random = 1.0
			},
		},
		{
			Generation =
			{
				grid_around_puppet = 30.0
			},
			Conditions =
			{
				min_distance_to_puppet = 10.0,
				isInNavigationMesh = true
			},
			Weights =
			{
				distance_to_puppet = -0.5,
				random = 1.0
			},
		},
		{
			Generation =
			{
				grid_around_puppet = 30.0
			},
			Conditions =
			{
				min_distance_to_puppet = 5.0,
				isInNavigationMesh = true
			},
			Weights =
			{
				distance_to_puppet = -0.5,
				random = 1.0
			},
		}
	});

	AI.RegisterTacticalPointQuery({
		Name = "SDKGrunt_CombatMove_CoverFromTarget",
		{
			Generation =
			{
				cover_from_attentionTarget_around_puppet = 25.0,
			},
			Conditions =
			{
				min_distance_to_puppet = 5.0,
				towards_the_referencePoint = true,
				min_distance_to_referencePoint = 4.0,
			},
			Weights =
			{
				distance_to_puppet = -1.0,
			},
		},
	});

	AI.RegisterTacticalPointQuery({
		Name = "SDK_Grunt_DefendArea_Cover",
		{
			Generation =
			{
				cover_from_attentionTarget_around_puppet = 15.0,	-- notice: size is the same as the hardcoded size of the defend-area (Assignments.lua)
			},
			Conditions =
			{
				hasShootingPosture_to_attentionTarget = true,
				min_distance_to_attentionTarget = 5.0,
			},
			Weights =
			{
				distance_to_puppet = -1.0,
			},
		},
	});

	AI.RegisterTacticalPointQuery({
		Name = "SDK_Grunt_DefendArea_RandomPointAroundRefPoint",
		{
			Generation =
			{
				grid_around_referencePoint = 10.0
			},
			Conditions =
			{
				isInNavigationMesh = true
			},
			Weights =
			{
				random = 1.0
			},
		},
	});

	AI.RegisterTacticalPointQuery({
		Name = "SDK_Grunt_HoldPosition_Cover",
		{
			Generation =
			{
				cover_from_attentionTarget_around_referencePoint = 3.0
			},
			Conditions =
			{
				hasShootingPosture_to_attentionTarget = true,
				min_distance_to_attentionTarget = 5.0,
			},
			Weights =
			{
				distance_to_puppet = -1.0,
			},
		},
	});

end
