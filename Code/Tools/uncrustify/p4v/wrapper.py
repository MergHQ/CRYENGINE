#!/usr/bin/env python3
"""
Script for freezing 'p4v_uncrustify.py'.
To freeze run::
    pyinstaller pyinstaller_onefile.spec
"""
import importlib.machinery

if __name__ == '__main__':
    script = importlib.machinery.SourceFileLoader('p4v_uncrustify', 'p4v_uncrustify.py').load_module()
    script.main()
