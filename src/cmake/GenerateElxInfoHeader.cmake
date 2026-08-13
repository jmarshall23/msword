# GenerateElxInfoHeader.cmake
#
# Reconstruye elxinfo.h extrayendo la carga de pantalla oculta directamente del
# MERGEELX original de Microsoft (OpusEtAl/tools/src/mergeelx.c).
#
# Sustituye a cmake/GenerateElxStid.ps1, que el arbol publicado referencia
# (CMakeLists.txt:81-84) pero nunca versiono. Sin PowerShell.
#
# Que emite MERGEELX
# ------------------
# WriteElxInfo() (mergeelx.c:467-694) escribe ELXINFO.H en dos bloques con
# guardas independientes:
#
#   #ifdef elkAppMac ... #endif   tablas de dialogos EL: rgeldi[], rgichName[],
#                                 rgchElkNames[], mpelkistName[]
#   #ifdef STID      ... #endif   pantalla oculta: rgksp[], mpstiderc[],
#                                 rgstid[], rgrgb[]
#
# Solo el segundo bloque es reproducible desde el archivo. El primero depende de
# los .elx intermedios que producia el Dialog Editor, ejecutable ausente del
# archivo; src/Opus/dlg/ conserva los 86 .des de origen pero ningun .elx. Ese es
# el limite que documenta src/port/original/elxinfo.h, tomado aqui como
# referencia de formato: se conserva la guarda #ifdef elkAppMac vacia e inerte.
#
# Opus/elxprocs.c (incluido por Opus/elsubs.c) referencia rgeldi, rgchElkNames,
# rgichName y mpelkistName. Para que esa unidad compile, la guarda elkAppMac
# emite esas tablas con las dimensiones que fija src/port/original/elxdefs.h y
# con contenido cero. No se inventan datos de dialogo: se materializa la
# frontera inerte que el archivo ya documenta.
#
# El bloque STID lo consume Opus/eldlg.c, que define STID en la linea 58 e
# incluye elxinfo.h en la 60. CheckRgksp() (eldlg.c:878-...) compara la tecla
# pulsada con rgksp[0] y exige que rgksp[1..sizeof-2] esten pulsadas, luego
# descifra mpstiderc con XOR '9' e indexa por rgstid.
#
# Uso:
#   cmake -DINPUT=<ruta>/mergeelx.c -DOUTPUT=<ruta>/elxinfo.h \
#         -P GenerateElxInfoHeader.cmake

cmake_minimum_required(VERSION 3.25)

if(NOT DEFINED INPUT)
    message(FATAL_ERROR "GenerateElxInfoHeader: falta -DINPUT=<ruta a mergeelx.c>")
endif()
if(NOT DEFINED OUTPUT)
    message(FATAL_ERROR "GenerateElxInfoHeader: falta -DOUTPUT=<ruta a elxinfo.h>")
endif()
if(NOT EXISTS "${INPUT}")
    message(FATAL_ERROR "GenerateElxInfoHeader: no existe el archivo de entrada: ${INPUT}")
endif()

file(READ "${INPUT}" opus_raw)

set(OPUS_SEMI_TOKEN "@@OPUS_SEMICOLON@@")
set(OPUS_BSLASH_TOKEN "@@OPUS_BACKSLASH@@")

string(REPLACE ";" "${OPUS_SEMI_TOKEN}" opus_raw "${opus_raw}")
string(REPLACE "\r\n" "\n" opus_raw "${opus_raw}")
string(REPLACE "\r" "\n" opus_raw "${opus_raw}")
string(REPLACE "\n" ";" opus_lines "${opus_raw}")

set(opus_payload "")
set(opus_state "buscando")
set(opus_emitted 0)

foreach(opus_line IN LISTS opus_lines)
    if(opus_state STREQUAL "buscando")
        # mergeelx.c:589 cierra el bloque elkAppMac y abre el bloque STID en la
        # misma llamada. No se copia: aqui se reconstruyen ambas guardas.
        if(opus_line MATCHES "#ifdef STID")
            set(opus_state "copiando")
        endif()
        continue()
    endif()

    # Todas las llamadas de la region STID son de una sola linea y de la forma
    #   fprintf(pfile, "<literal>");
    if(NOT opus_line MATCHES "^[ \t]*fprintf\\(pfile, \"(.*)\"\\)${OPUS_SEMI_TOKEN}[ \t]*$")
        # Lineas en blanco intercaladas dentro de la region: no aportan salida.
        if(opus_line MATCHES "^[ \t]*$")
            continue()
        endif()
        message(FATAL_ERROR
            "GenerateElxInfoHeader: dentro del bloque STID de ${INPUT} hay una "
            "construccion no reconocida. La extraccion supone llamadas fprintf "
            "de una sola linea.\n  Linea: ${opus_line}")
    endif()
    set(opus_text "${CMAKE_MATCH_1}")

    # Desescapado del literal C. La barra invertida se aparta primero para que su
    # restauracion no vuelva a interpretarse.
    string(REPLACE "\\\\" "${OPUS_BSLASH_TOKEN}" opus_text "${opus_text}")
    string(REPLACE "\\n" "\n" opus_text "${opus_text}")
    string(REPLACE "\\t" "\t" opus_text "${opus_text}")
    string(REPLACE "\\\"" "\"" opus_text "${opus_text}")
    string(REPLACE "${OPUS_BSLASH_TOKEN}" "\\" opus_text "${opus_text}")

    # Fin del bloque STID.
    if(opus_text STREQUAL "#endif\n")
        set(opus_state "terminado")
        break()
    endif()

    # StringMap era una extension del compilador C de 16 bits de Microsoft que
    # resolvia la etapa de recursos de cadenas; no es expresable como
    # inicializador estatico en C nativo. Opus/wordtech/word.h:274-278 documenta
    # su firma: el segundo argumento 0 pide un sz (terminado en NUL) y 1 pide un
    # st (con byte de cuenta al frente).
    #
    # Aqui solo aparece la forma sz. Se traduce a literal, igual que hizo el
    # autor del port en Opus/renum.c:77, Opus/FILE2.C:560 y Opus/automcr.h:15
    # para el mismo problema.
    #
    # eldlg.c:890-893 confirma la forma sz: rgksp[0] es la tecla disparadora y
    # el bucle recorre rgksp[1] .. rgksp[sizeof-2], es decir los caracteres
    # restantes sin contar el NUL final.
    if(opus_text MATCHES "StringMap")
        if(NOT opus_text MATCHES "StringMap\\(\"([^\"]*)\",[ \t]*0,[ \t]*[0-9]+\\)")
            message(FATAL_ERROR
                "GenerateElxInfoHeader: uso de StringMap no soportado en el "
                "bloque STID de ${INPUT}. Solo se traduce la forma sz "
                "(segundo argumento 0); la forma st requiere byte de cuenta.\n"
                "  Texto: ${opus_text}")
        endif()
        set(opus_literal "${CMAKE_MATCH_1}")
        string(REGEX REPLACE
            "StringMap\\(\"[^\"]*\",[ \t]*0,[ \t]*[0-9]+\\)"
            "\"${opus_literal}\""
            opus_text "${opus_text}")
    endif()

    string(APPEND opus_payload "${opus_text}")
    math(EXPR opus_emitted "${opus_emitted} + 1")
