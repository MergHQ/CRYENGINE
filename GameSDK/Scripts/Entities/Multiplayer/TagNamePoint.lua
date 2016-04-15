----------------------------------------------------------------------------------------------------
--  Crytek Source File.
--  Copyright (C), Crytek Studios, 2009.
----------------------------------------------------------------------------------------------------
--  $Id$
--  $DateTime$
--  Description: Tag Name point entity - used to provide positional information for rendering fake player tag names
--
----------------------------------------------------------------------------------------------------
--  History:
--  - 10/07/2012  : Created by Jonathan Bunner
--
----------------------------------------------------------------------------------------------------
TagNamePoint =
{
	Client = {},
	Server = {},

	Editor={
		Icon="spectator.bmp", -- Just something for now
		DisplayArrow=0,
	},
	
	Properties=
	{
		bEnabled    = 1,
	},
}

function TagNamePoint.Client:OnInit()
--System.Log( "TagNamePoint.Client:OnInit()" );
	if (not self.bInitialized) then
		--System.Log( "About to call RegisterTag Name Point" );
		self.bInitialized = 1;
		--Game.RegisterTagNamePoint(self.id); --michiel
	end
end



