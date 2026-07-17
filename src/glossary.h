#ifndef GLOSSARY_H
#define GLOSSARY_H

// Rules glossary - this is the file to edit to add/change entries. No
// other files need to change to add one.
//
// Entries are organized into pages (currently KEYWORDS and TOKENS),
// switched with LEFT/RIGHT on the glossary list screen. Each entry is
// a term (shown in the scrollable list) plus up to GLOSSARY_MAX_LINES
// lines of definition (shown when you pick that term), tagged with
// which page it belongs to. There's no word-wrapping in code - each
// line is printed exactly as written, so definitions need to be
// manually broken into lines that fit. This keeps the code simple/
// robust rather than depending on a wrapping algorithm that could get
// column math wrong somewhere - a real risk without a local compiler
// to test against.
//
// LIMITS (screen is 20 columns wide, entries print starting at column
// 1, so there's a 1-column margin on each side):
//   - GLOSSARY_TERM_MAXLEN  (17) - max characters in a term name.
//   - GLOSSARY_LINE_MAXLEN  (17) - max characters per definition line.
//   - GLOSSARY_MAX_LINES    (6)  - max lines per definition. Raise
//     this (and add more "" rows to every existing entry below to
//     match) if a definition needs more room - just keep in mind more
//     lines eats into the screen's total 18 rows.
//   - GLOSSARY_PAGE_NAME_MAXLEN (16) - max characters in a page name.
//
// HOW TO ADD AN ENTRY: add a term to glossary_terms, a matching row to
// glossary_def_lines (same index, up to 6 line strings, pad unused
// lines with "" - the array must stay rectangular), and a matching
// page number to glossary_page (same index again - PAGE_KEYWORDS or
// PAGE_TOKENS, or a new page you've added below). Bump GLOSSARY_COUNT
// to match. Entries don't need to be grouped together by page in these
// arrays - they get filtered/sorted onto the right page automatically
// at display time.
//
// HOW TO ADD A NEW PAGE: add a PAGE_* constant, add its name to
// glossary_page_names, and bump GLOSSARY_PAGE_COUNT. No other code
// needs to change - main.c's page navigation works off these counts.
// Every page needs at least one entry - an empty page is a real edge
// case the navigation code doesn't specifically handle well.
//
// Placeholder entries below are just structural examples, not real
// Sorcery: Contested Realm rules text - replace them with the actual
// glossary terms/definitions.

#define PAGE_KEYWORDS  0u
#define PAGE_TOKENS    1u
#define GLOSSARY_PAGE_COUNT  2u

static const char *glossary_page_names[GLOSSARY_PAGE_COUNT] = {
    "KEYWORDS",
    "TOKENS",
};

#define GLOSSARY_COUNT              6u
#define GLOSSARY_TERM_MAXLEN        17u
#define GLOSSARY_LINE_MAXLEN        17u
#define GLOSSARY_MAX_LINES          6u
#define GLOSSARY_PAGE_NAME_MAXLEN   16u

static const char *glossary_terms[GLOSSARY_COUNT] = {
    "Airborne",
    "Burn",
    "Deathtouch",
    "Ranged",
    "Ash Token",
    "Wound Token",
};

static const uint8_t glossary_page[GLOSSARY_COUNT] = {
    PAGE_KEYWORDS,  // Airborne
    PAGE_KEYWORDS,  // Burn
    PAGE_KEYWORDS,  // Deathtouch
    PAGE_KEYWORDS,  // Ranged
    PAGE_TOKENS,    // Ash Token
    PAGE_TOKENS,    // Wound Token
};

static const char *glossary_def_lines[GLOSSARY_COUNT][GLOSSARY_MAX_LINES] = {
    // Airborne
    {
        "Example placeholder",
        "text - replace with",
        "the real rule text",
        "for this term.",
        "",
        "",
    },
    // Burn
    {
        "Example placeholder",
        "definition line two",
        "",
        "",
        "",
        "",
    },
    // Deathtouch
    {
        "Example placeholder",
        "spanning a couple",
        "of lines to show",
        "how longer entries",
        "can look.",
        "",
    },
    // Ranged
    {
        "Example placeholder",
        "",
        "",
        "",
        "",
        "",
    },
    // Ash Token
    {
        "Example placeholder",
        "token entry - swap",
        "for real token rules",
        "",
        "",
        "",
    },
    // Wound Token
    {
        "Example placeholder",
        "token entry.",
        "",
        "",
        "",
        "",
    },
};

#endif // GLOSSARY_H
