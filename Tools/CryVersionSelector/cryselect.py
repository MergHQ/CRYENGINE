#!/usr/bin/env python3

import argparse
import os.path
import sys
import json

import shutil
import subprocess
import tempfile

import win32api, win32ui, win32con
from win32com.shell import shell
import pywintypes

import cryproject, cryregistry

def command_title (args):
	return {
	'upgrade': 'Upgrade project',
	'projgen': 'Generate solution',
	'build': 'Build Solution',
	'edit': 'Edit game',
	'open': 'Run game',
	'monodev': 'Edit C# code'
	}.get (args.command, '')

#--- errors

def error_project_not_found (args):
	message= "'%s' not found.\n" % args.project_file
	if args.silent:
		sys.stderr.write (message)
	else:
		win32ui.MessageBox (message, command_title (args), win32con.MB_OK | win32con.MB_ICONERROR)
	sys.exit (404)

def error_project_json_decode (args):
	message= "Unable to parse '%s'.\n" % args.project_file
	if args.silent:
		sys.stderr.write (message)
	else:
		win32ui.MessageBox (message, command_title (args), win32con.MB_OK | win32con.MB_ICONERROR)
	sys.exit (1)

def error_engine_path_not_found (args, engine_version):
	message= "CryEngine '%s' has not been registered locally.\n" % engine_version
	if args.silent:
		sys.stderr.write (message)
	else:
		win32ui.MessageBox (message, command_title (args), win32con.MB_OK | win32con.MB_ICONERROR)
	sys.exit (1)

def print_subprocess (cmd):
	print (' '.join (map (lambda a: '"%s"' % a, cmd)))

#---

def python3_path():
	program= 'python3.exe'
	path= shutil.which (program)
	if path:
		return path

	path= 'C:\\'
	folders= [name for name in os.listdir(path) if name.lower().startswith ('python3') and os.path.isdir (os.path.join (path, name))]
	if not folders:
		return program
	
	folders.sort()
	folders.reverse()
	return os.path.join (path, folders[0], program)

#--- UNINSTALL ---

def uninstall_integration():
	for SubKey in (os.path.join ('Software', 'Classes', 'CrySelect.engine'), os.path.join ('Software', 'Classes', 'CrySelect.project')):
		try:
			win32api.RegDeleteTree (win32con.HKEY_CURRENT_USER, SubKey)
		except pywintypes.error:
			pass
		try:
			win32api.RegDeleteTree (win32con.HKEY_LOCAL_MACHINE, SubKey)
		except pywintypes.error:
			pass

def cmd_uninstall (args):
	uninstall_integration()

	registry_path= cryregistry.path (cryregistry.ENGINE_FILENAME)
	if os.path.isfile (registry_path):
		os.remove (registry_path)

#--- INSTALL ---
#http://stackoverflow.com/questions/2123762/add-menu-item-to-windows-context-menu-only-for-specific-filetype
#http://sbirch.net/tidbits/context_menu.html
#http://code.activestate.com/recipes/286159-using-ctypes-to-manipulate-windows-registry-and-to/

