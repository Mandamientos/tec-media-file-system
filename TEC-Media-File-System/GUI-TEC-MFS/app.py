import ttkbootstrap as ttk
from viewManager import ViewManager

class App(ttk.Window):
    def __init__(self):
        super().__init__(themename="flatly")
        self.title("TEC Media File System")
        self.geometry("600x450")

        self.manager = ViewManager(self)
        self.manager.show("connect")