VTOLSpawnPoint =
{
	Server = {},
	Client = {},

	Editor = {
		Icon		= "Item.bmp",
		IconOnTop	= 1,
		ShowBounds  = 1,
	},
}

--------------------------------------------------------------------------
function VTOLSpawnPoint.Server:OnInit()
	self:SetFlags(ENTITY_FLAG_CLIENT_ONLY,0);
end

--------------------------------------------------------------------------
function VTOLSpawnPoint.Client:OnInit()
	self:SetFlags(ENTITY_FLAG_CLIENT_ONLY,0);
end
