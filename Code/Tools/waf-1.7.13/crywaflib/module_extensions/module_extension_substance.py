# Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

import os
from waflib import Logs
from waflib.TaskGen import feature
from waflib.CryModuleExtension import module_extension
from waflib.Utils import run_once

@module_extension('substance')
def module_extensions_substance(ctx, kw, entry_prefix, platform, configuration):
	
	if not platform.startswith('win'):
		return		
		
	kw[entry_prefix + 'includes'] += [ 
		ctx.CreateRootRelativePath('Code/SDKs/SubstanceEngines/include'), 
		ctx.CreateRootRelativePath('Code/SDKs/SubstanceEngines/3rdparty/tinyxml') 
	]
	
	kw[entry_prefix + 'libpath'] += [
		ctx.CreateRootRelativePath('Code/SDKs/SubstanceEngines/lib/win32-msvc2015-64/') + ('/debug_md' if configuration == "debug" else '/release_md'),
		ctx.CreateRootRelativePath('Code/SDKs/SubstanceEngines/3rdparty/tinyxml/lib/win32-msvc2015-64/') + ('/debug_md' if configuration == "debug" else '/release_md')
	]
	
	kw[entry_prefix + 'lib'] += ['substance_framework']
		
	if not platform  == 'project_generator':
		kw[entry_prefix + 'features'] += [ 'copy_substance_binaries' ]

@feature('copy_substance_binaries')
@run_once
def feature_copy_substance_binaries(self):	
	bld 		  = self.bld
	platform	  = bld.env['PLATFORM']
	configuration = bld.env['CONFIGURATION']
		
	if platform  == 'project_generator':
		return
		
	if  platform != 'win_x64':
		Logs.error('[ERROR] Substance is not supported on plaform "%s"' % platform)
		return
		
	msvc_version = bld.env['MSVC_VERSION']
	if msvc_version == '14.0':
		msvc_dir = 'win32-msvc2015-64'
	else:
		Logs.error('[ERROR] Substance is not supported for MSVC version "%s"' % msvc_version)
		return
		
	subfolder = ('debug' if configuration == 'debug' else 'release') + ('_md' if '/MD' in self.env['CXXFLAGS'] else '_mt')
			
	files = ['substance_linker.dll', 'substance_sse2_blend.dll']

	output_folder = bld.get_output_folders(platform, configuration)[0]
	substance_bin = bld.CreateRootRelativePath('Code/SDKs/SubstanceEngines/bin/' + msvc_dir + '/' + subfolder ) 
	
	for f in files:	
		self.create_task('copy_outputs', bld.root.make_node(substance_bin + '/' + f), output_folder.make_node(f))
