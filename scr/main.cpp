#include <iostream>
#include <iomanip>
#include <algorithm>
#include <limits>
#include <string>
#include <vector>
#include <filesystem>

#include "terminal.hpp"
#include "ImageConverter.hpp"
#include "FileBrowser.hpp"

namespace fs = std::filesystem;
using namespace std;

static void printHeader() {
    clearScreen();
    cout << Color::BCYAN;
    cout << "  ╔══════════════════════════════════════════════════════════╗\n";
    printBoxLine("               IMAGE FORMAT CONVERTER  v1.0               ", 58);
    printBoxLine("              Конвертор форматов изображений              ", 58);
    cout << "  ╚══════════════════════════════════════════════════════════╝\n";
    cout << Color::RESET << '\n';
}

static int readInt(const string& prompt, int lo, int hi) {
    int value = 0;
    while (true) {
        cout << Color::BCYAN << prompt << Color::RESET;
        if (cin >> value) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            if (value >= lo && value <= hi) return value;
        } else {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
        printWarning("Введите число от " + to_string(lo) + " до " + to_string(hi) + ".");
    }
}

static string readLine(const string& prompt) {
    string line;
    cout << Color::BCYAN << prompt << Color::RESET;
    getline(cin, line);
    const auto first = line.find_first_not_of(" \t");
    if (first == string::npos) return "";
    line = line.substr(first);
    const auto last = line.find_last_not_of(" \t");
    if (last != string::npos) line = line.substr(0, last + 1);
    return line;
}

static void printChannelName(int channels) {
    switch (channels) {
        case 1: cout << " (Grayscale)";          break;
        case 2: cout << " (Grayscale + Alpha)";  break;
        case 3: cout << " (RGB)";                 break;
        case 4: cout << " (RGBA — с прозрачн.)"; break;
        default: break;
    }
}

static void printImageInfo(const ImageInfo& info) {
    const long long px = static_cast<long long>(info.width) * info.height;
    cout << '\n';
    printLine();
    cout << Color::BOLD << "  Информация об изображении\n" << Color::RESET;
    printLine();
    printInfo("  Файл     : ", info.filepath);
    printInfo("  Формат   : ", ImageConverter::formatToString(info.format));
    printInfo("  Размер   : ", to_string(info.width) + " × " + to_string(info.height) + " px");
    cout << Color::GRAY << "  Каналы   : " << Color::RESET
              << Color::WHITE << info.channels;
    printChannelName(info.channels);
    cout << Color::RESET << '\n';
    printInfo("  Пикселей : ", to_string(px));
    printLine();
}


static ImageFormat chooseTargetFormat(ImageFormat currentFmt = ImageFormat::UNKNOWN) {
    const auto formats = ImageConverter::supportedFormats();
    cout << '\n' << Color::BOLD << "  Выберите формат вывода:\n\n" << Color::RESET;
    for (int i = 0; i < static_cast<int>(formats.size()); ++i) {
        const bool isCurrent = (formats[i] == currentFmt);
        cout << "  " << Color::BCYAN << "[" << (i + 1) << "]" << Color::RESET
                  << " " << left << setw(6)
                  << ImageConverter::formatToString(formats[i])
                  << Color::GRAY << " — " << Color::RESET
                  << ImageConverter::formatDescription(formats[i]);
        if (isCurrent)
            cout << Color::YELLOW << "  ← текущий" << Color::RESET;
        cout << '\n';
    }
    cout << "  " << Color::BCYAN << "[0]" << Color::RESET << " Отмена\n\n";

    const int choice = readInt("  Ваш выбор: ", 0, static_cast<int>(formats.size()));
    return (choice == 0) ? ImageFormat::UNKNOWN : formats[choice - 1];
}

struct Quality { int jpeg = 90; int webp = 90; };

