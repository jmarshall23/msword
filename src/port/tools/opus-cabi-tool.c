#include "opus_x64_compat.h"

#undef native
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir_one(path) _mkdir(path)
#else
#include <sys/stat.h>
#define mkdir_one(path) mkdir(path, 0777)
#endif

#ifndef NEAR
#define NEAR
#endif
#include "sdmver.h"
#include "sdm.h"

/* Reconstructed outputs of Microsoft's missing Dialog Editor compiler.  The
   original MKCMD parser consumes only each header's numeric cabi definition;
   compiling the contracts here lets sizeof reflect the native pointer width
   and alignment used by the x64 source modules. */
#include "about.hs"
#include "abspos.hs"
#include "apprun.hs"
#include "asgn2key.hs"
#include "asgn2mnu.hs"
#include "autosave.hs"
#include "bookmark.hs"
#include "catalog.hs"
#include "catprog.hs"
#include "catsrch.hs"
#include "char.hs"
#include "chgpr.hs"
#include "cmpfile.hs"
#include "confirmr.hs"
#include "cust.hs"
#include "doc.hs"
#include "docstat.hs"
#include "docsum.hs"
#include "edmacro.hs"
#include "edstyle.hs"
#include "footnote.hs"
#include "glsy.hs"
#include "goto.hs"
#include "header.hs"
#include "hyphen.hs"
#include "index.hs"
#include "indexent.hs"
#include "insbreak.hs"
#include "insfield.hs"
#include "insfile.hs"
#include "inspgnum.hs"
#include "inspic.hs"
#include "mrgstyle.hs"
#include "new.hs"
#include "newopen.hs"
#include "open.hs"
#include "para.hs"
#include "password.hs"
#include "pastelnk.hs"
#include "pict.hs"
#include "print.hs"
#include "printmrg.hs"
#include "prompt.hs"
#include "recorder.hs"
#include "renmacro.hs"
#include "renstyle.hs"
#include "renum.hs"
#include "replace.hs"
#include "revmark.hs"
#include "ribbon.hs"
#include "ribbon3.hs"
#include "ruler.hs"
#include "ruler3.hs"
#include "runmacro.hs"
#include "saveas.hs"
#include "search.hs"
#include "sect.hs"
#include "showvars.hs"
#include "sort.hs"
#include "spell.hs"
#include "spellmm.hs"
#include "style.hs"
#include "tablecmd.hs"
#include "tablefmt.hs"
#include "tableins.hs"
#include "tabletxt.hs"
#include "tabs.hs"
#include "thesaur.hs"
#include "toc.hs"
#include "username.hs"
#include "usrdlg.hs"
#include "viewpref.hs"
#include "vrfcnvtr.hs"

static int create_directories(const char* path) {
    char* copy;
    char* cursor;
    size_t length;
    int ok = 1;

    if (path == NULL || path[0] == '\0') return 0;
    length = strlen(path);
    copy = (char*)malloc(length + 1);
    if (copy == NULL) return 0;
    memcpy(copy, path, length + 1);

    cursor = copy;
    if (length >= 3 && copy[1] == ':' &&
        (copy[2] == '/' || copy[2] == '\\')) {
        cursor = copy + 3;
    } else if (copy[0] == '/' || copy[0] == '\\') {
        cursor = copy + 1;
    }

    for (; *cursor != '\0'; ++cursor) {
        if (*cursor == '/' || *cursor == '\\') {
            char saved = *cursor;
            *cursor = '\0';
            if (copy[0] != '\0' && mkdir_one(copy) != 0 && errno != EEXIST) {
                ok = 0;
                break;
            }
            *cursor = saved;
        }
    }
    if (ok && mkdir_one(copy) != 0 && errno != EEXIST) ok = 0;
    free(copy);
    return ok;
}

static int write_cabi(const char* directory, const char* file_name,
                      const char* macro_name, unsigned value) {
    size_t directory_length = strlen(directory);
    size_t file_length = strlen(file_name);
    int needs_separator =
        directory_length != 0 && directory[directory_length - 1] != '/' &&
        directory[directory_length - 1] != '\\';
    char* path = (char*)malloc(directory_length + (size_t)needs_separator +
                               file_length + 1);
    FILE* output;
    int ok;

    if (path == NULL) return 0;
    memcpy(path, directory, directory_length);
    if (needs_separator) path[directory_length++] = '/';
    memcpy(path + directory_length, file_name, file_length + 1);

    output = fopen(path, "w");
    free(path);
    if (output == NULL) return 0;
    fprintf(output, "#define %s %u\n", macro_name, value);
    ok = ferror(output) == 0 && fclose(output) == 0;
    return ok;
}

#define WRITE_CABI(file_name, macro_name)                                      \
    success = write_cabi(output_directory, file_name, #macro_name,             \
                         (unsigned)(macro_name)) &&                            \
              success

