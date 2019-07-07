# Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
from waflib.Errors import BuildError, WafError
from waflib.Configure import conf
from waflib import Errors, Logs, Utils

import cPickle as pickle
import thread
import threading
import sys
import Queue
import subprocess
import multiprocessing

import os
import socket
import atexit

import remote_task

try:
	import _winreg
except:
	pass

###########################################
################## IB #######################
###########################################

remote_task_process = None

def close_process(process):
	if process:
		process.kill()
		outs, errs = process.communicate()

@Utils.memoize
def get_ib_folder():
	IB_settings = _winreg.OpenKey(_winreg.HKEY_LOCAL_MACHINE, "Software\\Wow6432Node\\Xoreax\\Incredibuild\\Builder", 0, _winreg.KEY_READ)
	(ib_folder,type) = _winreg.QueryValueEx(IB_settings, 'Folder')
	return str(ib_folder)
	
@Utils.memoize
def get_ib_licence_string():
	result = subprocess.check_output( [ str(get_ib_folder())+ '/BuildConsole.exe', '/QueryLicense'])
	return result

def execute_waf_through_ib(bld, ib_folder, ib_executable, ib_extra_commands, has_multi_core_license):
	
	# Setup command coordinator	
	bld.cmd_coordinator = remote_task.Local_CommandCoordinator()
	
	# Build Command Line
	cmd_line_args = []
	for arg in sys.argv[1:]:
		if arg != 'generate_uber_files':
			cmd_line_args += [ arg ]
			
	cmd_line_args.append('--internal-dont-check-recursive-execution=True') # Add special option to not start IB from within IB
	command_line_options = ' '.join(cmd_line_args) # Recreate command line
	cry_waf = bld.path.make_node('Code/Tools/waf-1.7.13/bin/cry_waf.exe')
	command = cry_waf.abspath() + ' ' + command_line_options

	try:
		p = subprocess.Popen ([ '%s/%s' % (str(ib_folder), ib_executable), "/command=" + command, "/useidemonitor", "/nologo", "/MaxCPUS=250"] + ib_extra_commands)
	except:
		raise BuildError()

	p.wait()
		
def execute_ib_as_remote_service(bld, ib_folder, ib_executable, ib_extra_commands, has_multi_core_license):

	# Setup command coordinator	
	max_local_waf_jobs = 0 if has_multi_core_license else bld.jobs # If we have a multicore licence allow IB to schedule local tasks. Otherwise allow WAF to do so
	bld.jobs = bld.options.jobs =  int(bld.options.incredibuild_max_cores) + max_local_waf_jobs
	bld.cmd_coordinator = remote_task.IB_CommandCoordinator_Client(max_local_waf_jobs)
	bld.is_build_master = True
	
	# Build Command Line
	command =  bld.path.make_node('Code/Tools/waf-1.7.13/bin/cry_waf_remote_task.exe').abspath() + ' --use_socket=' + str(bld.cmd_coordinator.get_active_socket())
	
	# Get correct incredibuild installation folder to not depend on PATH
	ib_folder = get_ib_folder()
	
	if max_local_waf_jobs > 0:
		ib_extra_commands.append("/AvoidLocal=On")		
	
	try:
		remote_task_process = subprocess.Popen (['%s/%s' % (str(ib_folder), ib_executable), "/command=" + command, "/useidemonitor", "/nologo", "/MaxCPUS=250"] + ib_extra_commands)
	except:
		raise BuildError()
						
	atexit.register(close_process, remote_task_process)

