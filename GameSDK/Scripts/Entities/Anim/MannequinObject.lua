--------------------------------------------------------------------------
--	Crytek Source File.
-- 	Copyright (C), Crytek Studios, 1999-2014.
--------------------------------------------------------------------------
--	File Name        : MannequinObject.lua
--	Version          : v1.00
--	Created          : 13/05/2014 by Jean Geffroy
--------------------------------------------------------------------------

MannequinObject =
{

	Properties =
	{
		objModel = "",
		fileActionController = "",
		fileAnimDatabase3P = "",
	},

	Editor = 
	{
		Icon = "user.bmp",
		IconOnTop = 1,
	},
}


function MannequinObject:OnPropertyChange()
	-- The OnPropertyChange callback is forwarded to script directly by the editor.
	-- As most of this entity is written in C++, we just want to send a notification
	-- that a property has changed, and deal with it there.
	self:ProcessBroadcastEvent( "OnPropertyChange" );
end