static Quality chooseQuality(ImageFormat targetFormat) {
    Quality q;
    if (targetFormat == ImageFormat::JPEG) {
        cout << '\n';
        printInfo("  ", "Качество JPEG (1–100). Рекомендуется: 85–95.");
        q.jpeg = readInt("  Качество JPEG: ", 1, 100);
    }
    if (targetFormat == ImageFormat::WEBP) {
        cout << '\n';
        printInfo("  ", "Качество WebP (1–100). Рекомендуется: 80–90.");
        q.webp = readInt("  Качество WebP: ", 1, 100);
    }
    return q;
}

static string getFilePath(const string& startDir) {
    cout << '\n';
    cout << "  " << Color::BCYAN << "[1]" << Color::RESET << " Обзор файлов (браузер)\n";
    cout << "  " << Color::BCYAN << "[2]" << Color::RESET << " Ввести путь вручную\n";
    cout << "  " << Color::BCYAN << "[0]" << Color::RESET << " Отмена\n\n";
    const int m = readInt("  Ваш выбор: ", 0, 2);
    if (m == 0) return "";
    if (m == 1) return browseForFile(startDir);
    return readLine("  Путь к файлу: ");
}

static string getDirPath(const string& startDir) {
    cout << '\n';
    cout << "  " << Color::BCYAN << "[1]" << Color::RESET << " Обзор папок (браузер)\n";
    cout << "  " << Color::BCYAN << "[2]" << Color::RESET << " Ввести путь вручную\n";
    cout << "  " << Color::BCYAN << "[0]" << Color::RESET << " Отмена\n\n";
    const int m = readInt("  Ваш выбор: ", 0, 2);
    if (m == 0) return "";
    if (m == 1) return browseForDirectory(startDir);
    return readLine("  Путь к папке: ");
}

static void runSingleConversion() {
    // Шаг 1: выбор файла
    printHeader();
    cout << Color::BOLD << "  Конвертация файла\n" << Color::RESET;

    const string startDir  = fs::current_path().string();
    const string inputPath = getFilePath(startDir);
    if (inputPath.empty()) return;

    // Шаг 2: информация о файле + выбор формата
    printHeader();
    cout << Color::BOLD << "  Конвертация файла\n" << Color::RESET;

    const ImageInfo info = ImageConverter::getInfo(inputPath);
    if (!info.valid) {
        printError("Не удалось открыть файл: " + inputPath);
        pressEnter();
        return;
    }

    printImageInfo(info);

    const ImageFormat targetFormat = chooseTargetFormat(info.format);
    if (targetFormat == ImageFormat::UNKNOWN) return;

    if (targetFormat == info.format) {
        printWarning("Файл уже в формате " + ImageConverter::formatToString(targetFormat) + ".");
        pressEnter();
        return;
    }

    // Шаг 3: качество (только для JPEG/WebP)
    if (targetFormat == ImageFormat::JPEG || targetFormat == ImageFormat::WEBP) {
        printHeader();
        cout << Color::BOLD << "  Конвертация файла\n" << Color::RESET;
    }
    const Quality q = chooseQuality(targetFormat);

    // Шаг 4: конвертация и результат
    printHeader();
    cout << Color::BOLD << "  Конвертация файла\n" << Color::RESET;

    cout << '\n';
    cout << Color::GRAY << "  Конвертирование..." << Color::RESET;
    cout.flush();

    const ConversionResult result = ImageConverter::convert(inputPath, targetFormat, q.jpeg, q.webp);

    cout << "\r                    \r";
    printLine();
    if (result.success) {
        printSuccess("Готово!");
        printInfo("  Сохранено : ", result.outputPath);
    } else {
        printError(result.message);
    }
    printLine();
    pressEnter();
}

static vector<string> collectImages(const fs::path& dir) {
    vector<string> files;
    error_code ec;
    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        if (!entry.is_regular_file(ec)) continue;
        if (ImageConverter::detectFormat(entry.path().string()) != ImageFormat::UNKNOWN)
            files.push_back(entry.path().string());
    }
    sort(files.begin(), files.end());
    return files;
}

