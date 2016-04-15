#############################################################################
## Crytek Source File
## Copyright (C) 2015, Crytek Studios
##
## Creator: Nico Moss
## Date: July 03, 2015
## Description: WAF  based build system
#############################################################################

from waflib import Configure, Logs
from waflib.Configure import conf

MODULE_EXTENSIONS = {}

def module_extension(feature_name):
	# Decorator that accepts the function
	def decorator(function):		
		# Wrapper function takes arguments for the decorated function
		def wrapped(ctx,kw,entry_prefix,platform,configuration,*args, **kwargs):	
			function(ctx,kw,entry_prefix,platform,configuration, *args, **kwargs)	
		# Save feature to feature list
		try:
			if not wrapped in MODULE_EXTENSIONS[feature_name]:
				MODULE_EXTENSIONS[feature_name] += [wrapped]
		except:
			MODULE_EXTENSIONS[feature_name] = [wrapped]
		return wrapped
	return decorator
	
@conf 
def execute_module_extension(ctx, extension_name, kw, entry_prefix, platform, configuration):
	if extension_name in MODULE_EXTENSIONS:
		for extentsion in MODULE_EXTENSIONS[extension_name]:
			extentsion(ctx, kw, entry_prefix, platform, configuration)
	else:
		ctx.fatal('[Error]: Extension "%s" is not defined in WAF' % extension_name)