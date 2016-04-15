PrecacheCamera = {
	Editor={
		Icon="PrecacheCamera.bmp",
	},
};

function PrecacheCamera:OnSpawn()
	self:SetFlags(ENTITY_FLAG_CLIENT_ONLY, 0);
end
