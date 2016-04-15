RaisingWater = {
	
	type = "Trigger",

	Properties = {
		WaterVolume="",
		height_start=0,
		height_end=3,
		fSpeed=0.1, -- between 0 and 1 please
		fUpdateTime=0.1, -- in seconds
		bDrain=0,
		fDrainSpeed=0.1,
	},
	bStopped=0,
	bDraining=0,
	Editor={
		--Model="Objects/Editor/T.cgf",
		Icon="item.bmp",
	},

}

-------------------------------------------------------------------------------
function RaisingWater:OnPropertyChange()
	self:OnReset();
end;

-------------------------------------------------------------------------------
function RaisingWater:OnInit()
	self:OnReset();
end;

-------------------------------------------------------------------------------
function RaisingWater:OnShutDown()
end;

-------------------------------------------------------------------------------
function RaisingWater:OnSave(props)	
	props.currlevel = self.currlevel
	props.bStopped = self.bStopped
	props.bDraining = self.bDraining
	props.band_height = self.band_height
	props.speed = self.speed
	props.drainspeed = self.drainspeed
end;

-------------------------------------------------------------------------------
function RaisingWater:OnLoad(props)	
	self.currlevel = props.currlevel
	self.bStopped = props.bStopped
	self.bDraining = props.bDraining
	self.band_height = props.band_height
	self.speed = props.speed
	self.drainspeed = props.drainspeed
	if (self.currlevel>0.0001) then
		self:OnTimer();	
	end
end;

-------------------------------------------------------------------------------
function RaisingWater:OnReset()
	self:Activate(1);
	self.currlevel=0;
	self.bStopped=0;
	self.bDraining=0;
	self.band_height=self.Properties.height_end-self.Properties.height_start;
	self.speed=(self.Properties.height_end-self.Properties.height_start)*(self.Properties.fSpeed/100);
	self.drainspeed=(self.Properties.height_end-self.Properties.height_start)*(self.Properties.fDrainSpeed/100);
	System.SetWaterVolumeOffset(self.Properties.WaterVolume,0,0,0);
end;

-------------------------------------------------------------------------------
function RaisingWater:Event_RaiseWater()
	self:OnReset();
	self:SetTimer(self.Properties.fUpdateTime*1000,0);
	BroadcastEvent(self, "RaiseWater");
end;
-------------------------------------------------------------------------------
function RaisingWater:Event_StopWater()
	self.bStopped=1;
	if(self.Properties.bDrain==1)then
		self.bDraining=1;
	end;
	BroadcastEvent(self, "StopWater");
end;
-------------------------------------------------------------------------------
function RaisingWater:Event_WaterEnded()
	BroadcastEvent(self, "WaterEnded");
end;

-------------------------------------------------------------------------------
function RaisingWater:OnTimer()

	if(self.bDraining==1)then
		self.currlevel=self.currlevel-self.drainspeed;
		System.SetWaterVolumeOffset(self.Properties.WaterVolume,0,0,self.currlevel);
		if(math.abs(self.currlevel)>math.abs(self.Properties.height_start))then
			self:SetTimer(self.Properties.fUpdateTime*1000,0);
		else
			System.SetWaterVolumeOffset(self.Properties.WaterVolume,0,0,self.Properties.height_start);
			self:Event_WaterEnded();
		end;	
	else	
		self.currlevel=self.currlevel+self.speed;
		System.SetWaterVolumeOffset(self.Properties.WaterVolume,0,0,self.currlevel);
		if(math.abs(self.currlevel)<math.abs(self.band_height))then
			if(self.bStopped==0)then
				self:SetTimer(self.Properties.fUpdateTime*1000,0);
			end;
		else
			System.SetWaterVolumeOffset(self.Properties.WaterVolume,0,0,self.band_height);
			self:Event_WaterEnded();
		end;	
	end;
	
end;

RaisingWater.FlowEvents =
{
	Inputs =
	{
		RaiseWater = { RaisingWater.Event_RaiseWater, "bool" },
		StopWater = { RaisingWater.Event_StopWater, "bool" },
	},
	Outputs =
	{
		RaiseWater = "bool",
		StopWater = "bool",
		WaterEnded = "bool",
	},
}
