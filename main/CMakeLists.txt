idf_component_register(SRCS "main.c"
                            "drivers/ssd1306.c" "drivers/font_6x8.c"
                            "ui/ui.c" "ui/ui_common.c" "ui/menu.c" "ui/about.c"
                            "input/input.c" "input/calibrate.c"
                            "tests/test_ui.c" "tests/circle_test.c"
                            "tests/display_test.c"
                            "lcd_st7735.c" "lcd_ui.c"
                     INCLUDE_DIRS "." "drivers" "ui" "input" "tests")