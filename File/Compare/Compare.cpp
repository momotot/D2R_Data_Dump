#include "Compare.h"

// compare the select folders
void CompareDumps(const std::string& oldRoot, const std::string& newRoot) {
    std::filesystem::path oldPath(oldRoot);
    std::filesystem::path newPath(newRoot);

    if (!std::filesystem::exists(oldPath) || !std::filesystem::exists(newPath)) {
        std::cerr << "[Error] One of the comparison directories does not exist:\n"
            << "  old: " << oldRoot << "\n"
            << "  new: " << newRoot << "\n";
        return;
    }

    std::set<std::string> oldFiles, newFiles;

    auto collectFiles = [](const std::filesystem::path& root, std::set<std::string>& out) {
        for (const auto& p : std::filesystem::recursive_directory_iterator(root)) {
            if (std::filesystem::is_regular_file(p)) {
                // store path relative to the root folder
                out.insert(std::filesystem::relative(p.path(), root).generic_string());
            }
        }
        };

    collectFiles(oldPath, oldFiles);
    collectFiles(newPath, newFiles);

    // Find added and removed files
    std::vector<std::string> removed, added;
    std::set_difference(oldFiles.begin(), oldFiles.end(),
        newFiles.begin(), newFiles.end(),
        std::back_inserter(removed));
    std::set_difference(newFiles.begin(), newFiles.end(),
        oldFiles.begin(), oldFiles.end(),
        std::back_inserter(added));

    std::cout << "\n[Comparison] "
        << oldPath.filename() << " -> " << newPath.filename() << "\n";

    if (removed.empty() && added.empty()) {
        std::cout << "No file changes detected.\n";
    }
    else {
        if (!removed.empty()) {
            std::cout << "REMOVED (" << removed.size() << "):\n";
            for (const auto& f : removed) std::cout << " - " << f << "\n";
        }
        if (!added.empty()) {
            std::cout << "ADDED (" << added.size() << "):\n";
            for (const auto& f : added) std::cout << " + " << f << "\n";
        }
    }
}

// check what folder sfor earlier dumps exists
std::vector<std::string> GetAvailableDumpVersions(const std::string& currentVersion) {
    std::vector<std::string> versions;
    const std::string base = "output";

    if (!std::filesystem::exists(base)) return versions;

    for (const auto& verEntry : std::filesystem::directory_iterator(base)) {
        if (!verEntry.is_directory()) continue;
        std::string folderName = verEntry.path().filename().string();

        // skip the current version folder
        if (folderName == currentVersion) continue;

        // skip potential png folders
        if (folderName.find("png") != std::string::npos) continue;

        versions.push_back(verEntry.path().string());
    }

    std::sort(versions.rbegin(), versions.rend());
    return versions;
}