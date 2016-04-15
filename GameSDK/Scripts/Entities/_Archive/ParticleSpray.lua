ParticleSpray = {

	Properties = {
		TimeDelay = 0.1, -- how often particle system is spawned.
		
		fUpdateRadius=100,

		bActive=1,
		nType=0, -- Type of particles 0=Billboard,1=Water splashes,2=Underwater,3=LineParticle,
		Focus = 4,
		clrColorStart={1,1,1},
		clrColorEnd={1,1,1},
		Speed = 1.2,
		nCount = 1,
		Size = 0.2,
		SizeSpeed = 2.2,
		LifeTime = 2,
		FadeInTime = 0.5,
		nFramesSpeed = 0,
		nFrames = 0,
		Tail = 0, -- Tail lengths.
		Bounciness = 0.5,
		bPhysics = 0, -- Enable or disable real particle physics.
		-- Blend types
		bAdditiveBlend = 0,
		bColorBlend = 0,
		nDrawOrder = 0,

		Gravity = { X=0,Y=0,Z=1 },
		Rotation = { X=0,Y=0,Z=0 },
		Textures = {
			fileTex1="Textures\\carsmoke.tga",
			fileTex2="",
			fileTex3="",
			fileTex4="",
			fileTex5="",
		},
		-- Objects to spawn instead of texture.
		Objects = {
			objObject1="",
			objObject2="",
			objObject3="",
			objObject4="",
			objObject5="",
		},

		turbulence_size = 0,
		turbulence_speed = 0,
		bLinearSizeSpeed = 0,
		ShaderName="ParticleLight",
		SpaceLoopBoxSize = { X=0,Y=0,Z=0 },
		bBindToCamera = 0,
		bNoIndoor = 0,

		-- Child process definition.
		ChildSpawnPeriod = 0,
		ChildProcess =
		{
			nType=0, -- Type of particles 0=Billboard,1=Water splashes,2=Underwater,3=LineParticle,
			Focus = 0.7,
			clrColorStart={1,1,1},
			clrColorEnd={1,1,1},
			Speed = 1.2,
			nCount = 2,
			Size = 1.2,
			SizeSpeed = 0.2,
			LifeTime = 1,
			FadeInTime = 0.5,
			nFrames = 0,
			Tail = 0,
			Bounciness = 0.5,
			bPhysics = 0,-- Enable or disable real particle physics.
			-- Blend types
			bAdditiveBlend =0,
			bColorBlend = 0,
			nDrawOrder = 0,
			Gravity = { X=0,Y=0,Z=0 },
			Rotation = { X=0,Y=0,Z=0 },
			fileTexture = "",
			objGeometry = "",
			SpaceLoopBoxSize = { X=0,Y=0,Z=0 },
			bBindToCamera = 0,
			bNoIndoor = 0,
		},
	},
	Editor={
		Model="Objects/Editor/Particles.cgf",
	},
	--Textures = {
		--System.LoadTexture("textures\\Particle.tga"),
	--},
	TotDelta = 0,
	CurrTid = 0,
};

-------------------------------------------------------
function ParticleSpray:OnInit()
	-- Create particle table.
	self:RegisterState( "Active" );
	self:RegisterState( "Idle" );
	
	self.ParticleParams = {};
	self.Textures={};
	self.TexturesId={};
	self.Objects={};
	self.Direction = {x=0,y=0,z=1};
	self:Activate(0);
	self:SetRegisterInSectors(1);
	self:SetRadius(0.01);
	self:OnReset();
	self:NetPresent(nil);
	
	self:SetUpdateType( eUT_PotVisible );
end

-------------------------------------------------------
function ParticleSpray:Property2ParticleTable( Params,Properties )
	Params.focus = Properties.Focus;
	Params.speed = Properties.Speed;
	Params.count = Properties.nCount;
	Params.size = Properties.Size;
	Params.size_speed = Properties.SizeSpeed / 10;
	Params.lifetime = Properties.LifeTime;
	Params.fadeintime = Properties.FadeInTime;
	Params.frames = Properties.nFramesSpeed;
	Params.tail_length = Properties.Tail;
	Params.bouncyness = Properties.Bounciness;
	Params.physics = Properties.bPhysics;
	Params.BindToCamera = Properties.bBindToCamera;
	Params.NoIndoor = Properties.bNoIndoor;
	Params.draw_last = Properties.nDrawOrder;
	Params.particle_type = Properties.nType;

	Params.blend_type = 0;
	if (Properties.bColorBlend ~= 0) then
		Params.blend_type = 1;
	end
	if (Properties.bAdditiveBlend ~= 0) then
		Params.blend_type = 2;
	end

	Params.gravity = {};
	Params.gravity.x  = Properties.Gravity.X;
	Params.gravity.y  = Properties.Gravity.Y;
	Params.gravity.z  = Properties.Gravity.Z;

	Params.SpaceLoopBoxSize = {};
	Params.SpaceLoopBoxSize.x  = Properties.SpaceLoopBoxSize.X;
	Params.SpaceLoopBoxSize.y  = Properties.SpaceLoopBoxSize.Y;
	Params.SpaceLoopBoxSize.z  = Properties.SpaceLoopBoxSize.Z;

	Params.rotation = {};
	Params.rotation.x  = Properties.Rotation.X;
	Params.rotation.y  = Properties.Rotation.Y;
	Params.rotation.z  = Properties.Rotation.Z;

	Params.start_color = {};
	Params.start_color[1]  = Properties.clrColorStart[1];
	Params.start_color[2]  = Properties.clrColorStart[2];
	Params.start_color[3]  = Properties.clrColorStart[3];

	Params.end_color = {};
	Params.end_color[1]  = Properties.clrColorEnd[1];
	Params.end_color[2]  = Properties.clrColorEnd[2];
	Params.end_color[3]  = Properties.clrColorEnd[3];

	Params.turbulence_size  = Properties.turbulence_size;
	Params.turbulence_speed = Properties.turbulence_speed;
	Params.bLinearSizeSpeed = Properties.bLinearSizeSpeed;
	Params.ChildSpawnPeriod = Properties.ChildSpawnPeriod;

	Params.ShaderName = Properties.ShaderName;
