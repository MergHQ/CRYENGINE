#!/usr/bin/env python
# encoding: utf-8
# Philipp Bender, 2012
# Matt Clarkson, 2012

from waflib.Task import *
from waflib.TaskGen import extension
from waflib.TaskGen import feature,after_method,before_method
from waflib import Logs
import os
import sys

"""
A simple tool to integrate protocol buffers into your build system.

    def configure(conf):
        conf.load('compiler_cxx cxx protoc')

    def build(bld):
        bld(
                features = 'cxx cxxprogram'
                source   = 'main.cpp file1.proto proto/file2.proto',
                includes = '. proto',
                target   = 'executable')

"""

is_host_linux = 'linux' in sys.platform
proto_plugins_rpath = 'Tools/crycloud/protobuf/plugins/' + ('linux_x64' if is_host_linux else 'win_x64')
cpp_plugin_file = '/protobuf_cppplugin' if is_host_linux else '/protobuf_cppplugin.exe'

class protoc(Task):
	color   = 'BLUE'
	ext_out = ['.h', 'pb.cc']
	after = []

	def run(self):
		Task.run(self)
		bld = self.generator.bld
		key = self.uid()
		self.env['SRC'] = self.inputs[0].abspath()
		run_str =  '{PROTOC} -I{PROTOC_IMPORT_PATH} {PROTOC_FLAGS} {PROTOCPLUGIN_FLAGS} {PROTOCPLUGIN} {SRC}'
		self.exec_command(run_str.format(**self.env), cwd=getattr(self, 'cwd', None), env=self.env.env or None)

	# make the plugin an implicit dependency
	def scan(self):
		plugin_node = self.generator.bld.path.find_resource(proto_plugins_rpath + cpp_plugin_file)
		return ([plugin_node], [])

	def runnable_status(self):
		"""
		Override :py:meth:`waflib.Task.TaskBase.runnable_status` to determine if the task is ready
		to be run (:py:attr:`waflib.Task.Task.run_after`)
		"""
		#return 0 # benchmarking

		for t in self.run_after:
			if not t.hasrun:
				return ASK_LATER

		bld = self.generator.bld

		# first compute the signature
		try:
			new_sig = self.signature()
		except Errors.TaskNotReady:
			return ASK_LATER

		# compare the signature to a signature computed previously
		key = self.uid()
		try:
			prev_sig = bld.task_sigs[key]
		except KeyError:
			Logs.info("task: task %r must run as it was never run before or the task code changed" % self)
			return RUN_ME

		# This logic is different from Task.runnable_status.
		# In Task it compares all node.sig in outputs and new_sig. If they are different, it will return RUN_ME to run this task.
		# Here we only need to check whether the raw proto files are changed or the target .cc and .h files are deleted. The protoc will generate new .cc and .h files
		for node in self.outputs:
			if not os.path.isfile(node.abspath()):
				return RUN_ME

		if new_sig != prev_sig:
			return RUN_ME
		return SKIP_ME


@extension('.proto')
def process_protoc(self, node):
	proto_compiler_folder = self.bld.CreateRootRelativePath('Code/SDKs/protobuf')
	proto_plugin_folder = self.bld.CreateRootRelativePath(proto_plugins_rpath)
	protoc_file = '/bin/linux/protoc' if is_host_linux else '/bin/win_x86/protoc.exe'

	generated_node = self.path.make_node('GeneratedFiles')
	generated_node.mkdir()
	cpp_node = generated_node.make_node(node.change_ext('.pb.cc').name)
	hpp_node = generated_node.make_node(node.change_ext('.pb.h').name)
	task = self.create_task('protoc', node, [cpp_node, hpp_node])
	if self.bld.cmd == "msvs":
		self.source.extend(task.outputs)

	cpptask = self.create_compiled_task('cxx', cpp_node)
	cpptask.disable_pch = True

	if hasattr(self,'protobuf_import_path') and len(self.protobuf_import_path)>0:
		list = [os.path.normpath(os.path.join(self.path.abspath(), s)) for s in self.protobuf_import_path]
		import_path = (':' if is_host_linux else ';').join(list)
	else:
		import_path = node.parent.abspath()

	self.env.PROTOC_IMPORT_PATH = import_path
	self.env.PROTOC_FLAGS = '--cpp_out=%s' % generated_node.abspath()
	self.env.PROTOCPLUGIN_FLAGS = '--cry_out=%s' % generated_node.abspath()
	self.env.PROTOC = proto_compiler_folder + protoc_file
	self.env.PROTOCPLUGIN = '--plugin=protoc-gen-cry=%s' % (os.path.normpath(proto_plugin_folder + cpp_plugin_file))

	use = getattr(self, 'use', '')
	if not 'PROTOBUF' in use:
		self.use = self.to_list(use) + ['PROTOBUF']


def configure(conf):
	v = conf.env
	proto_compiler_folder = conf.CreateRootRelativePath('Code/SDKs/protobuf')
	if 'linux' in sys.platform:
		v['PROTOC'] = proto_compiler_folder + '/bin/linux/protoc'
	else:
		v['PROTOC'] = proto_compiler_folder + '/bin/win_x86/protoc.exe'
	#conf.check_cfg(package="protobuf", uselib_store="PROTOBUF", args=['--cflags', '--libs'])
	conf.find_program('protoc', var='PROTOC')
