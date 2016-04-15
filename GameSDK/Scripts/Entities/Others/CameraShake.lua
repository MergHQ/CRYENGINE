--------------------------------------------------------------------------
--	Crytek Source File.
-- 	Copyright (C), Crytek Studios, 2001-2005.
--------------------------------------------------------------------------
--	$Id$
--	$DateTime$
--	Description: Camera Shake helper entity
--
--------------------------------------------------------------------------
--  History:
--  - 06:12:2005   19:00 : Created by alexl
--
--------------------------------------------------------------------------

CameraShake =
{
	Properties =
	{
		vector_Position = {x=0, y=0, z=0 },
		fRadius = 10.0,
		fStrength = 1.0,
		fDuration = 2.0,
		fFrequency = 0.5
	},

	Editor =
	{
		Icon = "shake.bmp",
	},
}

function CameraShake:OnPropertyChange()
end

function CameraShake:Event_Shake(sender)
	g_gameRules:ClientViewShake(self.Properties.vector_Position, nil, 0.0, self.Properties.fRadius,self.Properties.fStrength,self.Properties.fDuration,self.Properties.fFrequency);
end

CameraShake.FlowEvents =
{
	Inputs =
	{
		Shake = { CameraShake.Event_Shake, "bool" },
	},
	Outputs =
	{
	},
}
