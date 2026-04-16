#pragma once
#include <string>
#include <vector>

// Interactive terminal file browser.
// Returns selected file path, or empty string if cancelled.
std::string browseForFile(const std::string& startDir = ".");
std::string browseForDirectory(const std::string& startDir = ".");
