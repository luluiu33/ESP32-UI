idf_component_register(SRCS "main.c"
                             "drivers/ssd1306.c" "drivers/font_6x8.c"
                             "drivers/display.c" "drivers/lcd_st7735.c"
                             "ui/ui.c" "ui/ui_common.c" "ui/menu.c" "ui/about.c"
                             "input/input.c" "input/calibrate.c"
                             "tests/test_ui.c" "tests/circle_test.c"
                             "tests/display_test.c"
                      INCLUDE_DIRS "." "drivers" "ui" "input" "tests")

add_custom_command(TARGET ${COMPONENT_LIB} PRE_BUILD
    COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/increment_build.py
    COMMENT "Auto-increment FW_BUILD"
)