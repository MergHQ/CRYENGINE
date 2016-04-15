ChainSwing = 
{
	Properties=
	{
		mass = 20,

		fileModel="Objects/glm/shipwreck/stuff/chain_swing.cgf",
		bCheckCollisions=1,
		bCheckTerrainCollisions=0,
		bShootable=1,
		coll_dist=0.025,
		material="mat_default",
		gravity={x=0,y=0,z=-9.81},
		damping=0.5,
		friction=0.1,
		sleep_speed=0.01,
		max_time_step=0.01,
		rope_name="rope",
		num_ropes=1,
		AttachTo="",
		AttachToPart=-1,
		AttachToUp="",
		AttachToPartUp=-1,
		bDetachOnDamage=1,
		bAwake=1,
		LowSpec = {
		  bKeepCollisions = 1,
		  max_time_step = 0.03,
		  sleep_speed = 0.04,
		}
	},
	bWasReset = 0,
	bWasDetached = 0,

	Editor=
	{
		Model="Objects/Editor/LightSphear.cgf",
	},

	sChainSound=nil,
	sChainBroken=nil,
	--sChainCollide=nil,
	sChainSoundFilename="Sounds/objectimpact/chain.wav",
	--sChainBrokenFilename="Sounds/objectimpact/chain.wav",
	--sChainCollideFilename="Sounds/objectimpact/chain.wav",
}

function ChainSwing:OnReset()

	self.lightId=nil;
	self:NetPresent(nil);
	self:Activate(1);
	
	local RopeParams = {
		entity_id_1 = -1,
		entity_part_id_1 = 0,
		surface_idx = Game:GetMaterialIDByName(self.Properties.material),
	};
	local LowPhysParams = {
	  bCheckCollisions = self.Properties.bCheckCollisions*self.Properties.LowSpec.bKeepCollisions,
	  bCheckTerrainCollisions = self.Properties.bCheckTerrainCollisions*self.Properties.LowSpec.bKeepCollisions
	}
	local qual=tonumber(getglobal("physics_quality"));
	
	self:LoadCharacter(self.Properties.fileModel,0);
	self:PhysicalizeCharacter(self.Properties.mass,0,0,0);
	for i=1,self.Properties.num_ropes do
	  local rope_name = self.Properties.rope_name;
		if i>1 or self.Properties.num_ropes>1 then
			rope_name = rope_name..i;
		end
		--System.LogToConsole( rope_name );
		self:SetCharacterPhysicParams(0,rope_name, PHYSICPARAM_ROPE,self.Properties);
		self:SetCharacterPhysicParams(0,rope_name, PHYSICPARAM_ROPE,RopeParams);
		self:SetCharacterPhysicParams(0,rope_name, PHYSICPARAM_SIMULATION,self.Properties);
		if (qual==0) then
		  self:SetCharacterPhysicParams(0,rope_name, PHYSICPARAM_ROPE,LowPhysParams);
		  self:SetCharacterPhysicParams(0,rope_name, PHYSICPARAM_SIMULATION,self.Properties.LowSpec);
	  end
	  if (self.Properties.bAwake==0) then
	    self:AwakeCharacterPhysics(0,rope_name,0);
	  end
	end
	self:DrawObject(0, 1);
	self.bWasReset = 1;
	self.bWasDetached = 0;
	
	self.sChainSound=Sound.Load3DSound(self.sChainSoundFilename, 0, 0, 255, 1, 10);
	--self.sChainBroken=Sound.Load3DSound(self.sChainBrokenFilename, 0, 0, 255, 1, 10);
	--self.sChainCollide=Sound.Load3DSound(self.sChainCollideFilename, 0, 0, 255, 1, 10);
end

