----------------------------------------------------------------------------------------------------
--  Crytek Source File.
--  Copyright (C), Crytek Studios, 2009.
----------------------------------------------------------------------------------------------------
--  $Id$
--  $DateTime$
--  Description: Camera point entity
--
----------------------------------------------------------------------------------------------------
--  History:
--  - 03/07/2012  : Created by Jonathan Bunner
--
----------------------------------------------------------------------------------------------------
CameraPoint =
{
	Client = {},
	Server = {},

	Editor={
		Icon="spectator.bmp", -- Just something for now
		DisplayArrow=1,
	},
	
	Properties=
	{
		bEnabled    = 1,
		PitchLimits = 45.0,
		YawLimits   = 45.0,
	},
}