def cmd_install (args):

	uninstall_integration()
	
	#---

	if getattr( sys, 'frozen', False ):
		ScriptPath= os.path.abspath (sys.executable)
		engine_commands= (
			('add', 'Register engine', '"%s" add "%%1"' % ScriptPath),
		)

		project_commands= (
			('edit', 'Edit game', '"%s" edit "%%1"' % ScriptPath),
			('open', 'Run game', '"%s" open "%%1"' % ScriptPath),
			('monodev', 'Edit C# code', '"%s" monodev "%%1"' % ScriptPath),
			('_build', 'Build solution', '"%s" build "%%1"' % ScriptPath),
			('_projgen', 'Generate solution', '"%s" projgen "%%1"' % ScriptPath),			
			('_switch', 'Switch engine version', '"%s" switch "%%1"' % ScriptPath),
		)
	else:
		ScriptPath= os.path.abspath (__file__)
		PythonPath= python3_path()
		engine_commands= (
			('add', 'Register engine', '"%s" "%s" add "%%1"' % (PythonPath, ScriptPath)),
		)
		
		project_commands= (
			('edit', 'Edit game', '"%s" "%s" edit "%%1"' % (PythonPath, ScriptPath)),
			('open', 'Run game', '"%s" "%s" open "%%1"' % (PythonPath, ScriptPath)),
			('monodev', 'Edit C# code', '"%s" monodev "%%1"' % ScriptPath),
			('_build', 'Build solution', '"%s" "%s" build "%%1"' % (PythonPath, ScriptPath)),
			('_projgen', 'Generate solution', '"%s" "%s" projgen "%%1"' % (PythonPath, ScriptPath)),			
			('_switch', 'Switch engine version', '"%s" "%s" switch "%%1"' % (PythonPath, ScriptPath)),
		)

	#---
	
	current_user_is_admin= shell.IsUserAnAdmin()
	key= False and win32con.HKEY_LOCAL_MACHINE or win32con.HKEY_CURRENT_USER
	hClassesRoot= win32api.RegOpenKeyEx (key, 'Software\\Classes')
	#https://msdn.microsoft.com/en-us/library/windows/desktop/cc144152(v=vs.85).aspx

	#--- .cryengine

	FriendlyTypeName= 'CryEngine version'
	ProgID= 'CrySelect.engine'
	AppUserModelID= 'CrySelect.engine'
	DefaultIcon= os.path.abspath (os.path.join (os.path.dirname(ScriptPath), 'editor_icon.ico'))

	hProgID= win32api.RegCreateKey (hClassesRoot, ProgID)
	win32api.RegSetValueEx (hProgID, None, None, win32con.REG_SZ, FriendlyTypeName)
	win32api.RegSetValueEx (hProgID, 'AppUserModelID', None, win32con.REG_SZ, AppUserModelID)
	win32api.RegSetValueEx (hProgID, 'FriendlyTypeName', None, win32con.REG_SZ, FriendlyTypeName)
	win32api.RegSetValue (hProgID, 'DefaultIcon', win32con.REG_SZ, DefaultIcon)

	#---

	hShell= win32api.RegCreateKey (hProgID, 'shell')
	win32api.RegCloseKey (hProgID)
		
	for action, title, command in engine_commands:
		hAction= win32api.RegCreateKey (hShell, action)
		win32api.RegSetValueEx (hAction, None, None, win32con.REG_SZ, title)
		win32api.RegSetValueEx (hAction, 'Icon', None, win32con.REG_SZ, DefaultIcon)
		win32api.RegSetValue (hAction, 'command', win32con.REG_SZ, command)
		win32api.RegCloseKey (hAction)
	
	action= 'add'
	win32api.RegSetValueEx (hShell, None, None, win32con.REG_SZ, action)
	win32api.RegCloseKey (hShell)
	
	#---
	
	hCryProj= win32api.RegCreateKey (hClassesRoot, cryregistry.ENGINE_EXTENSION)
	win32api.RegSetValueEx (hCryProj, None, None, win32con.REG_SZ, ProgID)
	win32api.RegCloseKey (hCryProj)
			
	#--- .cryproject
		
	FriendlyTypeName= 'CryEngine project'
	ProgID= 'CrySelect.project'
	AppUserModelID= 'CrySelect.project'
	DefaultIcon= os.path.abspath (os.path.join (os.path.dirname(ScriptPath), 'editor_icon16.ico'))
	
	hProgID= win32api.RegCreateKey (hClassesRoot, ProgID)
	win32api.RegSetValueEx (hProgID, None, None, win32con.REG_SZ, FriendlyTypeName)
	win32api.RegSetValueEx (hProgID, 'AppUserModelID', None, win32con.REG_SZ, AppUserModelID)
	win32api.RegSetValueEx (hProgID, 'FriendlyTypeName', None, win32con.REG_SZ, FriendlyTypeName)
	win32api.RegSetValue (hProgID, 'DefaultIcon', win32con.REG_SZ, DefaultIcon)

	#---	

	hShell= win32api.RegCreateKey (hProgID, 'shell')
	win32api.RegCloseKey (hProgID)
		
	for action, title, command in project_commands:
		hAction= win32api.RegCreateKey (hShell, action)
		win32api.RegSetValueEx (hAction, None, None, win32con.REG_SZ, title)
		win32api.RegSetValueEx (hAction, 'Icon', None, win32con.REG_SZ, DefaultIcon)
		win32api.RegSetValue (hAction, 'command', win32con.REG_SZ, command)
		win32api.RegCloseKey (hAction)
	
	action= 'edit'
	win32api.RegSetValueEx (hShell, None, None, win32con.REG_SZ, action)
	win32api.RegCloseKey (hShell)
	
	#---
	
	hCryProj= win32api.RegCreateKey (hClassesRoot, '.cryproject')
	win32api.RegSetValueEx (hCryProj, None, None, win32con.REG_SZ, ProgID)
	win32api.RegCloseKey (hCryProj)