endforeach()

if(NOT opus_state STREQUAL "terminado")
    message(FATAL_ERROR
        "GenerateElxInfoHeader: no se localizo un bloque STID completo en "
        "${INPUT} (estado final: ${opus_state}). Se esperaba el ancla "
        "'#ifdef STID' seguida de fprintf's y un '#endif' de cierre.")
endif()

# Comprobacion de integridad: los cuatro simbolos que Opus/eldlg.c espera.
foreach(opus_symbol rgksp mpstiderc rgstid rgrgb)
    if(NOT opus_payload MATCHES "${opus_symbol}")
        message(FATAL_ERROR
            "GenerateElxInfoHeader: la carga extraida de ${INPUT} no define "
            "'${opus_symbol}', que Opus/eldlg.c necesita.")
    endif()
endforeach()

set(opus_header
"/* Archivo generado por src/cmake/GenerateElxInfoHeader.cmake. No editar. */
/* Fuente: la carga de pantalla oculta del MERGEELX original de Microsoft.  */

#pragma once

/*
 * MERGEELX emite el cuerpo de las tablas de dialogos EL solo cuando elxdefs.h
 * ha definido elkAppMac. Esas tablas se derivan de los .elx intermedios que
 * producia el Dialog Editor, ejecutable que no forma parte del archivo, de modo
 * que aqui la guarda queda deliberadamente vacia: el mismo limite inerte que
 * documenta src/port/original/elxinfo.h.
 *
 * Opus/eldlg.c incluye elxinfo.h sin elxdefs.h y con STID definido, asi que solo
 * recibe el bloque de abajo.
 */

#ifdef elkAppMac
/*
 * Tablas EL neutras. Opus/elxprocs.c (incluido por Opus/elsubs.c) referencia
 * rgeldi, rgichName, rgchElkNames y mpelkistName; sin ellas la unidad no
 * compila. Aqui se declaran con las dimensiones que fija
 * src/port/original/elxdefs.h y con contenido cero: no se inventan datos de
 * dialogo, se materializa la frontera inerte.
 *
 * ELDI termina en un miembro flexible (ELFD rgelfd[], Opus/el.h:314), que no
 * puede inicializarse dentro de un arreglo en C nativo. Se rodea con un
 * registro de cabecera de igual disposicion y una macro que lo presenta como
 * ELDI *, tal como resolvio el autor del port en la version PowerShell de este
 * generador (jmarshall23/msword PR #3, cmake/GenerateElxStid.ps1).
 */
typedef struct _OPUS_X64_EMPTY_ELDI
	{
	HID hid;
	CABI cabi;
	unsigned celfd;
	} OPUS_X64_EMPTY_ELDI;
static csconst OPUS_X64_EMPTY_ELDI vopusX64EmptyEldi = {0};
#define rgeldi ((ELDI *)&vopusX64EmptyEldi)
csconst unsigned rgichName[ibstElkAppMac] = {0};
csconst unsigned char rgchElkNames[1] = {0};
csconst unsigned mpelkistName[elkAppMac] = {0};
#endif

#ifdef STID
${opus_payload}#endif
")

string(REPLACE "${OPUS_SEMI_TOKEN}" ";" opus_header "${opus_header}")

get_filename_component(opus_outdir "${OUTPUT}" DIRECTORY)
if(opus_outdir)
    file(MAKE_DIRECTORY "${opus_outdir}")
endif()

set(opus_previous "")
if(EXISTS "${OUTPUT}")
    file(READ "${OUTPUT}" opus_previous)
endif()
if(NOT opus_previous STREQUAL opus_header)
    file(WRITE "${OUTPUT}" "${opus_header}")
endif()

message(STATUS
    "GenerateElxInfoHeader: ${opus_emitted} lineas de carga STID -> ${OUTPUT}")
