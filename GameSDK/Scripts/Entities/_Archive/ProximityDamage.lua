ProximityDamage = {
	type = "ProximityDamage",
	Properties = {	
		fDamageRate = 70,	-- health per second
		bEnabled = 1,	
		fRadius = 3,
		fHeight = 2,
		bShakeOnly=0,
		bTriggerOnce=0,
		nShakeType=0,
		bSkipPlayer = 0,
		bSkipAI = 0,
		nDamageType=0, -- 0 normal damage (hud melee) / 1 gas damage
		},
	Editor={
		Model="Objects/Editor/T.cgf",
	},

	damageTime = 0,
	lastDamageTime = 0,
	bTriggeredServer=0,
	bTriggeredClient=0,
}




function ProximityDamage:CliSrv_OnInit()

	self:RegisterState("Inactive");
	self:RegisterState("Active");
	self:OnReset(  );

end

-----------------------------------------------------------------------------
function ProximityDamage:OnPropertyChange()
	self:OnReset(  );
end

-----------------------------------------------------------------------------
function ProximityDamage:OnReset( )
	self:Activate(0);
	self:SetUpdateType( eUT_Physics );
	self:TrackColliders(1);

	self.bTriggeredServer=0;
	self.bTriggeredClient=0;
	self.nNumColliders = 0;

	if( self.Properties.bEnabled == 1) then
		
--System.Log("\001 proxDamage active ");		
		self:GotoState("Active");
	else	
	
--System.Log("\001 proxDamage inactive ");	
		self:GotoState("Inactive");
	end	

	self:SetBBox( {x=-self.Properties.fRadius, y=-self.Properties.fRadius, z=0},
		{x=self.Properties.fRadius, y=self.Properties.fRadius, z=self.Properties.fHeight} );
	
	self.damageTime = 1/self.Properties.fDamageRate;
	self.lastDamageTime = _time + self.damageTime;
	self.lastLocalTime = _time + 1;	
	
	
--System.Log("\001 proxDamage rate "..self.damageTime);
	
end


-----------------------------------------------------------------------------
--	
function ProximityDamage:OnContactClient( collider )

	if (self.bTriggeredClient==1) then
		do return end
	else
		if (self.Properties.bTriggerOnce==1) then
			self.bTriggeredClient=1;
		end			
	end	

--Hud.label = "inside ProximityDamage entity";

	if(self:CanBeDamaged(collider)==0) then return end
	
	if (_time - self.lastLocalTime <= 1) then return end
	
	self.lastLocalTime = _time + .6 + random(1,100)/200;
	
	-- do some clienteffect	
	if (self.Properties.nShakeType==0) then
		local ShakeAmount = self.Properties.fDamageRate * 2.2;		--was 0.5
		if ( ShakeAmount > 45 ) then ShakeAmount = 45; end
		if(random(1,100)<50) then
			ShakeAmount = -ShakeAmount;
		end	
		collider.cnt:ShakeCamera( {x=0, y=0, z=1}, ShakeAmount, 4, .25);
	else
		collider.cnt:ShakeCameraL(0.1, 11.73, 1.73);
	end

	if (self.Properties.bShakeOnly==0) then	
		-- set default melee damage
		local damage_type="MeleeDamageNormal";
		
		-- set gas melee damage..
		if (self.Properties.nDamageType==1) then
			damage_type="MeleeDamageGas";
		end
		
		Hud:OnMeleeDamage(damage_type);		
	end

end

-----------------------------------------------------------------------------
--	
function ProximityDamage:OnContactServer( collider )

	if (self.bTriggeredServer==1) then
		do return end
	else
		if (self.Properties.bTriggerOnce==1) then
			self.bTriggeredServer=1;
		end			
	end	

	if((self:CanBeDamaged(collider)==0) or (self.Properties.bShakeOnly==1)) then return end
--Hud.label = "inside ProximityDamage entity  SERVER";

	if (collider.dmgTime == nil) then 
		collider.dmgTime = _time + self.damageTime;
		return
	end	

	if (_time - collider.dmgTime <= 1) then return end

