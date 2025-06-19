from views.connectView import ConnectView
from views.filesView import FilesView

class ViewManager:
    def __init__(self, root):
        self.root = root
        self.views = {
            "connect": ConnectView,
            "files": FilesView
        }
        self.currentFrame = None
        self.stub = None
    
    def show(self, viewName):
        if self.currentFrame:
            self.currentFrame.destroy()
        
        viewClass = self.views.get(viewName)
        self.currentFrame = viewClass(self.root, self)
        self.currentFrame.pack(fill="both", expand=True)