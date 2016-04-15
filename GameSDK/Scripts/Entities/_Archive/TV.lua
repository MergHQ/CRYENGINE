
--Script.LoadScript("scripts/Default/Entities/AI/SoundSupressor.lua");
TV={
	PropertiesInstance = {
		object_Model="Objects/Outdoor/props/PORTABLE_TV/portable_TV.cgf",
		object_ModelDestroyed="",
		nDamage=5,
		fSndRadius = 25,
		fSndVolume = 255,
		AliveSoundLoop={
			sndFilename="",
			InnerRadius=1,
			OuterRadius=10,
			nVolume=255,
		},
		DeadSoundLoop={
			sndFilename="",
			InnerRadius=1,
			OuterRadius=10,
			nVolume=255,
		},
		DyingSound={
			sndFilename="",
			InnerRadius=1,
			OuterRadius=10,
			nVolume=255,
		},

--		nPlayerDamage=100,
--		nPlayerDamageRadius=3,
	},

	Properties = {
		mass = 5,
	},

}

function TV:OnInit()
	self:Activate(0);
	AI:RegisterWithAI(self.id, AIOBJECT_SNDSUPRESSOR, self.Properties);
	self:SetAICustomFloat( self.PropertiesInstance.fSndRadius );


	self:OnReset();
end

function TV:OnPropertyChange()
	self:OnReset();
end

function TV:OnShutDown()

end



function TV:OnReset()


	if (self.PropertiesInstance.object_Model ~= self.CurrModel) then
		self.CurrModel = self.PropertiesInstance.object_Model;
		self:LoadObject(self.PropertiesInstance.object_Model,0,1);
		self:DrawObject(0,1);
		self:CreateStaticEntity( 10,100 );
		self:CreateRigidBody( 0,self.Properties.Mass,0 );		
	end
	if ((self.PropertiesInstance.object_ModelDestroyed ~="") and 
	(self.PropertiesInstance.object_ModelDestroyed ~= self.CurrDestroyedModel)) then
		self.CurrDestroyedModel = self.PropertiesInstance.object_ModelDestroyed;
		self:LoadObject(self.PropertiesInstance.object_ModelDestroyed,1,1);
		self:DrawObject(1,0);
		self:CreateStaticEntity( 10,100 );
	end
	
	-- stop old sounds
	if (self.DyingSoundLoop and Sound.IsPlaying(self.DyingSound)) then
		Sound.StopSound(self.DyingSound);
	end
	if (self.DeadSoundLoop and Sound.IsPlaying(self.DeadSoundLoop)) then
		Sound.StopSound(self.DeadSoundLoop);
	end
	if (self.AliveSoundLoop and Sound.IsPlaying(self.AliveSoundLoop)) then
		Sound.StopSound(self.AliveSoundLoop);
	end
	-- load sounds
	local SndTbl;
	SndTbl=self.PropertiesInstance.AliveSoundLoop;
	if (SndTbl.sndFilename~="") then
		self.AliveSoundLoop=Sound.Load3DSound(SndTbl.sndFilename, 0);
		if (self.AliveSoundLoop) then
			Sound.SetSoundPosition(self.AliveSoundLoop, self:GetPos());
			Sound.SetSoundLoop(self.AliveSoundLoop, 1);
			Sound.SetSoundVolume(self.AliveSoundLoop, SndTbl.nVolume);
			Sound.SetMinMaxDistance(self.AliveSoundLoop, SndTbl.InnerRadius, SndTbl.OuterRadius);
		end
	else
		self.AliveSoundLoop=nil;
	end
	SndTbl=self.PropertiesInstance.DeadSoundLoop;
	if (SndTbl.sndFilename~="") then
		self.DeadSoundLoop=Sound.Load3DSound(SndTbl.sndFilename, 0);
		if (self.DeadSoundLoop) then
			Sound.SetSoundPosition(self.DeadSoundLoop, self:GetPos());
			Sound.SetSoundLoop(self.DeadSoundLoop, 1);
			Sound.SetSoundVolume(self.DeadSoundLoop, SndTbl.nVolume);
			Sound.SetMinMaxDistance(self.DeadSoundLoop, SndTbl.InnerRadius, SndTbl.OuterRadius);
		end
	else
		self.DeadSoundLoop=nil;
	end
	SndTbl=self.PropertiesInstance.DyingSound;
	if (SndTbl.sndFilename~="") then
		self.DyingSound=Sound.Load3DSound(SndTbl.sndFilename, 0);
		if (self.DyingSound) then
			Sound.SetSoundPosition(self.DyingSound, self:GetPos());
			Sound.SetSoundVolume(self.DyingSound, SndTbl.nVolume);
			Sound.SetMinMaxDistance(self.DyingSound, SndTbl.InnerRadius, SndTbl.OuterRadius);
		end
	else
		self.DyingSound=nil;
	end

	self.curr_damage=self.PropertiesInstance.nDamage;

	System.Log("---RESET TV");
	self:GoAlive();
end

function TV:GoAlive()

	self:EnablePhysics(1);
	self:DrawObject(0,1);
	self:DrawObject(1,0);
	if (self.DeadSoundLoop) then
		Sound.StopSound(self.DeadSoundLoop);
		System.Log("stopping dead-loop");
	end
	self:GotoState( "Active" );
end

function TV:GoDead()
	if (self.DyingSound and (not Sound.IsPlaying(self.DyingSound))) then
		Sound.PlaySound(self.DyingSound);
		System.Log("starting dying");
	end
	if (self.DeadSoundLoop and (not Sound.IsPlaying(self.DeadSoundLoop))) then
		Sound.PlaySound(self.DeadSoundLoop);
		System.Log("starting dead-loop");
	end

	self:GotoState( "Dead" );
end

TV.Active={
	OnBeginState=function(self)
		self:Event_On();
	end,
	OnDamage = function(self,hit)
--		System.Log("ON DAMAGE "..self.id.." AMOUNT="..hit.damage);

			
		self.curr_damage=self.curr_damage-hit.damage;
		
		if(self.curr_damage<=0)then
			self:GoDead();
		end
	end,
}

TV.Dead={
	OnBeginState=function(self)
		System.Log("enter dead");
	
		self:Event_Off();
				
		self:DrawObject(0,0);
		if(self.PropertiesInstance.object_ModelDestroyed~="")then
			self:DrawObject(1,1);
		else
			self:EnablePhysics(0);		
		end
	end,
}


function TV:Event_OnDamage( sender )
	BroadcastEvent( self,"OnDamage" );
end


function TV:Event_On( sender )

	self:TriggerEvent(AIEVENT_ENABLE);

	if (self.AliveSoundLoop and (not Sound.IsPlaying(self.AliveSoundLoop))) then
		Sound.PlaySound(self.AliveSoundLoop);
		System.Log("starting alive-loop");
	end

end

function TV:Event_Off( sender )

	self:TriggerEvent(AIEVENT_DISABLE);

	if (self.AliveSoundLoop) then
		Sound.StopSound(self.AliveSoundLoop);
		System.Log("stopping alive-loop");
	end
end

TV.FlowEvents =
{
	Inputs =
	{
		Off = { TV.Event_Disable, "bool" },
		On = { TV.Event_Enable, "bool" },
		OnDamage = { TV.Event_Enable, "bool" },
	},
	Outputs =
	{
		Off = "bool",
		On = "bool",
		OnDamage = "bool",
	},
}
