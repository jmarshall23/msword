#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int OpusSdmRenderDialogPreview(unsigned short hid, unsigned int *pixels,
                               int width, int height);

int OpusSaveDocumentAsDocx(int doc, const char *path) {
    (void)doc;
    (void)path;
    return 0;
}

enum {
    kIddSaveAs = 4,
    kIddAbout = 44,
    kPreviewWidth = 206,
    kPreviewHeight = 104
};

static int count_nonwhite(const unsigned int *pixels, size_t count) {
    int total = 0;
    for (size_t index = 0; index < count; ++index) {
        total += pixels[index] != 0xffffffffu;
    }
    return total;
}

static int write_ppm(const char *path, const unsigned int *pixels,
                     size_t count) {
    FILE *file = fopen(path, "wb");
    if (file == NULL) return 20;
    if (fprintf(file, "P6\n%d %d\n255\n", kPreviewWidth, kPreviewHeight) < 0) {
        fclose(file);
        return 21;
    }
    for (size_t index = 0; index < count; ++index) {
        const unsigned int pixel = pixels[index];
        if (fputc((int)((pixel >> 16) & 0xffu), file) == EOF ||
            fputc((int)((pixel >> 8) & 0xffu), file) == EOF ||
            fputc((int)(pixel & 0xffu), file) == EOF) {
            fclose(file);
            return 22;
        }
    }
    return fclose(file) == 0 ? 0 : 23;
}

static int read_ppm(const char *path, unsigned char *rgb, size_t rgb_count) {
    int width = 0;
    int height = 0;
    int max_value = 0;
    FILE *file = fopen(path, "rb");
    if (file == NULL) return 30;
    if (fscanf(file, "P6\n%d %d\n%d\n", &width, &height, &max_value) != 3) {
        fclose(file);
        return 31;
    }
    if (width != kPreviewWidth || height != kPreviewHeight || max_value != 255) {
        fclose(file);
        return 32;
    }
    if (fread(rgb, 1, rgb_count, file) != rgb_count) {
        fclose(file);
        return 33;
    }
    if (fgetc(file) != EOF) {
        fclose(file);
        return 34;
    }
    return fclose(file) == 0 ? 0 : 35;
}

static int compare_ppm(const char *path, const unsigned int *pixels,
                       size_t count) {
    const size_t rgb_count = count * 3u;
    unsigned char *actual = (unsigned char *)malloc(rgb_count);
    unsigned char *expected = (unsigned char *)malloc(rgb_count);
    if (actual == NULL || expected == NULL) {
        free(actual);
        free(expected);
        return 40;
    }
    for (size_t index = 0; index < count; ++index) {
        const unsigned int pixel = pixels[index];
        actual[index * 3u] = (unsigned char)((pixel >> 16) & 0xffu);
        actual[index * 3u + 1u] = (unsigned char)((pixel >> 8) & 0xffu);
        actual[index * 3u + 2u] = (unsigned char)(pixel & 0xffu);
    }

    int result = read_ppm(path, expected, rgb_count);
    if (result == 0 && memcmp(actual, expected, rgb_count) != 0) {
        for (size_t index = 0; index < rgb_count; ++index) {
            if (actual[index] != expected[index]) {
                fprintf(stderr, "%s differs at rgb byte %llu: got %u want %u\n",
                        path, (unsigned long long)index, actual[index],
                        expected[index]);
                break;
            }
        }
        result = 41;
    }
    free(actual);
    free(expected);
    return result;
}

static int check_dialog(unsigned short hid, const char *reference_path,
                        int minimum_nonwhite, int write_reference) {
    const size_t count = (size_t)kPreviewWidth * (size_t)kPreviewHeight;
    unsigned int *pixels = (unsigned int *)calloc(count, sizeof(*pixels));
    if (pixels == NULL) return 10;

    if (!OpusSdmRenderDialogPreview(hid, pixels, kPreviewWidth,
                                    kPreviewHeight)) {
        free(pixels);
        return 11;
    }

    const int nonwhite = count_nonwhite(pixels, count);
    if (nonwhite < minimum_nonwhite) {
        fprintf(stderr, "hid %u nonwhite %d, expected at least %d\n", hid,
                nonwhite, minimum_nonwhite);
        free(pixels);
        return 12;
    }

    const int result = write_reference ? write_ppm(reference_path, pixels, count)
                                       : compare_ppm(reference_path, pixels, count);
    free(pixels);
    return result;
}

int main(int argc, char **argv) {
    int write_reference = 0;
    const char *about_path = NULL;
    const char *save_as_path = NULL;
    if (argc == 3) {
        about_path = argv[1];
        save_as_path = argv[2];
    } else if (argc == 4 && strcmp(argv[1], "--write-references") == 0) {
        write_reference = 1;
        about_path = argv[2];
        save_as_path = argv[3];
    } else {
        fprintf(stderr, "usage: %s [--write-references] ABOUT.ppm SAVE-AS.ppm\n",
                argv[0]);
        return 2;
    }

    int result = check_dialog(kIddAbout, about_path, 100, write_reference);
    if (result != 0) return result;

    result = check_dialog(kIddSaveAs, save_as_path, 100, write_reference);
    if (result != 0) return result + 10;

    return 0;
}
