import os
import sys
import shutil
import distutils
import argparse
from distutils import dir_util

import errno


def copy_files(src, dst):
    try:
        shutil.copytree(src, dst)
    except OSError as exc: 
        # Target already exists
        if exc.errno == errno.EEXIST:
            print('Target directory already exists...')
            distutils.dir_util.copy_tree(src, dst)
        # Source does not exist
        elif exc.errno == errno.ENOENT:
            print('Source directory does not exist')
            distutils.dir_util.copy_tree(src, dst)
        else:
            print('************************************************')
            print(os.strerror(exc.errno))
            raise


def copy_wwise_files(source_dir):
    target_path = os.path.join('Code', 'SDKs', 'Audio', 'AK')
    copy_files(os.path.join(source_dir, 'SDK', 'include', 'AK'), target_path)

    # CRYENGINE expects the structure to be a little different, so create a 'lib' directory to house everything
    target_lib_dir = os.path.join(target_path, 'lib')
    
    if not os.path.exists(target_lib_dir):
        os.mkdir(target_lib_dir)

    print('************************************************')
    print('***           Copying for Windows            ***')
    print('************************************************')
    
    # the various build, platform and compilation modes
    ak_platforms = ['x64', 'Win32']
    vs_versions = ['vc110', 'vc120', 'vc140']
    compilemodes = ['Debug', 'Profile', 'Release']

    for platform in ak_platforms:
        for vsVer in vs_versions:
            for cm in compilemodes:
                dest_dir = os.path.join(target_lib_dir, platform.lower(), cm.lower())

                # copy the libraries
                lib_source_path = os.path.join(source_dir, 'SDK', platform + '_' + vsVer, cm, 'lib')

                if os.path.exists(lib_source_path):
                    finalpath = os.path.join(dest_dir, vsVer.lower())
                    print("Copying to: ", finalpath)
                    copy_files(lib_source_path, finalpath)
                else:
                    print('Could not find source folder: ', lib_source_path)
    
    print('************************************************')
    print('***           Copying for Linux              ***')
    print('************************************************')
    
    foldermappings = [('Linux_x64', 'linux')]
    
    for ak_dir, cry_dir in foldermappings:
        for cm in compilemodes:
            dest_dir = os.path.join(target_lib_dir, cry_dir.lower(), 'x64', cm.lower())

            # copy the libraries
            lib_source_path = os.path.join(source_dir, 'SDK', ak_dir, cm, 'lib')
            
            if os.path.exists(lib_source_path):
                print("Copying to: ", dest_dir)
                copy_files(lib_source_path, dest_dir)
            else:
                print('Could not find source folder: ', lib_source_path)
                
    print('\nFinished copying the Wwise SDK files.\n')


parser = argparse.ArgumentParser(description='Copy Wwise SDK')
parser.add_argument('wwisedir')
parser.add_argument('scriptdir', nargs='?', default=os.getcwd())
args = parser.parse_args()

wwise_dir = args.wwisedir
script_dir = args.scriptdir

for quotemark in ["'", '"']:
    if script_dir.startswith(quotemark) and script_dir.endswith(quotemark):
        script_dir = script_dir[1:-1]

for quotemark in ["'", '"']:
    if wwise_dir.startswith(quotemark) and wwise_dir.endswith(quotemark):
        wwise_dir = wwise_dir[1:-1]

os.chdir(os.path.join(script_dir, '..', '..'))

copy_wwise_files(wwise_dir)
