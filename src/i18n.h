#ifndef THING_FILE_I18N_H_
#define THING_FILE_I18N_H_

#include <string>

/* User-facing string lookup.
 *
 * The English string is the key. T("System:") returns the translation when one
 * is loaded and a pointer into the literal itself otherwise, so a missing or
 * partial translation degrades to English with no call-site handling.
 *
 * Identical English needing different translations takes a context prefix that
 * is stripped from the displayed text on fallback: T("verb|Open") and
 * T("noun|Open") are distinct keys, both rendering "Open" when untranslated.
 * Only a "token|Text" prefix -- non-empty token, no spaces or tabs -- is
 * stripped, so a literal pipe in real UI text stays visible.
 *
 * Wrap ONLY user-facing text. Log messages, paths, config keys, shell command
 * names, and file extensions stay unwrapped.
 *
 * init() is called once before CResourceManager loads fonts (the font stack
 * depends on the language); after that the module performs no I/O, so T() is
 * safe on the render path.
 */

#define T(s) (::i18n::t(s))

namespace i18n {

/* Resolve the language and load the first valid table. Search order:
 *   1. $USERDATA_PATH/Thing-File/i18n/<lang>.tsv   (live reviewer override)
 *   2. $THING_FILE_I18N_DIR/<lang>.tsv             (native dev build dir)
 *   3. <res_dir>/i18n/<lang>.tsv                   (packaged table)
 * The language is $UMRK_LANGUAGE, then $JAWAKA_LANGUAGE; missing, empty, or
 * unknown means "en", which loads nothing. Any table problem is logged once
 * and falls back to English; localization can never block startup.
 */
void init(const std::string &res_dir);

/* The language actually in use, "en" when no table loaded. */
const char *language();

/* True when the active language should get the CJK face first in the font
 * stack. */
bool is_cjk();

/* Translate `english`. Never returns NULL for a non-NULL argument; returns the
 * (context-stripped) English key on any miss. The pointer is owned by the
 * table or the call-site literal and stays valid for the process lifetime. */
const char *t(const char *english);

/* vsnprintf-style formatting of an already-translated format string, for the
 * handful of messages with embedded values. Call sites keep the whole message
 * (format included) as one key: i18n::fmt(T("%zu selected:"), n). */
std::string fmt(const char *format, ...);

} // namespace i18n

#endif // THING_FILE_I18N_H_
