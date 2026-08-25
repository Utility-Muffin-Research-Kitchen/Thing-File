#include "i18n.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>

/* TSV table loader. One bounded read at init(); after that T() is a map lookup
 * and performs no I/O. Any problem -- missing file, malformed or duplicate or
 * format-incompatible entry -- skips that entry (or that table) and falls back
 * to English: translation trouble must never keep the file manager from
 * opening. Mirrors the proven shape of Jawaka's internal/i18n loader, minus the
 * compiled-table fast path (a file-manager table is two orders of magnitude
 * smaller than the launcher's). */

namespace i18n {

namespace {

constexpr std::size_t kMaxTableBytes = 1024 * 1024;
constexpr std::size_t kMaxLanguageLen = 15;

std::map<std::string, std::string> g_entries;
bool g_loaded = false;
std::string g_language = "en";

bool language_tag_valid(const char *lang)
{
    if (!lang || !lang[0]) return false;
    for (const char *p = lang; *p; ++p) {
        if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
              (*p >= '0' && *p <= '9') || *p == '_' || *p == '-'))
            return false;
    }
    return std::strlen(lang) <= kMaxLanguageLen;
}

bool language_is_cjk(const char *lang)
{
    return lang && (std::strncmp(lang, "zh", 2) == 0 ||
                    std::strncmp(lang, "ja", 2) == 0 ||
                    std::strncmp(lang, "ko", 2) == 0);
}

/* A key may carry a "context|" prefix so identical English can translate two
 * ways. Display strips it, so an untranslated T("verb|Open") still shows
 * "Open" rather than leaking the disambiguator to the user. Only a prefix
 * without spaces counts, so a literal pipe in real UI text is left alone. */
const char *strip_context(const char *key)
{
    const char *bar = std::strchr(key, '|');
    if (!bar || bar == key) return key;
    for (const char *p = key; p < bar; ++p) {
        if (*p == ' ' || *p == '\t') return key;
    }
    return bar + 1;
}

/* printf-conversion fingerprint of a string, for validating translations of
 * format strings. A translation that turns %zu into %s would make vsnprintf
 * read the wrong argument, so a value whose conversions do not match its
 * key's -- same count, same order, same modifiers -- is refused and the
 * English format used instead. */
int fmt_sig(const char *s, char *out, std::size_t out_size)
{
    int n = 0;
    std::size_t o = 0;
    const char *p = s;
    while (*p) {
        if (*p != '%') { ++p; continue; }
        ++p;
        if (*p == '%') { ++p; continue; }   /* literal %% */
        if (!*p) return -1;                 /* trailing lone % */
        while (*p && std::strchr("-+ #0123456789.*'", *p)) ++p;
        while (*p && std::strchr("hlLqjzt", *p)) {
            if (o + 1 >= out_size) return -1;
            out[o++] = *p++;
        }
        if (!*p) return -1;
        if (o + 1 >= out_size) return -1;
        out[o++] = *p++;
        ++n;
    }
    if (o >= out_size) return -1;
    out[o] = '\0';
    return n;
}

bool fmt_compatible(const char *key, const char *val)
{
    char a[64], b[64];
    int na = fmt_sig(key, a, sizeof(a));
    int nb = fmt_sig(val, b, sizeof(b));
    if (na < 0 || nb < 0) return false;
    return na == nb && std::strcmp(a, b) == 0;
}

std::string trim_eol(std::string line)
{
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
        line.pop_back();
    return line;
}

/* Loads "english<TAB>translation" lines. '#' lines are comments. Returns false
 * and fills `why` for a file that cannot serve as a table at all; individual
 * bad rows are skipped and only counted. */
