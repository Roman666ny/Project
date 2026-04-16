#include "FileBrowser.hpp"
#include "ImageConverter.hpp"
#include "terminal.hpp"

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>
#include <limits>

namespace fs = std::filesystem;
using namespace std;

static string trim(const string& s) {
    const auto first = s.find_first_not_of(" \t");
    if (first == string::npos) return "";
    const auto last = s.find_last_not_of(" \t");
    return s.substr(first, last - first + 1);
}

static string truncate(const string& s, size_t maxLen) {
    return (s.size() <= maxLen) ? s : s.substr(0, maxLen - 3) + "...";
}

static string readInput() {
    string line;
    cout << Color::BCYAN << "  > " << Color::RESET;
    getline(cin, line);
    return trim(line);
}

struct Entry { string path, name; bool isDir; };

static vector<Entry> listDirectory(const fs::path& dir) {
    vector<Entry> dirs, files;
    error_code ec;
    for (const auto& e : fs::directory_iterator(dir, ec)) {
        if (ec) break;
        Entry entry;
        entry.path  = e.path().string();
        entry.name  = e.path().filename().string();
        entry.isDir = e.is_directory(ec);
        if (entry.name.empty() || entry.name[0] == '.') continue;
        if (entry.isDir)
            dirs.push_back(entry);
        else if (ImageConverter::detectFormat(entry.path) != ImageFormat::UNKNOWN)
            files.push_back(entry);
    }
    sort(dirs.begin(),  dirs.end(),  [](const Entry& a, const Entry& b){ return a.name < b.name; });
    sort(files.begin(), files.end(), [](const Entry& a, const Entry& b){ return a.name < b.name; });
    vector<Entry> result;
    result.insert(result.end(), dirs.begin(),  dirs.end());
    result.insert(result.end(), files.begin(), files.end());
    return result;
}

static string browserLoop(const string& startDir, bool selectDir) {
    fs::path current = fs::absolute(startDir);

    while (true) {
        error_code ec;
        if (!fs::is_directory(current, ec)) current = current.parent_path();

        const auto entries = listDirectory(current);

        clearScreen();

        cout << Color::BCYAN;
        cout << "  ╔══════════════════════════════════════════════════════════╗\n";
        if (selectDir)
            printBoxLine("  Обзор папок", 13);
        else
            printBoxLine("  Обзор файлов", 14);
        cout << "  ╚══════════════════════════════════════════════════════════╝\n";
        cout << Color::RESET;

        cout << "\n  " << Color::GRAY << "Папка: " << Color::RESET
                  << Color::WHITE << current.string() << Color::RESET << "\n\n";

        cout << "  " << Color::BCYAN << "[0]" << Color::RESET
                  << Color::GRAY << "  ↑  .." << Color::RESET << '\n';
        printLine();

        int idx = 1;
        for (const auto& e : entries) {
            cout << "  " << Color::BCYAN << "[" << idx++ << "]"
                      << Color::RESET << "  ";
            if (e.isDir) {
                cout << Color::YELLOW << "[папка] " << Color::RESET
                          << truncate(e.name, 45) << "\n";
            } else {
                const string fmt = ImageConverter::formatToString(
                    ImageConverter::detectFormat(e.path));
                cout << Color::GREEN << left << setw(7)
                          << ("[" + fmt + "]") << Color::RESET
                          << " " << truncate(e.name, 40) << '\n';
            }
        }

        if (selectDir) {
            printLine();
            cout << "  " << Color::BCYAN << "[" << idx << "]"
                      << Color::RESET << Color::BGREEN
                      << "  + Выбрать текущую папку\n" << Color::RESET;
        }

        printLine();
        cout << Color::GRAY << "  Введите номер, путь или " << Color::RESET
                  << Color::YELLOW << "'q'" << Color::RESET
                  << Color::GRAY << " для отмены\n" << Color::RESET;

        const string input = readInput();
        if (input == "q" || input == "Q") return "";

        bool isNum = !input.empty();
        for (char c : input)
            if (!isdigit(static_cast<unsigned char>(c))) { isNum = false; break; }

        if (isNum) {
            const int n = stoi(input);
            if (n == 0) { current = current.parent_path(); continue; }
            if (selectDir && n == static_cast<int>(entries.size()) + 1)
                return current.string();
            if (n >= 1 && n <= static_cast<int>(entries.size())) {
                const Entry& chosen = entries[n - 1];
                if (chosen.isDir) { current = chosen.path; continue; }
                if (!selectDir) return chosen.path;
                printWarning("Это файл. Выберите папку.");
                continue;
            }
            printWarning("Номер вне диапазона.");
            continue;
        }

        error_code ec2;
        if (!input.empty()) {
            const fs::path manual = input;
            if (fs::is_regular_file(manual, ec2) && !selectDir) {
                if (ImageConverter::detectFormat(manual.string()) != ImageFormat::UNKNOWN)
                    return manual.string();
                printWarning("Формат не поддерживается.");
            } else if (fs::is_directory(manual, ec2)) {
                if (selectDir) return manual.string();
                current = manual;
            } else {
                printWarning("Путь не найден.");
            }
        }
    }
}

string browseForFile(const string& startDir) {
    return browserLoop(startDir, false);
}

string browseForDirectory(const string& startDir) {
    return browserLoop(startDir, true);
}
