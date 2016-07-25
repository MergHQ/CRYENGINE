# To be used with PyInstaller
# Command line: pyinstaller pyinstaller_setup_dl_sdks_onefile.spec
a = Analysis(['dl_sdks_exe.py'],
             hiddenimports=['platform'],
             runtime_hooks=None)

# Remove specific binaries
a.binaries = a.binaries - TOC([
 ('sqlite3.dll', '', ''),
 ('_hashlib', '', ''),
 ('_sqlite3', '', '')])
 
# Remove clashing dependency duplicate 
for d in a.datas:
	if 'pyconfig' in d[0]: 
		a.datas.remove(d)
		break
				
pyz = PYZ(a.pure)
exe = EXE(pyz,
          a.scripts,
          a.binaries,
          a.zipfiles,
          a.datas,
          name='download_sdks.exe',
          debug=False,
          strip=None,
          upx=True,
          console=True,
)
