#include <iostream>
#include <string>
#include <Windows.h>
#include <fstream>
#include <chrono>
#include <algorithm>

#include <CascLib.h>


// Create dir if it doesnt exist
void CreateDirectoryIfNotExists(const std::string& path) {
    if (!CreateDirectoryA(path.c_str(), NULL)) {
        if (GetLastError() != ERROR_ALREADY_EXISTS) {
            std::cerr << "[Error] Failed to create directory: " << path << "\n";
        }
    }
}

// create a directory tree (all subdirectiores leading to the target)
void CreateDirectoryTree(const std::string& fullPath) {
    std::string path = fullPath;
    std::replace(path.begin(), path.end(), '/', '\\');  // Windows uses backslashes

    size_t pos = 0;
    while ((pos = path.find('\\', pos + 1)) != std::string::npos) {
        std::string part = path.substr(0, pos);
        CreateDirectoryA(part.c_str(), nullptr);
    }
    CreateDirectoryA(path.c_str(), nullptr);
}

// helper to convert a std::string to std::wstring
std::wstring s2ws(const std::string& s)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring ws(len - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &ws[0], len);
    return ws;
}

// save the file 
bool SaveFileToDisk(const std::string& fileName, const void* data, DWORD size,
    const std::string& extensionFolder) {

    std::string relativePath = fileName;

    // Remove leading "data:" or "data\" or "data/" 
    const std::string prefixes[] = { "data:", "data\\", "data/" };
    for (const auto& prefix : prefixes) {
        if (relativePath.find(prefix) == 0) {
            relativePath = relativePath.substr(prefix.size());
            break;
        }
    }

    std::replace(relativePath.begin(), relativePath.end(), '\\', '/');

    // build the output path
    std::string outputRoot = "./output/" + extensionFolder + "/";
    std::string fullPath = outputRoot + relativePath;  

    // create the directory tree
    size_t lastSlash = fullPath.find_last_of('/');
    if (lastSlash != std::string::npos) {
        std::string dir = fullPath.substr(0, lastSlash);
        CreateDirectoryTree(dir);
    }
    else {
        CreateDirectoryTree(outputRoot);
    }

    // write the file
    std::ofstream out(fullPath, std::ios::binary);
    if (!out) {
        std::cerr << "[Error] Cannot write: " << fullPath << "\n";
        return false;
    }

    out.write(static_cast<const char*>(data), size);
    out.close();

    std::cout << "[Info] Saved: " << fullPath << "\n";

    return true;
}

// check if a file path starts with the specified filter path
bool IsFileInPath(const std::string& filePath, const std::string& filterPath) {
    return filePath.find(filterPath) == 0;  // Check if file path starts with the filter path
}


int main() {

    // Default installation folder of D2R with the Data folder
    std::string dataPath = R"(C:\Program Files (x86)\Diablo II Resurrected\Data\)";

    // input from the user to dump all files of the selected type
    std::string fileExtension;
    std::cout << "Enter extension to dump (e.g. .sprite, .json): ";
    std::cin >> fileExtension;

    std::string extensionFolder = fileExtension;
    if (!extensionFolder.empty() && extensionFolder[0] == '.') {
        extensionFolder = extensionFolder.substr(1);
    }

    HANDLE hStorage = nullptr;
    if (!CascOpenStorage(s2ws(dataPath).c_str(), 0, &hStorage)) {
        std::cerr << "[Error] Cannot open CASC storage.\n";
        return 1;
    }
    std::cout << "[Info] CASC storage opened.\n";

    CASC_FIND_DATA fd{};
    HANDLE hFind = CascFindFirstFile(hStorage, "*", &fd, nullptr);
    if (!hFind || hFind == INVALID_HANDLE_VALUE) {
        std::cerr << "[Error] No files found.\n";
        CascCloseStorage(hStorage);
        return 1;
    }

    // Keep track of some stats
    int filesDumped = 0;
    auto start = std::chrono::high_resolution_clock::now();

    // iterate over the files
    do {
        std::string filePath = fd.szFileName;
        std::string filePathLower = filePath;
        std::transform(filePathLower.begin(), filePathLower.end(), filePathLower.begin(), ::tolower);

        // default to skip all files containing lowend
        if (filePathLower.find("lowend") != std::string::npos) {
            continue;  
        }

        if (filePath.find(fileExtension) != std::string::npos) {
            HANDLE hFile = nullptr;
            if (CascOpenFile(hStorage, fd.szFileName, 0, CASC_OPEN_BY_NAME, &hFile)) {
                CASC_FILE_FULL_INFO info{};
                if (CascGetFileInfo(hFile, CascFileFullInfo, &info, sizeof(info), nullptr)) {
                    void* buf = malloc(info.ContentSize);
                    if (buf) {
                        DWORD read = 0;
                        if (CascReadFile(hFile, buf, info.ContentSize, &read)) {
                            if (SaveFileToDisk(filePath, buf, read, extensionFolder))
                                ++filesDumped;
                        }
                        free(buf);
                    }
                }
                CascCloseFile(hFile);
            }
        }
    } while (CascFindNextFile(hFind, &fd));

    // clean up
    CascFindClose(hFind);
    CascCloseStorage(hStorage);


    // dump stats to console
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "\n[Summary] Dumped " << filesDumped << " *" << fileExtension << " files in " << elapsed.count() << " s.\n";

    return 0;
}