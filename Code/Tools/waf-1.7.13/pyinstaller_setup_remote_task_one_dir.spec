# To be used with PyInstaller
# Command line: pyinstaller pyinstaller_setup_remote_task_one_dir.spec

a = Analysis(['cry_waf_remote_task'],
             hiddenimports=[
				'cPickle',
				'thread',
				'subprocess',
				'socket'
				],
             runtime_hooks=None,
			 excludes=[
			 'unittest',
			 'riscos',
			 'riscos',
			 'riscosenviron',
			 'riscospath',
			 'ssl'
			 ]
			 )

# [EXAMPLE]
# Manually remove entire packages		
#a.excludes = [x for x in a.pure if x[0].startswith("waflib")]

# [EXAMPLE]
# Add a single missing dll
#a.binaries = a.binaries + [
#  ('opencv_ffmpeg245_64.dll', 'C:\\Python27\\opencv_ffmpeg245_64.dll', 'BINARY')]	
	
# [EXAMPLE]
## Remove specific binaries
#a.binaries = a.binaries - TOC([
# ('sqlite3.dll', '', ''),
# ('_sqlite3', '', ''),
# ('_ssl', '', '')])
 
# Remove clashing dependency duplicate 
for d in a.datas:
	if 'pyconfig' in d[0]: 
		a.datas.remove(d)
		break
				
pyz = PYZ(a.pure)
exe = EXE(pyz,
          a.scripts,
          name= 'cry_waf_remote_task.exe',
          debug=False,
          strip=None,
          upx=True,
          console=True,
		  exclude_binaries=True,
		  icon='Icon.ico')
					
coll = COLLECT(exe,
               a.binaries,
               a.zipfiles,
               a.datas,
               strip=None,
               upx=True,
               name='cry_waf_remote_task')
