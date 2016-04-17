TagPoint = {
	type = "TagPoint",

	Editor = {
		Icon = "TagPoint.bmp",
	},
}

-------------------------------------------------------
function TagPoint:OnSpawn()
	self:SetFlags(ENTITY_FLAG_CLIENT_ONLY,0);
end

-------------------------------------------------------
function TagPoint:OnInit()
	CryAction.RegisterWithAI(self.id, AIOBJECT_WAYPOINT);
end

