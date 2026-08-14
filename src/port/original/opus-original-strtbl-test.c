#include <string.h>

int IdFromSzgSz(int string_group, char *text);
int CchSzFromSzgId(char *destination, int string_group, int id, int capacity);

/* res.c owns this routine in the full engine. The focused storage test keeps
 * its behavior local so it can execute before the resource module is linked. */
int ChUpperLookup(const int character) {
    return character >= 'a' && character <= 'z' ? character - ('a' - 'A')
                                                : character;
}

int main(void) {
    enum { szgFltNormal = 1, fltAuthor = 17 };

    char keyword[] = "author";
    if (IdFromSzgSz(szgFltNormal, keyword) != fltAuthor) {
        return 1;
    }

    char output[256] = {0};
    const int count = CchSzFromSzgId(output, szgFltNormal, fltAuthor,
                                    (int)sizeof(output));
    if (count != 7 || strcmp(output, "AUTHOR") != 0) {
        return 2;
    }
    return 0;
}
