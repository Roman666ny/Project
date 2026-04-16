#pragma once
#include <iostream>
#include <string>
#include <limits>

#ifdef _WIN32
#include <windows.h>
#endif

// Коды цветов ANSI (работают на Linux/macOS и Windows 10+)
namespace Color {
    inline const char* RESET  = "\033[0m";
    inline const char* BOLD   = "\033[1m";
    inline const char* DIM    = "\033[2m";
    inline const char* CYAN   = "\033[36m";
    inline const char* BCYAN  = "\033[1;36m";
    inline const char* GREEN  = "\033[32m";
    inline const char* BGREEN = "\033[1;32m";
    inline const char* RED    = "\033[31m";
    inline const char* BRED   = "\033[1;31m";
    inline const char* YELLOW = "\033[33m";
    inline const char* GRAY   = "\033[90m";
    inline const char* WHITE  = "\033[97m";
}

// Настройка консоли: UTF-8 и ANSI-последовательности на Windows
inline void setupConsole() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (GetConsoleMode(hOut, &dwMode))
        SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

// Очистка экрана
inline void clearScreen() {
    std::cout << "\033[2J\033[1;1H" << std::flush;
}

// Ожидание нажатия Enter
inline void pressEnter(const std::string& msg = "  Нажмите Enter для продолжения...") {
    std::cout << '\n' << Color::GRAY << msg << Color::RESET << '\n';
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// Горизонтальный разделитель
inline void printLine(int width = 62) {
    std::cout << Color::GRAY << std::string(width, '-') << Color::RESET << '\n';
}

// Сообщение об успехе
inline void printSuccess(const std::string& msg) {
    std::cout << Color::BGREEN << "  + " << Color::RESET << Color::GREEN << msg << Color::RESET << '\n';
}

// Сообщение об ошибке
inline void printError(const std::string& msg) {
    std::cout << Color::BRED << "  x " << Color::RESET << Color::RED << msg << Color::RESET << '\n';
}

// Предупреждение
inline void printWarning(const std::string& msg) {
    std::cout << Color::YELLOW << "  ! " << msg << Color::RESET << '\n';
}

// Информационная строка с меткой и значением
inline void printInfo(const std::string& label, const std::string& value) {
    std::cout << Color::GRAY << "  " << label << Color::RESET
              << Color::WHITE << value << Color::RESET << '\n';
}

// Строка внутри рамки: visualLen — экранная ширина text (символов, не байт)
inline void printBoxLine(const std::string& text, int visualLen, int boxWidth = 58) {
    int pad = boxWidth - visualLen;
    std::cout << "  " << "║" << text << std::string(pad > 0 ? pad : 0, ' ') << "║\n";
}
