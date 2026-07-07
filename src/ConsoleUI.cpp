#include "ConsoleUI.h"

#include <algorithm>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

// ---------------------------------------------------------------------------
// ANSI colors (supported by Windows Terminal, conhost on Win10+, and all
// Unix terminals). Virtual terminal processing is enabled in enableAnsi().
// ---------------------------------------------------------------------------
constexpr const char* RESET  = "\033[0m";
constexpr const char* BOLD   = "\033[1m";
constexpr const char* DIM    = "\033[2m";
constexpr const char* CYAN   = "\033[36m";
constexpr const char* GREEN  = "\033[32m";
constexpr const char* RED    = "\033[31m";
constexpr const char* YELLOW = "\033[33m";

void enableAnsi() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8); // render the box-drawing characters
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (out != INVALID_HANDLE_VALUE && GetConsoleMode(out, &mode)) {
        SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#endif
}

/// Number of Unicode code points (display columns for Latin text).
size_t displayWidth(const std::string& utf8) {
    size_t width = 0;
    for (unsigned char c : utf8) {
        if ((c & 0xC0) != 0x80) ++width; // count everything but continuation bytes
    }
    return width;
}

/// Pad with spaces up to `width` display columns.
std::string pad(const std::string& text, size_t width) {
    size_t current = displayWidth(text);
    return current >= width ? text : text + std::string(width - current, ' ');
}

/// Truncate to `maxWidth` columns, ending with an ellipsis if cut.
std::string truncate(const std::string& text, size_t maxWidth) {
    if (displayWidth(text) <= maxWidth) {
        return text;
    }
    std::string cut;
    size_t width = 0;
    for (size_t i = 0; i < text.size();) {
        unsigned char c = text[i];
        size_t charLen = (c < 0x80) ? 1 : (c < 0xE0) ? 2 : (c < 0xF0) ? 3 : 4;
        if (width + 1 > maxWidth - 1) break;
        cut += text.substr(i, charLen);
        ++width;
        i += charLen;
    }
    return cut + "…"; // …
}

/// Trim leading/trailing whitespace.
std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

constexpr int MIN_YEAR = 1000;
constexpr int MAX_YEAR = 2100;
constexpr size_t MAX_TEXT_LENGTH = 60;

} // namespace

// ---------------------------------------------------------------------------

ConsoleUI::ConsoleUI(RecordManager& manager) : m_manager(manager) {
    enableAnsi();
}

void ConsoleUI::run() {
    printBanner();

    RecordManager::LoadReport report = m_manager.load();
    if (report.fileExisted) {
        std::ostringstream msg;
        msg << report.loaded << " record(s) loaded from " << m_manager.filePath();
        printInfo(msg.str());
        if (report.corruptLines > 0) {
            std::ostringstream warn;
            warn << report.corruptLines << " corrupt line(s) ignored in the data file";
            printError(warn.str());
        }
    } else {
        printInfo("No data file yet - starting with an empty catalogue.");
    }

    try {
        while (true) {
            printMenu();
            int choice = readInt("Your choice", 0, 7);
            std::cout << '\n';
            switch (choice) {
                case 1: handleAddRecord();    break;
                case 2: handleDisplayAll();   break;
                case 3: handleSearch();       break;
                case 4: handleUpdateRecord(); break;
                case 5: handleDeleteRecord(); break;
                case 6: handleSortRecords();  break;
                case 7: handleExportCsv();    break;
                case 0:
                    printSuccess("All changes are saved. Goodbye!");
                    return;
            }
        }
    } catch (const EndOfInput&) {
        // stdin closed (Ctrl+Z / end of script): everything is already saved
        std::cout << '\n';
        printSuccess("Input stream closed - all changes are saved. Goodbye!");
    }
}

// --------------------------------------------------------------------------
// Menu actions
// --------------------------------------------------------------------------

