#!/usr/bin/env python
# encoding: utf-8
# Brant Young, 2007
# Christopher Bolte: Created a copy to easier adjustment to crytek specific changes

"Process *.rc* files for C/C++: X{.rc -> [.res|.rc.o]}"

import re, traceback
from waflib import Task, Logs, Utils
from waflib.TaskGen import extension,feature, before_method
from waflib.Tools import c_preproc
from waflib.Configure import conf
import os

@extension('.rc')
def rc_file(self, node):
	"""
	Bind the .rc extension to a winrc task
	"""
	platform = self.bld.env['PLATFORM']
	if platform != 'win_x86' and platform != 'win_x64':	
		return	
	
	obj_ext = '.' + str(self.idx) + '.rc.o'
	if self.env['WINRC_TGT_F'] == '/fo':
		obj_ext = '.' + str(self.idx) +'.res'
	
	rctask = self.create_task('winrc', node, node.change_ext(obj_ext))
		
	#print rctask.env['INCLUDES']
	rctask.env = rctask.env.derive()	
	rctask.env.detach()
		
	if hasattr(self, 'winres_includes'):		
		if isinstance(self.winres_includes, list):
			rctask.env['INCLUDES'] += self.winres_includes
		else:
			rctask.env['INCLUDES'] += [ self.winres_includes ]
	if hasattr(self, 'winres_defines'):
		if isinstance(self.winres_defines, list):
			rctask.env['DEFINES'] += self.winres_defines
		else:
			rctask.env['DEFINES'] += [ self.winres_defines ]

	for path in self.includes:
		if os.path.isabs(path):
			rctask.env['INCLUDES'] += [ path ]
		else:
			path_node = self.path.make_node(path)
			rctask.env['INCLUDES'] += [ path_node.abspath() ]

	try:
		self.compiled_tasks.append(rctask)
	except AttributeError:
		self.compiled_tasks = [rctask]

re_lines = re.compile(
	'(?:^[ \t]*(#|%:)[ \t]*(ifdef|ifndef|if|else|elif|endif|include|import|define|undef|pragma)[ \t]*(.*?)\s*$)|'\
	'(?:^\w+[ \t]*(ICON|BITMAP|CURSOR|HTML|FONT|MESSAGETABLE|TYPELIB|REGISTRY|D3DFX)[ \t]*(.*?)\s*$)',
	re.IGNORECASE | re.MULTILINE)

class rc_parser(c_preproc.c_parser):
	def filter_comments(self, filepath):
		code = Utils.readf(filepath)
		if c_preproc.use_trigraphs:
			for (a, b) in c_preproc.trig_def: code = code.split(a).join(b)
		code = c_preproc.re_nl.sub('', code)
		code = c_preproc.re_cpp.sub(c_preproc.repl, code)
		ret = []
		for m in re.finditer(re_lines, code):
			if m.group(2):
				ret.append((m.group(2), m.group(3)))
			else:
				ret.append(('include', m.group(5)))
		return ret

	def addlines(self, node):
		self.currentnode_stack.append(node.parent)
		filepath = node.abspath()

		self.count_files += 1
		if self.count_files > c_preproc.recursion_limit:
			raise c_preproc.PreprocError("recursion limit exceeded")
		pc = self.parse_cache
		Logs.debug('preproc: reading file %r', filepath)
		try:
			lns = pc[filepath]
		except KeyError:
			pass
		else:
			self.lines.extend(lns)
			return

		try:
			lines = self.filter_comments(filepath)
			lines.append((c_preproc.POPFILE, ''))
			lines.reverse()
			pc[filepath] = lines
			self.lines.extend(lines)
		except IOError:
			raise c_preproc.PreprocError("could not read the file %s" % filepath)
		except Exception:
			if Logs.verbose > 0:
				Logs.error("parsing %s failed" % filepath)
				traceback.print_exc()

class winrc(Task.Task):
	"""
	Task for compiling resource files
	"""
	run_str = '${WINRC} ${WINRCFLAGS} ${CPPPATH_ST:INCLUDES} ${DEFINES_ST:DEFINES} ${WINRC_TGT_F} ${TGT} ${WINRC_SRC_F} ${SRC}'
	color   = 'BLUE'

	def scan(self):
		tmp = rc_parser(self.generator.includes_nodes)
		tmp.start(self.inputs[0], self.env)
		nodes = tmp.nodes
		names = tmp.names

		if Logs.verbose:
			Logs.debug('deps: deps for %s: %r; unresolved %r' % (str(self), nodes, names))

		return (nodes, names)

@conf
def load_rc_tool(conf):
	"""
	Detect the programs RC or windres, depending on the C/C++ compiler in use
	"""
	v = conf.env	
	
	if not v['WINRC']:
		conf.fatal('Error: Resource compiler "WINRC" path has not been set.')

	v['WINRC_TGT_F'] = '/fo'
	v['WINRC_SRC_F'] = ''

	v['WINRCFLAGS'] = [
		'/l0x0409',	# Set default language
		'/nologo'	# Hide Logo
	]

