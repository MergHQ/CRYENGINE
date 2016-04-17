AI.TacticalPositionManager.Helicopter = AI.TacticalPositionManager.Helicopter or {};

function AI.TacticalPositionManager.Helicopter:OnInit()

	AI.RegisterTacticalPointQuery({
		Name = "HelicopterFindGoodFiringSpot",
		{
			Parameters =
			{
				density = 8.0,
				horizontalSpacing = 4,
			},
			Generation =
			{
				pointsInDesignerPath_around_puppet = 75.0,
			},
			Conditions =
			{
				min_distance2d_to_attentionTarget = 15.0,
				canShootTwoRayTest_to_attentionTarget = true,
			},
			Weights =
			{
				distance_from_puppet = -1.0,
			},
		},
	});

end









