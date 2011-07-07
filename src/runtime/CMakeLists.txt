macro(EMBED_FILE INFILE INNAME)
    add_custom_command(
        OUTPUT ${INFILE}.embed.h
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/embed.py
                ${CMAKE_CURRENT_SOURCE_DIR}/${INFILE}
                ${CMAKE_CURRENT_BINARY_DIR}/${INFILE}.embed.h
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/embed.py
                ${CMAKE_CURRENT_SOURCE_DIR}/${INFILE})

    add_custom_target(generate_${INNAME} DEPENDS
                      ${CMAKE_CURRENT_BINARY_DIR}/${INFILE}.embed.h)
endmacro(EMBED_FILE)

macro(COMPILE_BITCODE INFILE INNAME)
    add_custom_command(
        OUTPUT ${INFILE}.bc
        COMMAND clang -c -emit-llvm -x cl -O2
                ${CMAKE_CURRENT_SOURCE_DIR}/${INFILE}
                -o ${CMAKE_CURRENT_BINARY_DIR}/${INFILE}.bc
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${INFILE})

    add_custom_command(
        OUTPUT ${INFILE}.bc.embed.h
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/embed.py
                ${CMAKE_CURRENT_BINARY_DIR}/${INFILE}.bc
                ${CMAKE_CURRENT_BINARY_DIR}/${INFILE}.bc.embed.h
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/embed.py
                ${CMAKE_CURRENT_BINARY_DIR}/${INFILE}.bc)

    add_custom_target(generate_${INNAME} DEPENDS
                      ${CMAKE_CURRENT_BINARY_DIR}/${INFILE}.bc.embed.h)
endmacro(COMPILE_BITCODE)

EMBED_FILE(stdlib.h stdlib_h)
COMPILE_BITCODE(stdlib.c stdlib_c)