RC_FILE_TEMPLATE='''// Microsoft Visual C++ generated resource script.
//
#include "resource.h"

#define APSTUDIO_READONLY_SYMBOLS
/////////////////////////////////////////////////////////////////////////////
//
// Generated from the TEXTINCLUDE 2 resource.
//
#include "winres.h"

/////////////////////////////////////////////////////////////////////////////
#undef APSTUDIO_READONLY_SYMBOLS

${if getattr(project, 'has_cursors', False)}
/////////////////////////////////////////////////////////////////////////////
// Neutral resources

#if !defined(AFX_RESOURCE_DLL) || defined(AFX_TARG_NEU)
LANGUAGE LANG_NEUTRAL, SUBLANG_NEUTRAL
#pragma code_page(1252)

/////////////////////////////////////////////////////////////////////////////
//
// Cursor
//

"${project.cursor_resource_name}"   CURSOR                  "${project.cursor_name}"

#endif    // Neutral resources
/////////////////////////////////////////////////////////////////////////////
${endif}
		
/////////////////////////////////////////////////////////////////////////////
// Russian resources

#if !defined(AFX_RESOURCE_DLL) || defined(AFX_TARG_ENU)
LANGUAGE LANG_ENGLISH, SUBLANG_ENGLISH_US
#pragma code_page(1252)

#ifdef APSTUDIO_INVOKED
/////////////////////////////////////////////////////////////////////////////
//
// TEXTINCLUDE
//

1 TEXTINCLUDE 
BEGIN
    "resource.h\\0"
END

2 TEXTINCLUDE 
BEGIN
    "#include ""winres.h""\\r\\n"
    "\\0"
END

3 TEXTINCLUDE 
BEGIN
    "\\r\\n"
    "\\0"
END

#endif    // APSTUDIO_INVOKED

${if hasattr(project, 'has_icons')}
// Icon with lowest ID value placed first to ensure application icon
// remains consistent on all systems.
${project.icon_resource_name}                ICON                    "${project.icon_name}"
${endif}
#endif    // English (United States) resources
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// German (Germany) resources

#if !defined(AFX_RESOURCE_DLL) || defined(AFX_TARG_DEU)
LANGUAGE LANG_GERMAN, SUBLANG_GERMAN
#pragma code_page(1252)

/////////////////////////////////////////////////////////////////////////////
//
// Version
//

VS_VERSION_INFO VERSIONINFO
 FILEVERSION ${project.file_version}
 PRODUCTVERSION ${project.product_version}
 FILEFLAGSMASK 0x17L
#ifdef _DEBUG
 FILEFLAGS 0x1L
#else
 FILEFLAGS 0x0L
#endif
 FILEOS 0x4L
 FILETYPE 0x2L
 FILESUBTYPE 0x0L
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "000904b0"
        BEGIN
            VALUE "CompanyName", "${project.company_name}"
            VALUE "FileVersion", "${project.file_version}"
            VALUE "LegalCopyright", "${project.copyright}"
            VALUE "ProductName", "${project.product_name}"
            VALUE "ProductVersion", "${project.product_version}"
        END
    END
    BLOCK "VarFileInfo"
    BEGIN
        VALUE "Translation", 0x9, 1200
    END
END

#endif    // German (Germany) resources
/////////////////////////////////////////////////////////////////////////////



#ifndef APSTUDIO_INVOKED
/////////////////////////////////////////////////////////////////////////////
//
// Generated from the TEXTINCLUDE 3 resource.
//


/////////////////////////////////////////////////////////////////////////////
#endif    // not APSTUDIO_INVOKED
'''

COMPILE_TEMPLATE = '''def f(project):
	lst = []
	def xml_escape(value):
		return value.replace("&", "&amp;").replace('"', "&quot;").replace("'", "&apos;").replace("<", "&lt;").replace(">", "&gt;")

	%s

	#f = open('cmd.txt', 'w')
	#f.write(str(lst))
	#f.close()
	return ''.join(lst)
'''

