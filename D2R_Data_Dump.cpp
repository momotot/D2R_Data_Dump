#include <iostream>
#include <string>
#include <Windows.h>
#include <fstream>
#include <chrono>
#include <algorithm>

#include <CascLib.h>

#include "File/File.h"

int main() {

    // input from the user to dump all files of the selected type
    std::string fileExtension;
    std::cout << "Enter extension to dump (e.g. .sprite, .json, .all (to dump all)): ";
    std::cin >> fileExtension;

    std::string extensionFolder = fileExtension;
    if (!extensionFolder.empty() && extensionFolder[0] == '.') {
        extensionFolder = extensionFolder.substr(1);
    }
    if (extensionFolder == "all") {
        extensionFolder = "all";
    }

    HANDLE hStorage = nullptr;
    if (!CascOpenStorage(s2ws(g_dataPath).c_str(), 0, &hStorage)) {
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

    // get current patch version
    std::string patchVersion = GetExeVersion();
    if (patchVersion.empty()) {
        std::cerr << "[Warning] Could not detect D2R version.\n";
        patchVersion = "unknown";
    }
    else {
        std::cout << "[Info] Detected D2R version: " << patchVersion << "\n";
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
        bool shouldDump = (fileExtension == ".all") || (filePath.find(fileExtension) != std::string::npos);
        if (!shouldDump) continue;

        HANDLE hFile = nullptr;
        if (CascOpenFile(hStorage, fd.szFileName, 0, CASC_OPEN_BY_NAME, &hFile)) {
            CASC_FILE_FULL_INFO info{};
            if (CascGetFileInfo(hFile, CascFileFullInfo, &info, sizeof(info), nullptr)) {
                void* buf = malloc(info.ContentSize);
                if (buf) {
                    DWORD read = 0;
                    if (CascReadFile(hFile, buf, info.ContentSize, &read)) {
                        if (SaveFileToDisk(filePath, buf, read, extensionFolder, patchVersion))
                            ++filesDumped;
                    }
                    free(buf);
                }
            }
            CascCloseFile(hFile);
        }
        
    } while (CascFindNextFile(hFind, &fd));

    // dump stats to console
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "\n[Summary] Dumped " << filesDumped << " *" << fileExtension << " files in " << elapsed.count() << " s.\n";


    // option to compare a specific dump to other version
    std::cout << "\nCompare with a previous dump? (y/n): ";
    char doCompare = 'n';
    std::cin >> doCompare;

    if (std::tolower(doCompare) == 'y') {
        std::string safeVersion = GetSanitizedVersion(patchVersion);
        auto prevVersions = GetAvailableDumpVersions(safeVersion);
        if (prevVersions.empty()) {
            std::cout << "[Info] No previous dumps found.\n";
        }
        else {
            std::cout << "\nAvailable previous versions:\n";
            for (size_t i = 0; i < prevVersions.size(); ++i) {
                std::cout << " [" << (i + 1) << "] " << prevVersions[i] << '\n';
            }

            std::cout << "Select index (or any other key to skip): ";
            size_t input;
            if (std::cin >> input && input >= 1 && input <= prevVersions.size()) {
                size_t idx = input - 1;  

                std::filesystem::path currentRoot = std::filesystem::path("output") / safeVersion;
                std::filesystem::path oldRoot = prevVersions[idx];

                std::cout << "[Info] Comparing current: " << safeVersion << " with old: " << prevVersions[idx] << "\n";

                if (!std::filesystem::exists(currentRoot)) {
                    std::cout << "[Error] Current root does not exist: " << currentRoot << "\n";
                }

                if (!std::filesystem::exists(oldRoot)) {
                    std::cout << "[Error] Old root does not exist: " << oldRoot << "\n";
                }

                // Compare subfolders
                for (const auto& entry : std::filesystem::directory_iterator(currentRoot)) {
                    if (!entry.is_directory()) continue;

                    std::filesystem::path typeFolder = entry.path().filename();
                    std::filesystem::path oldTypePath = oldRoot / typeFolder;
                    std::filesystem::path newTypePath = currentRoot / typeFolder;

                    if (std::filesystem::exists(oldTypePath) && std::filesystem::exists(newTypePath)) {
                        CompareDumps(oldTypePath.string(), newTypePath.string());
                    }
                    else {
                        std::cout << "[Skip] " << typeFolder << " (missing in one of the versions)\n";
                    }
                }
            }
            else {
                std::cout << "[Info] Comparison skipped.\n";
            }
        }
    }

    // clean up
    CascFindClose(hFind);
    CascCloseStorage(hStorage);


    return 0;
}