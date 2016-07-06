#!/usr/bin/env python3
"""
Script for freezing 'download_sdks.py'.
To freeze run::

    pyinstaller pyinstaller_setup_dl_sdks_onefile.spec
"""
import importlib.machinery
from urllib import request  # Workaround so it can be imported in download_sdks.py


if __name__ == '__main__':
    dl_sdks = importlib.machinery.SourceFileLoader('dl_sdks','download_sdks.py').load_module()
    dl_sdks.main()

