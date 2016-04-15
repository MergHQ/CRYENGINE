JointGen = {
	Properties = {
		object_BreakTemplate = "objects/broken_cube.cgf",
		file_Material = "",
		bHierarchical = 1, --[0,1,1,"Keep unbroken nodes until something inside them breaks"]
		Impulse = 1000, --[0,1000000,10,"Sample impulse that should cause breakange if applied at this location"]
		Radius = 0.05, --[0,10,0.01,"Radius of the samping sphere"]
		MinSize = 0.0003, --[0,1,0.0001,"Skip cut pieces below this volume (0..1, in fractions of the original node's volume)"]
		Timeout = -1, --[-1,1000000,1,"Destroy broken pieces after this timout expires, in seconds. <0 = no timeout"]
		TimeoutVariance = 0, --[0,1000,1,"Random variance of Timeout, in seconds"]
		TimeoutInvis = -1, --[-1,1000000,1,"Similar to Timeout, but only ticks when a piece is invisible, and is reset if it becomes visible"]
		TimeoutMaxVol = 0, --[0,1000,0.001,"Don't apply Timeout to pieces with volume larger than this, in m^3. 0 = no limit"]
		ParticleEffect = "",
		Scale = 1, --[0,10,0.1,"Extra scale for the break template, relative to the auto-computed one"]
		Offset = {
			x=0, --[-1,1,0.01,"Extra offset for the break template, relative to the autocomputed one. Set in fractions of the original size"]
			y=0, --[-1,1,0.01,"Extra offset for the break template, relative to the autocomputed one. Set in fractions of the original size"]
			z=0, --[-1,1,0.01,"Extra offset for the break template, relative to the autocomputed one. Set in fractions of the original size"]
		},
		ei_Alignment = -1,
		AlignmentSeed = -1, --[-1,1000000000,1,"Random seed for random orientation and timeout variance. Only one JointGen per object needs it. -1 = no seed"]
	},

	Editor = {
		Icon = "Shake.bmp",
	},
}

function JointGen:OnReset()
end

function JointGen:OnPropertyChange()
	self:OnReset()
end

function JointGen:OnSpawn()
	self:OnReset()
end
