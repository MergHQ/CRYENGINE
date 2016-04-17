# Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

import os
from waflib import Logs
from waflib.TaskGen import feature
from waflib.CryModuleExtension import module_extension
from waflib.Utils import run_once

@module_extension('mono')
def module_extensions_mono(ctx, kw, entry_prefix, platform, configuration):
	
	if not platform.startswith('win'):
		return		
	
	kw[entry_prefix + 'defines'] += [ 'USE_MONO_BRIDGE' ]
	kw[entry_prefix + 'includes'] += [ ctx.CreateRootRelativePath('Code/SDKs/Mono/include/mono-2.0') ]

	if platform == 'win_x86':
		kw[entry_prefix + 'libpath'] += [ ctx.CreateRootRelativePath('Code/SDKs/Mono/lib/x86') ]
	elif platform == 'win_x64':
		kw[entry_prefix + 'libpath'] += [ ctx.CreateRootRelativePath('Code/SDKs/Mono/lib/x64') ]
		
	if not platform  == 'project_generator':
		kw[entry_prefix + 'features'] += [ 'copy_mono_binaries' ]

@feature('copy_mono_binaries')
@run_once
def feature_copy_mono_binaries(self):
	bld 			= self.bld
	platform	= bld.env['PLATFORM']
	configuration = bld.env['CONFIGURATION']
		
	if platform  == 'project_generator':
		return
		
	bin_location = {'win_x86' : '/x86/'	, 'win_x64' : '/x64/'}
	bin_items = [ 'mono-2.0.dll', 'msvcr120.dll', 'msvcp120.dll' ]
			
	if not platform in bin_location:
		Logs.error('[ERROR] Mono is not supported on plaform by WAF: %s' % platform)
		
	# Copy core mono binary
	output_folder = bld.get_output_folders(platform, configuration)[0]
	for item in bin_items:
		mono_bin = bld.CreateRootRelativePath('Code/SDKs/Mono/bin') + bin_location[platform] + item
		self.create_task('copy_outputs', bld.root.make_node(mono_bin), output_folder.make_node(os.path.basename(mono_bin)))

	# Copy additional required binaries at runtime
	mono_base = "Code/SDKs/Mono"
	mono_base_len = len(mono_base)
	mono_node = bld.root.make_node(mono_base)
	mono_lib_node = mono_node.make_node("lib/mono")
	mono_etc_node = mono_node.make_node("etc/mono")
	
	tgt_folder = output_folder.make_node("mono")
	
	for src_path_node in [mono_lib_node, mono_etc_node]:
		for path, subdirs, files in os.walk(src_path_node.abspath(), followlinks=True):
			path_node = bld.root.make_node(path)
			stripped_path = path[mono_base_len:]
			for name in files:		
				if name.endswith('.dll') or name.endswith('.config'):
					src_node = path_node.make_node(name)
					tgt_node = tgt_folder.make_node(stripped_path).make_node(name)
					#print "== tgt", tgt_node.abspath()
					self.create_task('copy_outputs', src_node, tgt_node)
