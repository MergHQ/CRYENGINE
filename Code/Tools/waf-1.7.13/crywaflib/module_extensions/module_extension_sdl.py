# Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

import os
from waflib import Logs
from waflib.TaskGen import feature
from waflib.CryModuleExtension import module_extension
from waflib.Utils import run_once

@module_extension('sdl2')
def module_extensions_sdl2(ctx, kw, entry_prefix, platform, configuration):

	if platform.startswith('win_x86'):
		kw[entry_prefix + 'includes'] += [ ctx.CreateRootRelativePath('Code/SDKs/SDL2/include/win') ]
		kw[entry_prefix + 'lib']      += [ 'SDL2' ]
		kw[entry_prefix + 'libpath']  += [ ctx.CreateRootRelativePath('Code/SDKs/SDL2/lib/win_x86') ]

	elif platform.startswith('win_x64'):
		kw[entry_prefix + 'includes'] += [ ctx.CreateRootRelativePath('Code/SDKs/SDL2/include/win') ]
		kw[entry_prefix + 'lib']      += [ 'SDL2' ]
		kw[entry_prefix + 'libpath']  += [ ctx.CreateRootRelativePath('Code/SDKs/SDL2/lib/win_x64') ]

	elif platform.startswith('linux_x64'):
		kw[entry_prefix + 'includes'] += [ ctx.CreateRootRelativePath('Code/SDKs/SDL2/include/linux') ]
		kw[entry_prefix + 'lib']      += [ 'SDL2' ]
		kw[entry_prefix + 'libpath']  += [ ctx.CreateRootRelativePath('Code/SDKs/SDL2/lib/linux_x64') ]
	elif platform.startswith('android'):
		kw[entry_prefix + 'includes'] += [ ctx.CreateRootRelativePath('Code/SDKs/SDL2/include/android-armeabi-v7a') ]
		kw[entry_prefix + 'lib']      += [ 'SDL2' ]
		kw[entry_prefix + 'libpath']  += [ ctx.CreateRootRelativePath('Code/SDKs/SDL2/lib/android-armeabi-v7a') ]
	else:
		return

	if not platform  == 'project_generator':
		kw[entry_prefix + 'features'] += [ 'copy_sdl2_binaries' ]

@module_extension('sdl2_ext')
def module_extensions_sdl2_ext(ctx, kw, entry_prefix, platform, configuration):

	if platform.startswith('android'):
		kw[entry_prefix + 'includes'] += [ ctx.CreateRootRelativePath('Code/Tools/SDLExtension/src/include') ]
		kw[entry_prefix + 'lib']      += [ 'SDL2Ext' ]
		kw[entry_prefix + 'libpath']  += [ ctx.CreateRootRelativePath('Code/Tools/SDLExtension/lib/android-armeabi-v7a') ]
	else:
		return		

@feature('copy_sdl2_binaries')
@run_once
def feature_copy_sdl2_binaries(self):
	bld 			= self.bld
	platform		= bld.env['PLATFORM']
	configuration	= bld.env['CONFIGURATION']

	if platform  == 'project_generator':
		return

	if platform == 'win_x64':
		files_to_copy = ['SDL2.dll']
		libfolder =  'win_x64'

	elif platform =='win_x86':
		files_to_copy = ['SDL2.dll']
		libfolder =  'win_x86'

	elif platform.startswith('linux_x64'):
		files_to_copy = ['libSDL2-2.0.so.0']
		libfolder = 'linux_x64'
	elif platform.startswith('android'):
		return # do nothing
	
	else:
		Logs.error('[ERROR] WAF does not support SDL2 for platform %s.' % platform)
		return

	sdl2_libpath = bld.CreateRootRelativePath('Code/SDKs/SDL2/lib') + os.sep + libfolder

	output_folder = bld.get_output_folders(platform, configuration)[0]

	if hasattr(self, 'output_sub_folder'):
		if os.path.isabs(self.output_sub_folder):
			output_folder = bld.root.make_node(self.output_sub_folder)
		else:
			output_folder = output_folder.make_node(self.output_sub_folder)

	for f in files_to_copy:
		if os.path.isfile(os.path.join(sdl2_libpath, f)):
			self.create_task('copy_outputs', self.bld.root.make_node(os.path.join(sdl2_libpath, f)), output_folder.make_node(f))
		else:
			Logs.error('[ERROR] Could not copy %s: file not found in %s.' % (f, sdl2_libpath))


############################################################################################################

@module_extension('sdl_mixer')
def module_extensions_sdl_mixer(ctx, kw, entry_prefix, platform, configuration):

	if platform.startswith('win'):
		kw[entry_prefix + 'includes'] += [ ctx.CreateRootRelativePath('Code/SDKs/Audio/SDL_mixer/include') ]
		kw[entry_prefix + 'lib'] += [ 'SDL2_mixer' ]		
		arch = ('x64' if platform == 'win_x64' else 'x86')		
		kw[entry_prefix + 'libpath'] += [ ctx.CreateRootRelativePath('Code/SDKs/Audio/SDL_mixer/lib') + os.sep + arch ]
	elif platform.startswith('linux'):
		kw[entry_prefix + 'includes'] += [ ctx.CreateRootRelativePath('Code/SDKs/Audio/SDL_mixer/include') ]
		Logs.warn('[WARNING] SDL_Mixer binaries missing')
	else:
		Logs.error('[ERROR] SDL_Mixer is not supported for plaform %s by WAF' % platform)
		
	if not platform  == 'project_generator':
		kw[entry_prefix + 'features'] += [ 'copy_sdl2_mixer_binaries' ]

@feature('copy_sdl2_mixer_binaries')
@run_once
def feature_copy_sdl_mixer_binaries(self):
	bld 			= self.bld
	platform	= bld.env['PLATFORM']
	configuration = bld.env['CONFIGURATION']
		
	if platform  == 'project_generator':
		return
		
	sdl_mixer_binaries = [ 'SDL2_mixer', 'libogg-0', 'libvorbis-0', 'libvorbisfile-3', 'smpeg2' ]
	output_folder = bld.get_output_folders(platform, configuration)[0]
	
	if platform.startswith('win'):
		arch = ('x64' if platform == 'win_x64' else 'x86')
		extention = '.dll'

		# Copy SDL mixer
		sdl_mixer_base = bld.CreateRootRelativePath('Code/SDKs/Audio/SDL_mixer/lib') + os.sep  + arch + os.sep
		for bin_name in sdl_mixer_binaries:
			sdl_mixer_bin = sdl_mixer_base +  bin_name
			self.create_task('copy_outputs', bld.root.make_node(sdl_mixer_bin + extention), output_folder.make_node(bin_name + extention))
	elif platform.startswith('linux'):
		pass
	else:
		Logs.error('[ERROR] SDL_Mixer is not supported for plaform %s by WAF.'  % platform)