function ChainSwing:OnUpdate()
  if (self.bWasReset and self.bWasReset==1) then
  	if (((self.Properties.AttachTo and self.Properties.AttachTo~="") or (self.Properties.AttachTo and self.Properties.AttachTo~="")) 
  	    and self.bWasDetached==0) 
  	then
  	  local RopeParams = {};
  	  local ent;
  	  if (self.Properties.AttachToUp and self.Properties.AttachToUp~="") then
  	    ent = System.GetEntityByName(self.Properties.AttachToUp);
  	    if ent then
  	      RopeParams.entity_id_1 = ent.id;
  	      if (self.Properties.AttachToPartUp) then
	          RopeParams.entity_part_id_1 = self.Properties.AttachToPartUp;
	        end
  	    end
  	  end
  	  if (self.Properties.AttachTo and self.Properties.AttachTo~="") then  
	      ent = System.GetEntityByName(self.Properties.AttachTo);
	      if ent then
	        RopeParams.entity_id_2 = ent.id;
	        if (self.Properties.AttachToPart) then
	          RopeParams.entity_part_id_2 = self.Properties.AttachToPart;
	        end
	      end
	    end  
	    for i=1,self.Properties.num_ropes do
	      local rope_name = self.Properties.rope_name;
		    if i>1 or self.Properties.num_ropes>1 then
			    rope_name = rope_name..i;
		    end
		    self:SetCharacterPhysicParams(0,rope_name, PHYSICPARAM_ROPE,RopeParams);
	    end
	  end
	  self.bWasReset = 0;
  end
end

function ChainSwing:OnPropertyChange()
	self:OnReset();
end

function ChainSwing:OnDamage( hit )

	if (hit.shooter) then 
		if (hit.shooter.ai) then 
			do return end;
		end
	end

	--System.LogToConsole("dir="..hit.dir.x..","..hit.dir.y..","..hit.dir.z);
	--System.LogToConsole("pos="..hit.pos.x..","..hit.pos.y..","..hit.pos.z);
	--System.LogToConsole("impact_force="..hit.impact_force_mul);
	if (self.Properties.bDetachOnDamage and self.Properties.bDetachOnDamage~=0) then
	  local RopeParams = {
	    entity_id_1 = -1,
	    entity_id_2 = -2,  
	  }
	  if (self.Properties.AttachToUp and self.Properties.AttachToUp~="") then
  	  local ent = System.GetEntityByName(self.Properties.AttachToUp);
  	  if ent then
  	    RopeParams.entity_id_1 = ent.id;
  	    if (self.Properties.AttachToPartUp) then
	        RopeParams.entity_part_id_1 = self.Properties.AttachToPartUp;
	      end
  	  end
  	end
	  self:SetCharacterPhysicParams(0,self.Properties.rope_name, PHYSICPARAM_ROPE,RopeParams);
	  self.bWasDetached = 1;
	  self:Event_ChainBroken();	  	  	  
 	end

	if( hit.ipart ) then
		self:AddImpulse( hit.ipart, hit.pos, hit.dir, hit.impact_force_mul );
	else	
		self:AddImpulse( -1, hit.pos, hit.dir, hit.impact_force_mul );
	end	

	if (self.sChainSound and not Sound.IsPlaying(self.sChainSound)) then
		Sound.SetSoundPosition(self.sChainSound, self:GetPos());
		Sound.PlaySound(self.sChainSound);						
	end
end

function ChainSwing:OnInit()
	self:OnReset();
end

function ChainSwing:Event_ChainBroken(sender)
	BroadcastEvent( self,"ChainBroken");
	if (self.sChainBroken) then
		Sound.SetSoundPosition(self.sChainBroken, self:GetPos());
		Sound.PlaySound(self.sChainBroken);						
	end
end

--function ChainSwing:OnCollide(hit)
--	System.Log("On collide");
--	if (self.sChainCollide and not Sound.IsPlaying(self.sChainCollide)) then
--		Sound.SetSoundPosition(self.sChainCollide, self:GetPos());
--		Sound.PlaySound(self.sChainCollide);						
--	end	
--end

function ChainSwing:OnWrite( stm )
end

function ChainSwing:OnRead( stm )
end

function ChainSwing:OnSave( stm )
  stm:WriteInt(self.bWasDetached);
end

function ChainSwing:OnLoad( stm )
  self.bWasDetached = stm:ReadInt();  
end 

function ChainSwing:OnShutDown()
end

ChainSwing.FlowEvents =
{
	Inputs =
	{
		ChainBroken = { ChainSwing.Event_ChainBroken, "bool" },
	},
	Outputs =
	{
		ChainBroken = "bool",
	},
}
