#ifndef CONSOLE_UI_H
#define CONSOLE_UI_H

#include <string>
#include <vector>

#include "RecordManager.h"

/**
 * ConsoleUI — the menu-driven command line interface.
 *
 * Responsible for everything the user sees and types:
 * colored output, table rendering, input reading and validation.
 * All business logic is delegated to RecordManager.
 */
class ConsoleUI {
public:
    explicit ConsoleUI(RecordManager& manager);

    /// Main loop: show the menu and dispatch until the user exits.
    void run();

private:
    RecordManager& m_manager;

    // --- Menu actions (one per feature) ---
    void handleAddRecord();
    void handleDisplayAll();
    void handleSearch();
    void handleUpdateRecord();
    void handleDeleteRecord();
    void handleSortRecords();
    void handleExportCsv();

    // --- Rendering helpers ---
    void printBanner() const;
    void printMenu() const;
    void printTable(const std::vector<const Book*>& books) const;
    void printSuccess(const std::string& message) const;
    void printError(const std::string& message) const;
    void printInfo(const std::string& message) const;
    void pause() const;

    // --- Validated input helpers ---
    /// Read a whole line; throws EndOfInput when stdin closes.
    std::string readLine(const std::string& prompt) const;

    /// Keep asking until the user types an integer in [min, max].
    int readInt(const std::string& prompt, int min, int max) const;

    /// Like readInt but an empty line returns `defaultValue`.
    int readIntWithDefault(const std::string& prompt, int min, int max, int defaultValue) const;

    /// Keep asking until the user types a non-empty text without '|' (max 60 chars).
    std::string readText(const std::string& prompt) const;

    /// Like readText but an empty line returns `defaultValue`.
    std::string readTextWithDefault(const std::string& prompt, const std::string& defaultValue) const;

    /// Yes/no confirmation, defaults to no.
    bool confirm(const std::string& prompt) const;

    /// Signals that stdin was closed (Ctrl+Z / end of piped input).
    struct EndOfInput {};
};

#endif // CONSOLE_UI_H