end

function ParticleSpray:OnPropertyChange()
	self:OnReset();
	
	if (self.Properties.bActive ~= 0) then
		self:CreateParticleEmitter( self.ParticleParams, self.Properties.TimeDelay );
	end
end

-------------------------------------------------------
function ParticleSpray:OnReset()
	self:Property2ParticleTable( self.ParticleParams,self.Properties );

	-- Childs.
	self.ParticleParams.ChildSpawnPeriod = self.Properties.ChildSpawnPeriod;
	if (self.Properties.ChildProcess.fileTexture ~= "" or self.Properties.ChildProcess.objGeometry ~= "") then
		self.ParticleParams.ChildProcess = {};
		self:Property2ParticleTable( self.ParticleParams.ChildProcess,self.Properties.ChildProcess );
	else
		self.ParticleParams.ChildProcess = nil;
	end

	-- Reload textures
	local reloadTextures = 0;
	for index, value in pairs(self.Properties.Textures) do
		if (self.Textures[index] ~= value and value ~= "") then
			reloadTextures = 1;
		end
	end

	if (reloadTextures) then
		self.ReloadTexures( self );
	end
	self.ParticleParams.tid = self:GetRandomTextureId();
	
	self:Activate(1); -- another pot vis System will check is update needed or not
	self:SetUpdateRadius( self.Properties.fUpdateRadius );
	
	self:GotoState( "" );
	if (self.Properties.bActive ~= 0) then
		self:GotoState( "Active" );
	else
		self:GotoState( "Idle" );
	end
end

function ParticleSpray:OnSave(stm)
end

function ParticleSpray:OnLoad(stm)
end

function ParticleSpray:Event_Enable()
	self:GotoState( "Active" );
end

function ParticleSpray:Event_Disable()
	self:GotoState( "Idle" );
end

function ParticleSpray:ReloadTexures()
	local ind = 1;
	self.TextureIds = {};
	for index, value in pairs(self.Properties.Textures) do
		if (value ~= "") then
			if (self.Properties.nFrames > 0) then
				self.TexturesId[ind] = System.LoadAnimatedTexture( value,self.Properties.nFrames );
			else
				self.TexturesId[ind] = System.LoadTexture( value );
			end
			ind = ind + 1;
		end
		self.Textures[index] = value;
	end

	if (self.Properties.ChildProcess.fileTexture == "") then
		self.ParticleParams.ChildProcess = nil;
	else
		self.ChildTexture = self.Properties.ChildProcess.fileTexture;
		self.ParticleParams.ChildProcess = {};
		self:Property2ParticleTable( self.ParticleParams.ChildProcess,self.Properties.ChildProcess );
		self.ParticleParams.ChildProcess.tid = System.LoadTexture( self.ChildTexture );
	end
end

function ParticleSpray:ReloadObjects()
	local ind = 1;
	self.ObjectIds = {};
	for index, value in pairs(self.Properties.Objects) do
		if (value ~= "") then
			--self.ObjectId[ind] = System.LoadTexture( value );
			ind = ind + 1;
		end
		self.Objects[index] = value;
	end
end

-------------------------------------------------------
function ParticleSpray:GetRandomTextureId()
	local table = self.TexturesId;
	local len = getn(table);

	if len>0 then
		local rnd = random( 1,len );
		return table[rnd];
	else
		return nil;
	end
end

-------------------------------------------------------------------------------
-- Active State
-------------------------------------------------------------------------------
ParticleSpray.Active =
{
	OnBeginState = function( self )
		self:Activate(1);
		self:CreateParticleEmitter( self.ParticleParams, self.Properties.TimeDelay );
		self.bHaveEmitter = 1;
	end,
	OnEndState = function( self )
		self.bHaveEmitter = nil;
		self:DeleteParticleEmitter( );
		self:Activate(0);
	end,
}

-------------------------------------------------------------------------------
-- Idle State
-------------------------------------------------------------------------------
ParticleSpray.Idle =
{
	OnBeginState = function( self )
		self:Activate(0);
		if (self.bHaveEmitter) then
			self:DeleteParticleEmitter(0);
			self.bHaveEmitter = nil;
		end
	end,
}

-------------------------------------------------------
function ParticleSpray:OnShutDown()
end

ParticleSpray.FlowEvents =
{
	Inputs =
	{
		Disable = { ParticleSpray.Event_Disable, "bool" },
		Enable = { ParticleSpray.Event_Enable, "bool" },
	},
	Outputs =
	{
		Disable = "bool",
		Enable = "bool",
	},
}
