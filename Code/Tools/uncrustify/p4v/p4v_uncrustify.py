#! /usr/bin/env python3
import argparse
import os
import subprocess
import sys
import P4

CPP_FILE_ENDINGS = ['.cpp', '.cxx', '.inl', '.h', '.hpp']


def main():
    args, filelist = parse_arguments()

    p4 = P4.P4()
    p4.user = args.user
    p4.client = args.client
    p4.connect()

    workspace_root = p4.fetch_client()['Root']

    cmd = ['Code/Tools/uncrustify/uncrustify', '-l', 'CPP',
           '--config={}/Code/Tools/uncrustify/CryEngine_uncrustify.cfg'.format(workspace_root)]

    if args.apply:
        cmd.extend(['--replace', '--no-backup'])
    else:
        cmd.append('--check')

    if args.changelist:
        filelist = get_changelist_files(p4, args.changelist)

    files_to_process = filter_cpp_files(filelist)

    success = True

    if files_to_process:
        for file in files_to_process:
            try:
                subprocess.check_call(cmd + [file])
            except subprocess.CalledProcessError:
                success = False
    else:
        print('No files to process.')

    return 0 if success else 1


def get_changelist_files(p4, changelist):
    """
    From a changelist number, retrieve the local paths of all files in the changelist.

    :param p4: Perforce interface.
    :param changelist: Changelist number.
    :return: Local paths of all files in the changelist.
    """
    description = p4.run_describe(["-s", changelist])[0]

    if 'depotFile' not in description:
        print('This script does not edit shelved files.'.format(changelist))
        return None

    changelist_files = []
    # Retrieve local paths for all files in the changelist.
    for filename in description['depotFile']:
        changelist_files.append(p4.run_fstat(['-Op', filename + '#have'])[0]['path'])

    # Print some data for debugging purposes.
    return changelist_files


def filter_cpp_files(filelist):
    """
    Take the list of files and remove CryPhysics and non-C++ files.
    Uncrustify will attempt to format any file as C++ if we tell it to, which can ruin shaders, for instance.

    :return: List of C++ files that should be formatted.
    """
    to_style = []
    for filepath in filelist:
        # CryPhysics does not receive code formatting.
        if 'cryphysics' in filepath.lower():
            continue

        # Only try to format actual C++ files.
        if os.path.splitext(filepath)[1].lower() not in CPP_FILE_ENDINGS:
            continue

        to_style.append(filepath)
    return to_style


def parse_arguments():
    parser = argparse.ArgumentParser(description='Build Template.')
    parser.add_argument('--apply', action='store_true', default=False, help='Overwrite files with formatted ones.')
    parser.add_argument('--user', default='', help='Perforce username.')
    parser.add_argument('--client', default='', help='Perforce client.')
    parser.add_argument('--changelist', default='', help='Changelist containing files on which to run Uncrustify.')
    return parser.parse_known_args()


if __name__ == "__main__":
    sys.exit(main())
