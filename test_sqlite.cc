#include "sqlitepp.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include <cassert>
#include <algorithm>

int main(...) {
    sqlite db(":memory:");
    db.execute("CREATE TABLE IF NOT EXISTS words (word TEXT PRIMARY KEY, sort TEXT NOT NULL) WITHOUT ROWID");
    db.execute("CREATE INDEX IF NOT EXISTS sorts ON words (sort)");

    {{
        sqlite::statement && s = db.prepare("INSERT INTO words VALUES(@w,@s)");
        std::string w;
        std::ifstream f("/usr/share/dict/words");
        while (f) try {
            f >> w;
            std::transform(w.begin(), w.end(), w.begin(), tolower);
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
