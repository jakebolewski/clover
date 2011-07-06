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

EMBED_FILE(stdlib.h stdlib_h)
