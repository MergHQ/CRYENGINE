Storm = {
	type = "StormController",
	Properties = {	
		fRainAmount = 0.5, -- amount of rain, range [0..1]
		vWindDir={0,1,0}, -- wind direction
		nRandomFrequency=50, -- chance of occurring of generating a random lighting, range [0..100]
		nSoundDistortionTime=3, -- distortion time for player and AI, in seconds
		nDistanceFromTerrain=512,
		},

	Editor={
		Model="Objects/Editor/T.cgf",
	},
	
	bInside=0,
	fLastFadeCoeff=1,
	nCurrentPos=0,
	fCurrentRain=0.5,

	LightningParticles = {
		focus = 90, -- default 90
		start_color = {1,1,1},
		end_color =  {1,1,1},
		rotation={x=0.0,y=0.0,z=0}, -- default z=1.75
		speed = 0, -- default 0
		count = 1, -- default 2
		size = 512, size_speed=0, -- default size = .025 -- default size_speed = 0.5
		gravity={x=0.0,y=0.0,z=0}, -- default z=0.2
		lifetime=0.75, -- default 0.75
		frames=9,
		blend_type = 2,
		tid = System.LoadAnimatedTexture("textures/animated/lightning/lightning_y%02d.tga",9),
	},


}


-----------------------------------------------------------------------------
function Storm:OnInit()
	self.thunder_sounds = {
		Sound.Load3DSound("sounds\\thunder\\tclap1.wav",SOUND_UNSCALABLE,0,255,7,1000),
		Sound.Load3DSound("sounds\\thunder\\tclap2.wav",SOUND_UNSCALABLE,0,255,7,1000),
		Sound.Load3DSound("sounds\\thunder\\tclap3.wav",SOUND_UNSCALABLE,0,255,7,1000),
	}		
	self:Activate(1);
	self:OnReset();
end

-----------------------------------------------------------------------------
function Storm:OnPropertyChange()
	self:OnReset();
end

-----------------------------------------------------------------------------
function Storm:OnReset()
	self.bInside=0;
	self.fLastFadeCoeff=1;
	self.nCurrentPos=0;	
	self.fCurrentRain=self.Properties.fRainAmount;
end

-----------------------------------------------------------------------------
function Storm:OnProceedFadeArea( player,areaId,fadeCoeff )

	--System.Log(" )) Fading value "..fadeCoeff);	
	
	self.fLastFadeCoeff=fadeCoeff;
	
end

-----------------------------------------------------------------------------
function Storm:SetThunder()
	
	--System.Log("THUNDER!!!!");	

	-- draw the lighting
	-----------------------------------------------------------------------------
	if (self.nCurrentPos==1) or (self.nCurrentPos==4) then

		local explPos=self:GetPos();
		explPos.x=random(-20,20);
		explPos.y=random(-20,20);	
		explPos.z=explPos.z+self.Properties.nDistanceFromTerrain;
		Particle.CreateParticle(explPos, { x = 0, y = 0, z = 1.0 }, self.LightningParticles);

	end

	-- play the thunder sound after the lightning
	-----------------------------------------------------------------------------
	if (self.nCurrentPos==10) then	
	
		local ThunderPos=self:GetPos();
		local Snd=self.thunder_sounds[random(1, 3)];		
		--ThunderPos.x=ThunderPos.x;
		--ThunderPos.y=ThunderPos.y;

		Sound.SetSoundPosition(Snd,ThunderPos);
		Sound.PlaySound(Snd);

		AI:ApplySoundDistortion(self.Properties.nSoundDistortionTime*1000);

		-- restore back status
		self.nCurrentPos=0;		

		-- set the time for the next thunder	
		local NextTime=random(0,100-self.Properties.nRandomFrequency)*100*2;
		
		self:SetTimer(NextTime);		

		-- change randomly the rain intensity
		--System.Log("Rain is "..self.fCurrentRain);	
		self.fCurrentRain=random(0,16*self.Properties.fRainAmount)/16.0;
		Game:ApplyStormToEnvironment(self.Properties.vWindDir,self.fCurrentRain);
	else

	-- set next time and change world color
	-----------------------------------------------------------------------------
		
		local NextTime=random(0,100-self.Properties.nRandomFrequency)*10;

		if (self.nCurrentPos==0) or (self.nCurrentPos==6) then
			Hud:OnLightning();
		end	

		-- set next thunder time in ms
		--System.Log("Timer set to "..NextTime.." ms");

		self:SetTimer(NextTime);
		
		self.nCurrentPos=self.nCurrentPos+1;

	end
end

-----------------------------------------------------------------------------
function Storm:OnEnterArea( player,areaId )

	--System.Log("--> Entering Storm Area "..areaId);	
	
	self:SetTimer(1000);
	self.bInside=1;
	self.nCurrentPos=0;
	Game:ApplyStormToEnvironment(self.Properties.vWindDir,self.Properties.fRainAmount);
end

-----------------------------------------------------------------------------
function Storm:OnLeaveArea( player,areaId )

	--System.Log("--> Leaving Storm Area "..areaId);
	self.bInside=0;
	Game:ApplyStormToEnvironment(self.Properties.vWindDir,0);
	self.nCurrentPos=0;
end

-----------------------------------------------------------------------------
function Storm:OnShutDown()
	self.thunder_sounds = nil;
end

-----------------------------------------------------------------------------
function Storm:Client_OnTimer()
	
	if (self.bInside==1) then
		self:SetThunder();	
	end
end

--////////////////////////////////////////////////////////////////////////////////////////
--////////////////////////////////////////////////////////////////////////////////////////
--// CLIENT functions definitions
--////////////////////////////////////////////////////////////////////////////////////////
--////////////////////////////////////////////////////////////////////////////////////////

Storm.Client = {

	OnTimer = Storm.Client_OnTimer,	
}