static void runBatchConversion() {
    // Шаг 1: выбор папки
    printHeader();
    cout << Color::BOLD << "  Пакетная конвертация папки\n" << Color::RESET;

    const string startDir = fs::current_path().string();
    const string dirPath  = getDirPath(startDir);
    if (dirPath.empty()) return;

    // Шаг 2: список файлов + выбор формата
    printHeader();
    cout << Color::BOLD << "  Пакетная конвертация папки\n" << Color::RESET;

    error_code ec;
    if (!fs::is_directory(dirPath, ec)) {
        printError("Папка не найдена: " + dirPath);
        pressEnter();
        return;
    }

    const auto files = collectImages(dirPath);
    if (files.empty()) {
        printWarning("В папке не найдено поддерживаемых изображений.");
        pressEnter();
        return;
    }

    cout << '\n';
    printLine();
    cout << Color::BOLD << "  Найдено файлов: " << Color::BCYAN
              << files.size() << Color::RESET << "\n\n";
    for (size_t i = 0; i < files.size(); ++i) {
        const string name = fs::path(files[i]).filename().string();
        const string fmt  = ImageConverter::formatToString(ImageConverter::detectFormat(files[i]));
        cout << "  " << Color::GRAY << setw(3) << (i + 1) << ". " << Color::RESET
                  << left << setw(32) << name
                  << Color::GRAY << fmt << Color::RESET << '\n';
    }
    printLine();

    const ImageFormat targetFormat = chooseTargetFormat();
    if (targetFormat == ImageFormat::UNKNOWN) return;

    // Шаг 3: качество (только для JPEG/WebP)
    if (targetFormat == ImageFormat::JPEG || targetFormat == ImageFormat::WEBP) {
        printHeader();
        cout << Color::BOLD << "  Пакетная конвертация папки\n" << Color::RESET;
    }
    const Quality q = chooseQuality(targetFormat);

    // Шаг 4: прогресс и итоги
    printHeader();
    cout << Color::BOLD << "  Пакетная конвертация папки\n" << Color::RESET;
    cout << '\n';
    printLine();
    int ok = 0, skipped = 0, failed = 0;

    for (size_t i = 0; i < files.size(); ++i) {
        const string name = fs::path(files[i]).filename().string();
        cout << "  " << Color::GRAY << "[" << setw(3) << (i + 1) << "/"
                  << files.size() << "]" << Color::RESET
                  << "  " << left << setw(30) << name;
        cout.flush();

        if (ImageConverter::detectFormat(files[i]) == targetFormat) {
            cout << Color::YELLOW << " пропущен" << Color::RESET << '\n';
            ++skipped;
            continue;
        }

        const ConversionResult r = ImageConverter::convert(files[i], targetFormat, q.jpeg, q.webp);
        if (r.success) {
            cout << Color::GREEN << " → " << fs::path(r.outputPath).filename().string()
                      << Color::RESET << '\n';
            ++ok;
        } else {
            cout << Color::RED << " ошибка" << Color::RESET << '\n';
            ++failed;
        }
    }

    printLine();
    cout << "  " << Color::BGREEN << "Конвертировано: " << ok << Color::RESET;
    cout << "   " << Color::YELLOW << "Пропущено: " << skipped << Color::RESET;
    if (failed > 0)
        cout << "   " << Color::RED << "Ошибок: " << failed << Color::RESET;
    cout << '\n';
    printLine();
    pressEnter();
}

int main() {
    setupConsole();

    while (true) {
        printHeader();

        cout << "  " << Color::BCYAN << "[1]" << Color::RESET << "  Конвертировать файл\n";
        cout << "  " << Color::BCYAN << "[2]" << Color::RESET << "  Конвертировать папку  "
                  << Color::GRAY << "(пакетно)\n" << Color::RESET;
        cout << "  " << Color::BCYAN << "[0]" << Color::RESET << "  Выход\n\n";

        const int choice = readInt("  Ваш выбор: ", 0, 2);
        switch (choice) {
            case 1: runSingleConversion(); break;
            case 2: runBatchConversion();  break;
            case 0:
                clearScreen();
                cout << Color::BCYAN << "  До свидания!\n\n" << Color::RESET;
                return 0;
        }
    }
}
