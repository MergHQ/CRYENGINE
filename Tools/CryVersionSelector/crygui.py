#!/usr/bin/env python3

import os
import sys

import tkinter as tk
from tkinter import ttk

import cryselect
import cryregistry


class CryWindowBase(tk.Frame):
    def __init__(self):
        root = tk.Tk()
        super().__init__(root)

        self.root = root
        self.layout(self.root)

    def center_window(self):
        self.root.update_idletasks()
        width = self.root.winfo_width()
        height = self.root.winfo_height()
        x = (self.root.winfo_screenwidth() // 2) - (width // 2)
        y = (self.root.winfo_screenheight() // 2) - (height // 2)
        self.root.geometry('+{}+{}'.format(x, y))

    def close(self):
        self.root.destroy()


class CryWindowPlugin(CryWindowBase):
    def __init__(self, plugin_file):
        super().__init__()
        self.plugin_file = plugin_file

    def layout(self, root):
        windowWidth = 400
        windowHeight = 60
        root.title("Register CRYENGINE plugin")
        root.minsize(width=windowWidth, height=windowHeight)
        root.resizable(width=False, height=False)
        self.center_window()

        iconfile = "editor_icon16.ico"
        if not hasattr(sys, "frozen"):
            iconfile = os.path.join(os.path.dirname(__file__), iconfile)
        else:
            iconfile = os.path.join(sys.prefix, iconfile)
        root.iconbitmap(iconfile)

        top = tk.Frame(root)
        top.pack(side='top', fill='both', expand=True, padx=5, pady=5)

        self.combo = ttk.Combobox(top, values=self.keys, state='readonly')
        self.combo.pack(fill='x', expand=True, side='right', padx=2)

        bottom = tk.Frame(root)
        bottom.pack(side='bottom', fill='both', expand=True, padx=5, pady=5)

        self.cancel = tk.Button(bottom, text='Cancel',
                                width=10, command=self.close)
        self.cancel.pack(side='right', padx=2)

        self.ok = tk.Button(bottom, text='OK', width=10,
                            command=self.command_ok)
        self.ok.pack(side='right', padx=2)


class CryWindowAddPlugin(CryWindowPlugin):
    def __init__(self, plugin_file):
        engine_registry = cryregistry.load_engines()
        self.installed_engines = {}
        for it in engine_registry:
            engine = engine_registry[it]
            engine_info = engine['info']
            if 'name' in engine_info:
                self.installed_engines[engine_info['name']] = engine
            else:
                self.installed_engines[engine['uri']] = engine

        self.keys = sorted(self.installed_engines.keys())

        super().__init__(plugin_file)
        self.root.title("Register CRYENGINE plugins")

    def command_ok(self):
        key = self.keys[self.combo.current()]
        engine = self.installed_engines[key]
        engine_id = engine.get('info', {}).get('id')
        self.close()
        return cryselect.add_plugins(engine_id, [self.plugin_file])


class CryWindowRemovePlugin(CryWindowPlugin):
    def __init__(self, plugin_file):
        engine_registry = cryregistry.load_engines()
        self.installed_engines = {}
        for it in engine_registry:
            engine = engine_registry[it]
            engine_info = engine['info']
            engine_plugins = engine.get('plugins', [])

            for it in engine_plugins:
                if engine_plugins[it].get('uri') == plugin_file:
                    if 'name' in engine_info:
                        self.installed_engines[engine_info.get(
                            'name')] = engine
                    else:
                        self.installed_engines[engine.get('uri')] = engine
                    break

        self.keys = sorted(self.installed_engines.keys())

        super().__init__(plugin_file)
        self.root.title("Unregister CRYENGINE plugin")

    def command_ok(self):
        key = self.keys[self.combo.current()]
        engine = self.installed_engines[key]
        engine_id = engine.get('info', {}).get('id')
        self.close()
        return cryselect.remove_plugins(engine_id, [self.plugin_file])
