import PySide2
from PySide2 import QtWidgets
import SandboxBridge

class TestWindow(QtWidgets.QWidget):

    def __init__(self):
        super(TestWindow, self).__init__()
        self.setLayout(QtWidgets.QVBoxLayout())
        self.layout().addWidget(QtWidgets.QPushButton("HELLO SANDBOX, I COME FROM THE WORLD OF PYTHON"))

    def __del__(self):
        print "TestWindow Python Object Deleted"

SandboxBridge.register_window(TestWindow, "Test Window", category="Test", needs_menu_item=True, menu_path="Test", unique=False)
