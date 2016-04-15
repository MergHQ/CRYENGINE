ViewDist = {
	type = "ViewDistController",
	Properties = {	
		MaxViewDist = 25,
		fFadeTime = 1,
		},
	Editor={
		Model="Editor/Objects/T.cgf",
	},
}

function ViewDist:OnSpawn()
  self:SetFlagsExtended(ENTITY_FLAG_EXTENDED_NEEDS_MOVEINSIDE, 0);
end

function ViewDist:OnSave(tbl)
	tbl.outsideViewDist = self.outsideViewDist
	tbl.occupied = self.occupied
	tbl.fadeamt = self.fadeamt
	tbl.lasttime = self.lasttime
	tbl.exitfrom = self.exitfrom
end

function ViewDist:OnLoad(tbl)
	self.outsideViewDist = tbl.outsideViewDist
	self.occupied = tbl.occupied
	self.fadeamt = tbl.fadeamt
	self.lasttime = tbl.lasttime
	self.exitfrom = tbl.exitfrom
end

function ViewDist:OnInit()
	self.outsideViewDist = 0;
	self.occupied = 0;

	self:OnReset();

	self.outsideViewDist = System.ViewDistanceGet( );

	self:RegisterForAreaEvents(1);
end

function ViewDist:OnPropertyChange()
	self:OnReset();
end

function ViewDist:OnReset()

	if(self.occupied == 1 ) then
		self:OnLeaveArea( );
	end	
		
	self.occupied = 0;
	
	self:KillTimer(0);
end
-----------------------------------------------------------------------------
--	fade: 0-out 1-in
function ViewDist:OnProceedFadeArea( player,areaId,fadeCoeff )

--System.LogToConsole("--> FadeIS "..fadeCoeff.." vDist "..Lerp(self.outsideViewDist, self.Properties.MaxViewDist, Math.Sqrt( fadeCoeff )));
--	System.SetViewDistance(1200);

--	if(player ~= _localplayer) then
--		return
--	end	

	self:FadeViewDist(fadeCoeff);

end

function ViewDist:ResetValues()

	System.ViewDistanceSet( self.outsideViewDist);
end

-----------------------------------------------------------------------------
function ViewDist:OnEnterArea( player,areaId )

--	if(player ~= _localplayer) then
--		return
--	end	

--System.LogToConsole("--> Entering ViewDist Area "..areaId);

	if(self.occupied == 1) then return end
	
	self.outsideViewDist = System.ViewDistanceGet( );
	self.occupied = 1;
end

-----------------------------------------------------------------------------
function ViewDist:OnLeaveArea( player,areaId )

--System.LogToConsole("--> Leaving ViewDist Area "..areaId);

--	if(player ~= _localplayer) then
--		return
--	end	
	
	self:ResetValues();
	self.occupied = 0;
end
-----------------------------------------------------------------------------
function ViewDist:OnShutDown()
	self:RegisterForAreaEvents(0);
end

-----------------------------------------------------------------------------
function ViewDist:Event_Enable( sender )
		
	if(self.occupied == 0 ) then
		
		if (self.fadeamt and self.fadeamt<1) then
			self:ResetValues();
		end
		
		self:OnEnterArea( );
				
		self.fadeamt = 0;
		self.lasttime = _time;
		self.exitfrom = nil;
	end	
	
	self:SetTimer(0, 1);
	
	BroadcastEvent( self,"Enable" );
end

function ViewDist:Event_Disable( sender )
		
	if(self.occupied == 1 ) then
		
		--self:OnLeaveArea( );
		self.occupied = 0;
		
		self.fadeamt = 0;
		self.lasttime = _time;
		self.exitfrom = 1;
	end	
	
	self:SetTimer(0, 1);
	
	BroadcastEvent( self,"Disable" );
end

function ViewDist:OnTimer()

	--System.Log("Ontimer ");

	self:SetTimer(0, 1);
	
	if (self.fadeamt) then
		
		local delta = _time - self.lasttime;
		self.lasttime = _time;
		
		self.fadeamt = self.fadeamt + (delta / self.Properties.fFadeTime);
		
		if (self.fadeamt>=1) then
			
			self.fadeamt = 1;
			self:KillTimer(0);
		end
		
		----------------------------------------------
		--fade	
		local fadeCoeff = self.fadeamt;
		
		if (self.exitfrom) then
			fadeCoeff = 1 - fadeCoeff;	
		end
		
		fadeCoeff = math.sqrt( fadeCoeff );
		
		self:FadeViewDist(fadeCoeff);
		--self:OnProceedFadeArea( nil,0,self.fadeamt );
	else
		self:KillTimer(0);
	end
end

function ViewDist:FadeViewDist(fadeCoeff)

	local cCoeff = math.sqrt( fadeCoeff );
	fadeCoeff = cCoeff;
	
	System.ViewDistanceSet( Lerp(self.outsideViewDist, self.Properties.MaxViewDist, fadeCoeff) );
end

ViewDist.FlowEvents =
{
	Inputs =
	{
		Disable = { ViewDist.Event_Disable, "bool" },
		Enable = { ViewDist.Event_Enable, "bool" },
	},
	Outputs =
	{
		Disable = "bool",
		Enable = "bool",
	},
}
