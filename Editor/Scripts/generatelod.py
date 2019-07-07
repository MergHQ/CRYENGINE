# Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

'''
Generates a 50% lod for the selected model

@argument name="Lod Percentage", type="string", default="50.0f"
'''

import sys
import time

percentage = float(sys.argv[1])

selectedcgf = lodtools.getselected()
selectedmaterial = lodtools.getselectedmaterial()

loadedmodel = lodtools.loadcgf(selectedcgf)
loadedmaterial = lodtools.loadmaterial(selectedmaterial)

if loadedmodel == True and loadedmaterial == True:
	lodtools.generatelodchain()
	finished = 0.0
	while finished >= 0.0:
		finished = lodtools.generatetick()
		print('Lod Chain Generation progress: ' + str(finished))
		time.sleep(1)
		if finished == 1.0:
			break
	
	lodtools.createlod(1,percentage)
	lodtools.generatematerials(1,512,512)
	lodtools.savetextures(1)
	lodtools.savesettings()
	lodtools.reload()

