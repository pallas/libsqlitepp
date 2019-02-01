#ifndef LACE__SQLITEPP_H
#define LACE__SQLITEPP_H

#include <lace/do_not_copy.h>
#include <lace/singleton.h>

#include <string>
#include <vector>
#include <stdexcept>

struct sqlite3;
struct sqlite3_stmt;

class sqlite : public lace::do_not_copy {
public:
    sqlite(const char * = "");
    ~sqlite();

    struct type { enum type_t { unknown, i, d, text, blob, null }; };

    class statement : public lace::do_not_copy {
        statement(struct sqlite3 *, const char *, int);
    public:
        ~statement();

        unsigned parameters() const;
        int index(const char *) const;

        unsigned columns() const;
        int execute();
        void reset();
        bool step();

        statement & clear();
        statement & bind(int);
        statement & bind(int, int);
        statement & bind(int, double);
        statement & bind(int, const char *);
        statement & bind(int, const char *, ::size_t);
        statement & bind(int, const std::string &);
        statement & bind(int, const wchar_t *);
        statement & bind(int, const wchar_t *, ::size_t);
        statement & bind(int, const std::wstring &);
        statement & bind(int, const void *, ::size_t);
        statement & bind(int, const std::vector<uint8_t> &);

        statement & bind(const char *);
        statement & bind(const char *, int);
        statement & bind(const char *, double);
        statement & bind(const char *, const char *);
        statement & bind(const char *, const char *, ::size_t);
        statement & bind(const char *, const std::string &);
        statement & bind(const char *, const wchar_t *);
        statement & bind(const char *, const wchar_t *, ::size_t);
        statement & bind(const char *, const std::wstring &);
        statement & bind(const char *, const void *, ::size_t);
        statement & bind(const char *, const std::vector<uint8_t> &);

        const char * name(int) const;

        sqlite::type::type_t type(int) const;

        bool is_null(int) const;
        bool is_i(int) const;
        bool is_d(int) const;
        bool is_text(int) const;
        bool is_blob(int) const;

        int64_t i(int) const;
        double d(int) const;

        std::string text(int) const;
        std::wstring wtext(int) const;
        std::vector<uint8_t> blob(int) const;

    private:
        struct sqlite3_stmt *_;
        friend class sqlite;
        [[ noreturn ]] void oops();
    };

    int execute(const char *, bool fail = true);
    bool flush();

    statement prepare(const char *);
    statement prepare(const char *, int);
    statement prepare(const std::string &);

    class backup {
        backup(const char *, struct sqlite3 *, const char *);
        backup(struct sqlite3 *, const char *, const char *);
    public:
        ~backup();

        bool step(int = -1);

        int pages();
        int todo();
        int done();

    private:
        struct sqlite3_backup *_;
        struct sqlite3 *db;
        friend class sqlite;
        [[ noreturn ]] void oops();
    };

    backup backup_to(const char *, const char * = "main");
    backup restore_from(const char *, const char * = "main");

    bool autocommit();

    class error : public std::runtime_error {
    public:
        error(const char *);
    };

private:
    struct sqlite3 *_;
    [[ noreturn ]] void oops();
};

#endif//LACE__SQLITEPP_H
