AIGroup =
{
	defendPressureThreshold = 0.75,
	
	OnSaveAI = function(self, save)
		save.AreaSearchCoordId = self.AreaSearchCoordId
		save.investigateMemberDeathID = self.investigateMemberDeathID
	end,
	
	OnLoadAI = function(self, saved)
		self.AreaSearchCoordId = saved.AreaSearchCoordId
		self.investigateMemberDeathID = saved.investigateMemberDeathID
	end,
}