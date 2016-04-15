CharacterAttachHelper = {
	Editor={
		Icon="Magnet.bmp",
	},
	Properties =
	{
		BoneName = "Bip01 Head",
	},
}

--------------------------------------------------------------------------
function CharacterAttachHelper:OnInit()
	self:MakeAttachment();
end

--------------------------------------------------------------------------
function CharacterAttachHelper:OnStartGame()
  self:MakeAttachment();
end

--------------------------------------------------------------------------
function CharacterAttachHelper:OnPropertyChange()
	self:MakeAttachment();
end

--------------------------------------------------------------------------
function CharacterAttachHelper:OnBindThis()
	self:MakeAttachment();
end

--------------------------------------------------------------------------
function CharacterAttachHelper:OnUnBindThis()
	self:DestroyAttachment();
end

--------------------------------------------------------------------------
function CharacterAttachHelper:OnShutDown()
	self:DestroyAttachment();
end

--------------------------------------------------------------------------
function CharacterAttachHelper:MakeAttachment()
	local parent = self:GetParent();
	if (parent) then
		parent:DestroyAttachment( 0,self:GetName() );

	  -- Make sure the entity ignores transformation of the parent.
		self:EnableInheritXForm(0);

		-- Attach this entity to the parent.
		parent:CreateBoneAttachment( 0,self.Properties.BoneName,self:GetName() );
		parent:SetAttachmentObject( 0,self:GetName(),self.id,-1,0 );
	end
end

--------------------------------------------------------------------------
function CharacterAttachHelper:DestroyAttachment()
	local parent = self:GetParent();
	if (parent) then
		parent:DestroyAttachment( 0,self:GetName() );
	end
end

--------------------------------------------------------------------------
CharacterAttachHelper.Server = 
{
	OnBindThis = CharacterAttachHelper.OnBindThis,
	OnUnBindThis = CharacterAttachHelper.OnUnBindThis,
};
CharacterAttachHelper.Client = 
{
	OnBindThis = CharacterAttachHelper.OnBindThis,
	OnUnBindThis = CharacterAttachHelper.OnUnBindThis,
};