#--- ADD ---

def add_engines (*engine_files):
	engine_registry= cryregistry.load_engines()
	
	added= []
	for engine_file in engine_files:
		if os.path.splitext (engine_file)[1] != cryregistry.ENGINE_EXTENSION:
			continue
		
		engine= cryproject.load (engine_file)
		info= engine['info']
		engine_id= info['id']
				
		engine_data= { 'uri': os.path.abspath (engine_file), 'info': info }		
		prev= engine_registry.setdefault (engine_id, engine_data)
		if prev is engine_data or prev != engine_data:
			engine_registry[engine_id]= engine_data
			added.append (engine_id)

	if added:
		cryregistry.save_engines (engine_registry)

def cmd_add (args):
	add_engines (*args.project_files)

#--- REMOVE ---

def cmd_remove (args):
	engine_registry= cryregistry.load_engines()

	removed= []
	for engine_file in args.project_files:
		engine= cryproject.load (engine_file)
		engine_id= engine['info']['id']
		if engine_id in engine_registry:
			del engine_registry[engine_id]
			removed.append (engine_id)
			
	if removed:
		cryregistry.save_engines (engine_registry)

#--- PURGE ---
		
def cmd_purge (args):
	db= registry_load()

	removed= []
	for k, v in db.items():
		try:
			manifest_isvalid (v['uri'])
		except:
			del db[k]
			removed.append (k)
	
	if removed:
		registry_save (db)
		
	exit (HTTP_OK, {'removed': removed})

#--- SWITCH ---

import uuid

import tkinter as tk
from tkinter import ttk
from tkinter import filedialog

def cryswitch_enginenames (engines):
	return list (map (lambda a: (a[1]), engines))

