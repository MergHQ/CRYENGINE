# Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

import sys
import itertools
from tools_shelf_actions import *

general.set_cvar('r_DebugGBuffer', 0)
general.set_cvar('e_defaultmaterial', 0)
general.set_cvar('r_TexBindMode', 0)
general.set_cvar('p_draw_helpers', '0')
general.set_cvar('r_showlines', 0)
general.set_cvar('r_wireframe', 0)
general.set_cvar('r_shownormals', 0)
general.set_cvar('r_ShowTangents', 0)
general.set_cvar('r_TexelsPerMeter', float(0))
general.set_cvar('e_DebugDraw', 0)
general.set_cvar('e_LodMin', 0)


if len(sys.argv) > 1:
	mode = sys.argv[1]
	if mode == 'Fullshading':
		general.set_cvar('r_DebugGBuffer', 0)	
	elif mode == 'Normals':
		toggleCvarsValue('mode_%s' % mode, 'r_DebugGBuffer', 0, 1)
	elif mode == 'Smoothness':
		toggleCvarsValue('mode_%s' % mode, 'r_DebugGBuffer', 0, 2)
	elif mode == 'Reflectance':
		toggleCvarsValue('mode_%s' % mode, 'r_DebugGBuffer', 0, 3)
	elif mode == 'Albedo':
		toggleCvarsValue('mode_%s' % mode, 'r_DebugGBuffer', 0, 4)
	elif mode == 'Lighting_Model':
		toggleCvarsValue('mode_%s' % mode, 'r_DebugGBuffer', 0, 5)
	elif mode == 'Translucency':
		toggleCvarsValue('mode_%s' % mode, 'r_DebugGBuffer', 0, 6)
	elif mode == 'Sun_self_shadowing':
		toggleCvarsValue('mode_%s' % mode, 'r_DebugGBuffer', 0, 7)
	elif mode == 'Subsurface_scattering':
		toggleCvarsValue('mode_%s' % mode, 'r_DebugGBuffer', 0, 8)
	elif mode == 'Specular_validation_overlay':
		toggleCvarsValue('mode_%s' % mode, 'r_DebugGBuffer', 0, 9)
	elif mode == 'default_material':
		toggleCvarsValue('mode_%s' % mode, 'e_defaultmaterial', 0, 1)
	elif mode == 'default_material_normals':
		toggleCvarsValue('mode_%s' % mode, 'r_TexBindMode', 0, 6)
	elif mode == 'collisionshapes':
		toggleCvarsValue('mode_%s' % mode, 'p_draw_helpers', '0', '1')
	elif mode == 'shaded_wireframe':
		toggleCvarsValue('mode_%s' % mode, 'r_showlines', 0, 2)
	elif mode == 'wireframe':
		toggleCvarsValue('mode_%s' % mode, 'r_wireframe', 0, 1)
	elif mode == 'Vnormals':	
		toggleCvarsValue('mode_%s' % mode, 'r_shownormals', 0, 1)
	elif mode == 'Tangents':
		toggleCvarsValue('mode_%s' % mode, 'r_ShowTangents', 0, 1)	
	elif mode == 'texelspermeter360':
		toggleCvarsValue('mode_%s' % mode, 'r_TexelsPerMeter', float(0), float(256))
	elif mode == 'texelspermeterpc':
		toggleCvarsValue('mode_%s' % mode, 'r_TexelsPerMeter', float(0), float(512))
	elif mode == 'texelspermeterpc2':
		toggleCvarsValue('mode_%s' % mode, 'r_TexelsPerMeter', float(0), float(1024))
	elif mode == 'lods':
		cycleList1 = ["e_DebugDraw 0", "e_DebugDraw 3", "e_DebugDraw -3"]
		cycleConsolValue('mode_%s' % mode, cycleList1)
	elif mode == 'lods_level':
		cycleList2 = ["e_LodMin 0", "e_LodMin 1", "e_LodMin 2", "e_LodMin 3", "e_LodMin 4", "e_LodMin 5"]
		cycleConsolValue('mode_%s' % mode, cycleList2)