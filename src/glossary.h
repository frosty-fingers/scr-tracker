#ifndef GLOSSARY_H
#define GLOSSARY_H

// Rules glossary - this is the file to edit to add/change entries. No
// other files need to change to add one.
//
// Each entry is a term (shown in the scrollable list) plus up to
// GLOSSARY_MAX_LINES lines of definition (shown when you pick that
// term). There's no word-wrapping in code - each line is printed
// exactly as written, so definitions need to be manually broken into
// lines that fit. This keeps the code simple/robust rather than
// depending on a wrapping algorithm that could get column math wrong
// somewhere - a real risk without a local compiler to test against.
//
// LIMITS (screen is 20 columns wide, entries print starting at column
// 1, so there's a 1-column margin on each side):
//   - GLOSSARY_TERM_MAXLEN  (18) - max characters in a term name.
//   - GLOSSARY_LINE_MAXLEN  (18) - max characters per definition line.
//   - GLOSSARY_MAX_LINES    (6)  - max lines per definition. Raise
//     this (and add more "" rows to every existing entry below to
//     match) if a definition needs more room - just keep in mind more
//     lines eats into the screen's total 18 rows.
//
// HOW TO ADD ONE: add a term to glossary_terms, and a matching row to
// glossary_def_lines (same index) with up to 6 line strings - pad
// unused lines with "" (empty string, not omitted - the array must
// stay rectangular). Bump GLOSSARY_COUNT to match.
//
// Placeholder entries below are just structural examples, not real
// Sorcery: Contested Realm rules text - replace them with the actual
// glossary terms/definitions.

#define GLOSSARY_COUNT        4u
#define GLOSSARY_TERM_MAXLEN  18u
#define GLOSSARY_LINE_MAXLEN  18u
#define GLOSSARY_MAX_LINES    6u

static const char *glossary_terms[GLOSSARY_COUNT] = {
    "Airborne",
    "Stealth",
    "Lethal",
    "Ranged",
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
    // Stealth
    {
        "Example placeholder",
        "definition line two",
        "",
        "",
        "",
        "",
    },
    // Lethal
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
};

#endif // GLOSSARY_H
