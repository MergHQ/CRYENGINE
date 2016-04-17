BFly = {
	type = "BFlyController",
	Properties = {	
		BFlyNumber = 23,
		},
	Editor={
		Model="Objects/Editor/T.cgf",
	},
	
	outsideBFlyNumber = 0,
}


function BFly:OnInit()
end
-----------------------------------------------------------------------------
--	fade: 0-out 1-in
function BFly:OnProceedFadeArea( player,areaId,fadeCoeff )

--System.LogToConsole("--> bfly FadeIS "..fadeCoeff );
--	System.SetViewDistance(1200);

--	if(player ~= _localplayer) then
--		return
--	end	
--local	cCoeff = sqrt( fadeCoeff );
--	fadeCoeff = cCoeff
	System.SetBFCount( Lerp(self.outsideBFlyNumber, self.Properties.BFlyNumber, fadeCoeff) );
end

-----------------------------------------------------------------------------
function BFly:OnEnterArea( player,areaId )

--	if(player ~= _localplayer) then
--		return
--	end	

--System.LogToConsole("--> Entering BFLY Area "..areaId);
	
	self.outsideBFlyNumber = System.GetBFCount( );
end

-----------------------------------------------------------------------------
function BFly:OnLeaveArea( player,areaId )

--System.LogToConsole("--> Leaving BFLY Area "..areaId);

--	if(player ~= _localplayer) then
--		return
--	end	
	
	System.SetBFCount( self.outsideBFlyNumber);

end
-----------------------------------------------------------------------------
function BFly:OnShutDown()
end
