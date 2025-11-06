#pragma once

#include <Windows.h>
#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <algorithm>
#include <vector>

#include "../Misc/Misc.h"
#include "Compare/Compare.h"

extern const std::string g_dataPath;
extern const std::string g_d2rPath;

std::string GetExeVersion();
void CreateDirectoryIfNotExists(const std::string& path);
void CreateDirectoryTree(const std::string& fullPath);
bool SaveFileToDisk(const std::string& fileName, const void* data, DWORD size,
	const std::string& extensionFolder, const std::string& version);
bool IsFileInPath(const std::string& filePath, const std::string& filterPath);
std::string GetSanitizedVersion(const std::string& str);