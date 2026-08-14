/* AMD64 entry adapters for the three FETchn3.ASM routines whose complete
 * reference implementations remain in wordtech/fetch.c. */
int C_CacheParaCa(void* range);
int C_SetDirty(int buffer_page);
int C_FAbsPap(int document, void* paragraph_properties);

void CacheParaCa(void* range) {
    (void)C_CacheParaCa(range);
}

void SetDirty(const int buffer_page) {
    (void)C_SetDirty(buffer_page);
}

int FAbsPap(const int document, void* paragraph_properties) {
    return C_FAbsPap(document, paragraph_properties);
}
