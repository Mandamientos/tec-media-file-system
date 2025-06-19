import ttkbootstrap as ttk
from tkinter import ttk as tkstd
from tkinter import StringVar
from tkinter import filedialog, messagebox
import tkinter.font as tkfont
import threading
import os

from gen import tec_mfs_pb2_grpc
from gen import tec_mfs_pb2
import grpc

class FilesView(ttk.Frame):
    def __init__(self, parent, manager):
        super().__init__(parent)
        self.manager = manager

        self.searchVar = StringVar()
        self.font = tkfont.Font(family="Inter", size=24, weight="bold")

        ttk.Label(self, text="Documentos", anchor="w", font=self.font).pack(fill="x", pady=10, padx=20)
        ttk.Separator(self, orient="horizontal").pack(fill="x", pady=15)

        searchLabelFrame = ttk.LabelFrame(self, text="Buscador de documentos", padding=15)
        searchLabelFrame.pack(fill="x", padx=30, pady=10)

        searchFrame = ttk.Frame(searchLabelFrame)
        searchFrame.pack(fill="x")

        ttk.Entry(searchFrame, textvariable=self.searchVar, width=40).pack(side="left", padx=(0, 10))
        ttk.Button(searchFrame, text="Buscar", command=self.filterList).pack(side="left")
        ttk.Button(searchFrame, text="Actualizar").pack(side="left", padx=(10, 0))
        ttk.Button(searchFrame, text="Subir archivo", bootstyle="success", command=self.uploadDocument).pack(side="right")

        self.tree = tkstd.Treeview(self, columns=("Nombre"), show="headings")

        self.tree.heading("Nombre", text="Nombre")
        self.tree.column("Nombre", anchor="center", stretch=True)
        
        self.tree.pack(fill="both", expand=True, padx=20, pady=10)

        advancedControls = ttk.LabelFrame(self, text="Controles avanzados", padding=15)
        advancedControls.pack(fill="x", padx=30, pady=10)
        ttk.Button(advancedControls, text="Eliminar", bootstyle="danger", command=self.deleteDocument).pack(fill="x")
        ttk.Button(advancedControls, text="Descargar", bootstyle="success", command=self.downloadDocument).pack(fill="x")

        self.allFiles = []

        self.refreshList() 
    
    def refreshList(self):
        def loadFilesFromServer():
            try:
                stub = self.manager.stub
                if not stub:
                    messagebox.showerror("Error", "No hay conexión con el controlador.")
                    return
                
                response = stub.GetDocumentList(tec_mfs_pb2.Empty())

                self.allFiles = [
                    (file.file_id, file.file_name)
                    for file in response.files
                ]

                self.updateTree(self.allFiles)
            
            except grpc.RpcError as e:
                messagebox.showerror("Error", f"No se pudo obtener la lista de documentos:\n{e.details()}")
            
            except Exception as e:
                messagebox.showerror("Error", f"Error inesperado:\n{str(e)}")
            
        threading.Thread(target=loadFilesFromServer, daemon=True).start()
    
    def updateTree(self, data):
        for item in self.tree.get_children():
            self.tree.delete(item)
        for file_id, file_name in data:
            self.tree.insert("", "end", values=(file_id, file_name))
    
    def filterList(self):
        query = self.searchVar.get().lower()
        filtered = [f for f in self.allFiles if query in f[1].lower()]
        self.updateTree(filtered)
    
    def uploadDocument(self):
        filepath = filedialog.askopenfilename(
            title="Selecciona el archivo PDF",
            filetypes=[("PDF files", "*.pdf")]
        )

        if not filepath:
            return
        
        try:
            with open(filepath, "rb") as f:
                content = f.read()
            
            filename = os.path.basename(filepath)

            request = tec_mfs_pb2.FileRequest(
                filename=filename,
                content=content
            )

            stub = self.manager.stub

            if not stub:
                messagebox.showerror("Error", "No hay conexión con el controlador.")
                return
            
            response = stub.AddDocument(request)

            if response.success:
                messagebox.showinfo("Éxito", f"Archivo '{filename}' subido correctamente.")
                self.refreshList()
            else:
                messagebox.showerror("Error", f"No se pudo subir el archivo: {response.message}")
        except Exception as e:
            messagebox.showerror("Error", f"Ocurrió un error al subir el archivo:\n{str(e)}")

    def downloadDocument(self):
        selected = self.tree.selection()
        if not selected:
            messagebox.showwarning("Advertencia", "Debes de seleccionar algún elemento de la lista.")
            return
        
        filename = self.tree.item(selected[0])["values"][0]
        
        try:
            stub = self.manager.stub
            if not stub:
                messagebox.showerror("Error", "No hay conexión con el controlador.")
                return
            
            request = tec_mfs_pb2.FileRequest(filename=filename)
            response = stub.GetDocument(request)

            if not response.file_data:
                messagebox.showerror("Error", "El archivo recibido está vacío o inválido.")
                return

            save_path = filedialog.asksaveasfilename(
                defaultextension=".pdf",
                filetypes=[("PDF files", "*.pdf")],
                initialfile=filename
            )

            if not save_path:
                return
            
            with open(save_path, "wb") as f:
                f.write(response.file_data)
            
            messagebox.showinfo("Éxito", f"Archivo guardado en:\n{save_path}")

        except grpc.RpcError as e:
            messagebox.showerror("Error", f"No se pudo descargar el documento:\n{e.details()}")
        except Exception as e:
            messagebox.showerror("Error inesperado", f"{type(e).__name__}: {str(e)}")
    
    def deleteDocument(self):
        selected = self.tree.selection()
        if not selected:
            messagebox.showwarning("Advertencia", "Debes seleccionar un documento para eliminar.")
            return
        
        fileId = self.tree.item(selected[0])["values"][0]

        confirm = messagebox.askyesno(
            "Confirmar borrado",
            f"¿Estás seguro de que quieres borrar {fileId}?"
        )
        if not confirm:
            return
        
        try:
            stub = self.manager.stub
            if not stub:
                messagebox.showerror("Error", "No hay conexión con el controlador.")
                return
            
            request = tec_mfs_pb2.FileRequest(filename=fileId)
            response = stub.DeleteDocument(request)

            if response.success:
                messagebox.showinfo("Éxito", f"Archivo '{fileId}' eliminado correctamente.")
                self.refreshList()
            else:
                messagebox.showerror("Error", f"No se pudo eliminar el archivo:\n{response.message}")

        except grpc.RpcError as e:
            messagebox.showerror("Error", f"Error de red al eliminar el documento:\n{e.details()}")
        except Exception as e:
            messagebox.showerror("Error inesperado", f"{type(e).__name__}: {str(e)}")