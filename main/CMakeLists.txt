set(SLIDE_PLAYER_DIR ../slide_player)
set(SLIDE_PLAYER_SOURCES
    ${SLIDE_PLAYER_DIR}/src/slide_player.c)

idf_component_register(
    SRCS
        main.c
        ${SLIDE_PLAYER_SOURCES}
    INCLUDE_DIRS
        .
        ${SLIDE_PLAYER_DIR}/inc)
