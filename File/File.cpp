#include "File.h"

const std::string g_dataPath = R"(C:\Program Files (x86)\Diablo II Resurrected\Data\)";
const std::string g_d2rPath = R"(C:\Program Files (x86)\Diablo II Resurrected\D2R.exe)";

// extract the d2r current patch version
std::string GetExeVersion() {
    std::wstring d2Path = s2ws(g_d2rPath);

    DWORD handle = 0;
    DWORD size = GetFileVersionInfoSizeW(d2Path.c_str(), &handle);
    if (size == 0) return "";

    std::vector<char> data(size);
    if (!GetFileVersionInfoW(d2Path.c_str(), handle, size, data.data())) return "";

    void* pValue = nullptr;
    UINT len = 0;
    if (VerQueryValueW(data.data(),
        L"\\StringFileInfo\\040904b0\\ProductVersion",
        &pValue, &len) && pValue != nullptr)
    {
        std::wstring wversion(static_cast<wchar_t*>(pValue));
        return std::string(wversion.begin(), wversion.end());
    }

    return "";
}

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

// save the file 
bool SaveFileToDisk(const std::string& fileName, const void* data, DWORD size,
    const std::string& extensionFolder, const std::string& version) {

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


    std::string safeVersion = GetSanitizedVersion(version);

    // we put output/patch as the default for this dump
    std::string outputRoot = "./output/" + safeVersion + "/" + extensionFolder + "/";
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
    return filePath.find(filterPath) == 0;  
}

std::string GetSanitizedVersion(const std::string& str) {
    std::string safeVersion = str.empty() ? "unknown" : str;
    std::string sanitizedVersion = safeVersion;
    std::replace(sanitizedVersion.begin(), sanitizedVersion.end(), ' ', '_');
    std::replace(sanitizedVersion.begin(), sanitizedVersion.end(), ',', '_');
    std::replace(sanitizedVersion.begin(), sanitizedVersion.end(), '.', '_');

    return sanitizedVersion;
}