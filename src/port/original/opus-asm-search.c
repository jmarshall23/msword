#include "opus_x64_compat.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* AMD64 translation boundary for SEARCHN.ASM and WORDGREP.ASM.
 *
 * Microsoft's portable search implementation is retained in search.c.  The
 * first four exports replace the assembly-only release entry points with
 * calls to that original C implementation.  Wordgrep is translated below as
 * a streaming regular-expression matcher over the original DOS file handle.
 */

enum GrepAtomKind {
    kGrepAtomLiteral,
    kGrepAtomAny,
    kGrepAtomNonSpace
};

typedef struct GrepAtom {
    enum GrepAtomKind kind;
    unsigned char value;
    int repeat;
} GrepAtom;

typedef struct StreamingGrep {
    GrepAtom *atoms;
    size_t atom_count;
    unsigned char *states;
    unsigned char *next_states;
    int ignore_case;
    int matched;
} StreamingGrep;

static unsigned char fold_ansi(unsigned char value) {
    char byte = (char)value;
    CharUpperBuffA(&byte, 1);
    return (unsigned char)byte;
}

static int compile_pattern(const char *pattern, GrepAtom **atoms,
                           size_t *atom_count) {
    const unsigned char *current;
    size_t capacity;
    size_t count = 0;
    GrepAtom *result;

    *atoms = NULL;
    *atom_count = 0;
    if (pattern == NULL) {
        return 1;
    }

    capacity = strlen(pattern) + 1;
    result = (GrepAtom *)calloc(capacity, sizeof(*result));
    if (result == NULL) {
        return 0;
    }

    current = (const unsigned char *)pattern;
    while (*current != 0) {
        int escaped = 0;
        unsigned char value = *current++;
        GrepAtom atom;

        if (value == '\\' && *current != 0) {
            escaped = 1;
            value = *current++;
        }

        if (!escaped && value == '*' && count != 0) {
            result[count - 1].repeat = 1;
            continue;
        }

        atom.kind = kGrepAtomLiteral;
        atom.value = value;
        atom.repeat = 0;
        if (!escaped && value == '.') {
            atom.kind = kGrepAtomAny;
        } else if (!escaped && value == '#') {
            atom.kind = kGrepAtomNonSpace;
        }
        result[count++] = atom;
    }

    *atoms = result;
    *atom_count = count;
    return 1;
}

static int atom_matches(const GrepAtom *atom, unsigned char byte,
                        int ignore_case) {
    switch (atom->kind) {
    case kGrepAtomAny:
        return 1;
    case kGrepAtomNonSpace:
        return byte != 0 && byte != '\t' && byte != '\n' && byte != '\r' &&
               byte != ' ';
    case kGrepAtomLiteral:
        return ignore_case ? fold_ansi(byte) == fold_ansi(atom->value)
                           : byte == atom->value;
    }
    return 0;
}

static void add_epsilon_closure(const GrepAtom *atoms, size_t atom_count,
                                unsigned char *states) {
    size_t index;
    for (index = 0; index < atom_count; ++index) {
        if (states[index] != 0 && atoms[index].repeat) {
            states[index + 1] = 1;
        }
    }
}

static void free_matcher(StreamingGrep *matcher) {
    free(matcher->atoms);
    free(matcher->states);
    free(matcher->next_states);
    memset(matcher, 0, sizeof(*matcher));
}

static int init_matcher(StreamingGrep *matcher, const char *pattern,
                        int ignore_case) {
    memset(matcher, 0, sizeof(*matcher));
    if (!compile_pattern(pattern, &matcher->atoms, &matcher->atom_count)) {
        return 0;
    }
    matcher->states = (unsigned char *)calloc(matcher->atom_count + 1, 1);
    matcher->next_states =
        (unsigned char *)calloc(matcher->atom_count + 1, 1);
    if (matcher->states == NULL || matcher->next_states == NULL) {
        free_matcher(matcher);
        return 0;
    }
    matcher->ignore_case = ignore_case;
    matcher->matched = matcher->atom_count == 0;
    return 1;
}

