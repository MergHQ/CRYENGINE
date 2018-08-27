import cPickle as pickle
import thread
import threading
import subprocess
import errno
import sys
import logging

import os
import socket

listening_ports = 512
command_end_keyword = "!end"

# Incredibuild Workaround:
#
# Xoreax Support ticket: #DMS-651-80979
#
# Description:
# Incredibuild has issues with the clang C compiler (clang.exe). The C++ compiler (clang++.exe ) works fine.
# This issue has been observed when compiling with the Android NDK r16b clang compiler toolchain.
# The same command line executes correctly if build locally without IB.
#
# Workaround:
# Replace all "\\" in the cmd line with "/"
#
# Example:
# Cmd (simplified) :  clang.exe -Ic:\\Foo c:\\Project\my_c_file.c -c -o c:\\Project\my_object_file.o
# Error produced   :  clang.exe : error : no such file or directory: 'c:Projectmy_c_file.c'
use_incredibuild_workaround = True

class Local_CommandCoordinator(object):
		
	def execute_command(self, cmd, **kw):

		# Replace all "\\" with "/" in command line	
		if use_incredibuild_workaround:
			if isinstance(cmd,list):				
				cmd[:] = [w.replace('\\\\', '/').replace('\\', '/') for w in cmd] #inplace so on error we get the same string
			else:
				cmd = cmd.replace('\\\\', '/').replace('\\', '/') # strings are imutable so we need to create a temp string

 		try:
			if kw['stdout'] or kw['stderr']:
				p = subprocess.Popen(cmd, **kw)
				(out, err) = p.communicate() # [blocking]
				ret = p.returncode
			else:
				out, err = (None, None)
				ret = subprocess.Popen(cmd, **kw).wait()
		except Exception as e:
			print '[ERROR] Execution failure: %s (%s)' % (str(e), cmd)
			raise Errors.WafError('Execution failure: %s (%s)' % (str(e), cmd), ex=e)
		
		return (ret, out, err)

def handle_connection_thread(client_connection):
	socket_data_str = ''

	while True:
		# Receive data 
		data = client_connection.recv(4096) # [blocking]
		socket_data_str += data
		# Check for end command token
		if socket_data_str.endswith(command_end_keyword):
			socket_data_str = socket_data_str[:-len(command_end_keyword)]
			break

	# Load and execute command
	(cmd, kw) = pickle.loads(socket_data_str)
	
	command_coordinator = Local_CommandCoordinator()
	(ret, out, err) = command_coordinator.execute_command(cmd, **kw)

	# Remove IB specific message footer 
	# e.g. {{xgTaskID=00000008}}
	if out:
		out_as_lines = out.splitlines()[:-1]
		out = '\r\n'.join(out_as_lines)	
	
	if out:
		# Cleanup show includes when printing to IB Monitor output
		if '/showIncludes' in cmd:
			INCLUDE_PATTERN = 'Note: including file:'
			out_filtered = []		
			for line in out_as_lines:
				if not line.startswith(INCLUDE_PATTERN):
					out_filtered.append(line)
			out_as_lines = out_filtered	
			
	out_local = '\r\n'.join(out_as_lines)	
	if out_local:
		logging.error(out_local)
	if err:
		logging.error(err)
	
	# Sent executed command reponse
	pickle_str = pickle.dumps((ret, out, err))
	client_connection.sendall(pickle_str)
		
	# Close connection
	client_connection.close()
		
class IB_CommandCoordinator_Server(object):
	def enter_command_loop(self):
		logging.basicConfig(level=logging.INFO, format='%(message)s')
		
		socket_port = 0
		for arg in sys.argv[1:]:
			if arg.startswith('--use_socket='):
				socket_port = int(arg[len('--use_socket='):])
				
		logging.info("[INFO] Start Command Server\n")
		
		serversocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		serversocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR,1)
		serversocket.bind(('localhost', socket_port))
		serversocket.listen(listening_ports) 
		logging.info( "[INFO] Command-Server connected to port %s\n" % serversocket.getsockname()[1])

		while True:
			# Wait for connection
			client_connection, client_address = serversocket.accept() # [blocking]
			thread.start_new_thread(handle_connection_thread, (client_connection,))
		
		serversocket.close()
		logging.info("[INFO] Shutdown Command Server\n")
			
			
class IB_CommandCoordinator_Client(Local_CommandCoordinator):

	def __init__(self, max_num_local_none_ib_tasks):	
		
		self.ib_remote_executables = {
		'cl.exe': True,
		'clang.exe': True,
		'clang++.exe': True,
		'gcc.exe': True,
		'gcc++.exe': True,
		}
		
		self.local_build_semaphore = threading.Semaphore(max_num_local_none_ib_tasks)
		self.max_num_local_none_ib_tasks = 0

		# Create a socket and close it immediatly
		# This will reserve the socket port for some timout period. 
		# In the meantime the server is able to SO_REUSEADDR the pocket.
		# This way we can communicate an unused socket port to the external server.
		clientsocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		clientsocket.bind(('localhost', 0))		
		self.socket_port = clientsocket.getsockname()[1]
		clientsocket.close()
		
		print "[INFO] Command-Client reserved port %s" % self.socket_port
			
	def get_active_socket(self):
		return self.socket_port
		
	def execute_command(self, cmd, **kw):	
	
		# Always try as many tasks locally as we can
		run_task_local = self.local_build_semaphore.acquire(False)
			
		# Check if we can run this task remote [not blocking]
		if not run_task_local:
			run_task_local = not self.ib_remote_executables.get(os.path.basename(cmd[0]), False)
			
			# If not remote, acquire semaphore [blocking]
			if run_task_local:
				self.local_build_semaphore.acquire()
				
		# Commands that can be executed remotly
		if run_task_local: 
			(ret, out, err) = super(IB_CommandCoordinator_Client, self).execute_command(cmd, **kw) #[LOCAL]
			self.local_build_semaphore.release()	# Release semaphore			
		else:
			(ret, out, err) = self.submit_remote_command(cmd, **kw) #[REMOTE]

		return (ret, out, err)		
		
	def connect_to_server(self):
		while True:
			try:
				clientsocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
				clientsocket.connect(('localhost', self.get_active_socket()))
				break
			except:
				continue
		
		return clientsocket
		
	def submit_remote_command(self, cmd, **kw):
		# Try to connect
		clientsocket = self.connect_to_server()

		# Send command to server
		cmd_tuple = (cmd, kw)
		pickle_string = pickle.dumps(cmd_tuple)
		clientsocket.sendall(pickle_string + command_end_keyword)
				
		# Wait for server response
		answer_str = ''
		while True:
			try:		
				data = clientsocket.recv(4096) # [blocking]
			except socket.error as error:
				if error.errno == errno.WSAECONNRESET:
					clientsocket = self.connect_to_server()
					continue
			if not data:  # check for closed connection
				break			
			answer_str += data
		clientsocket.close()
		
		# return response
		return pickle.loads(answer_str)	