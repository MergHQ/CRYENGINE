----------------------------------------------------------------------------------------------------
--  Crytek Source File.
--  Copyright (C), Crytek Studios, 2001-2005.
----------------------------------------------------------------------------------------------------
--  $Id$
--  $DateTime$
--  Description: this is a global place for all ais to share information (formation poits taken, enemy pos, etc)
--
----------------------------------------------------------------------------------------------------
--  History:
--  - 22:nov:2005   : Created by Kirill Bulatsev
--
----------------------------------------------------------------------------------------------------´



AIBlackBoard = 	{

}	

function AIBlackBoard_Reset()

	for i,v in pairs(AIBlackBoard) do
		AIBlackBoard[i] = nil;
	end
--	AIBlackBoard = {};

end    
