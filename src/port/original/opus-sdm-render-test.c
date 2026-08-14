#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int OpusSdmRenderDialogPreview(unsigned short hid, unsigned int *pixels,
                               int width, int height);

enum {
    kIddSaveAs = 4,
    kIddAbout = 44,
    kPreviewWidth = 206,
    kPreviewHeight = 104
};

static uint64_t hash_pixels(const unsigned int *pixels, size_t count) {
    uint64_t hash = 1469598103934665603ull;
    for (size_t index = 0; index < count; ++index) {
        hash ^= (uint64_t)pixels[index];
        hash *= 1099511628211ull;
    }
    return hash;
}

static int count_nonwhite(const unsigned int *pixels, size_t count) {
    int total = 0;
    for (size_t index = 0; index < count; ++index) {
        total += pixels[index] != 0xffffffffu;
    }
    return total;
}

static int check_dialog(unsigned short hid, uint64_t expected_hash,
                        int minimum_nonwhite) {
    const size_t count = (size_t)kPreviewWidth * (size_t)kPreviewHeight;
    unsigned int *pixels = (unsigned int *)calloc(count, sizeof(*pixels));
    if (pixels == NULL) return 10;

    if (!OpusSdmRenderDialogPreview(hid, pixels, kPreviewWidth,
                                    kPreviewHeight)) {
        free(pixels);
        return 11;
    }

    const uint64_t hash = hash_pixels(pixels, count);
    const int nonwhite = count_nonwhite(pixels, count);
    if (hash != expected_hash || nonwhite < minimum_nonwhite) {
        fprintf(stderr,
                "hid %u hash 0x%016llx nonwhite %d, expected 0x%016llx\n",
                hid, (unsigned long long)hash, nonwhite,
                (unsigned long long)expected_hash);
        free(pixels);
        return 12;
    }

    free(pixels);
    return 0;
}

int main(void) {
    int result = check_dialog(kIddAbout, 0x04b39d022329740dull, 100);
    if (result != 0) return result;

    result = check_dialog(kIddSaveAs, 0xc3a48e26c3741edaull, 100);
    if (result != 0) return result + 10;

    return 0;
}
