#pragma once

#include <iostream>
#include <filesystem>
#include <set>
#include <string>

void CompareDumps(const std::string& oldVersionRoot, const std::string& newVersionRoot);
std::vector<std::string> GetAvailableDumpVersions(const std::string& currentVersion);