def init_ib_client(bld):	
	# Check if we can execute remotely with our current licenses	
	if not bld.is_option_true('use_incredibuild_win') and ('win32' in bld.cmd or 'win64' in bld.cmd):
		Logs.warn('[WARNING] Incredibuild for Windows targets disabled by build option')
		return False
	
	if not bld.is_option_true('use_incredibuild_win') and ('win_x86' in bld.cmd or 'win_x64' in bld.cmd):
		Logs.warn('[WARNING] Incredibuild for Windows targets disabled by build option')
		return False
		
	if not bld.is_option_true('use_incredibuild_durango') and 'durango' in bld.cmd:
		Logs.warn('[WARNING] Incredibuild for Durango targets disabled by build option')
		return False
		
	if not bld.is_option_true('use_incredibuild_orbis') and 'orbis' in bld.cmd:
		Logs.warn('[WARNING] Incredibuild for Orbis targets disabled by build option')
		return False
	
	result = get_ib_licence_string()	
	
	has_make_and_build_license = 'Make && Build Tools' in result
	has_dev_tools_acceleration_license = 'Dev Tools Acceleration' in result	
	has_multi_core_license = 'Cores' in result
	
	if not has_make_and_build_license and not has_dev_tools_acceleration_license:
		Logs.warn('Neither "Make && Build Tools" nor " Dev Tools Acceleration"	package found. You need a least one. Incredibuild disabled.')
		return False
		
	if not 'PlayStation' in result and 'orbis' in bld.cmd:
		Logs.warn('Playstation Extension Package not found! Incredibuild will build locally only.')

	#Disabled as current build pipeline does not require the licence
	#if not 'Xbox One' in result and 'durango' in bld.cmd:
	#	Logs.warn('Xbox One Extension Package not found! Incredibuild will build locally only.')

	# Get correct incredibuild installation folder to not depend on PATH
	ib_folder = get_ib_folder()	
	ib_executable = 'xgconsole.exe' if has_dev_tools_acceleration_license else 'BuildConsole.exe'
	ib_extra_commands = ['/profile=Code\\Tools\\waf-1.7.13\\profile.xml'] if has_dev_tools_acceleration_license else []
	
	"/profile=Code\\Tools\\waf-1.7.13\\profile.xml",
	
	# Cancel any other IB build		
	try:
		p = subprocess.check_output([ str(ib_folder) + '/BuildConsole.exe', "/stop", "/nologo"])
	except subprocess.CalledProcessError as e:
		# According to IB docs: BuildConsole returns the code 3 when a build has been stopped. If /stop was used and no build is currently running, 2 is returned.
		# However in pratice the process ends with a 0 value as no exception is raised by subprocess check_output.
		pass

	if bld.is_option_true('incredibuild_use_experimental_pipeline'):
		execute_ib_as_remote_service(bld, ib_folder, ib_executable, ib_extra_commands, has_multi_core_license)		
	else:		
		execute_waf_through_ib(bld, ib_folder, ib_executable, ib_extra_commands, has_multi_core_license)
		
	return True

############
### IB END ###
############

@conf
def instance_is_build_master(conf):
	return conf.is_build_master

@conf
def set_cmd_coordinator(bld, coordinator_name):
	
	# Number of paralle tasks
	bld.jobs = bld.options.jobs = multiprocessing.cpu_count() - 2
	
	if coordinator_name == 'IB':			
		Logs.info("[WAF] Run Incredibuild")
		ib_successfully_initialized = init_ib_client(bld)		
	
	if coordinator_name == 'Local' or not ib_successfully_initialized:
		bld.cmd_coordinator = remote_task.Local_CommandCoordinator()
	
@conf
def setup_command_coordinator(bld):
	
	bld.is_build_master = True
	coordinator = 'Local'	
	if not Utils.unversioned_sys_platform() == 'win32':
		bld.set_cmd_coordinator('Local') # Don't use recursive execution on non-windows hosts
		return
	
	if 'clean_' in bld.cmd or 'generate_uber_files' in bld.cmd or 'msvs' in bld.cmd:
		bld.set_cmd_coordinator('Local')
		return

	## Don't use IB for special single file operations
	#if bld.is_option_true('show_includes') or bld.is_option_true('show_preprocessed_file') or bld.is_option_true('show_disassembly') or bld.options.file_filter != "":
	#	bld.set_cmd_coordinator('Local')
	#	return
		
	if bld.is_option_true('internal_dont_check_recursive_execution'):
		bld.set_cmd_coordinator('Local')
		bld.jobs = bld.options.jobs =  int(bld.options.incredibuild_max_cores)
		return
			
	if bld.is_option_true('use_incredibuild'):
		bld.set_cmd_coordinator('IB')
		bld.is_build_master = bld.is_option_true('incredibuild_use_experimental_pipeline')
		return
	else:
		Logs.warn('[WARNING] Incredibuild disabled by build option')
		bld.set_cmd_coordinator('Local')
		return
