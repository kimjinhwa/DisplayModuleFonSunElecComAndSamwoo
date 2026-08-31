/*******************************************************************************
 * Size: 14 px
 * Bpp: 1
 * Opts: --bpp 1 --size 14 --font C:\Users\ServerManager\Documents\SquareLine\assets\arial.ttf -o C:\Users\ServerManager\Documents\SquareLine\assets\ui_font_arial14.c --format lvgl -r 0x20-0x7f --no-compress --no-prefilter
 ******************************************************************************/

#include "../ui.h"

#ifndef UI_FONT_ARIAL14
#define UI_FONT_ARIAL14 1
#endif

#if UI_FONT_ARIAL14

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xff, 0x20,

    /* U+0022 "\"" */
    0xb6, 0xd0,

    /* U+0023 "#" */
    0x24, 0x48, 0x97, 0xf2, 0x49, 0x12, 0x7f, 0x48,
    0x91, 0x20,

    /* U+0024 "$" */
    0x11, 0xe9, 0x64, 0x91, 0xc3, 0x85, 0x16, 0x5d,
    0x5e, 0x10, 0x40,

    /* U+0025 "%" */
    0x60, 0x92, 0x22, 0x44, 0x49, 0x6, 0x20, 0x9,
    0x82, 0x48, 0x49, 0x11, 0x22, 0x24, 0x83, 0x0,

    /* U+0026 "&" */
    0x38, 0x44, 0x44, 0x6c, 0x38, 0x78, 0xdb, 0x8e,
    0x86, 0xcf, 0x79,

    /* U+0027 "'" */
    0xf0,

    /* U+0028 "(" */
    0x29, 0x49, 0x24, 0x91, 0x24, 0x40,

    /* U+0029 ")" */
    0x89, 0x22, 0x49, 0x25, 0x25, 0x0,

    /* U+002A "*" */
    0x27, 0x48, 0xa5, 0x0,

    /* U+002B "+" */
    0x10, 0x20, 0x47, 0xf1, 0x2, 0x4, 0x0,

    /* U+002C "," */
    0xe0,

    /* U+002D "-" */
    0xf0,

    /* U+002E "." */
    0x80,

    /* U+002F "/" */
    0x13, 0x22, 0x24, 0x44, 0x48, 0x80,

    /* U+0030 "0" */
    0x39, 0x28, 0x61, 0x86, 0x18, 0x61, 0x85, 0x27,
    0x80,

    /* U+0031 "1" */
    0x13, 0x7d, 0x11, 0x11, 0x11, 0x10,

    /* U+0032 "2" */
    0x3c, 0xcd, 0x8, 0x10, 0x60, 0x82, 0x18, 0x20,
    0xc1, 0xf8,

    /* U+0033 "3" */
    0x38, 0x9b, 0x10, 0x20, 0xc3, 0x1, 0x81, 0xc2,
    0x8c, 0xe0,

    /* U+0034 "4" */
    0x8, 0x30, 0x61, 0x46, 0x89, 0x22, 0x7f, 0x8,
    0x10, 0x20,

    /* U+0035 "5" */
    0x7d, 0xc, 0x20, 0xfb, 0x30, 0x41, 0x87, 0x27,
    0x0,

    /* U+0036 "6" */
    0x39, 0x38, 0x60, 0xbb, 0x38, 0x61, 0x85, 0x33,
    0x80,

    /* U+0037 "7" */
    0xfe, 0x8, 0x30, 0x41, 0x82, 0x4, 0x18, 0x20,
    0x40, 0x80,

    /* U+0038 "8" */
    0x39, 0x14, 0x51, 0x39, 0x38, 0x61, 0x87, 0x27,
    0x80,

    /* U+0039 "9" */
    0x73, 0x28, 0x61, 0x87, 0x37, 0x41, 0x87, 0x27,
    0x0,

    /* U+003A ":" */
    0x81,

    /* U+003B ";" */
    0x81, 0xc0,

    /* U+003C "<" */
    0x6, 0x39, 0xc6, 0xe, 0x3, 0x1, 0x80,

    /* U+003D "=" */
    0xfe, 0x0, 0x7, 0xf0,

    /* U+003E ">" */
    0x81, 0xc0, 0x60, 0x31, 0xce, 0x30, 0x0,

    /* U+003F "?" */
    0x3c, 0xcf, 0x8, 0x10, 0x61, 0x86, 0x8, 0x10,
    0x0, 0x40,

    /* U+0040 "@" */
    0xf, 0xc0, 0xc3, 0x8, 0x4, 0x8e, 0xb4, 0x8c,
    0xcc, 0x66, 0x42, 0x32, 0x11, 0x91, 0x94, 0xcd,
    0x93, 0xb0, 0xc0, 0x13, 0x3, 0x7, 0xe0,

    /* U+0041 "A" */
    0x8, 0xa, 0x5, 0x2, 0xc3, 0x61, 0x10, 0x8c,
    0xfe, 0x41, 0x60, 0xf0, 0x20,

    /* U+0042 "B" */
    0xfd, 0xe, 0xc, 0x18, 0x7f, 0xa1, 0xc1, 0x83,
    0xf, 0xf0,

    /* U+0043 "C" */
    0x3e, 0x31, 0x90, 0x50, 0x8, 0x4, 0x2, 0x1,
    0x2, 0x41, 0x31, 0x8f, 0x0,

    /* U+0044 "D" */
    0xfc, 0x82, 0x83, 0x81, 0x81, 0x81, 0x81, 0x81,
    0x82, 0x82, 0xfc,

    /* U+0045 "E" */
    0xff, 0x2, 0x4, 0x8, 0x1f, 0xe0, 0x40, 0x81,
    0x3, 0xf8,

    /* U+0046 "F" */
    0xfe, 0x8, 0x20, 0x83, 0xf8, 0x20, 0x82, 0x8,
    0x0,

    /* U+0047 "G" */
    0x1e, 0x30, 0x90, 0x30, 0x8, 0x4, 0x3e, 0x3,
    0x1, 0x40, 0xb0, 0xc7, 0x80,

    /* U+0048 "H" */
    0x81, 0x81, 0x81, 0x81, 0x81, 0xff, 0x81, 0x81,
    0x81, 0x81, 0x81,

    /* U+0049 "I" */
    0xff, 0xe0,

    /* U+004A "J" */
    0x8, 0x42, 0x10, 0x84, 0x21, 0x8c, 0x5c,

    /* U+004B "K" */
    0x82, 0x84, 0x88, 0x90, 0xb0, 0xf0, 0xd8, 0x8c,
    0x84, 0x86, 0x83,

    /* U+004C "L" */
    0x82, 0x8, 0x20, 0x82, 0x8, 0x20, 0x82, 0xf,
    0xc0,

    /* U+004D "M" */
    0xc1, 0xe0, 0xf0, 0x74, 0x5a, 0x2d, 0x16, 0xdb,
    0x29, 0x94, 0xce, 0x62, 0x20,

    /* U+004E "N" */
    0x81, 0xc1, 0xe1, 0xa1, 0xb1, 0x99, 0x8d, 0x85,
    0x87, 0x83, 0x81,

    /* U+004F "O" */
    0x3e, 0x31, 0x90, 0x50, 0x18, 0xc, 0x6, 0x3,
    0x1, 0x41, 0x31, 0x8f, 0x0,

    /* U+0050 "P" */
    0xfd, 0xe, 0xc, 0x18, 0x7f, 0xa0, 0x40, 0x81,
    0x2, 0x0,

    /* U+0051 "Q" */
    0x3e, 0x31, 0x90, 0x50, 0x18, 0xc, 0x6, 0x3,
    0x1, 0x45, 0x31, 0x87, 0xe0, 0x10,

    /* U+0052 "R" */
    0xfe, 0x41, 0xa0, 0x50, 0x28, 0x37, 0xf2, 0x31,
    0xc, 0x82, 0x41, 0xa0, 0x60,

    /* U+0053 "S" */
    0x3c, 0x62, 0x41, 0x40, 0x78, 0x1e, 0x3, 0x41,
    0x41, 0x62, 0x3c,

    /* U+0054 "T" */
    0xff, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x10, 0x10, 0x10,

    /* U+0055 "U" */
    0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81,
    0x81, 0x42, 0x3c,

    /* U+0056 "V" */
    0xc0, 0xa0, 0xd0, 0x4c, 0x22, 0x31, 0x10, 0xc8,
    0x2c, 0x14, 0xe, 0x3, 0x0,

    /* U+0057 "W" */
    0xc3, 0xa, 0x38, 0xd1, 0x44, 0x8a, 0x24, 0x59,
    0x36, 0x58, 0xa2, 0x85, 0x14, 0x28, 0xa1, 0xc3,
    0x4, 0x10,

    /* U+0058 "X" */
    0x41, 0xb1, 0x8c, 0x82, 0xc1, 0xc0, 0x60, 0x70,
    0x6c, 0x23, 0x30, 0xb0, 0x60,

    /* U+0059 "Y" */
    0xc1, 0xb0, 0x88, 0x86, 0xc1, 0x40, 0xe0, 0x20,
    0x10, 0x8, 0x4, 0x2, 0x0,

    /* U+005A "Z" */
    0x7f, 0x3, 0x6, 0x4, 0xc, 0x18, 0x30, 0x30,
    0x60, 0xc0, 0xff,

    /* U+005B "[" */
    0xea, 0xaa, 0xaa, 0xb0,

    /* U+005C "\\" */
    0x88, 0x44, 0x44, 0x22, 0x22, 0x10,

    /* U+005D "]" */
    0xd5, 0x55, 0x55, 0x70,

    /* U+005E "^" */
    0x10, 0xc2, 0x9a, 0x4d, 0x10,

    /* U+005F "_" */
    0xff,

    /* U+0060 "`" */
    0x90,

    /* U+0061 "a" */
    0x7b, 0x10, 0x4f, 0xe6, 0x18, 0xdd,

    /* U+0062 "b" */
    0x82, 0x8, 0x3e, 0xca, 0x18, 0x61, 0x87, 0x2f,
    0x0,

    /* U+0063 "c" */
    0x79, 0x38, 0x20, 0x82, 0x14, 0xde,

    /* U+0064 "d" */
    0x4, 0x10, 0x5f, 0x4e, 0x18, 0x61, 0x85, 0x37,
    0xc0,

    /* U+0065 "e" */
    0x38, 0x9a, 0x17, 0xe8, 0x10, 0x93, 0x1c,

    /* U+0066 "f" */
    0x32, 0x11, 0xe4, 0x21, 0x8, 0x42, 0x10,

    /* U+0067 "g" */
    0x75, 0x38, 0x61, 0x86, 0x14, 0xdd, 0x6, 0x37,
    0x80,

    /* U+0068 "h" */
    0x82, 0x8, 0x2e, 0xc6, 0x18, 0x61, 0x86, 0x18,
    0x40,

    /* U+0069 "i" */
    0x9f, 0xe0,

    /* U+006A "j" */
    0x20, 0x12, 0x49, 0x24, 0x92, 0xc0,

    /* U+006B "k" */
    0x82, 0x8, 0x26, 0x92, 0x8e, 0x2c, 0x92, 0x68,
    0x80,

    /* U+006C "l" */
    0xff, 0xe0,

    /* U+006D "m" */
    0xf7, 0x66, 0x62, 0x31, 0x18, 0x8c, 0x46, 0x23,
    0x11,

    /* U+006E "n" */
    0xbb, 0x18, 0x61, 0x86, 0x18, 0x61,

    /* U+006F "o" */
    0x38, 0x8a, 0xc, 0x18, 0x30, 0x51, 0x1c,

    /* U+0070 "p" */
    0xbb, 0x28, 0x61, 0x86, 0x1c, 0xbc, 0x82, 0x8,
    0x0,

    /* U+0071 "q" */
    0x75, 0x38, 0x61, 0x86, 0x14, 0xcf, 0x4, 0x10,
    0x40,

    /* U+0072 "r" */
    0xf8, 0x88, 0x88, 0x88,

    /* U+0073 "s" */
    0x7a, 0x38, 0x3c, 0x1e, 0x18, 0x5e,

    /* U+0074 "t" */
    0x9, 0x74, 0x92, 0x49, 0x80,

    /* U+0075 "u" */
    0x86, 0x18, 0x61, 0x86, 0x18, 0xdd,

    /* U+0076 "v" */
    0xc6, 0x89, 0x13, 0x62, 0x85, 0xe, 0x8,

    /* U+0077 "w" */
    0x8c, 0x73, 0x34, 0xc9, 0x3a, 0x52, 0x8c, 0xc3,
    0x30, 0xcc,

    /* U+0078 "x" */
    0x44, 0xd8, 0xa0, 0x83, 0x85, 0x1b, 0x63,

    /* U+0079 "y" */
    0xc6, 0x89, 0x13, 0x62, 0x85, 0xe, 0x8, 0x10,
    0x61, 0x80,

    /* U+007A "z" */
    0xfc, 0x21, 0x84, 0x21, 0xc, 0x3f,

    /* U+007B "{" */
    0x19, 0x8, 0x42, 0x13, 0xc, 0x21, 0x8, 0x42,
    0xc,

    /* U+007C "|" */
    0xff, 0xfc,

    /* U+007D "}" */
    0xc1, 0x8, 0x42, 0x10, 0x64, 0x21, 0x8, 0x42,
    0x60,

    /* U+007E "~" */
    0xf3, 0x3c
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 62, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 62, .box_w = 1, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 3, .adv_w = 80, .box_w = 3, .box_h = 4, .ofs_x = 1, .ofs_y = 7},
    {.bitmap_index = 5, .adv_w = 125, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 15, .adv_w = 125, .box_w = 6, .box_h = 14, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 26, .adv_w = 199, .box_w = 11, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 42, .adv_w = 149, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 53, .adv_w = 43, .box_w = 1, .box_h = 4, .ofs_x = 1, .ofs_y = 7},
    {.bitmap_index = 54, .adv_w = 75, .box_w = 3, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 60, .adv_w = 75, .box_w = 3, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 66, .adv_w = 87, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 6},
    {.bitmap_index = 70, .adv_w = 131, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 77, .adv_w = 62, .box_w = 1, .box_h = 3, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 78, .adv_w = 75, .box_w = 4, .box_h = 1, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 79, .adv_w = 62, .box_w = 1, .box_h = 1, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 80, .adv_w = 62, .box_w = 4, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 86, .adv_w = 125, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 95, .adv_w = 125, .box_w = 4, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 101, .adv_w = 125, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 111, .adv_w = 125, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 121, .adv_w = 125, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 131, .adv_w = 125, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 140, .adv_w = 125, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 149, .adv_w = 125, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 159, .adv_w = 125, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 168, .adv_w = 125, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 177, .adv_w = 62, .box_w = 1, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 178, .adv_w = 62, .box_w = 1, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 180, .adv_w = 131, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 187, .adv_w = 131, .box_w = 7, .box_h = 4, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 191, .adv_w = 131, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 198, .adv_w = 125, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 208, .adv_w = 227, .box_w = 13, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 231, .adv_w = 149, .box_w = 9, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 244, .adv_w = 149, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 254, .adv_w = 162, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 267, .adv_w = 162, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 278, .adv_w = 149, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 288, .adv_w = 137, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 297, .adv_w = 174, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 310, .adv_w = 162, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 321, .adv_w = 62, .box_w = 1, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 323, .adv_w = 112, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 330, .adv_w = 149, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 341, .adv_w = 125, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 350, .adv_w = 187, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 363, .adv_w = 162, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 374, .adv_w = 174, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 387, .adv_w = 149, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 397, .adv_w = 174, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 411, .adv_w = 162, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 424, .adv_w = 149, .box_w = 8, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 435, .adv_w = 137, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 446, .adv_w = 162, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 457, .adv_w = 149, .box_w = 9, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 470, .adv_w = 211, .box_w = 13, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 488, .adv_w = 149, .box_w = 9, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 501, .adv_w = 149, .box_w = 9, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 514, .adv_w = 137, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 525, .adv_w = 62, .box_w = 2, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 529, .adv_w = 62, .box_w = 4, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 535, .adv_w = 62, .box_w = 2, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 539, .adv_w = 105, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 544, .adv_w = 125, .box_w = 8, .box_h = 1, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 545, .adv_w = 75, .box_w = 2, .box_h = 2, .ofs_x = 1, .ofs_y = 9},
    {.bitmap_index = 546, .adv_w = 125, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 552, .adv_w = 125, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 561, .adv_w = 112, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 567, .adv_w = 125, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 576, .adv_w = 125, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 583, .adv_w = 62, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 590, .adv_w = 125, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 599, .adv_w = 125, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 608, .adv_w = 50, .box_w = 1, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 610, .adv_w = 50, .box_w = 3, .box_h = 14, .ofs_x = -1, .ofs_y = -3},
    {.bitmap_index = 616, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 625, .adv_w = 50, .box_w = 1, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 627, .adv_w = 187, .box_w = 9, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 636, .adv_w = 125, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 642, .adv_w = 125, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 649, .adv_w = 125, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 658, .adv_w = 125, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 667, .adv_w = 75, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 671, .adv_w = 112, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 677, .adv_w = 62, .box_w = 3, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 682, .adv_w = 125, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 688, .adv_w = 112, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 695, .adv_w = 162, .box_w = 10, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 705, .adv_w = 112, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 712, .adv_w = 112, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 722, .adv_w = 112, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 728, .adv_w = 75, .box_w = 5, .box_h = 14, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 737, .adv_w = 58, .box_w = 1, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 739, .adv_w = 75, .box_w = 5, .box_h = 14, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 748, .adv_w = 131, .box_w = 7, .box_h = 2, .ofs_x = 1, .ofs_y = 4}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LV_VERSION_CHECK(8, 0, 0)
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LV_VERSION_CHECK(8, 0, 0)
    .cache = &cache
#endif
};


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LV_VERSION_CHECK(8, 0, 0)
const lv_font_t ui_font_arial14 = {
#else
lv_font_t ui_font_arial14 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 15,          /*The maximum line height required by the font*/
    .base_line = 3,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc           /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
};



#endif /*#if UI_FONT_ARIAL14*/

