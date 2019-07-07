# Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

import sys
import itertools
from tools_shelf_actions import *

if len(sys.argv) > 1:
	mode = sys.argv[1]
	if mode == 'e_GlobalFog':
		toggleCvarsValue('mode_%s' % mode, 'e_Fog', 1, 0)	
	elif mode == 'e_Clouds':
		toggleCvarsValue('mode_%s' % mode, 'e_Clouds', 1, 0)
	elif mode == 'r_Rain':
		toggleCvarsValue('mode_%s' % mode, 'r_Rain', 1, 0)	
	elif mode == 'e_Sun':
		toggleCvarsValue('mode_%s' % mode, 'e_Sun', 1, 0)
	elif mode == 'e_Skybox':
		toggleCvarsValue('mode_%s' % mode, 'e_Skybox', 1, 0)
	elif mode == 'e_TimeOfDay':
		cycleList1 = ["e_TimeOfDay 0", "e_TimeOfDay 1.5", "e_TimeOfDay 3", "e_TimeOfDay 4.5", "e_TimeOfDay 6","e_TimeOfDay 7.5","e_TimeOfDay 9","e_TimeOfDay 10.5", "e_TimeOfDay 12","e_TimeOfDay 13.5", "e_TimeOfDay 15","e_TimeOfDay 16.5", "e_TimeOfDay 18","e_TimeOfDay 19.5","e_TimeOfDay 21","e_TimeOfDay 22.5"]
		cycleConsolValue('mode_%s' % mode, cycleList1)
	elif mode == 'r_SSReflections':
		toggleCvarsValue('mode_%s' % mode, 'r_SSReflections', 1, 0)
	elif mode == 'e_Shadows':
		toggleCvarsValue('mode_%s' % mode, 'e_Shadows', 1, 0)
	elif mode == 'r_TransparentPasses':
		toggleCvarsValue('mode_%s' % mode, 'r_TransparentPasses', 1, 0)
	elif mode == 'e_GI':
		toggleCvarsValue('mode_%s' % mode, 'e_GI', 1, 0)
	elif mode == 'r_ssdo':
		toggleCvarsValue('mode_%s' % mode, 'r_ssdo', 1, 0)	
	elif mode == 'e_DynamicLights':
		toggleCvarsValue('mode_%s' % mode, 'e_DynamicLights', 1, 0)
	elif mode == 'e_Brushes':
		toggleCvarsValue('mode_%s' % mode, 'e_Brushes', 1, 0)
	elif mode == 'designerObjects':
		toggleHideMaskValues('solids')
	elif mode == 'e_Entities':
		toggleCvarsValue('mode_%s' % mode, 'e_Entities', 1, 0)
	elif mode == 'PrefabObjects':
		toggleHideMaskValues('prefabs')
	elif mode == 'e_Vegetation':
		toggleCvarsValue('mode_%s' % mode, 'e_Vegetation', 1, 0)
	elif mode == 'e_Decals':
		toggleCvarsValue('mode_%s' % mode, 'e_Decals', 1, 0)
	elif mode == 'e_Terrain':
		toggleCvarsValue('mode_%s' % mode, 'e_Terrain', 1, 0)
	elif mode == 'e_Particles':
		toggleCvarsValue('mode_%s' % mode, 'e_Particles', 1, 0)
	elif mode == 'r_Flares':
		toggleCvarsValue('mode_%s' % mode, 'r_Flares', 1, 0)
	elif mode == 'r_Beams':
		toggleCvarsValue('mode_%s' % mode, 'r_Beams', 1, 0)
	elif mode == 'e_FogVolumes':
		toggleCvarsValue('mode_%s' % mode, 'e_FogVolumes', 1, 0)
	elif mode == 'e_WaterOcean':
		toggleCvarsValue('mode_%s' % mode, 'e_WaterOcean', 1, 0)	
	elif mode == 'e_WaterVolumes':
		toggleCvarsValue('mode_%s' % mode, 'e_WaterVolumes', 1, 0)
	elif mode == 'e_Wind':
		toggleCvarsValue('mode_%s' % mode, 'e_Wind', 1, 0)
	elif mode == 'e_BBoxes':
		toggleCvarsValue('mode_%s' % mode, 'e_BBoxes', 0, 1)