int main(int argc, char** argv) {
    const char* output_directory;
    int success = 1;

    if (argc != 2) return 2;
    output_directory = argv[1];
    if (!create_directories(output_directory)) return 3;

    WRITE_CABI("about.hs", cabiCABABOUT);
    WRITE_CABI("abspos.hs", cabiCABABSPOS);
    WRITE_CABI("apprun.hs", cabiCABAPPRUN);
    WRITE_CABI("asgn2key.hs", cabiCABCHANGEKEYS);
    WRITE_CABI("asgn2mnu.hs", cabiCABASSIGNTOMENU);
    WRITE_CABI("autosave.hs", cabiCABAUTOSAVE);
    WRITE_CABI("bookmark.hs", cabiCABINSBOOKMARK);
    WRITE_CABI("catalog.hs", cabiCABCATALOG);
    WRITE_CABI("catprog.hs", cabiCABCATSRHPROG);
    WRITE_CABI("catsrch.hs", cabiCABCATSEARCH);
    WRITE_CABI("char.hs", cabiCABCHARACTER);
    WRITE_CABI("chgpr.hs", cabiCABCHGPR);
    WRITE_CABI("cmpfile.hs", cabiCABCMPFILE);
    WRITE_CABI("confirmr.hs", cabiCABCONFIRMREPL);
    WRITE_CABI("cust.hs", cabiCABCUSTOMIZE);
    WRITE_CABI("doc.hs", cabiCABDOCUMENT);
    WRITE_CABI("docstat.hs", cabiCABDOCSTAT);
    WRITE_CABI("docsum.hs", cabiCABDOCSUM);
    WRITE_CABI("edmacro.hs", cabiCABEDMACRO);
    WRITE_CABI("edstyle.hs", cabiCABDEFINESTYLE);
    WRITE_CABI("footnote.hs", cabiCABINSERTFTN);
    WRITE_CABI("glsy.hs", cabiCABGLOSSARY);
    WRITE_CABI("goto.hs", cabiCABGOTO);
    WRITE_CABI("header.hs", cabiCABHEADER);
    WRITE_CABI("hyphen.hs", cabiCABHYPHEN);
    WRITE_CABI("index.hs", cabiCABINDEX);
    WRITE_CABI("indexent.hs", cabiCABINDEXENTRY);
    WRITE_CABI("insbreak.hs", cabiCABINSBREAK);
    WRITE_CABI("insfield.hs", cabiCABINSFIELD);
    WRITE_CABI("insfile.hs", cabiCABINSFILE);
    WRITE_CABI("inspgnum.hs", cabiCABINSPGNUM);
    WRITE_CABI("inspic.hs", cabiCABINSPIC);
    WRITE_CABI("mrgstyle.hs", cabiCABMERGESTYLE);
    WRITE_CABI("new.hs", cabiCABNEWDOC);
    WRITE_CABI("newopen.hs", cabiCABNEWOPEN);
    WRITE_CABI("open.hs", cabiCABOPEN);
    WRITE_CABI("para.hs", cabiCABPARALOOKS);
    WRITE_CABI("password.hs", cabiCABFILEPSWD);
    WRITE_CABI("pastelnk.hs", cabiCABPASTELINK);
    WRITE_CABI("pict.hs", cabiCABFORMATPIC);
    WRITE_CABI("print.hs", cabiCABPRINT);
    WRITE_CABI("printmrg.hs", cabiCABPRINTMERGE);
    WRITE_CABI("prompt.hs", cabiCABPROMPT);
    WRITE_CABI("recorder.hs", cabiCABRECORDER);
    WRITE_CABI("renmacro.hs", cabiCABRENMACRO);
    WRITE_CABI("renstyle.hs", cabiCABRENAMESTYLE);
    WRITE_CABI("renum.hs", cabiCABRENUMPARAS);
    WRITE_CABI("replace.hs", cabiCABREPLACE);
    WRITE_CABI("revmark.hs", cabiCABREVMARKING);
    WRITE_CABI("ribbon.hs", cabiCABRIBBON);
    WRITE_CABI("ribbon3.hs", cabiCABRIBBON3);
    WRITE_CABI("ruler.hs", cabiCABRULER);
    WRITE_CABI("ruler3.hs", cabiCABRULER3);
    WRITE_CABI("runmacro.hs", cabiCABRUNMACRO);
    WRITE_CABI("saveas.hs", cabiCABSAVE);
    WRITE_CABI("search.hs", cabiCABSEARCH);
    WRITE_CABI("sect.hs", cabiCABSECTION);
    WRITE_CABI("showvars.hs", cabiCABSHOWVARS);
    WRITE_CABI("sort.hs", cabiCABSORT);
    WRITE_CABI("spell.hs", cabiCABSPELLER);
    WRITE_CABI("spellmm.hs", cabiCABSPELLERMM);
    WRITE_CABI("style.hs", cabiCABAPPLYSTYLE);
    WRITE_CABI("tablecmd.hs", cabiCABEDITTABLE);
    WRITE_CABI("tablefmt.hs", cabiCABFORMATTABLE);
    WRITE_CABI("tableins.hs", cabiCABINSERTTABLE);
    WRITE_CABI("tabletxt.hs", cabiCABTABLETOTEXT);
    WRITE_CABI("tabs.hs", cabiCABTABS);
    WRITE_CABI("thesaur.hs", cabiCABTHESAURUS);
    WRITE_CABI("toc.hs", cabiCABTOC);
    WRITE_CABI("username.hs", cabiCABUSERNAME);
    WRITE_CABI("usrdlg.hs", cabiCABUSRDLG);
    WRITE_CABI("viewpref.hs", cabiCABVIEWPREF);
    WRITE_CABI("vrfcnvtr.hs", cabiCABEXCR);
    return success ? 0 : 4;
}
