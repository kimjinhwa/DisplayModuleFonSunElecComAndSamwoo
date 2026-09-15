SET(SOURCES screens/ui_MainScreen.c
    screens/ui_SettingScreen.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    ui_events.c
    fonts/ui_font_arial16.c
    fonts/ui_font_arial28.c
    fonts/ui_font_malgunFont1.c)

add_library(ui ${SOURCES})