bool load_tsv(const std::string &path, std::map<std::string, std::string> &out,
              std::string &why)
{
    FILE *fp = std::fopen(path.c_str(), "rb");
    if (!fp) { why = "cannot open"; return false; }
    if (std::fseek(fp, 0, SEEK_END) != 0) { std::fclose(fp); why = "seek failed"; return false; }
    long size = std::ftell(fp);
    if (size <= 0 || (long)kMaxTableBytes < size) {
        std::fclose(fp);
        why = size <= 0 ? "empty" : "too large";
        return false;
    }
    std::rewind(fp);

    std::string text((std::size_t)size + 1, '\0');
    std::size_t got = std::fread(&text[0], 1, (std::size_t)size, fp);
    std::fclose(fp);
    text.resize(got);
    if (std::strlen(text.c_str()) != got) { why = "embedded NUL"; return false; }

    std::set<std::string> seen;
    std::size_t skipped = 0;
    std::size_t start = 0;
    while (start < text.size()) {
        std::size_t end = text.find('\n', start);
        if (end == std::string::npos) end = text.size();
        std::string line = trim_eol(text.substr(start, end - start));
        start = end + 1;
        if (line.empty() || line[0] == '#') continue;
        std::size_t tab = line.find('\t');
        if (tab == std::string::npos || tab == 0) { ++skipped; continue; }
        std::string key = line.substr(0, tab);
        std::string val = line.substr(tab + 1);
        if (!seen.insert(key).second) {
            out.erase(key);  /* ambiguous duplicate: this key falls back to English */
            ++skipped;
            continue;
        }
        if (val.empty() || val.find('\t') != std::string::npos) { ++skipped; continue; }
        if (!fmt_compatible(key.c_str(), val.c_str())) { ++skipped; continue; }
        out.emplace(key, val);
    }

    if (out.empty()) {
        out.clear();
        why = "no usable entries";
        return false;
    }
    if (skipped > 0)
        std::fprintf(stderr, "i18n: %s: skipped %zu malformed/incompatible/duplicate line(s)\n",
                     path.c_str(), skipped);
    return true;
}

} // namespace

void init(const std::string &res_dir)
{
    const char *lang = std::getenv("UMRK_LANGUAGE");
    if (!lang || !lang[0]) lang = std::getenv("JAWAKA_LANGUAGE");
    g_entries.clear();
    g_loaded = false;
    g_language = language_tag_valid(lang) ? lang : "en";
    if (g_language == "en") return;

    std::vector<std::string> candidates;
    const char *userdata = std::getenv("USERDATA_PATH");
    if (userdata && userdata[0])
        candidates.push_back(std::string(userdata) + "/Thing-File/i18n/" +
                             g_language + ".tsv");
    const char *i18n_dir = std::getenv("THING_FILE_I18N_DIR");
    if (i18n_dir && i18n_dir[0])
        candidates.push_back(std::string(i18n_dir) + "/" + g_language + ".tsv");
    if (!res_dir.empty())
        candidates.push_back(res_dir + (res_dir.back() == '/' ? "" : "/") +
                             "i18n/" + g_language + ".tsv");

    /* First valid table wins; a missing key in that table falls back to
       English rather than chaining to a later table. */
    for (const std::string &path : candidates) {
        std::map<std::string, std::string> table;
        std::string why;
        if (load_tsv(path, table, why)) {
            g_entries = std::move(table);
            g_loaded = true;
            std::fprintf(stderr, "i18n: %s loaded from %s (%zu entries)\n",
                         g_language.c_str(), path.c_str(), g_entries.size());
            return;
        }
        if (why != "cannot open" && why != "empty")
            std::fprintf(stderr, "i18n: ignoring %s: %s\n", path.c_str(), why.c_str());
    }
    std::fprintf(stderr, "i18n: no table for %s; using English\n", g_language.c_str());
    /* Unknown/unavailable language: the session is effectively English. */
    g_language = "en";
}

const char *language()
{
    return g_language.c_str();
}

bool is_cjk()
{
    return language_is_cjk(g_language.c_str());
}

const char *t(const char *english)
{
    if (english == nullptr) return "";
    if (g_loaded) {
        auto it = g_entries.find(english);
        if (it != g_entries.end()) return it->second.c_str();
    }
    return strip_context(english);
}

std::string fmt(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    va_list copy;
    va_copy(copy, args);
    int needed = std::vsnprintf(nullptr, 0, format, copy);
    va_end(copy);
    if (needed < 0) {
        va_end(args);
        return std::string(strip_context(format ? format : ""));
    }
    std::string out((std::size_t)needed + 1, '\0');
    std::vsnprintf(&out[0], out.size(), format, args);
    va_end(args);
    out.resize((std::size_t)needed);
    return out;
}

} // namespace i18n
