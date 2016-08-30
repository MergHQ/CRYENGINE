#!/usr/bin/env python3

import os
import json

from win32com.shell import shell, shellcon

ENGINE_FILENAME= 'cryengine.json'
ENGINE_EXTENSION= '.cryengine'

def path (filename):
	#https://blogs.msdn.microsoft.com/patricka/2010/03/18/where-should-i-store-my-data-and-configuration-files-if-i-target-multiple-os-versions/	
	return os.path.join (shell.SHGetFolderPath (0, shellcon.CSIDL_COMMON_APPDATA, None, 0), 'Crytek', 'CRYENGINE', filename)

def load_engines():
	registry_path= path(ENGINE_FILENAME)
	
	try:
		file= open (registry_path, 'r')
		db= json.loads (file.read())
		file.close()
	except:
		db= {}
	
	return db

def save_engines (db):
	registry_path= path (ENGINE_FILENAME)
	registry_dir= os.path.dirname (registry_path)
	if not os.path.isdir (registry_dir):
		os.makedirs (registry_dir)
	
	file= open (registry_path, 'w')
	file.write (json.dumps (db, indent=4, sort_keys=True))
	file.close()

#---

#def project_file (db, k):
	#project= db.get(k)
	#return project['uri']

#def filter_plugin (db, k_list):
	#return filter (lambda k: db[k]['info']['type'] == 'plugin', k_list)		

#def filter_engine (db, k_list):
	#return filter (lambda k: db[k]['info']['type'] == 'engine', k_list)
	
def engine_path (db, engine_version):
	path= db.get (engine_version, {'uri': None})['uri']
	return path and os.path.dirname (path) or None

	# require= {}
	# reqdict_create (db, proj['require'], require)
	# require= reqdict_listsorted (require)
	# 
	# missing= []
	# engine= []
	# for k in require:
	# 	item= db.get (k, {'info':{}})
	# 	info= item['info']
	# 	if info.get ('type') == 'engine':
	# 		engine.append ((info['version'], item['uri']))
	# 		
	# engine.sort()
	# return os.path.dirname (engine[0][1])
