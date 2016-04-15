# Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

import os
from waflib import Logs, Utils
from waflib.TaskGen import feature
from waflib.CryModuleExtension import module_extension
from waflib.Utils import run_once

@Utils.memoize
def check_path_exists(path):
	if os.path.isdir(path):
		return True
	else:
		Logs.warn('[WARNING] ' + path + ' not found, this feature is excluded from this build.')
	return False

@module_extension('openvr')
def module_extensions_openvr(ctx, kw, entry_prefix, platform, configuration):
	if platform  == 'project_generator':
		return
	
	if not check_path_exists(ctx.CreateRootRelativePath('Code/SDKs/OpenVR')):
		return

	if not platform.startswith('win'):
		return		
		
	kw[entry_prefix + 'defines'] += [ 'USE_OPENVR', 'INCLUDE_OPENVR_SDK' ]
	kw[entry_prefix + 'includes'] += [ ctx.CreateRootRelativePath('Code/SDKs/OpenVR/headers') ]
	kw[entry_prefix + 'lib'] += [ 'openvr_api' ]

	if platform == 'win_x86':
		kw[entry_prefix + 'libpath'] += [ ctx.CreateRootRelativePath('Code/SDKs/OpenVR/lib/win32') ]
	elif platform == 'win_x64':
		kw[entry_prefix + 'libpath'] += [ ctx.CreateRootRelativePath('Code/SDKs/OpenVR/lib/win64') ]

	kw[entry_prefix + 'features'] += [ 'copy_openvr_binaries' ]

@feature('copy_openvr_binaries')
@run_once
def feature_copy_openvr_binaries(self):
	bld 		  = self.bld
	platform	  = bld.env['PLATFORM']
	configuration = bld.env['CONFIGURATION']
		
	if platform  == 'project_generator':
		return
		
	bin_location = {'win_x86' : '/win32/openvr_api.dll'	, 'win_x64' : '/win64/openvr_api.dll'}
			
	if not platform in bin_location:
		Logs.error('[ERROR] OpenVR is not supported on plaform by WAF: %s' % platform)
		
	output_folder = bld.get_output_folders(platform, configuration)[0]			
	openvr_bin = bld.CreateRootRelativePath('Code/SDKs/OpenVR/bin') + bin_location[platform]
	self.create_task('copy_outputs', bld.root.make_node(openvr_bin), output_folder.make_node(os.path.basename(openvr_bin)))
