#include "Book.h"

#include <sstream>
#include <vector>

Book::Book(int id, std::string title, std::string author, int year)
    : m_id(id),
      m_title(std::move(title)),
      m_author(std::move(author)),
      m_year(year) {}

std::string Book::toFileLine() const {
    std::ostringstream out;
    out << m_id << SEPARATOR << m_title << SEPARATOR << m_author << SEPARATOR << m_year;
    return out.str();
}

std::optional<Book> Book::fromFileLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    std::istringstream in(line);
    while (std::getline(in, field, SEPARATOR)) {
        fields.push_back(field);
    }

    if (fields.size() != 4) {
        return std::nullopt;
    }

    try {
        int id   = std::stoi(fields[0]);
        int year = std::stoi(fields[3]);
        if (fields[1].empty() || fields[2].empty()) {
            return std::nullopt;
        }
        return Book(id, fields[1], fields[2], year);
    } catch (const std::exception&) {
        return std::nullopt; // non-numeric ID or year -> corrupt line
    }
}

std::string Book::toCsvRow() const {
    std::ostringstream out;
    out << m_id << ',' << csvEscape(m_title) << ',' << csvEscape(m_author) << ',' << m_year;
    return out.str();
}

std::string Book::csvEscape(const std::string& field) {
    bool needsQuotes = field.find_first_of(",\"\n") != std::string::npos;
    if (!needsQuotes) {
        return field;
    }
    std::string escaped = "\"";
    for (char c : field) {
        if (c == '"') escaped += '"'; // double the quote
        escaped += c;
    }
    escaped += '"';
    return escaped;
}
