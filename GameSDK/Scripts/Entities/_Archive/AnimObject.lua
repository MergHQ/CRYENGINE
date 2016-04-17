AnimObject = {
	Properties = {
		bPhysicalize = 0,
		Animation = "default",
--		fileModel="Objects/characters/mercenaries/new_mercs/merc_m_heavy_a.cgf",
		fileModel="Objects/characters/story_characters/valerie/valeri.cgf",
		bPlaying=0,
		bAlwaysUpdate=0,
		attachmet1={
			boneName="weapon_bone",
			fileObject="Objects/Weapons/M4/M4_bind.cgf",
		},
		attachmet2={
			boneName="",
			fileObject="",
		},
		attachmet3={
			boneName="",
			fileObject="",
		},
		attachmet4={
			boneName="",
			fileObject="",
		},
--		boneName="weapon_bone",
--		fileAttachment="",
		
	},
	
};

function AnimObject:Event_StartAnimation(sender)
	self:StartAnimation(0,self.Properties.Animation);
end

function AnimObject:Event_StopAnimtion(sender)
	self:ResetAnimation(0, -1);
end


function AnimObject:OnReset()
	if (self.Properties.fileModel ~= "") then
		self:LoadCharacter(self.Properties.fileModel,0);

		if(self.Properties.bPhysicalize == 1) then
			self:CreateStaticEntity( 200, 0 );
			self:PhysicalizeCharacter(200, 0, 73, 0);
		end			
	end
	
	--for idx,piece in pairs(self.pieces) do
	
	-- Draw object loaded in slot 0 in normal mode
----	self:DrawObject(0, 1);
	if(self.Properties.attachmet1.fileObject ~= "") then
		self:LoadObject( self.Properties.attachmet1.fileObject, 0, 1);
--		self:DrawObject(0, 0);
		self:AttachObjectToBone( 0, self.Properties.attachmet1.boneName );
	end
	if(self.Properties.attachmet2.fileObject ~= "") then
		self:LoadObject( self.Properties.attachmet2.fileObject, 1, 1);
		self:AttachObjectToBone( 1, self.Properties.attachmet2.boneName );
--		self:DrawObject(1, 1);
	end
	if(self.Properties.attachmet3.fileObject ~= "") then
		self:LoadObject( self.Properties.attachmet3.fileObject, 2, 1);
		self:AttachObjectToBone( 2, self.Properties.attachmet3.boneName );
	end
	if(self.Properties.attachmet4.fileObject ~= "") then
		self:LoadObject( self.Properties.attachmet4.fileObject, 3, 1);
		self:AttachObjectToBone( 3, self.Properties.attachmet4.boneName );
	end
	
	
	-- Physicalize the object and give it an initial velocity to clear player properly
	if (self.Properties.bPlaying == 1) then
		self:StartAnimation(0,self.Properties.Animation);
	else
		self:ResetAnimation(0, -1);
	end
	
	if (self.Properties.bAlwaysUpdate == 1) then
		self:Activate(1);
		self:SetUpdateType( eUT_Unconditional );
	else
		self:Activate(0);
		self:SetUpdateType( eUT_Visible );
	end
end

function AnimObject:Event_HideAttached(sender)
	self:DetachObjectToBone( self.Properties.attachmet1.boneName );
--	self:DrawObject(0, 0);
--	self:DrawObject(1, 0);
--	self:DrawCharacter(0, 0);
--	self:DrawCharacter(1, 0);
	
end

function AnimObject:Event_ShowAttached(sender)
	self:AttachObjectToBone( 0, self.Properties.attachmet1.boneName );
--	self:DrawObject(0, 1);
--	self:DrawObject(0, 0);
end


function AnimObject:OnPropertyChange()
	self:OnReset();
end

function AnimObject:OnInit()
	self:OnReset();
end

function AnimObject:OnShutDown()
end

function AnimObject:OnSave(stm)
end

function AnimObject:OnLoad(stm)
end

AnimObject.FlowEvents =
{
	Inputs =
	{
		HideAttached = { AnimObject.Event_HideAttached, "bool" },
		ShowAttached = { AnimObject.Event_ShowAttached, "bool" },
		StartAnimation = { AnimObject.Event_StartAnimation, "bool" },
		StopAnimation = { AnimObject.Event_StopAnimation, "bool" },
	},
	Outputs =
	{
		HideAttached = "bool",
		ShowAttached = "bool",
		StartAnimation = "bool",
		StopAnimation = "bool",
	},
}
