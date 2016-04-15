local Behavior = CreateAIBehavior("HeliFireRockets",
{
	Constructor = function (self, entity)
		entity:SelectPrimaryWeapon()
		entity:SelectPipe(0, "do_nothing")
		
		local postures = 
		{
			{
				name = "StandAim",
				type = POSTURE_AIM,
				stance = STANCE_STAND,
				priority = 8.0,
				
				{
					name = "StandAimCenter",
					lean = 0.0,
					priority = 0.0,
				},
			},
			
			{
				name = "CrouchAim",
				type = POSTURE_AIM,
				stance = STANCE_CROUCH,
				priority = 8.0,
				
				{
					name = "CrouchAimCenter",
					lean = 0.0,
					priority = 0.0,
				},
			},
		}

		AI.SetPostures(entity.id, postures)
	end,
})