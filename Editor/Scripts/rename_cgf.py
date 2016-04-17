# Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

objects = general.get_all_objects("", "") # Get the name list of all objects in the level.
# If there is any object with the geometry file of "objects\\default\\primitive_box.cgf",
# change it to "objects\\default\\primitive_cube.cgf".
for obj in objects:
	geometry_file = general.get_entity_geometry_file(obj)
	if geometry_file == "objects\\default\\primitive_box.cgf":
		general.set_entity_geometry_file(obj, "objects\\default\\primitive_cube.cgf")
