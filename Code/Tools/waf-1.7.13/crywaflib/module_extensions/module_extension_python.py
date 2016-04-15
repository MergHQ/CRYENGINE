# Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

import os
from waflib import Logs
from waflib.TaskGen import feature
from waflib.CryModuleExtension import module_extension

@module_extension('python27')
def module_extensions_python27(ctx, kw, entry_prefix, platform, configuration):
	
	kw[entry_prefix + 'defines']  += [ 'USE_PYTHON_SCRIPTING', 'BOOST_PYTHON_STATIC_LIB', 'HAVE_ROUND' ]
	kw[entry_prefix + 'use']      += [ 'python27' ]
	kw[entry_prefix + 'includes'] += [ ctx.CreateRootRelativePath('Code/Libs/python'), ctx.CreateRootRelativePath('Code/SDKs/Python27/Include'), ctx.CreateRootRelativePath('Code/SDKs/boost') ]
	kw[entry_prefix + 'libpath']  += [ ctx.CreateRootRelativePath('Code/SDKs/boost/lib/64bit') ]