static void consume_matcher(StreamingGrep *matcher, unsigned char byte) {
    size_t index;

    if (matcher->matched) {
        return;
    }

    matcher->states[0] = 1;
    add_epsilon_closure(matcher->atoms, matcher->atom_count, matcher->states);

    memset(matcher->next_states, 0, matcher->atom_count + 1);
    for (index = 0; index < matcher->atom_count; ++index) {
        if (matcher->states[index] == 0 ||
            !atom_matches(&matcher->atoms[index], byte,
                          matcher->ignore_case)) {
            continue;
        }
        matcher->next_states[matcher->atoms[index].repeat ? index
                                                          : index + 1] = 1;
    }
    add_epsilon_closure(matcher->atoms, matcher->atom_count,
                        matcher->next_states);
    {
        unsigned char *temporary = matcher->states;
        matcher->states = matcher->next_states;
        matcher->next_states = temporary;
    }
    matcher->matched = matcher->states[matcher->atom_count] != 0;
}

static int all_matchers_matched(const StreamingGrep *matchers, int count) {
    int index;
    for (index = 0; index < count; ++index) {
        if (!matchers[index].matched) {
            return 0;
        }
    }
    return 1;
}

int32_t C_CpSearchSz(void *bmib, int32_t first, int32_t limit,
                     int32_t *next_report, void *progress_handle);
int32_t C_CpSearchSzBackward(void *bmib, int32_t first, int32_t limit);
int C_FMatchChp(void);
int C_FMatchPap(void);
int C_WCompRgchIndex(char *first, int first_count, char *second,
                     int second_count);

int CchReadDoshnd(HFILE file, void *buffer, unsigned int bytes);
long DwSeekDw(HFILE file, long offset, int origin);

int32_t N_CpSearchSz(void *bmib, int32_t first, int32_t limit,
                     int32_t *next_report, void *progress_handle) {
    return C_CpSearchSz(bmib, first, limit, next_report, progress_handle);
}

int32_t N_CpSearchSzBackward(void *bmib, int32_t first, int32_t limit) {
    return C_CpSearchSzBackward(bmib, first, limit);
}

int FMatchChp(void) { return C_FMatchChp(); }
int FMatchPap(void) { return C_FMatchPap(); }

int N_WCompRgchIndex(char *first, int first_count, char *second,
                     int second_count) {
    return C_WCompRgchIndex(first, first_count, second, second_count);
}

int wordgrep(void *memory, int memory_bytes, const char *const *strings,
             int string_count, unsigned char *results, HFILE file,
             int ignore_case, int32_t first, int32_t byte_count) {
    StreamingGrep *matchers;
    unsigned char *buffer;
    int index;
    int32_t remaining;

    if (strings == NULL || results == NULL || string_count <= 0) {
        return 0;
    }

    matchers = (StreamingGrep *)calloc((size_t)string_count,
                                       sizeof(*matchers));
    if (matchers == NULL) {
        return 0;
    }
    for (index = 0; index < string_count; ++index) {
        if (!init_matcher(&matchers[index], strings[index],
                          ignore_case != 0)) {
            while (index-- > 0) {
                free_matcher(&matchers[index]);
            }
            free(matchers);
            return 0;
        }
        if (matchers[index].matched) {
            results[index] = 1;
        }
    }

    if (memory == NULL || memory_bytes <= 0 || byte_count <= 0 ||
        DwSeekDw(file, first, FILE_BEGIN) < 0) {
        for (index = 0; index < string_count; ++index) {
            free_matcher(&matchers[index]);
        }
        free(matchers);
        return 0;
    }

    buffer = (unsigned char *)memory;
    remaining = byte_count;
    while (remaining > 0) {
        unsigned int request =
            (unsigned int)(remaining < memory_bytes ? remaining
                                                    : memory_bytes);
        int read = CchReadDoshnd(file, buffer, request);
        int byte_index;
        if (read <= 0) {
            break;
        }

        for (byte_index = 0; byte_index < read; ++byte_index) {
            for (index = 0; index < string_count; ++index) {
                if (results[index] == 0) {
                    consume_matcher(&matchers[index], buffer[byte_index]);
                    if (matchers[index].matched) {
                        results[index] = 1;
                    }
                }
            }
        }

        remaining -= read;
        if (read < (int)request ||
            all_matchers_matched(matchers, string_count)) {
            break;
        }
    }

    for (index = 0; index < string_count; ++index) {
        free_matcher(&matchers[index]);
    }
    free(matchers);
    return 0;
}