void ConsoleUI::handleAddRecord() {
    std::cout << BOLD << "-- Add a record --" << RESET << "\n\n";

    const int suggested = m_manager.nextAvailableId();
    int id;
    while (true) {
        std::ostringstream prompt;
        prompt << "ID " << DIM << "(Enter = " << suggested << ")" << RESET;
        id = readIntWithDefault(prompt.str(), 1, 999999, suggested);
        if (!m_manager.idExists(id)) break;
        printError("ID " + std::to_string(id) + " is already taken. Choose another one.");
    }

    const std::string title  = readText("Title");
    const std::string author = readText("Author");
    const int year = readInt("Year (" + std::to_string(MIN_YEAR) + "-" +
                             std::to_string(MAX_YEAR) + ")", MIN_YEAR, MAX_YEAR);

    Book book(id, title, author, year);
    if (m_manager.addRecord(book)) {
        printSuccess("Record #" + std::to_string(id) + " added and saved to " +
                     m_manager.filePath());
        printTable({ m_manager.searchRecordByID(id) });
    } else {
        printError("Could not save the record (is the data file writable?).");
    }
    pause();
}

void ConsoleUI::handleDisplayAll() {
    std::cout << BOLD << "-- All records --" << RESET << "\n";
    if (m_manager.empty()) {
        printInfo("The catalogue is empty. Add a record first (option 1).");
    } else {
        std::vector<const Book*> all;
        for (const Book& book : m_manager.records()) all.push_back(&book);
        printTable(all);
        printInfo(std::to_string(m_manager.size()) + " record(s) in total.");
    }
    pause();
}

void ConsoleUI::handleSearch() {
    std::cout << BOLD << "-- Search --" << RESET << "\n\n"
              << "  1. By ID\n"
              << "  2. By keyword (title or author)\n\n";
    int mode = readInt("Search mode", 1, 2);

    if (mode == 1) {
        int id = readInt("ID to search", 1, 999999);
        if (const Book* book = m_manager.searchRecordByID(id)) {
            printSuccess("Record found:");
            printTable({ book });
        } else {
            printError("No record with ID " + std::to_string(id) + ".");
        }
    } else {
        std::string keyword = readText("Keyword");
        auto matches = m_manager.searchRecordByKeyword(keyword);
        if (matches.empty()) {
            printError("No record matches \"" + keyword + "\".");
        } else {
            printSuccess(std::to_string(matches.size()) + " record(s) found:");
            printTable(matches);
        }
    }
    pause();
}

void ConsoleUI::handleUpdateRecord() {
    std::cout << BOLD << "-- Update a record --" << RESET << "\n\n";
    if (m_manager.empty()) {
        printInfo("The catalogue is empty - nothing to update.");
        pause();
        return;
    }

    int id = readInt("ID of the record to update", 1, 999999);
    const Book* book = m_manager.searchRecordByID(id);
    if (book == nullptr) {
        printError("No record with ID " + std::to_string(id) + ".");
        pause();
        return;
    }

    std::cout << "\nCurrent values " << DIM << "(press Enter to keep a value)"
              << RESET << ":\n";
    printTable({ book });

    const std::string title = readTextWithDefault(
        "New title  " + std::string(DIM) + "(Enter = keep)" + RESET, book->title());
    const std::string author = readTextWithDefault(
        "New author " + std::string(DIM) + "(Enter = keep)" + RESET, book->author());
    const int year = readIntWithDefault(
        "New year   " + std::string(DIM) + "(Enter = keep)" + RESET,
        MIN_YEAR, MAX_YEAR, book->year());

    if (m_manager.updateRecord(id, title, author, year)) {
        printSuccess("Record #" + std::to_string(id) + " updated:");
        printTable({ m_manager.searchRecordByID(id) });
    } else {
        printError("Update failed (is the data file writable?).");
    }
    pause();
}

void ConsoleUI::handleDeleteRecord() {
    std::cout << BOLD << "-- Delete a record --" << RESET << "\n\n";
    if (m_manager.empty()) {
        printInfo("The catalogue is empty - nothing to delete.");
        pause();
        return;
    }

    int id = readInt("ID of the record to delete", 1, 999999);
    const Book* book = m_manager.searchRecordByID(id);
    if (book == nullptr) {
        printError("No record with ID " + std::to_string(id) + ".");
        pause();
        return;
    }

    printTable({ book });
    if (confirm("Really delete this record?")) {
        if (m_manager.deleteRecord(id)) {
            printSuccess("Record #" + std::to_string(id) + " deleted.");
        } else {
            printError("Deletion failed (is the data file writable?).");
        }
    } else {
        printInfo("Deletion cancelled.");
    }
    pause();
}

