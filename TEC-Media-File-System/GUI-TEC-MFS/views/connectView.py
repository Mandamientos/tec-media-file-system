import time
import ttkbootstrap as ttk
from tkinter import ttk as tkstd
from tkinter import StringVar
import tkinter.font as tkfont
import threading

from gen import tec_mfs_pb2_grpc
from gen import tec_mfs_pb2
import grpc

class ConnectView(ttk.Frame):
    def __init__(self, parent, manager):
        super().__init__(parent)
        self.manager = manager
        self.font = tkfont.Font(family="Inter", size=36, weight="bold")
        self.ipAddress = StringVar()
        self.port = StringVar()

        ttk.Label(self, text="Conectarse", anchor="w", font=self.font).pack(fill="x", pady=20, padx=20)

        ttk.Separator(self, orient="horizontal").pack(fill="x", pady=15)

        connParameter = ttk.LabelFrame(self, text="Parámetros para la conexión", padding=15)
        connParameter.pack(fill="x", padx=30, pady=10)

        ipAddresLabel = ttk.Label(connParameter, text="Dirección IP", font=tkfont.Font(family="Inter", size="12", weight="bold")).pack(fill="x", pady=(10, 2))
        ipAddresEntry = ttk.Entry(connParameter, textvariable=self.ipAddress, width=30).pack(anchor="w")

        ttk.Separator(connParameter).pack(fill="x", pady=20)

        portLabel = ttk.Label(connParameter, text="Puerto", font=tkfont.Font(family="Inter", size="12", weight="bold")).pack(fill="x")
        portEntry = ttk.Entry(connParameter, textvariable=self.port, width=30).pack(anchor="w")


        self.connBtn = ttk.Button(self, text="Conectar", bootstyle="success", command=self.connect)
        self.connBtn.pack(pady=5)

        self.statusLabel = ttk.Label(self, text="", foreground="green", font=tkfont.Font(size=10, family="Inter"))
        self.statusLabel.pack(pady=10)
        self.statusLabel.pack_forget()
    
    def connect(self):
        self.statusLabel.pack_forget()
        if self.ipAddress.get() and self.port.get():
            address = f"{self.ipAddress.get()}:{self.port.get()}"
            self.showMessage("Tratando de conectarse...")
            threading.Thread(target=self.checkConnection, args=(address,), daemon=True).start()
        else:
            self.statusLabel.config(foreground="red")
            self.showMessage("No ingresaste correctamente la IP y el puerto.")

    def checkConnection(self, address: str):
        self.connBtn.state(["disabled"])
        channel = grpc.insecure_channel(address)
        stub = tec_mfs_pb2_grpc.FileSytemStub(channel)

        try:
            response = stub.GetSystemStatus(tec_mfs_pb2.Empty())
            self.statusLabel.config(foreground="green")
            self.showMessage("¡Conectado correctamente con el servidor!")
            time.sleep(2)
            self.manager.stub = stub
            self.manager.show("files")
            return
        except grpc.RpcError as e:
            self.statusLabel.config(foreground="red")
            self.showMessage("Se ha encontrado un error inesperado al intentar conectar.")
            print(e)
        
        self.connBtn.state(["!disabled"])

    
    def showMessage(self, message):
        self.statusLabel.config(text=message)
        self.statusLabel.pack(pady=10)