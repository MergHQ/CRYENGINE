Fog = {	
	type = "FogController",	
	Properties = {	
		GlobalDensityModifier = 1,
		AtmosphereHeightModifier = 0,
		FadeTime = 1
	},	
	Editor = {
		Icon = "Fog.bmp"
	},
	
	LastTime = 0,
	CurFadeAmount = 0
}


function Fog:OnUpdate( dt )
	self:SetFog();
end


function Fog:OnInit()
	self:OnReset();
end


function Fog:OnPropertyChange()
	self:OnReset();
end


function Fog:OnReset()
	self:KillTimer( 0 );
	self:KillTimer( 1 );
	self:ResetValues();
end

function Fog:OnSave(tbl)
	tbl.LastTime = self.LastTime;
	tbl.CurFadeAmount = self.CurFadeAmount;
end

function Fog:OnLoad(tbl)
	self.LastTime = tbl.LastTime;
	self.CurFadeAmount = tbl.CurFadeAmount;
end

function Fog:ResetValues()  
  self.LastTime = 0.0;
	self.CurFadeAmount = 0.0;
end


function Fog:SetFog()
	--System.LogToConsole( "Fog:SetFog "..tonumber(self.CurFadeAmount) );
	local fade = self.CurFadeAmount;
	local densityMod = self.Properties.GlobalDensityModifier * fade;
	local atmosphereMod = self.Properties.AtmosphereHeightModifier * fade;
	System.SetVolumetricFogModifiers( densityMod, atmosphereMod );
end


function Fog:OnEnterArea( player, areaId )
	self.LastTime = _time;
	self:KillTimer( 1 );
	self:SetTimer( 0, 1 );
	self:Activate( 1 );
end


function Fog:OnLeaveArea( player, areaId )
	self.LastTime = _time;
	self:KillTimer( 0 );
	self:SetTimer( 1, 1 );
end


function Fog:OnShutDown()
end


function Fog:OnTimer( timerId )
	
	local delta = _time - self.LastTime;
	self.LastTime = _time;
	
	if( timerId == 0 ) then

		self.CurFadeAmount = self.CurFadeAmount + (delta / self.Properties.FadeTime);
	
		if( self.CurFadeAmount >= 1.0 ) then
			self.CurFadeAmount = 1;
			self:KillTimer( 0 );
		else
			self:SetTimer( 0, 1 );		
		end
		
	else
	
		self.CurFadeAmount = self.CurFadeAmount - (delta / self.Properties.FadeTime);
		
		if( self.CurFadeAmount <= 0.0 ) then
			self.CurFadeAmount = 0.0;
			self:KillTimer( 1 );
			self:Activate( 0 );
			self:SetFog();
		else
			self:SetTimer( 1, 1 );		
		end				
		
	end
	
end
