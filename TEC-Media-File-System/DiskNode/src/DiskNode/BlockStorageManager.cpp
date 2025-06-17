#include "BlockStorageManager.h"
#include <fstream>
#include <filesystem>

BlockStorageManager::BlockStorageManager(const std::string& storagePath) : storagePath(storagePath) {
    std::filesystem::create_directories(storagePath);
}

bool BlockStorageManager::saveBlock(const tec_mfs::BlockData& blockData) {
    std::string filePath = storagePath + "/" + blockData.block_id() + ".binary";

    //Se abre un archivo para escribir los datos del bloque en el archivo 
    std::ofstream out(filePath, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "[ERROR] No se pudo abrir el archivo: " << filePath << std::endl;
        return false;
    }

    out.write(blockData.data().data(), blockData.data().size());
    out.close();

    std::cout << "[DiskNode] Bloque guardado en: " << filePath << std::endl;
    return true;
}

bool BlockStorageManager::loadBlock(const std::string& blockId, tec_mfs::BlockData& blockData) {
    std::string filePath = storagePath + "/" + blockId + ".binary";

    //Lee el contenido y asigna los datos 
    std::ifstream in(filePath, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "[ERROR] No se pudo abrir el bloque para lectura: " << filePath << std::endl;
        return false;
    }

    std::ostringstream ss;
    ss << in.rdbuf();
    std::string data = ss.str();

    blockData.set_block_id(blockId);
    blockData.set_data(data);

    std::cout << "[DiskNode] Bloque leído desde: " << filePath << ", tamaño: " << data.size() << std::endl;
    return true;
}
