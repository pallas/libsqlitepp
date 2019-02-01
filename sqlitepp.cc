#include "sqlitepp.h"

#include <sqlite3.h>

#include <cstring>
#include <cassert>
#include <codecvt>
#include <locale>
#include <stdexcept>

#include <lace/try.h>

namespace {

static void sqlitepp_constructor() __attribute__((constructor));
void sqlitepp_constructor() { sqlite3_initialize(); }

static void sqlitepp_destructor() __attribute__((destructor));
void sqlitepp_destructor() { sqlite3_shutdown(); }

}

sqlite::sqlite(const char * f) : _(NULL) {
    TRY(sqlite3_open_v2, f, &_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
}

sqlite::~sqlite() { sqlite3_close_v2(_); }

int
sqlite::execute(const char * sql, bool fail) {
    if (!sqlite3_exec(_, sql, NULL, NULL, NULL))
        return sqlite3_changes(_);

    if (fail) oops();
    return -1;
}

bool
sqlite::flush() {
    switch (sqlite3_db_cacheflush(_)) {
    case SQLITE_OK:
        return true;
    case SQLITE_BUSY:
        return false;
    default:
        oops();
    }
}

sqlite::statement sqlite::prepare(const char * s) { return { _, s, -1}; }
sqlite::statement sqlite::prepare(const char * s, int l) { return { _, s, l+1}; }
sqlite::statement sqlite::prepare(const std::string & s) { return { _, s.data(), (int)s.length()+1}; }

bool sqlite::autocommit() { return sqlite3_get_autocommit(_); }

sqlite::statement::statement(struct sqlite3 *db, const char *s, int l) {
    if (sqlite3_prepare_v3(db, s, l, SQLITE_PREPARE_PERSISTENT, &_, NULL))
        throw error(sqlite3_errmsg(db));
}

sqlite::statement::~statement() { sqlite3_finalize(_); }

unsigned sqlite::statement::parameters() const { return sqlite3_bind_parameter_count(_); }
int sqlite::statement::index(const char *s) const { return sqlite3_bind_parameter_index(_, s); }

unsigned sqlite::statement::columns() const { return sqlite3_column_count(_); }

int
sqlite::statement::execute() {
    while (step()) { }
    return sqlite3_changes(sqlite3_db_handle(_));
}

void sqlite::statement::reset() { if (sqlite3_reset(_)) oops(); }

bool
sqlite::statement::step() {
    switch(sqlite3_step(_)) {
    case SQLITE_ROW:
        return true;
    case SQLITE_DONE:
        reset();
        return false;
    default:
        sqlite3_reset(_);
        oops();
    }
}

sqlite::statement &
sqlite::statement::clear() {
    if (sqlite3_clear_bindings(_)) oops();
    return *this;
}

sqlite::statement &
sqlite::statement::bind(int c) {
    assert(c > 0 && c <= parameters());
    if (sqlite3_bind_null(_, c)) oops();
    return *this;
}

sqlite::statement &
sqlite::statement::bind(int c, int i) {
    assert(c > 0 && c <= parameters());
    if (sqlite3_bind_int64(_, c, i)) oops();
    return *this;
}

sqlite::statement &
sqlite::statement::bind(int c, double d) {
    assert(c > 0 && c <= parameters());
    if (sqlite3_bind_double(_, c, d)) oops();
    return *this;
}

sqlite::statement &
sqlite::statement::bind(int c, const char *s) {
    return bind(c, s, strlen(s));
}

sqlite::statement &
sqlite::statement::bind(int c, const char *s, ::size_t l) {
    assert(c > 0 && c <= parameters());
    if (sqlite3_bind_text(_, c, s, l, SQLITE_TRANSIENT)) oops();
    return *this;
}

sqlite::statement &
sqlite::statement::bind(int c, const std::string & s) {
    return bind(c, s.data(), s.length());
}

sqlite::statement &
sqlite::statement::bind(int c, const wchar_t *w) {
    std::wstring_convert<std::codecvt_utf8<wchar_t> > wc;
    return bind(c, wc.to_bytes(w));
}

sqlite::statement &
sqlite::statement::bind(int c, const wchar_t *w, ::size_t l) {
    std::wstring_convert<std::codecvt_utf8<wchar_t> > wc;
    return bind(c, wc.to_bytes(w, w + l));
}

sqlite::statement &
sqlite::statement::bind(int c, const std::wstring & w) {
    std::wstring_convert<std::codecvt_utf8<wchar_t> > wc;
    return bind(c, wc.to_bytes(w));
}

sqlite::statement &
sqlite::statement::bind(int c, const void *p, ::size_t l) {
    assert(c > 0 && c <= parameters());
    if (sqlite3_bind_blob(_, c, p, l, SQLITE_TRANSIENT)) oops();
    return *this;
}

sqlite::statement &
sqlite::statement::bind(int c, const std::vector<uint8_t> & v) {
    return bind(c, (void*)v.data(), v.size());
}

sqlite::statement & sqlite::statement::bind(const char * n) { return bind(index(n)); }
sqlite::statement & sqlite::statement::bind(const char * n, int i) { return bind(index(n), i); }
sqlite::statement & sqlite::statement::bind(const char * n, double d) { return bind(index(n), d); }
sqlite::statement & sqlite::statement::bind(const char * n, const char * s) { return bind(index(n), s); }
sqlite::statement & sqlite::statement::bind(const char * n, const char * s, ::size_t l) { return bind(index(n), s, l); }
sqlite::statement & sqlite::statement::bind(const char * n, const std::string & s) { return bind(index(n), s); }
sqlite::statement & sqlite::statement::bind(const char * n, const wchar_t * s) { return bind(index(n), s); }
sqlite::statement & sqlite::statement::bind(const char * n, const wchar_t * s, ::size_t l) { return bind(index(n), s, l); }
sqlite::statement & sqlite::statement::bind(const char * n, const std::wstring & w) { return bind(index(n), w); }
sqlite::statement & sqlite::statement::bind(const char * n, const void * p, ::size_t l) { return bind(index(n), p, l); }
sqlite::statement & sqlite::statement::bind(const char * n, const std::vector<uint8_t> & v) { return bind(index(n), v); }

const char * sqlite::statement::name(int i) const { return sqlite3_column_name(_, i); }

bool sqlite::statement::null(int c) const { return SQLITE_NULL == sqlite3_column_type(_, c); }
int64_t sqlite::statement::i(int c) const { return sqlite3_column_int64(_, c); }
double sqlite::statement::d(int c) const { return sqlite3_column_double(_, c); }

std::string
sqlite::statement::text(int c) const {
    if (const char * s = reinterpret_cast<const char *>(sqlite3_column_text(_, c)))
        return std::string(s, sqlite3_column_bytes(_, c));
    return {};
}

std::wstring
sqlite::statement::wtext(int c) const {
    std::wstring_convert<std::codecvt_utf8<wchar_t> > wc;
    if (const char * s = reinterpret_cast<const char *>(sqlite3_column_text(_, c)))
        return wc.from_bytes(s, s + sqlite3_column_bytes(_, c));
    return {};
}

std::vector<uint8_t>
sqlite::statement::blob(int c) const {
    std::vector<uint8_t> v;
    if (const uint8_t * s = reinterpret_cast<const uint8_t *>(sqlite3_column_blob(_, c))) {
        int l = sqlite3_column_bytes(_, c);
        v.reserve(l);
        v.assign(s, s + l);
    }
    return v;
}

sqlite::backup::backup(const char *to, struct sqlite3 *from, const char *name) {
    TRY(sqlite3_open_v2, to, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (!(_ = sqlite3_backup_init(db, "main", from, name))) {
        error e(sqlite3_errmsg(db));
        sqlite3_close_v2(db);
        throw e;
    }
}

sqlite::backup::backup(struct sqlite3 *to, const char *from, const char *name) {
    TRY(sqlite3_open_v2, from, &db, SQLITE_OPEN_READONLY, NULL);
    if (!(_ = sqlite3_backup_init(to, name, db, "main"))) {
        sqlite3_close_v2(db);
        throw error(sqlite3_errmsg(to));
    }
}

sqlite::backup::~backup() {
    sqlite3_backup_finish(_);
    sqlite3_close_v2(db);
}

bool
sqlite::backup::step(int n) {
    switch(int r = sqlite3_backup_step(_, n)) {
    case SQLITE_BUSY:
    case SQLITE_OK:
        return true;
    case SQLITE_DONE:
        return false;
    default:
        throw error(sqlite3_errstr(r));
    }
}

int sqlite::backup::pages() { return sqlite3_backup_pagecount(_); }
int sqlite::backup::todo() { return sqlite3_backup_remaining(_); }
int sqlite::backup::done() { return pages()-todo(); }

sqlite::backup sqlite::backup_to(const char * file, const char * name) { return { file, _, name }; }
sqlite::backup sqlite::restore_from(const char * file, const char * name) { return { _, file, name }; }

sqlite::error::error(const char * what) : std::runtime_error(what) { }

void sqlite::oops() { throw error(sqlite3_errmsg(_)); }
void sqlite::statement::oops() { throw error(sqlite3_errmsg(sqlite3_db_handle(_))); }

//
