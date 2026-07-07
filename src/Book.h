#ifndef BOOK_H
#define BOOK_H

#include <string>
#include <optional>

/**
 * Book — the record managed by the system.
 *
 * A record is a library book identified by a unique numeric ID,
 * with a title, an author and a publication year.
 *
 * The class also knows how to serialize itself to the storage file
 * (pipe-delimited line) and to a CSV row (RFC 4180 quoting).
 */
class Book {
public:
    Book() = default;
    Book(int id, std::string title, std::string author, int year);

    // --- Accessors ---
    int                id()     const { return m_id; }
    const std::string& title()  const { return m_title; }
    const std::string& author() const { return m_author; }
    int                year()   const { return m_year; }

    // --- Mutators (used by updateRecord) ---
    void setTitle(const std::string& title)   { m_title = title; }
    void setAuthor(const std::string& author) { m_author = author; }
    void setYear(int year)                    { m_year = year; }

    // --- Serialization ---
    /// One line of the storage file: id|title|author|year
    std::string toFileLine() const;

    /// Parse a storage line; returns std::nullopt if the line is corrupt.
    static std::optional<Book> fromFileLine(const std::string& line);

    /// One CSV row with proper quoting/escaping.
    std::string toCsvRow() const;

    /// Field separator used in the storage file ('|' is rejected in user input).
    static constexpr char SEPARATOR = '|';

private:
    int         m_id    = 0;
    std::string m_title;
    std::string m_author;
    int         m_year  = 0;

    static std::string csvEscape(const std::string& field);
};

#endif // BOOK_H