void ConsoleUI::handleSortRecords() {
    std::cout << BOLD << "-- Sort records --" << RESET << "\n\n";
    if (m_manager.size() < 2) {
        printInfo("Nothing to sort - the catalogue has fewer than two records.");
        pause();
        return;
    }

    std::cout << "  1. By ID\n  2. By title\n  3. By author\n  4. By year\n\n";
    int field = readInt("Sort by", 1, 4);
    std::string order = readTextWithDefault(
        "Order: (a)scending / (d)escending " + std::string(DIM) + "(Enter = a)" + RESET, "a");
    bool ascending = (order != "d" && order != "D");

    static const RecordManager::SortField FIELDS[] = {
        RecordManager::SortField::Id, RecordManager::SortField::Title,
        RecordManager::SortField::Author, RecordManager::SortField::Year
    };
    static const char* NAMES[] = { "ID", "title", "author", "year" };

    m_manager.sortRecords(FIELDS[field - 1], ascending);
    printSuccess(std::string("Records sorted by ") + NAMES[field - 1] +
                 (ascending ? " (ascending)" : " (descending)") + " and saved.");

    std::vector<const Book*> all;
    for (const Book& book : m_manager.records()) all.push_back(&book);
    printTable(all);
    pause();
}

void ConsoleUI::handleExportCsv() {
    std::cout << BOLD << "-- Export to CSV --" << RESET << "\n\n";
    if (m_manager.empty()) {
        printInfo("The catalogue is empty - nothing to export.");
        pause();
        return;
    }

    const std::string defaultPath = "data/records.csv";
    std::string path = readTextWithDefault(
        "Output file " + std::string(DIM) + "(Enter = " + defaultPath + ")" + RESET,
        defaultPath);

    if (m_manager.exportToCsv(path)) {
        printSuccess(std::to_string(m_manager.size()) + " record(s) exported to " + path);
    } else {
        printError("Could not write to \"" + path + "\". Check the path and permissions.");
    }
    pause();
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

void ConsoleUI::printBanner() const {
    std::cout << CYAN << BOLD
              << "\n"
              << "  ╔════════════════════════════════════════════╗\n"
              << "  ║          RECORD MANAGEMENT SYSTEM          ║\n"
              << "  ║       Library catalogue · C++ / OOP        ║\n"
              << "  ╚════════════════════════════════════════════╝\n"
              << RESET << '\n';
}

void ConsoleUI::printMenu() const {
    const char* items[] = {
        "Add a record",       "Display all records", "Search a record",
        "Update a record",    "Delete a record",     "Sort records",
        "Export to CSV"
    };
    std::cout << '\n' << CYAN << "  ┌─────────── MENU ────────────┐" << RESET << '\n';
    for (int i = 0; i < 7; ++i) {
        std::cout << CYAN << "  │ " << RESET
                  << BOLD << (i + 1) << RESET << ". " << pad(items[i], 24)
                  << CYAN << " │" << RESET << '\n';
    }
    std::cout << CYAN << "  │ " << RESET << BOLD << 0 << RESET << ". "
              << pad("Exit", 24) << CYAN << " │" << RESET << '\n'
              << CYAN << "  └─────────────────────────────┘" << RESET << "\n\n";
}

void ConsoleUI::printTable(const std::vector<const Book*>& books) const {
    // Dynamic column widths, capped so the table stays readable.
    size_t idW = 2, titleW = 5, authorW = 6, yearW = 4;
    for (const Book* b : books) {
        idW     = std::max(idW, std::to_string(b->id()).size());
        titleW  = std::max(titleW, displayWidth(b->title()));
        authorW = std::max(authorW, displayWidth(b->author()));
    }
    titleW  = std::min(titleW, size_t(38));
    authorW = std::min(authorW, size_t(24));

    auto line = [&](const char* left, const char* mid, const char* right) {
        std::string h = "─";
        std::cout << DIM << "  " << left;
        for (size_t i = 0; i < idW + 2; ++i)     std::cout << h;
        std::cout << mid;
        for (size_t i = 0; i < titleW + 2; ++i)  std::cout << h;
        std::cout << mid;
        for (size_t i = 0; i < authorW + 2; ++i) std::cout << h;
        std::cout << mid;
        for (size_t i = 0; i < yearW + 2; ++i)   std::cout << h;
        std::cout << right << RESET << '\n';
    };

    std::cout << '\n';
    line("┌", "┬", "┐");
    std::cout << "  " << DIM << "│ " << RESET << BOLD
              << pad("ID", idW) << RESET << DIM << " │ " << RESET << BOLD
              << pad("Title", titleW) << RESET << DIM << " │ " << RESET << BOLD
              << pad("Author", authorW) << RESET << DIM << " │ " << RESET << BOLD
              << pad("Year", yearW) << RESET << DIM << " │" << RESET << '\n';
    line("├", "┼", "┤");
    for (const Book* b : books) {
        std::cout << "  " << DIM << "│ " << RESET
                  << pad(std::to_string(b->id()), idW) << DIM << " │ " << RESET
                  << pad(truncate(b->title(), titleW), titleW) << DIM << " │ " << RESET
                  << pad(truncate(b->author(), authorW), authorW) << DIM << " │ " << RESET
                  << pad(std::to_string(b->year()), yearW) << DIM << " │" << RESET << '\n';
    }
    line("└", "┴", "┘");
}

void ConsoleUI::printSuccess(const std::string& message) const {
    std::cout << GREEN << "  ✔ " << message << RESET << '\n';
}

void ConsoleUI::printError(const std::string& message) const {
    std::cout << RED << "  ✖ " << message << RESET << '\n';
}

void ConsoleUI::printInfo(const std::string& message) const {
    std::cout << YELLOW << "  • " << message << RESET << '\n';
}

void ConsoleUI::pause() const {
    std::cout << DIM << "\n  Press Enter to continue..." << RESET;
    std::string ignored;
    if (!std::getline(std::cin, ignored)) {
        throw EndOfInput{};
    }
}

// ---------------------------------------------------------------------------
// Validated input
// ---------------------------------------------------------------------------

std::string ConsoleUI::readLine(const std::string& prompt) const {
    std::cout << "  " << prompt << ": ";
    std::string input;
    if (!std::getline(std::cin, input)) {
        throw EndOfInput{};
    }
    return trim(input);
}

int ConsoleUI::readInt(const std::string& prompt, int min, int max) const {
    while (true) {
        const std::string input = readLine(prompt);
        try {
            size_t consumed = 0;
            int value = std::stoi(input, &consumed);
            if (consumed == input.size() && value >= min && value <= max) {
                return value;
            }
        } catch (const std::exception&) {
            // fall through to the error message
        }
        printError("Please enter a whole number between " + std::to_string(min) +
                   " and " + std::to_string(max) + ".");
    }
}

int ConsoleUI::readIntWithDefault(const std::string& prompt, int min, int max,
                                  int defaultValue) const {
    while (true) {
        const std::string input = readLine(prompt);
        if (input.empty()) {
            return defaultValue;
        }
        try {
            size_t consumed = 0;
            int value = std::stoi(input, &consumed);
            if (consumed == input.size() && value >= min && value <= max) {
                return value;
            }
        } catch (const std::exception&) {
            // fall through to the error message
        }
        printError("Please enter a whole number between " + std::to_string(min) +
                   " and " + std::to_string(max) + ", or press Enter.");
    }
}

std::string ConsoleUI::readText(const std::string& prompt) const {
    while (true) {
        const std::string input = readLine(prompt);
        if (input.empty()) {
            printError("This field cannot be empty.");
        } else if (input.find(Book::SEPARATOR) != std::string::npos) {
            printError("The character '|' is not allowed.");
        } else if (displayWidth(input) > MAX_TEXT_LENGTH) {
            printError("Maximum " + std::to_string(MAX_TEXT_LENGTH) + " characters.");
        } else {
            return input;
        }
    }
}

std::string ConsoleUI::readTextWithDefault(const std::string& prompt,
                                           const std::string& defaultValue) const {
    while (true) {
        const std::string input = readLine(prompt);
        if (input.empty()) {
            return defaultValue;
        }
        if (input.find(Book::SEPARATOR) != std::string::npos) {
            printError("The character '|' is not allowed.");
        } else if (displayWidth(input) > MAX_TEXT_LENGTH) {
            printError("Maximum " + std::to_string(MAX_TEXT_LENGTH) + " characters.");
        } else {
            return input;
        }
    }
}

bool ConsoleUI::confirm(const std::string& prompt) const {
    const std::string answer = readLine(prompt + " (y/N)");
    return answer == "y" || answer == "Y" || answer == "yes" || answer == "Yes";
}
