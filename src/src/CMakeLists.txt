SET(SOURCES screens/ui_MainScreen.c
    screens/ui_SettingScreen.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    ui_events.c
    fonts/ui_font_Arial.c
    fonts/ui_font_arial14.c
    fonts/ui_font_arial16.c
    fonts/ui_font_arial26.c
    fonts/ui_font_arial28.c
    fonts/ui_font_Font1.c
    fonts/ui_font_Greek.c
    fonts/ui_font_malgunFont1.c
    fonts/ui_font_montserrat16.c
    fonts/ui_font_montserrat26.c
    fonts/ui_font_montserrat28.c)

add_library(ui ${SOURCES})
