/* AMD64 entry adapters for the original C PLC mutation algorithms retained
 * in wordtech/clsplc.c. */

int C_FOpenPlc(void** hplc, int index, int delta);
int C_FInsertInPlc(void** hplc, int index, long cp, char* data);
int C_FStretchPlc(void** hplc, int delta);
int C_ShrinkPlc(void** hplc, int new_max, int index, int delta);

int N_FOpenPlc(void** hplc, int index, int delta) {
    return C_FOpenPlc(hplc, index, delta);
}

int FOpenPlc(void** hplc, int index, int delta) {
    return N_FOpenPlc(hplc, index, delta);
}

int N_FInsertInPlc(void** hplc, int index, long cp, char* data) {
    return C_FInsertInPlc(hplc, index, cp, data);
}

int FInsertInPlc(void** hplc, int index, long cp, char* data) {
    return N_FInsertInPlc(hplc, index, cp, data);
}

int N_FStretchPlc(void** hplc, int delta) {
    return C_FStretchPlc(hplc, delta);
}

int N_ShrinkPlc(void** hplc, int new_max, int index, int delta) {
    C_ShrinkPlc(hplc, new_max, index, delta);
    return 0;
}
