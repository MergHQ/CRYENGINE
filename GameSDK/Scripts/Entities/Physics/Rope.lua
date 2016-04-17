Rope = 
{
	Server = {},
	Client = {},
	
	PropertiesInstance = {
		AttachTo="",
		AttachToPart=-1,
		AttachToUp="#world#",
		AttachToPartUp=-1,
	},
	Properties=
	{
		mass = 20,

		fileModel="Objects/glm/shipwreck/stuff/chain_swing.cgf",
		bCheckCollisions=1,
		bCheckTerrainCollisions=0,
		bShootable=1,
		bCheckAttachmentCollisions=1,
		coll_dist=0.01,
		material="mat_default",
		gravity={x=0,y=0,z=-9.81},
		damping=0.3,
		friction=2,
		friction_pull=0,
		sleep_speed=0.01,
		max_time_step=0.02,
		wind={x=0,y=0,z=0},
		wind_variance=0.2,
		air_resistance=0,
		sensor_size=0.05,
		max_force=0,
		rope_name="rope",
		num_ropes=1,
		slack=0,
		bDetachOnDamage=1,
		bDetachBothEnds=0,
		bDynamicTesselation=0,
		SegCount=6,
		SubVtxCount=3,
		pose_stiffness=0,
		pose_damping=0,
		pose_type=0,
		bAwake=1,
		LowSpec = {
		  bKeepCollisions = 1,
		  max_time_step = 0.03,
		  sleep_speed = 0.04,
		}
	},
	bWasDetached = 0,
	iFirstBone = 0,
	iLastBone = 0,

	Editor=
	{
		Icon="rope.bmp",
		IconOnTop=0,
	},

	sChainSound=nil,
	sChainBroken=nil,
	--sChainCollide=nil,
	sChainSoundFilename="Sounds/objectimpact/chain.wav",
	--sChainBrokenFilename="Sounds/objectimpact/chain.wav",
	--sChainCollideFilename="Sounds/objectimpact/chain.wav",
}

function Rope:OnSpawn()
	self:SetFlags(ENTITY_FLAG_CLIENT_ONLY, 0);
end

function Rope:OnReset()
	self.lightId=nil;
	--self:Activate(1);
	
	local RopeParams = {
		entity_name_1 = "#world#",
		entity_part_id_1 = 0,
		surface_idx = System.GetSurfaceTypeIdByName(self.Properties.material),
	};
	local LowPhysParams = {
	  bCheckCollisions = self.Properties.bCheckCollisions*self.Properties.LowSpec.bKeepCollisions,
	  bCheckTerrainCollisions = self.Properties.bCheckTerrainCollisions*self.Properties.LowSpec.bKeepCollisions,
	  bCheckAttachmentCollisions = self.Properties.bCheckAttachmentCollisions*self.Properties.LowSpec.bKeepCollisions
	}
	local qual = 1;
	--tonumber(getglobal("physics_quality"));
	
	local slen = string.len(self.Properties.fileModel);
	if string.lower(string.sub(self.Properties.fileModel,slen-2,slen))=="cgf" then
		self:LoadObject(0, self.Properties.fileModel);
	else
		self:LoadCharacter(0, self.Properties.fileModel);
	end
	self:SetSlotPos(0, g_Vectors.v000);	self:SetSlotAngles(0, g_Vectors.v000);
	self:ResetAnimation(0,0);
	self:Physicalize(0, PE_ROPE, self.Properties);
	local start = 1;
	if self.Properties.rope_name=="" then start=0; end
	for i=start,self.Properties.num_ropes-1+start do
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
	self:DrawSlot(0, 1);
  self:CharacterUpdateOnRender(0,1);
	self.bWasDetached = 0;
	
	--self.sChainSound=Sound.Load3DSound(self.sChainSoundFilename, 0, 255, 1, 10);
	--self.sChainBroken=Sound.Load3DSound(self.sChainBrokenFilename, 0, 255, 1, 10);
	--self.sChainCollide=Sound.Load3DSound(self.sChainCollideFilename, 0, 255, 1, 10);
end

