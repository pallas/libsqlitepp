#include "sqlitepp.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cwctype>
#include <codecvt>
#include <locale>
#include <cassert>
#include <algorithm>

int main(...) {
    sqlite db(":memory:");
    db.execute("CREATE TABLE IF NOT EXISTS words (word TEXT PRIMARY KEY, sort TEXT NOT NULL) WITHOUT ROWID");
    db.execute("CREATE INDEX IF NOT EXISTS sorts ON words (sort)");

    {{
        sqlite::statement && s = db.prepare("INSERT INTO words VALUES(@w,@s)");
        std::ifstream f("/usr/share/dict/words");
        while (f) try {
            std::string t;
            f >> t;
            std::wstring_convert<std::codecvt_utf8<wchar_t> > wc;
            std::wstring w = wc.from_bytes(t);
            std::transform(w.begin(), w.end(), w.begin(), towlower);
            s.bind(1, w);
            std::sort(w.begin(), w.end());
            s.bind(2, w);
            s.execute();
        } catch (std::exception &e) { }
    }}

    db.backup_to("words.db").step();

    {{
        sqlite::statement && s = db.prepare("SELECT word FROM words WHERE sort=(SELECT sort FROM words WHERE word = ?)");
        s.bind(1, "pots");

        assert(0 == strncmp(s.name(0)));
        while (s.step())
            std::cout << s.text(0) << std::endl;
    }}

    return EXIT_SUCCESS;
}

//