reg_act = re.compile(r"(?P<backslash>\\)|(?P<dollar>\$\$)|(?P<subst>\$\{(?P<code>[^}]*?)\})", re.M)
def compile_template(line):
	"""
	Compile a template expression into a python function (like jsps, but way shorter)
	"""
	extr = []
	def repl(match):
		g = match.group
		if g('dollar'): return "$"
		elif g('backslash'):
			return "\\"
		elif g('subst'):
			extr.append(g('code'))
			return "<<|@|>>"
		return None
	
	line2 = reg_act.sub(repl, line)
	params = line2.split('<<|@|>>')
	assert(extr)	

	indent = 0
	buf = []
	app = buf.append

	def app(txt):		
		buf.append(indent * '\t' + txt)

	for x in range(len(extr)):
		if params[x]:
			app("lst.append(%r)" % params[x])

		f = extr[x]
		if f.startswith('if') or f.startswith('for'):
			app(f + ':')
			indent += 1
		elif f.startswith('py:'):
			app(f[3:])
		elif f.startswith('endif') or f.startswith('endfor'):
			indent -= 1
		elif f.startswith('else') or f.startswith('elif'):
			indent -= 1
			app(f + ':')
			indent += 1
		elif f.startswith('xml:'):
			app('lst.append(xml_escape(%s))' % f[4:])
		else:
			#app('lst.append((%s) or "cannot find %s")' % (f, f))
			app('lst.append(%s)' % f)

	if extr:
		if params[-1]:
			app("lst.append(%r)" % params[-1])
		
	fun = COMPILE_TEMPLATE % "\n\t".join(buf)
	#print(fun)
	return Task.funex(fun)

class create_rc_file(Task.Task):
	color = 'YELLOW'
	
	def generate_file_content(self):
		tgen = self.generator
		bld = tgen.bld		
		project_name = tgen.project_name if hasattr(tgen, 'project_name') else tgen.target
		
		self.product_version = bld.get_product_version()
		self.file_version = bld.get_product_version()
		self.file_version = self.file_version.replace('.',',') # Everywhere a ',' is accept except for file version
		self.company_name = bld.get_company_name()
		self.copyright = bld.get_copyright()
		self.product_name = bld.get_product_name(tgen.target, project_name)

		template = compile_template(RC_FILE_TEMPLATE)	
		
		# assing to outself
		self.file_content = template(self)
		
	def run(self):	
		if not hasattr(self, 'file_content'):
			self.generate_file_content()
		self.outputs[0].write(self.file_content)	
		
	def runnable_status(self):						
		if super(create_rc_file, self).runnable_status() == Task.ASK_LATER:
			return Task.ASK_LATER
					
		if not hasattr(self, 'file_content'):
			self.generate_file_content()
		
		tgt = self.outputs[0].abspath()

		# If there any target file is missing, we have to copy
		try:
			stat_tgt = os.stat(tgt)
		except OSError:
			return Task.RUN_ME
		
		# Now compare both file stats
		for input in self.inputs:
			src = input.abspath()
			try:
				stat_src = os.stat(src)				
			except OSError:
				pass
			else:
				# same size and identical timestamps
				if stat_src.st_mtime >= stat_tgt.st_mtime + 2:			
					return Task.RUN_ME
		
		if self.outputs[0].read() != self.file_content:
			return Task.RUN_ME		
			
		# Everything fine, we can skip this task		
		return Task.SKIP_ME
		
@feature('generate_rc_file')
@before_method('process_source')
def generate_rc_file(self):
	platform = self.bld.env['PLATFORM']
	if platform != 'win_x86' and platform != 'win_x64':	
		return	

	# Generate Tasks to create rc file as well as copy the resources
	rc_task_inputs = []
	
	# Compute output folder for RC Files
	rc_file_folder = self.bld.get_bintemp_folder_node().make_node('rc_files')
	if hasattr(self, 'project_name'):
		rc_file_folder = rc_file_folder.make_node(self.project_name)
		
	rc_file = rc_file_folder.make_node(self.target + '.auto_gen.rc')
	rc_file_folder.mkdir()
			
	def _copy_rc_resource(file_name):
		source_file = self.resource_path.make_node(file_name)
		target_file = rc_file_folder.make_node(file_name)
		self.create_task( 'copy_outputs', source_file, target_file )
		rc_task_inputs.append( target_file )

	rc_task = self.create_task( 'create_rc_file', rc_task_inputs, rc_file )
	
	# Set special flags for launcher
	if self.target.startswith('WindowsLauncher'):			
		rc_task.has_icons = True
		rc_task.icon_name = 'WindowsIcon.ico'
		rc_task.icon_resource_name = 'IDI_ICON' # has to match define in launchers resource.h file e.g. "#define IDI_ICON	101"

		#[deprecated] Load cursor as resource (now loaded through .pak file)
		#rc_task.has_cursors = True
		#rc_task.cusor_name = 'Cursor_Green.cur'
		#rc_task.cursor_resource_name = 'IDC_CUSOR' # has to match define in launchers resource.h file e.g. "#define IDC_CUSOR 105"
					
	# Set special flags for launcher
	if self.target.startswith('DedicatedLauncher'):
		rc_task.has_icons = True
		rc_task.icon_name = 'WindowsServerIcon.ico'
		rc_task.icon_resource_name = 'IDI_ICON'
		
	# Copy resources
	resources =  getattr(rc_task, 'icon_name', "") 
	resources += getattr(rc_task, 'cusor_name', "")
	for file in  resources.split():
		_copy_rc_resource(file)			

	# Create rc compile task
	self.rc_file(rc_file)
	
	