class CrySwitch(tk.Frame):

	def layout (self, root):
		root.title ("Switch CRYENGINE version")
		root.minsize(width=400, height=60)
		root.resizable(width=False, height=False)
		root.attributes('-toolwindow', True)
		
		#---
		
		top = tk.Frame(root)
		top.pack(side='top', fill='both', expand= True, padx= 5, pady= 5)
		
		self.open = tk.Button(top, text="...", width=3, command=self.command_browse)
		self.open.pack(side='right', padx= 2)

		self.combo = ttk.Combobox(top, values= cryswitch_enginenames (self.engine_list), state='readonly')
		self.combo.pack(fill='x', expand= True, side='right', padx= 2)

		#---

		bottom = tk.Frame(root)
		bottom.pack(side='bottom', fill='both', expand= True, padx= 5, pady= 5)

		self.cancel = tk.Button(bottom, text='Cancel', width=10, command=self.close)
		self.cancel.pack(side='right', padx= 2)

		self.ok = tk.Button(bottom, text='OK', width=10, command=self.command_ok)
		self.ok.pack(side='right', padx= 2)
		
	def close (self):
		self.root.destroy()
		
	def __init__(self, project_file, engine_list, found):
		root= tk.Tk()
		super().__init__(root)
				
		self.root= root
		self.project_file= project_file
		self.engine_list= engine_list
				
		self.layout (self.root)
		if self.engine_list:
			self.combo.current (found)		
		
	def command_ok(self):
		i= self.combo.current()
		(engine_id, name)= self.engine_list[i]
		if engine_id is None:
			engine_dirname= name
			
			listdir= os.listdir (engine_dirname)
			engine_files= list (filter (lambda filename: os.path.isfile (os.path.join (engine_dirname, filename)) and os.path.splitext(filename)[1] == cryregistry.ENGINE_EXTENSION, listdir))
			engine_files.sort()
			if not engine_files:
				engine_id= "{%s}" % uuid.uuid4()
				engine_path= os.path.join (engine_dirname, os.path.basename (engine_dirname) + cryregistry.ENGINE_EXTENSION)
				file= open (engine_path, 'w')
				file.write (json.dumps ({'info': {'id': engine_id}}, indent=4, sort_keys=True))
				file.close()				
			else:
				engine_path= os.path.join (engine_dirname, engine_files[0])
				engine= cryproject.load (engine_path)
				engine_id= engine['info']['id']
			
			add_engines (engine_path)
				
		project= cryproject.load (self.project_file)
		if cryproject.engine_id (project) != engine_id:
			project['require']['engine']= engine_id
			cryproject.save (project, self.project_file)

		self.close()
		

			
	def command_browse (self):
		#http://tkinter.unpythonic.net/wiki/tkFileDialog
		file= filedialog.askdirectory(mustexist=True)
		if file:
			self.engine_list.append ((None, os.path.abspath (file)))
			self.combo['values']= cryswitch_enginenames (self.engine_list)
			self.combo.current (len (self.values) - 1)		

def cmd_switch (args):
	title= 'Switch CRYENGINE version'
	if not os.path.isfile (args.project_file):
		error_project_not_found(args)
			
	project= cryproject.load (args.project_file)
	if project is None:
		error_project_json_decode (args)

	#---
	
	engine_list= []
	
	engines= cryregistry.load_engines()
	for (engine_id, engine_data) in engines.items():
		engine_file= engine_data['uri']
		
		info= engine_data.get ('info', {})
		version= info.get ('version', 0)
		name= info.get ('name', os.path.dirname (engine_file))
				
		engine_list.append ((version, name, engine_id))
		
	engine_list.sort()
	engine_list.reverse()
	engine_list= list (map (lambda a: (a[2], a[1]), engine_list))
	
	#---
	
	engine_id= cryproject.engine_id (project)

	found= 0
	for i in range (len (engine_list)):
		(id_, name)= engine_list[i]
		if id_ == engine_id:
			found= i
			break

	#---
	
	app = CrySwitch (args.project_file, engine_list, found)
	app.mainloop()

#--- UPGRADE ---

def cmd_upgrade (args):
	registry= cryregistry.load_engines()
	engine_path= cryregistry.engine_path (registry, args.engine_version)
	if engine_path is None:
		error_engine_path_not_found (args, args.engine_version)
	
	if getattr( sys, 'frozen', False ):
		subcmd= [
			os.path.join (engine_path, 'Tools', 'CryVersionSelector', 'cryrun.exe')
		]
	else:
		subcmd= [
			python3_path(),
			os.path.join (engine_path, 'Tools', 'CryVersionSelector', 'cryrun.py')
		]
	subcmd.extend (sys.argv[1:])

	print_subprocess (subcmd)
	sys.exit (subprocess.call(subcmd))

#--- RUN ----

