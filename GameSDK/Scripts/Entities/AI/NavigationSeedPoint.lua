NavigationSeedPoint = {
  type = "NavigationSeedPoint",

	Editor = {
		Icon = "Seed.bmp",
	},
}

-------------------------------------------------------
function NavigationSeedPoint:OnInit()
	CryAction.RegisterWithAI(self.id, AIOBJECT_NAV_SEED);
end