#!/usr/bin/env python3

import sys
import os.path

has_tk = True
try:
    import tkinter as tk
    from tkinter import ttk
except ImportError:
    print("Skipping importing tkinter, because it's not installed.")
    has_tk = False

#--- const

CONFIGS = [
    {
        'title':'Visual Studio 2015 Win64',
        'cmake_toolchain': 'toolchain\windows\WindowsPC-MSVC.cmake',
        'cmake_generator': 'Visual Studio 14 2015 Win64',
        'cmake_builddir': 'solutions/win64',
    },
    {
        'title':'Visual Studio 2015 Win32',
        'cmake_toolchain': 'toolchain\windows\WindowsPC-MSVC.cmake',
        'cmake_generator': 'Visual Studio 14 2015',
        'cmake_builddir': 'solutions/win32',
    },

#Visual Studio 15 2017
    {
        'title':'Visual Studio 2017 Win64',
        'cmake_toolchain': 'toolchain\windows\WindowsPC-MSVC.cmake',
        'cmake_generator': 'Visual Studio 15 2017 Win64',
        'cmake_builddir': 'solutions/win64',
    },
    {
        'title':'Visual Studio 2017 Win32',
        'cmake_toolchain': 'toolchain\windows\WindowsPC-MSVC.cmake',
        'cmake_generator': 'Visual Studio 15 2017',
        'cmake_builddir': 'solutions/win32',
    }
]

def select_config():
    """
    Opens a GUI in which the user can select a CMake configuration.
    Returns the selected config, or None if no selection was made.
    """
    
    if not has_tk:
        return None

    iconfile = "editor_icon16.ico"
    if not hasattr(sys, "frozen"):
        iconfile = os.path.join(os.path.dirname(__file__), iconfile)
    else:
        iconfile = os.path.join(sys.prefix, iconfile)

    root = tk.Tk()
    root.iconbitmap(iconfile)
    app = CryProjgen(master=root)
    app.mainloop()
    return app.selected_config

if has_tk:
    class CryProjgen(tk.Frame):
        selected_config = None

        def __init__(self, master=None):
            super().__init__(master)
            self.parent = master
            self.parent.title("CRYENGINE CMake Project Generator")
            self.parent.minsize(300,100);
            self.pack()
            self.create_widgets()

        def create_widgets(self):

            tk.Label(self, text="Generate Configuration: ").pack()

            self.newselection = ''
            self.box_value = tk.StringVar()
            self.configs_box = ttk.Combobox(self, textvariable=self.box_value,width=40)

            config_list = []
            for config in CONFIGS:
                config_list.append(config['title'])
            self.configs_box['values'] = config_list

            self.configs_box.current(0)
            self.configs_box.pack()

            self.generate = tk.Button(self)
            self.generate["text"] = "Generate Solution"
            self.generate["command"] = self.generate_cmd
            self.generate.pack()

        def generate_cmd(self):
            current = self.configs_box.current()
            self.selected_config = CONFIGS[current]
            self.parent.destroy()

        def combo(self):
            self.newselection = CONFIGS[0]
            self.box_value = tk.StringVar()
            self.box = ttk.Combobox(self, textvariable=self.box_value)
            self.box['values'] = CONFIGS
            self.box.current(0)
            self.box.bind("<<ComboboxSelected>>", self.newselection)
            self.box.grid(column=0, row=0)