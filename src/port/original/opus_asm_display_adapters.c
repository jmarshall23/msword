/* AMD64 adapters for display assembly entry points whose original C bodies
 * are compiled from disp1.c. */

typedef struct OpusDisplayRect OpusDisplayRect;
int C_PatBltRc(void* hdc, OpusDisplayRect* rectangle, long raster_operation);

int PatBltRc(void* hdc, OpusDisplayRect* rectangle, long raster_operation) {
    C_PatBltRc(hdc, rectangle, raster_operation);
    return 0;
}
