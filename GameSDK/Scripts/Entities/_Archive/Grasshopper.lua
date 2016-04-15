Grasshopper = {
	type = "GrasshopperController",
	Properties = {	
		GrasshopperNumber = 29,
		objCGF1 = "Objects/characters/animals/grasshopper/grasshopper.cgf",
		objCGF2 = "",
		objCGF3 = "",
		objCGF4 = "",
		},
	Editor={
		Model="Objects/Editor/T.cgf",
	},
	
	outsideGrasshopperNumber = 0,
}


function Grasshopper:OnInit()

	if (strlen(self.Properties.objCGF1)>0) then
		self:LoadObject(self.Properties.objCGF1, 0, 1);
	end
	
	if (strlen(self.Properties.objCGF2)>0) then
		self:LoadObject(self.Properties.objCGF2, 1, 1);
	end

	if (strlen(self.Properties.objCGF3)>0) then
		self:LoadObject(self.Properties.objCGF3, 2, 1);		
	end

	if (strlen(self.Properties.objCGF4)>0) then
		self:LoadObject(self.Properties.objCGF4, 3, 1);	
	end

end
-----------------------------------------------------------------------------
--	fade: 0-out 1-in
function Grasshopper:OnProceedFadeArea( player,areaId,fadeCoeff )

--System.LogToConsole("--> FadeIS "..fadeCoeff.." vDist "..Lerp(self.outsideViewDist, self.Properties.MaxViewDist, Math.Sqrt( fadeCoeff )));
--	System.SetViewDistance(1200);

--	if(player ~= _localplayer) then
--		return
--	end	

--local	cCoeff = sqrt( fadeCoeff );
--	fadeCoeff = cCoeff
	System.SetGrasshopperCount( Lerp(self.outsideGrasshopperNumber, self.Properties.GrasshopperNumber, fadeCoeff) );
end

-----------------------------------------------------------------------------
function Grasshopper:OnEnterArea( player,areaId )

--	if(player ~= _localplayer) then
--		return
--	end	

--System.LogToConsole("--> Entering ViewDist Area "..areaId);

	self.outsideGrasshopperNumber = System.GetGrasshopperCount( );
	System.SetGrasshopperCGF(self.id);
	
end

-----------------------------------------------------------------------------
function Grasshopper:OnLeaveArea( player,areaId )

--System.LogToConsole("--> Leaving ViewDist Area "..areaId);

--	if(player ~= _localplayer) then
--		return
--	end	
	
	System.SetGrasshopperCount( self.outsideGrasshopperNumber);

end
-----------------------------------------------------------------------------
function Grasshopper:OnShutDown()
end