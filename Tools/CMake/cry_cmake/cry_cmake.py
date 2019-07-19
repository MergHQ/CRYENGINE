
import os
import sys
import subprocess
import tkinter as tk
from tkinter import ttk


CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))

# CRYENGINE_DIR = os.path.abspath(os.path.join(CURRENT_DIR,'../../../'))
CRYENGINE_DIR = os.getcwd()
CMAKE_DIR = os.path.abspath(os.path.join(CRYENGINE_DIR, 'Tools', 'CMake'))
CMAKE_EXE = os.path.abspath(os.path.join(CMAKE_DIR, 'Win32', 'bin', 'cmake.exe'))
CMAKE_GUI_EXE = os.path.abspath(os.path.join(CMAKE_DIR, 'Win32', 'bin', 'cmake-gui.exe'))
CODE_SDKS_DIR = os.path.abspath(os.path.join(CRYENGINE_DIR, 'Code', 'SDKs'))

CONFIGS = [
    # Visual Studio 2015 Express
    {
        'title': 'Visual Studio 2015 Express Win64',
        'cmake_toolchain': 'toolchain/windows/WindowsPC-MSVC.cmake',
        'cmake_generator': 'Visual Studio 14 2015 Win64',
        'cmake_builddir': 'solutions/win64',
        'compiler': {
            'key_path': r'\WDExpress.DTE.14.0'
        }
    },
    # Visual Studio 2015
    {
        'title': 'Visual Studio 2015 Win64',
        'cmake_toolchain': r'toolchain\windows\WindowsPC-MSVC.cmake',
        'cmake_generator': 'Visual Studio 14 2015 Win64',
        'cmake_builddir': 'solutions_cmake/win64',
        'compiler': {
            'key_path': r'\VisualStudio.DTE.14.0'
        }
    },
    # Visual Studio 2017
    {
        'title': 'Visual Studio 2017 Win64',
        'cmake_toolchain': r'toolchain\windows\WindowsPC-MSVC.cmake',
        'cmake_generator': 'Visual Studio 15 2017 Win64',
        'cmake_builddir': 'solutions_cmake/win64',
        'compiler': {
            'key_path': r'\VisualStudio.DTE.15.0'
        }
    },
    # Visual Studio 2019
    {
        'title': 'Visual Studio 2019 Win64',
        'cmake_toolchain': r'toolchain\windows\WindowsPC-MSVC.cmake',
        'cmake_generator': 'Visual Studio 16 2019',
        'cmake_options': ['-A x64'],
        'cmake_builddir': 'solutions_cmake/win64',
        'compiler': {
            'key_path': r'\VisualStudio.DTE.16.0'
        }
    }
]


def valid_configs():
    try:
        import winreg

        def valid_config(c):
            try:
                registry = winreg.ConnectRegistry(None, winreg.HKEY_CLASSES_ROOT)
                key = winreg.OpenKey(registry, c['compiler']['key_path'])
                return True
            except:
                return False
        return [c for c in CONFIGS if valid_config(c)]

    except ImportError:
        return CONFIGS

CONFIGS = valid_configs()


def center_window(win):
    win.update_idletasks()
    width = win.winfo_width()
    height = win.winfo_height()
    x = (win.winfo_screenwidth() // 2) - (width // 2)
    y = (win.winfo_screenheight() // 2) - (height // 2)
    win.geometry('{}x{}+{}+{}'.format(width, height, x, y))


def cmake_configure(generator,
                    srcdir,
                    builddir,
                    cmakeexe=CMAKE_EXE,
                    options=[],
                    toolchain=None):

    srcdir = srcdir.replace('\\', '/')
    builddir = builddir.replace('\\', '/')

    cmake_command = ['\"'+cmakeexe+'\"']

    cmake_command.append('-Wno-dev')

    if toolchain:
        toolchain = toolchain.replace('\\', '/')
        cmake_command.append('-DCMAKE_TOOLCHAIN_FILE=\"' + toolchain + '\"')

    cmake_command.append('\"' + srcdir + '\"')
    cmake_command.append('-B' + '\"'+builddir + '\"')

    cmake_command.append('-G\"'+generator+'\"')

    cmake_command.extend(options)

    print(' '.join(cmake_command))

    # ret = subprocess.call(cmake_command,shell=True)
    cmd = ' '.join(cmake_command)
    os.system("start /wait cmd /c \"" + cmd + '\"')

    # Start cmake-gui application
    subprocess.Popen([CMAKE_GUI_EXE, '-S' + srcdir, '-B' + builddir])
    sys.exit(0)


class Application(tk.Frame):
    def __init__(self, master=None):
        super().__init__(master)
        self.parent = master
        self.parent.title("CRYENGINE CMake Project Generator")
        self.parent.minsize(300, 100)
        self.pack()
        self.create_widgets()

    def create_widgets(self):
        tk.Label(self, text="Generate Configuration: ").pack()

        self.newselection = ''
        self.box_value = tk.StringVar()
        self.configs_box = ttk.Combobox(self, textvariable=self.box_value, width=40)
        # self.configs_box.minsize(300, 100);

        config_list = []
        for config in CONFIGS:
            config_list.append(config['title'])
        self.configs_box['values'] = config_list

        i = 0
        try:
            with open(os.path.expandvars(r'%APPDATA%\Crytek\CryENGINE\cry_cmake.cfg'), 'r') as f:
                last_choice = f.read()
            for c in CONFIGS:
                if c['title'] == last_choice:
                    i = CONFIGS.index(c)
        except:
            pass
        self.configs_box.current(i)
        # self.configs_box.bind("<<ComboboxSelected>>", self.newselection)
        # self.configs_box.grid(column=0, row=0)
        # self.configs_box.pack(side="top")
        self.configs_box.pack()

        self.generate = tk.Button(self)
        self.generate["text"] = "Generate Solution"
        self.generate["command"] = self.generate_cmd
        # self.generate.pack(side="top")
        self.generate.pack()

        # self.quit = tk.Button(self, text="QUIT", fg="red",command=root.destroy)
        # self.quit.pack(side="bottom")

    def generate_cmd(self):
        current = self.configs_box.current()
        config = CONFIGS[current]
        with open(os.path.expandvars(r'%APPDATA%\Crytek\CryENGINE\cry_cmake.cfg'), 'w') as f:
            f.write(config['title'])
        self.parent.destroy()
        cmake_configure(
            generator=config['cmake_generator'],
            srcdir=CRYENGINE_DIR,
            builddir=os.path.join(CRYENGINE_DIR, config['cmake_builddir']),
            toolchain=os.path.join(CMAKE_DIR, config['cmake_toolchain']),
            options=config.get('cmake_options', [])
        )


def main():
    iconfile = "icon.ico"
    if not hasattr(sys, "frozen"):
        iconfile = os.path.join(os.path.dirname(__file__), iconfile)
    else:
        iconfile = os.path.join(sys.prefix, iconfile)

    root = tk.Tk()
    root.iconbitmap(iconfile)
    app = Application(master=root)
    center_window(root)
    app.mainloop()