function Rope.Server:OnStartGame()
  if (self:GetBoneNameFromTable(0)) then
		local RopeParams = {};
		local RopeParams1 = {};
		local pos0={x=0,y=0,z=0};
		local pos1={x=0,y=0,z=0};
		local i=0;
		local bone="";
		local entEnd = System.GetEntityByName(self:GetName().."_end");
		local slen = string.len(self.Properties.rope_name);
		self.iFirstBone = -1;
		repeat 
			bone = self:GetBoneNameFromTable(i);
			if self.iFirstBone<0 and string.lower(string.sub(bone,1,slen))==self.Properties.rope_name then 
				self.iFirstBone = i; 
			end
			i = i+1;
		until bone=="" or i>50;
		self.iLastBone = i-2;

		if (entEnd) then
			pos0 = self:GetPos();	RopeParams.end1 = pos0;
			pos1 = entEnd:GetPos(); RopeParams.end2 = pos1;
		else
			pos0 = self:GetBonePos(self:GetBoneNameFromTable(self.iFirstBone));
			pos1 = self:GetBonePos(self:GetBoneNameFromTable(self.iLastBone));
		end
		if (self.Properties.bDynamicTesselation==1) then
			RopeParams.flags_mask = rope_subdivide_segs;
			RopeParams.flags = rope_subdivide_segs;
			RopeParams1.num_segs = self.Properties.SegCount;
			RopeParams1.num_subvtx = self.Properties.SubVtxCount;
			RopeParams.length = DistanceVectors(pos0,pos1)*(1+self.Properties.slack);
		end
	  
	  if (self.bWasDetached==2) then
			RopeParams.entity_name_1 = "#unattached#";	
	  elseif (self.PropertiesInstance.AttachToUp and self.PropertiesInstance.AttachToUp~="") then
	  	RopeParams.entity_name_1 = self.PropertiesInstance.AttachToUp;
      if (self.PropertiesInstance.AttachToPartUp) then
				RopeParams.entity_part_id_1 = self.PropertiesInstance.AttachToPartUp;
			end
	  else
			local ents = Physics.SamplePhysEnvironment(pos0, self.Properties.sensor_size);
			if (ents[3]) then
				RopeParams.entity_phys_id_1 = ents[3];
				RopeParams.entity_part_id_1 = ents[2];
			else
				RopeParams.entity_name_1 = "#unattached#";
			end	
	  end
	  
	  if (self.bWasDetached==1) then
			RopeParams.entity_name_2 = "#unattached#";
		elseif (self.PropertiesInstance.AttachTo and self.PropertiesInstance.AttachTo~="") then  
      RopeParams.entity_name_2 = self.PropertiesInstance.AttachTo;
      if (self.PropertiesInstance.AttachToPart) then
        RopeParams.entity_part_id_2 = self.PropertiesInstance.AttachToPart;
      end
    else
			local ents = Physics.SamplePhysEnvironment(pos1, self.Properties.sensor_size);
			if (ents[3]) then
				RopeParams.entity_phys_id_2 = ents[3];
				RopeParams.entity_part_id_2 = ents[2];
			else
				RopeParams.entity_name_2 = "#unattached#";	
			end	
    end
    
    for i=1,self.Properties.num_ropes do
      local rope_name = string.lower(self.Properties.rope_name);
	    if i>1 or self.Properties.num_ropes>1 then
		    rope_name = rope_name..i;
	    end
			self:SetCharacterPhysicParams(0,rope_name, PHYSICPARAM_FLAGS,RopeParams);
	    self:SetCharacterPhysicParams(0,rope_name, PHYSICPARAM_ROPE,RopeParams);
			self:SetCharacterPhysicParams(0,rope_name, PHYSICPARAM_ROPE,RopeParams1);
    end

		self:SetSlotPos(0, g_Vectors.v000);	self:SetSlotAngles(0, g_Vectors.v000);
		pos1 = self:GetBonePos(self:GetBoneNameFromTable(self.iFirstBone));
		System.LogToConsole("bone pos: "..pos1.x.." "..pos1.y.." "..pos1.z);
		self:SetSlotPos(0, self:VectorToLocal(-1,DifferenceVectors(self:GetWorldPos(), pos1)));
  end
end

function Rope:OnPropertyChange()
	self:OnReset();
end

function Rope.Server:OnHit( hit )
	--System.LogToConsole("dir="..hit.dir.x..","..hit.dir.y..","..hit.dir.z);
	--System.LogToConsole("pos="..hit.pos.x..","..hit.pos.y..","..hit.pos.z);

	if (hit.shooter) then 
		if (hit.shooter.ai) then 
			do return end;
		end
	end

	if (self.Properties.bDetachOnDamage and self.Properties.bDetachOnDamage~=0) then
		local RopeParams = {}
		if (self.Properties.bDetachBothEnds==0 or
				DistanceSqVectors(hit.pos, self:GetBonePos(self:GetBoneNameFromTable(self.iFirstBone))) >
				DistanceSqVectors(hit.pos, self:GetBonePos(self:GetBoneNameFromTable(self.iLastBone)))) then
			self.bWasDetached = 2;
			RopeParams.entity_name_2 = "#unattached";
		else
			self.bWasDetached = 1;
			RopeParams.entity_name_1 = "#unattached";
		end				
	  self:SetCharacterPhysicParams(0,self.Properties.rope_name, PHYSICPARAM_ROPE,RopeParams);
	  self:Event_ChainBroken();	  	  	  
 	end

	if (self.sChainSound and not Sound.IsPlaying(self.sChainSound)) then
		Sound.SetSoundPosition(self.sChainSound, self:GetPos());
		Sound.PlaySound(self.sChainSound);						
	end
end

function Rope.Server:OnInit()
	self:OnReset();
end

function Rope:Event_ChainBroken(sender)
	BroadcastEvent( self,"ChainBroken");
	if (self.sChainBroken) then
		Sound.SetSoundPosition(self.sChainBroken, self:GetPos());
		Sound.PlaySound(self.sChainBroken);						
	end
end

--function Rope:OnCollide(hit)
--	System.Log("On collide");
--	if (self.sChainCollide and not Sound.IsPlaying(self.sChainCollide)) then
--		Sound.SetSoundPosition(self.sChainCollide, self:GetPos());
--		Sound.PlaySound(self.sChainCollide);						
--	end	
--end

function Rope:OnSave( props )
	props.bWasDetached = self.bWasDetached
end

function Rope:OnLoad( props )
	self.bWasDetached = props.bWasDetached
end 

function Rope:OnShutDown()
end

Rope.FlowEvents =
{
	Inputs =
	{
		ChainBroken = { Rope.Event_ChainBroken, "bool" },
	},
	Outputs =
	{
		ChainBroken = "bool",
	},
}
