#ifndef RECORD_MANAGER_H
#define RECORD_MANAGER_H

#include <string>
#include <vector>

#include "Book.h"

/**
 * RecordManager — business logic and file persistence.
 *
 * Owns the in-memory list of records and keeps it in sync with the
 * storage file: every mutation (add/update/delete/sort) is saved
 * immediately, so no data is lost if the program is interrupted.
 */
class RecordManager {
public:
    enum class SortField { Id, Title, Author, Year };

    /// Result of loading the storage file.
    struct LoadReport {
        bool   fileExisted   = false;
        size_t loaded        = 0;
        size_t corruptLines  = 0;
    };

    explicit RecordManager(std::string filePath);

    /// Read all records from the storage file (silently skips corrupt lines).
    LoadReport load();

    // --- CRUD ---
    /// Add a record. Fails (returns false) if the ID is already taken.
    bool addRecord(const Book& book);

    /// All records, in current order.
    const std::vector<Book>& records() const { return m_records; }

    bool empty() const { return m_records.empty(); }
    size_t size() const { return m_records.size(); }

    /// Find a record by its ID; nullptr when not found.
    const Book* searchRecordByID(int id) const;

    /// Case-insensitive substring search on title and author.
    std::vector<const Book*> searchRecordByKeyword(const std::string& keyword) const;

    /// Update title/author/year of the record with the given ID (in place).
    bool updateRecord(int id, const std::string& title, const std::string& author, int year);

    /// Remove the record with the given ID.
    bool deleteRecord(int id);

    // --- Tools ---
    /// Sort records by the given field and persist the new order.
    void sortRecords(SortField field, bool ascending);

    /// Export all records to a CSV file (with header row).
    bool exportToCsv(const std::string& csvPath) const;

    /// Smallest positive ID not yet in use (suggested when adding).
    int nextAvailableId() const;

    /// True if a record with this ID exists.
    bool idExists(int id) const { return searchRecordByID(id) != nullptr; }

    const std::string& filePath() const { return m_filePath; }

private:
    std::string       m_filePath;
    std::vector<Book> m_records;

    /// Write all records back to the storage file.
    bool save() const;

    static std::string toLower(const std::string& text);
};

#endif // RECORD_MANAGER_H
