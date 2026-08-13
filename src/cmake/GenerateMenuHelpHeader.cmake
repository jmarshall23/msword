# GenerateMenuHelpHeader.cmake
#
# Reconstruye menuhelp.h a partir de MENUHELP.TXT, que produce el MKCMD original
# de Microsoft (OpusEtAl/tools/src/mkcmd.c:829-918).
#
# En la construccion historica, MENUHELP.TXT pasaba por STRINGPP, que emitia una
# tabla tokenizada rgsz[] consumida por Opus/menuhelp.c. El port a x64 sustituyo
# esa etapa: menuhelp.c:287 usa OPUS_X64_MENU_HELP_STRING(hpsy->iidstr) y espera
# que esta cabecera la defina.
#
# Contrato de indices, tomado de mkcmd.c:
#   - IszAddHelp()/FLookupHelp() (mkcmd.c:2923-2960) numeran cada cadena de ayuda
#     por su posicion 0-based en la lista enlazada vphsdBase.
#   - psy->u.scmd.iidstr = iszHelp (mkcmd.c:2309) guarda ese indice.
#   - El volcado de MENUHELP.TXT (mkcmd.c:902-916) recorre vphsdBase en ese mismo
#     orden y emite una linea "x,\t\"...\"" por entrada.
# Por lo tanto la n-esima linea de datos corresponde exactamente a iidstr == n-1.
# Cualquier linea descartada en silencio desplazaria todos los indices siguientes,
# de modo que este script trata como error fatal toda linea que no sea ni un
# comentario ';' ni una entrada bien formada.
#
# Uso:
#   cmake -DINPUT=<ruta>/MENUHELP.TXT -DOUTPUT=<ruta>/menuhelp.h \
#         -P GenerateMenuHelpHeader.cmake

cmake_minimum_required(VERSION 3.25)

if(NOT DEFINED INPUT)
    message(FATAL_ERROR "GenerateMenuHelpHeader: falta -DINPUT=<ruta a MENUHELP.TXT>")
endif()
if(NOT DEFINED OUTPUT)
    message(FATAL_ERROR "GenerateMenuHelpHeader: falta -DOUTPUT=<ruta a menuhelp.h>")
endif()
if(NOT EXISTS "${INPUT}")
    message(FATAL_ERROR "GenerateMenuHelpHeader: no existe el archivo de entrada: ${INPUT}")
endif()

file(READ "${INPUT}" opus_raw)

# CMake usa ';' como separador de listas y MENUHELP.TXT contiene ';' tanto en sus
# lineas de comentario como dentro de las cadenas de ayuda. Se protege antes de
# convertir el contenido en una lista de lineas y se restaura al escribir.
set(OPUS_SEMI_TOKEN "@@OPUS_SEMICOLON@@")
string(REPLACE ";" "${OPUS_SEMI_TOKEN}" opus_raw "${opus_raw}")

# MKCMD abre MENUHELP.TXT en modo texto: CRLF en Windows, LF en Linux.
string(REPLACE "\r\n" "\n" opus_raw "${opus_raw}")
string(REPLACE "\r" "\n" opus_raw "${opus_raw}")
string(REPLACE "\n" ";" opus_lines "${opus_raw}")

set(opus_entries "")
set(opus_count 0)
set(opus_lineno 0)

foreach(opus_line IN LISTS opus_lines)
    math(EXPR opus_lineno "${opus_lineno} + 1")

    if(opus_line STREQUAL "")
        continue()
    endif()

    # Lineas de encabezado de MKCMD: ';' ya sustituido por el token protector.
    if(opus_line MATCHES "^${OPUS_SEMI_TOKEN}")
        continue()
    endif()

    # mkcmd.c:915 -> fprintf(pflMH, "x,\t\"%s\"\n", phsd->sz)
    # El 'x' es el nombre del identificador de cadena, que MKCMD documenta como
    # ignorado (mkcmd.c:911-914).
    if(NOT opus_line MATCHES "^[^,]*,[ \t]*\"(.*)\"[ \t]*$")
        message(FATAL_ERROR
            "GenerateMenuHelpHeader: linea ${opus_lineno} de ${INPUT} con formato "
            "inesperado. Descartarla desplazaria los indices iidstr posteriores.\n"
            "  Linea: ${opus_line}")
    endif()
    set(opus_text "${CMAKE_MATCH_1}")

    # Escapado defensivo hacia literal C. MKCMD no escapa su '%s', de modo que un
    # '\' o un '"' en el .cmd de origen llegaria crudo hasta aqui.
    string(REPLACE "\\" "\\\\" opus_text "${opus_text}")
    string(REPLACE "\"" "\\\"" opus_text "${opus_text}")

    string(APPEND opus_entries "\t/* ${opus_count} */ \"${opus_text}\",\n")
    math(EXPR opus_count "${opus_count} + 1")
endforeach()

if(opus_count EQUAL 0)
    message(FATAL_ERROR
        "GenerateMenuHelpHeader: ${INPUT} no contiene ninguna cadena de ayuda.")
endif()

set(opus_header
"/* Archivo generado por src/cmake/GenerateMenuHelpHeader.cmake. No editar. */
/* Fuente: MENUHELP.TXT, producido por el MKCMD original de Microsoft.       */

#pragma once

/*
 * Opus/menuhelp.c:287 resuelve el texto de ayuda de la barra de menus con
 * OPUS_X64_MENU_HELP_STRING(hpsy->iidstr) y lo pasa a
 * CchCopySz(const CHAR *, CHAR *) (port/original/opus_x64_compat.h:367).
 *
 * El indice es el iidstr de 9 bits de la tabla de simbolos (Opus/cmdtbl.h:189
 * define iidstrNil como 0x1ff). menuhelp.c descarta iidstrNil antes de indexar,
 * pero la macro comprueba el rango de todos modos: un iidstr fuera de tabla
 * devuelve la cadena vacia en lugar de leer memoria ajena.
 */

#ifndef CODE
#define CODE csconst
#endif

CODE CHAR * CODE rgszMenuHelp [] =
	{
${opus_entries}	};

#define OPUS_X64_MENU_HELP_STRING_MAC \\
	((unsigned)(sizeof(rgszMenuHelp) / sizeof(rgszMenuHelp[0])))
#define OPUS_X64_MENU_HELP_STRING(iidstr) \\
	((unsigned)(iidstr) < OPUS_X64_MENU_HELP_STRING_MAC \\
		? rgszMenuHelp[(unsigned)(iidstr)] : (CODE CHAR *)\"\")
")

string(REPLACE "${OPUS_SEMI_TOKEN}" ";" opus_header "${opus_header}")

get_filename_component(opus_outdir "${OUTPUT}" DIRECTORY)
if(opus_outdir)
    file(MAKE_DIRECTORY "${opus_outdir}")
endif()

# Escritura idempotente: no tocar la marca de tiempo si el contenido no cambio,
# para no forzar recompilaciones de los 58 modulos que incluyen opuscmd.h.
set(opus_previous "")
if(EXISTS "${OUTPUT}")
    file(READ "${OUTPUT}" opus_previous)
endif()
if(NOT opus_previous STREQUAL opus_header)
    file(WRITE "${OUTPUT}" "${opus_header}")
endif()

message(STATUS
    "GenerateMenuHelpHeader: ${opus_count} cadenas de ayuda -> ${OUTPUT}")
