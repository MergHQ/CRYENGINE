#!/usr/bin/env python3

import json
import os.path

def load (path):
	try:
		file= open (path, 'r')
		proj= json.loads (file.read())
		file.close()
	except ValueError:
		proj= None

	return proj

def save (self, path):
	file= open (path, 'w')
	file.write (json.dumps (self, indent=4, sort_keys=True))
	file.close()
	
def is_valid (self):
	return True

def engine_id(self):
	return self.get ('require', {}).get ('engine')
	
def shared_tuple(self, platform, config):
	lib= self['content']['libs'][0]
	return (lib['name'], lib.get('shared', {}).get (platform))

def shared_dir(self, platform, config):
	name, shared= shared_tuple (self, platform, config)
	return os.path.dirname (shared)

def asset_dir(self):
	return self['content'].get ('assets', [None])[0]

def cmakelists_dir(self):
	return self['content']['code'][0]

def require_list(self):
	return self.get ('require', [])
		
		