--	if( _time - self.lastDamageTime <= 0) then return end

--	if( collider.ai and collider.inDamageArea==nil ) then
	if( collider.ai ) then		
--		AI:Signal(0, 1, "SHARED_IN_DAMAGEAREA", collider.id);

--System.Log("\001 proxDamage collider.ai health  "..collider.cnt.health);

--		collider:SelectPipe(0,"diethere");
		collider:TriggerEvent(AIEVENT_DISABLE);

	end

--System.Log("\001 proxDamage collider health  "..collider.cnt.health);

--	if(collider.cnt.health <= 0 ) then return end

	local	hit = {
			dir = {x=0, y=0, z=1},
			damage = self.Properties.fDamageRate,
			target = collider,
			shooter = collider,
			landed = 1,
			impact_force_mul_final=5,
			impact_force_mul=5,
			damage_type = "normal",
			};
	--hit.damage = _frametime/self.damageTime;			

	collider:Damage( hit );
	collider.dmgTime = _time + self.damageTime;
end

-----------------------------------------------------------------------------
function ProximityDamage:OnShutDown()
end


ProximityDamage.Server={
	OnInit=function(self)
		self:CliSrv_OnInit()
	end,
	OnShutDown=function(self)
	end,
	Inactive={
	},
	Active={
		OnBeginState=function(self)
		end,
		OnEndState=function(self)
			self:Activate(0);
		end,
		OnContact = ProximityDamage.OnContactServer,
		
		OnEnterArea = function( self,collider )
			if(self:CanBeDamaged(collider)==1) then 
				self.nNumColliders = self.nNumColliders + 1;
				self:Activate(1);
			end
		end,
		OnLeaveArea = function( self,collider )
			if(self:CanBeDamaged(collider)==1) then 
				self.nNumColliders = self.nNumColliders - 1;
				if (self.nNumColliders <= 0) then
					self:Activate(0);
				end
			end
		end
	},
}


ProximityDamage.Client={
	OnInit=function(self)
		self:CliSrv_OnInit()
	end,
	OnShutDown=function(self)
	end,
	Inactive={
	},
	Active={
		OnEndState=function(self)
			self:Activate(0);
		end,
		OnContact = ProximityDamage.OnContactClient,
		OnEnterArea = function( self,collider )
			if(self:CanBeDamaged(collider)==1) then 
				self.nNumColliders = self.nNumColliders + 1;
				self:Activate(1);
			end
		end,
		OnLeaveArea = function( self,collider )
			if(self:CanBeDamaged(collider)==1) then 
				self.nNumColliders = self.nNumColliders - 1;
				if (self.nNumColliders <= 0) then
					self:Activate(0);
				end
			end
		end
	},
}


----------------------------------------------------------------------------------------------------------------------------
--
function ProximityDamage:Event_Enable( params )

	self:GotoState("Active");

end

----------------------------------------------------------------------------------------------------------------------------
--
function ProximityDamage:Event_Disable( params )

	self:GotoState("Inactive");

end


-----------------------------------------------------------------------------
--	
function ProximityDamage:CanBeDamaged( collider )

	if (collider.type ~= "Player" ) then return 0 end
	if (collider.Properties.bTakeProximityDamage and collider.Properties.bTakeProximityDamage == 0) then return 0 end
	if (collider:IsDead()) then return 0 end	
	if (collider.Properties.bInvulnerable and collider.Properties.bInvulnerable == 1 ) then return 0 end
	if ((self.Properties.bSkipPlayer == 1) and (collider==_localplayer)) then return 0 end
	if ((self.Properties.bSkipAI == 1) and (collider.ai~=nil)) then return 0 end

	return 1
end

ProximityDamage.FlowEvents =
{
	Inputs =
	{
		Disable = { ProximityDamage.Event_Disable, "bool" },
		Enable = { ProximityDamage.Event_Enable, "bool" },
	},
	Outputs =
	{
		Disable = "bool",
		Enable = "bool",
	},
}
