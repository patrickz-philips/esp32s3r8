set(SLIDE_PLAYER_DIR ../slide_player)
file(GLOB_RECURSE SLIDE_PLAYER_SOURCES
    ${SLIDE_PLAYER_DIR}/src/*.c)

idf_component_register(
    SRCS main.c ${SLIDE_PLAYER_SOURCES}
    INCLUDE_DIRS . ${SLIDE_PLAYER_DIR}/inc)