def cmd_run (args):
	if not os.path.isfile (args.project_file):
		error_project_not_found (args)
	
	project= cryproject.load (args.project_file)
	if project is None:
		error_project_json_decode (args)
	
	engine_id= cryproject.engine_id(project)	
	engine_registry= cryregistry.load_engines()
	engine_path= cryregistry.engine_path (engine_registry, engine_id)
	if engine_path is None:
		error_engine_path_not_found (args, engine_id)
		
	#---

	if getattr( sys, 'frozen', False ):
		subcmd= [
			os.path.join (engine_path, 'Tools', 'CryVersionSelector', 'cryrun.exe')
		]
	else:
		subcmd= [
			python3_path(),
			os.path.join (engine_path, 'Tools', 'CryVersionSelector', 'cryrun.py')
		]

	subcmd.extend (sys.argv[1:])

	(temp_fd, temp_path)= tempfile.mkstemp(suffix='.out', prefix=args.command + '_', text=True)
	temp_file= os.fdopen(temp_fd, 'w')

	print_subprocess (subcmd)
	p= subprocess.Popen(subcmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
	for line in p.stdout:
		temp_file.write (line)
		sys.stdout.write (line)
	
	returncode= p.wait()
	temp_file.close()
	
	if not args.silent and returncode != 0:
		result= win32ui.MessageBox (p.stderr.read(), title, win32con.MB_OKCANCEL | win32con.MB_ICONERROR)
		if result == win32con.IDOK:
			subprocess.call(('notepad.exe', temp_path))
				
	os.remove (temp_path)
	sys.exit (returncode)

#--- MAIN ---

if __name__ == '__main__':
	parser= argparse.ArgumentParser()
	parser.add_argument ('--pause', action='store_true')
	parser.add_argument ('--silent', action='store_true')
	
	subparsers= parser.add_subparsers(dest='command')
	subparsers.required= True
		
	subparsers.add_parser ('install').set_defaults(func=cmd_install)
	subparsers.add_parser ('uninstall').set_defaults(func=cmd_uninstall)
		
	parser_add= subparsers.add_parser ('add')
	parser_add.add_argument ('project_files', nargs='+')
	parser_add.set_defaults(func=cmd_add)
	
	parser_remove= subparsers.add_parser ('remove')
	parser_remove.add_argument ('project_files', nargs='+')
	parser_remove.set_defaults(func=cmd_remove)
	
	subparsers.add_parser ('purge').set_defaults(func=cmd_purge)	
	
	parser_switch= subparsers.add_parser ('switch')
	parser_switch.add_argument ('project_file')
	parser_switch.set_defaults(func=cmd_switch)
	
	#---

	parser_upgrade= subparsers.add_parser ('upgrade')
	parser_upgrade.add_argument ('--engine_version', default='')
	parser_upgrade.add_argument ('project_file')
	#parser_upgrade.add_argument ('remainder', nargs=argparse.REMAINDER)
	parser_upgrade.set_defaults(func=cmd_upgrade)
	
	parser_projgen= subparsers.add_parser ('projgen')
	parser_projgen.add_argument ('project_file')
	parser_projgen.add_argument ('remainder', nargs=argparse.REMAINDER)
	parser_projgen.set_defaults(func=cmd_run)

	parser_build= subparsers.add_parser ('build')
	parser_build.add_argument ('project_file')
	parser_build.add_argument ('remainder', nargs=argparse.REMAINDER)
	parser_build.set_defaults(func=cmd_run)

	parser_edit= subparsers.add_parser ('edit')
	parser_edit.add_argument ('project_file')
	parser_edit.add_argument ('remainder', nargs=argparse.REMAINDER)
	parser_edit.set_defaults(func=cmd_run)

	parser_open= subparsers.add_parser ('open')
	parser_open.add_argument ('project_file')
	parser_open.add_argument ('remainder', nargs=argparse.REMAINDER)
	parser_open.set_defaults(func=cmd_run)

	parser_monodev= subparsers.add_parser ('monodev')
	parser_monodev.add_argument ('project_file')
	parser_monodev.add_argument ('remainder', nargs=argparse.REMAINDER)
	parser_monodev.set_defaults(func=cmd_run)

	#---
		
	args= parser.parse_args()
	args.func (args)
	if args.pause:
		input ('Press Enter to continue...')
