#include "RecordManager.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>

RecordManager::RecordManager(std::string filePath)
    : m_filePath(std::move(filePath)) {}

RecordManager::LoadReport RecordManager::load() {
    LoadReport report;
    std::ifstream file(m_filePath);
    if (!file.is_open()) {
        return report; // first run: no file yet, start empty
    }
    report.fileExisted = true;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        if (auto book = Book::fromFileLine(line)) {
            m_records.push_back(*book);
            ++report.loaded;
        } else {
            ++report.corruptLines;
        }
    }
    return report;
}

bool RecordManager::save() const {
    // Make sure the data directory exists (e.g. after a fresh clone).
    std::filesystem::path path(m_filePath);
    if (path.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
    }

    std::ofstream file(m_filePath, std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }
    for (const Book& book : m_records) {
        file << book.toFileLine() << '\n';
    }
    return file.good();
}

bool RecordManager::addRecord(const Book& book) {
    if (idExists(book.id())) {
        return false;
    }
    m_records.push_back(book);
    return save();
}

const Book* RecordManager::searchRecordByID(int id) const {
    auto it = std::find_if(m_records.begin(), m_records.end(),
                           [id](const Book& b) { return b.id() == id; });
    return it != m_records.end() ? &*it : nullptr;
}

std::vector<const Book*> RecordManager::searchRecordByKeyword(const std::string& keyword) const {
    std::vector<const Book*> matches;
    const std::string needle = toLower(keyword);
    for (const Book& book : m_records) {
        if (toLower(book.title()).find(needle) != std::string::npos ||
            toLower(book.author()).find(needle) != std::string::npos) {
            matches.push_back(&book);
        }
    }
    return matches;
}

bool RecordManager::updateRecord(int id, const std::string& title,
                                 const std::string& author, int year) {
    auto it = std::find_if(m_records.begin(), m_records.end(),
                           [id](const Book& b) { return b.id() == id; });
    if (it == m_records.end()) {
        return false;
    }
    it->setTitle(title);
    it->setAuthor(author);
    it->setYear(year);
    return save();
}

bool RecordManager::deleteRecord(int id) {
    auto it = std::find_if(m_records.begin(), m_records.end(),
                           [id](const Book& b) { return b.id() == id; });
    if (it == m_records.end()) {
        return false;
    }
    m_records.erase(it);
    return save();
}

void RecordManager::sortRecords(SortField field, bool ascending) {
    auto less = [field](const Book& a, const Book& b) {
        switch (field) {
            case SortField::Id:     return a.id() < b.id();
            case SortField::Title:  return toLower(a.title()) < toLower(b.title());
            case SortField::Author: return toLower(a.author()) < toLower(b.author());
            case SortField::Year:   return a.year() < b.year();
        }
        return false;
    };
    std::stable_sort(m_records.begin(), m_records.end(),
                     [&](const Book& a, const Book& b) {
                         return ascending ? less(a, b) : less(b, a);
                     });
    save();
}

bool RecordManager::exportToCsv(const std::string& csvPath) const {
    std::ofstream file(csvPath, std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }
    file << "ID,Title,Author,Year\n";
    for (const Book& book : m_records) {
        file << book.toCsvRow() << '\n';
    }
    return file.good();
}

int RecordManager::nextAvailableId() const {
    int id = 1;
    while (idExists(id)) {
        ++id;
    }
    return id;
}

std::string RecordManager::toLower(const std::string& text) {
    std::string lowered = text;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return lowered;
}
