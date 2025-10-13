/*******************************************************************************
 * Size: 15 px
 * Bpp: 1
 * Opts: --bpp 1 --size 15 --font E:/airowl_3.0/squareline/projects/ui_square_line/assets/fonts/Inter-VariableFont_opsz,wght.ttf -o E:/airowl_3.0/squareline/projects/ui_square_line/assets/fonts\ui_font_Inter15.c --format lvgl -r 0x20-0x7f --no-compress --no-prefilter
 ******************************************************************************/

#include "../ui.h"

#ifndef UI_FONT_INTER15
#define UI_FONT_INTER15 1
#endif

#if UI_FONT_INTER15

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xaa, 0xa8, 0x3c,

    /* U+0022 "\"" */
    0x99, 0x99,

    /* U+0023 "#" */
    0x11, 0x8, 0x84, 0x4f, 0xf3, 0x21, 0x10, 0x89,
    0xff, 0x22, 0x13, 0x19, 0x0,

    /* U+0024 "$" */
    0x10, 0x20, 0xe6, 0xb9, 0x32, 0x3c, 0x1e, 0x16,
    0x26, 0x4e, 0xb7, 0x82, 0x4, 0x0,

    /* U+0025 "%" */
    0x60, 0x92, 0x12, 0x44, 0x49, 0x6, 0x20, 0x8,
    0x2, 0x30, 0x49, 0x11, 0x24, 0x25, 0x83, 0x0,

    /* U+0026 "&" */
    0x38, 0x22, 0x11, 0x9, 0x83, 0x81, 0x83, 0x65,
    0x1a, 0x86, 0x63, 0x1f, 0x40,

    /* U+0027 "'" */
    0xf0,

    /* U+0028 "(" */
    0x69, 0x49, 0x24, 0x91, 0x26,

    /* U+0029 ")" */
    0xc9, 0x12, 0x49, 0x25, 0x2c,

    /* U+002A "*" */
    0x25, 0x5c, 0xea, 0x90,

    /* U+002B "+" */
    0x10, 0x20, 0x47, 0xf1, 0x2, 0x4, 0x0,

    /* U+002C "," */
    0xfa,

    /* U+002D "-" */
    0xf8,

    /* U+002E "." */
    0xf0,

    /* U+002F "/" */
    0x8, 0x46, 0x21, 0x18, 0x84, 0x23, 0x10, 0x8c,
    0x0,

    /* U+0030 "0" */
    0x3c, 0x42, 0xc3, 0x81, 0x81, 0x81, 0x81, 0x81,
    0xc3, 0x42, 0x3c,

    /* U+0031 "1" */
    0x37, 0x91, 0x11, 0x11, 0x11, 0x10,

    /* U+0032 "2" */
    0x39, 0x8e, 0x8, 0x10, 0x60, 0x82, 0x8, 0x20,
    0x83, 0xf8,

    /* U+0033 "3" */
    0x3c, 0x66, 0xc2, 0x2, 0x6, 0x1c, 0x2, 0x1,
    0xc1, 0x43, 0x3c,

    /* U+0034 "4" */
    0xc, 0x18, 0x50, 0xa2, 0x4c, 0x91, 0x62, 0xfe,
    0x8, 0x10,

    /* U+0035 "5" */
    0x7e, 0x81, 0x2, 0x7, 0x88, 0x80, 0x81, 0xc2,
    0x88, 0xe0,

    /* U+0036 "6" */
    0x3c, 0x89, 0xc, 0xb, 0x98, 0xa0, 0xc1, 0x82,
    0x88, 0xe0,

    /* U+0037 "7" */
    0xfe, 0xc, 0x10, 0x60, 0x83, 0x4, 0x18, 0x20,
    0xc1, 0x0,

    /* U+0038 "8" */
    0x3c, 0xcd, 0xa, 0x16, 0x67, 0x91, 0xc1, 0x83,
    0x8d, 0xf0,

    /* U+0039 "9" */
    0x38, 0x8a, 0xc, 0x18, 0x28, 0xce, 0x81, 0x84,
    0x88, 0xe0,

    /* U+003A ":" */
    0xf0, 0xf,

    /* U+003B ";" */
    0xf0, 0xe, 0xa0,

    /* U+003C "<" */
    0x2, 0x1c, 0xe7, 0xe, 0x7, 0x3, 0x80,

    /* U+003D "=" */
    0xfe, 0x0, 0x7, 0xf0,

    /* U+003E ">" */
    0x81, 0xc0, 0xe0, 0x70, 0xe7, 0x30, 0x0,

    /* U+003F "?" */
    0x7b, 0x38, 0x41, 0x8, 0xc2, 0x8, 0x0, 0xc3,
    0x0,

    /* U+0040 "@" */
    0xf, 0xc1, 0x83, 0x10, 0x4, 0x9f, 0x39, 0x98,
    0xc8, 0x46, 0x42, 0x32, 0x11, 0x99, 0x9a, 0x77,
    0x90, 0x0, 0x60, 0x0, 0xfc, 0x0,

    /* U+0041 "A" */
    0xc, 0x3, 0x1, 0xe0, 0x48, 0x33, 0xc, 0xc2,
    0x11, 0xfe, 0x40, 0x90, 0x2c, 0xc,

    /* U+0042 "B" */
    0xfd, 0xe, 0xc, 0x18, 0x7f, 0xa1, 0xc1, 0x83,
    0xf, 0xf0,

    /* U+0043 "C" */
    0x1e, 0x31, 0x90, 0x70, 0x8, 0x4, 0x2, 0x1,
    0x0, 0x41, 0xb1, 0x87, 0x80,

    /* U+0044 "D" */
    0xf8, 0x86, 0x82, 0x81, 0x81, 0x81, 0x81, 0x81,
    0x82, 0x86, 0xf8,

    /* U+0045 "E" */
    0xfe, 0x8, 0x20, 0x83, 0xf8, 0x20, 0x82, 0xf,
    0xc0,

    /* U+0046 "F" */
    0xfe, 0x8, 0x20, 0x83, 0xf8, 0x20, 0x82, 0x8,
    0x0,

    /* U+0047 "G" */
    0x1e, 0x31, 0x90, 0x70, 0x18, 0x4, 0x2, 0x1f,
    0x1, 0x40, 0xb0, 0x87, 0x80,

    /* U+0048 "H" */
    0x81, 0x81, 0x81, 0x81, 0x81, 0xff, 0x81, 0x81,
    0x81, 0x81, 0x81,

    /* U+0049 "I" */
    0xff, 0xe0,

    /* U+004A "J" */
    0x4, 0x10, 0x41, 0x4, 0x10, 0x61, 0x87, 0x37,
    0x80,

    /* U+004B "K" */
    0x83, 0x86, 0x8c, 0x98, 0xb0, 0xb0, 0xd8, 0x88,
    0x84, 0x86, 0x83,

    /* U+004C "L" */
    0x82, 0x8, 0x20, 0x82, 0x8, 0x20, 0x82, 0xf,
    0xc0,

    /* U+004D "M" */
    0xc0, 0x78, 0xf, 0x83, 0xd0, 0x5b, 0x1b, 0x22,
    0x66, 0x4c, 0xd9, 0x8a, 0x31, 0xc6, 0x10, 0x80,

    /* U+004E "N" */
    0xc1, 0xc1, 0xe1, 0xb1, 0xb1, 0x99, 0x8d, 0x8d,
    0x87, 0x83, 0x83,

    /* U+004F "O" */
    0x1e, 0x18, 0x64, 0xa, 0x1, 0x80, 0x60, 0x18,
    0x6, 0x1, 0x40, 0x98, 0x61, 0xe0,

    /* U+0050 "P" */
    0xfd, 0xa, 0xc, 0x18, 0x30, 0xbe, 0x40, 0x81,
    0x2, 0x0,

    /* U+0051 "Q" */
    0x1e, 0x18, 0x64, 0xa, 0x1, 0x80, 0x60, 0x18,
    0x6, 0x1, 0x46, 0x98, 0xe1, 0xf0, 0x2,

    /* U+0052 "R" */
    0xfd, 0xa, 0xc, 0x18, 0x30, 0xff, 0x46, 0x85,
    0xe, 0x8,

    /* U+0053 "S" */
    0x39, 0x8e, 0xc, 0xe, 0x7, 0x81, 0x81, 0x83,
    0x8d, 0xe0,

    /* U+0054 "T" */
    0xff, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
    0x8, 0x8, 0x8,

    /* U+0055 "U" */
    0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81,
    0x81, 0x42, 0x3c,

    /* U+0056 "V" */
    0x40, 0xd0, 0x26, 0x8, 0x86, 0x21, 0xc, 0x41,
    0x30, 0x48, 0x1a, 0x3, 0x80, 0xc0,

    /* U+0057 "W" */
    0x43, 0x5, 0xe, 0x14, 0x28, 0xd8, 0xa2, 0x26,
    0x88, 0x93, 0x22, 0x45, 0x8d, 0x14, 0x1c, 0x50,
    0x61, 0xc1, 0x83, 0x0,

    /* U+0058 "X" */
    0x60, 0x88, 0x63, 0x30, 0x68, 0xe, 0x3, 0x0,
    0xe0, 0x48, 0x33, 0x18, 0x64, 0x8,

    /* U+0059 "Y" */
    0xc1, 0xa0, 0x88, 0x86, 0xc1, 0x40, 0xe0, 0x20,
    0x10, 0x8, 0x4, 0x2, 0x0,

    /* U+005A "Z" */
    0xff, 0x3, 0x6, 0x4, 0x8, 0x18, 0x10, 0x20,
    0x60, 0xc0, 0xff,

    /* U+005B "[" */
    0xf2, 0x49, 0x24, 0x92, 0x4e,

    /* U+005C "\\" */
    0xc2, 0x10, 0x86, 0x10, 0x86, 0x10, 0x84, 0x30,
    0x80,

    /* U+005D "]" */
    0xe4, 0x92, 0x49, 0x24, 0x9e,

    /* U+005E "^" */
    0x23, 0x95, 0xb8, 0xc4,

    /* U+005F "_" */
    0xfe,

    /* U+0060 "`" */
    0xc8,

    /* U+0061 "a" */
    0x7b, 0x10, 0x4f, 0xc6, 0x18, 0xdd,

    /* U+0062 "b" */
    0x81, 0x2, 0x5, 0xcc, 0x50, 0x60, 0xc1, 0x83,
    0x8a, 0xe0,

    /* U+0063 "c" */
    0x38, 0x8a, 0x4, 0x8, 0x10, 0x11, 0x1c,

    /* U+0064 "d" */
    0x2, 0x4, 0x9, 0xd4, 0x70, 0x60, 0xc1, 0x82,
    0x8c, 0xe8,

    /* U+0065 "e" */
    0x38, 0x8a, 0xf, 0xf8, 0x10, 0x11, 0x9e,

    /* U+0066 "f" */
    0x19, 0x9, 0xf2, 0x10, 0x84, 0x21, 0x8,

    /* U+0067 "g" */
    0x3a, 0x8e, 0xc, 0x18, 0x30, 0x51, 0x9d, 0x3,
    0x8d, 0xf0,

    /* U+0068 "h" */
    0x82, 0x8, 0x3e, 0xce, 0x18, 0x61, 0x86, 0x18,
    0x40,

    /* U+0069 "i" */
    0xc2, 0xaa, 0xa8,

    /* U+006A "j" */
    0x60, 0x24, 0x92, 0x49, 0x25, 0x0,

    /* U+006B "k" */
    0x81, 0x2, 0x4, 0x28, 0x92, 0x2c, 0x78, 0x99,
    0x1a, 0x18,

    /* U+006C "l" */
    0xff, 0xe0,

    /* U+006D "m" */
    0xb9, 0xd8, 0xc6, 0x10, 0xc2, 0x18, 0x43, 0x8,
    0x61, 0xc, 0x21,

    /* U+006E "n" */
    0xbb, 0x38, 0x61, 0x86, 0x18, 0x61,

    /* U+006F "o" */
    0x38, 0x8a, 0xc, 0x18, 0x30, 0x51, 0x1c,

    /* U+0070 "p" */
    0xb9, 0x8a, 0xc, 0x18, 0x30, 0x71, 0x5c, 0x81,
    0x2, 0x0,

    /* U+0071 "q" */
    0x3a, 0x8e, 0xc, 0x18, 0x30, 0x51, 0x9d, 0x2,
    0x4, 0x8,

    /* U+0072 "r" */
    0xbc, 0x88, 0x88, 0x88,

    /* U+0073 "s" */
    0x7a, 0x38, 0x3c, 0x1c, 0x18, 0x5e,

    /* U+0074 "t" */
    0x44, 0xf4, 0x44, 0x44, 0x43,

    /* U+0075 "u" */
    0x86, 0x18, 0x61, 0x86, 0x1c, 0xdd,

    /* U+0076 "v" */
    0x43, 0x42, 0x62, 0x26, 0x24, 0x14, 0x18, 0x18,

    /* U+0077 "w" */
    0x8c, 0x71, 0x8b, 0x39, 0x2d, 0x65, 0x28, 0xa5,
    0xc, 0x61, 0x8,

    /* U+0078 "x" */
    0x62, 0x26, 0x3c, 0x18, 0x18, 0x34, 0x26, 0x42,

    /* U+0079 "y" */
    0x43, 0x42, 0x62, 0x26, 0x34, 0x14, 0x18, 0x18,
    0x18, 0x10, 0x70,

    /* U+007A "z" */
    0xfc, 0x31, 0x84, 0x21, 0x8c, 0x3f,

    /* U+007B "{" */
    0x19, 0x8, 0x42, 0x13, 0x4, 0x21, 0x8, 0x41,
    0x80,

    /* U+007C "|" */
    0xff, 0xff, 0xc0,

    /* U+007D "}" */
    0xc1, 0x8, 0x42, 0x10, 0x64, 0x21, 0x8, 0x4c,
    0x0,

    /* U+007E "~" */
    0x62, 0x92, 0x8e
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 68, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 69, .box_w = 2, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 4, .adv_w = 112, .box_w = 4, .box_h = 4, .ofs_x = 2, .ofs_y = 7},
    {.bitmap_index = 6, .adv_w = 152, .box_w = 9, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 19, .adv_w = 154, .box_w = 7, .box_h = 15, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 33, .adv_w = 236, .box_w = 11, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 49, .adv_w = 155, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 62, .adv_w = 72, .box_w = 1, .box_h = 4, .ofs_x = 2, .ofs_y = 7},
    {.bitmap_index = 63, .adv_w = 88, .box_w = 3, .box_h = 13, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 68, .adv_w = 88, .box_w = 3, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 73, .adv_w = 120, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 77, .adv_w = 159, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 84, .adv_w = 69, .box_w = 2, .box_h = 4, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 85, .adv_w = 110, .box_w = 5, .box_h = 1, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 86, .adv_w = 69, .box_w = 2, .box_h = 2, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 87, .adv_w = 86, .box_w = 5, .box_h = 13, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 96, .adv_w = 151, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 107, .adv_w = 98, .box_w = 4, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 113, .adv_w = 146, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 123, .adv_w = 148, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 134, .adv_w = 155, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 144, .adv_w = 142, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 154, .adv_w = 149, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 164, .adv_w = 136, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 174, .adv_w = 148, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 184, .adv_w = 149, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 194, .adv_w = 69, .box_w = 2, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 196, .adv_w = 72, .box_w = 2, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 199, .adv_w = 159, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 206, .adv_w = 159, .box_w = 7, .box_h = 4, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 210, .adv_w = 159, .box_w = 7, .box_h = 8, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 217, .adv_w = 123, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 226, .adv_w = 232, .box_w = 13, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 248, .adv_w = 166, .box_w = 10, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 262, .adv_w = 157, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 272, .adv_w = 175, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 285, .adv_w = 173, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 296, .adv_w = 144, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 305, .adv_w = 142, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 314, .adv_w = 179, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 327, .adv_w = 178, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 338, .adv_w = 64, .box_w = 1, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 340, .adv_w = 137, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 349, .adv_w = 161, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 360, .adv_w = 136, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 369, .adv_w = 217, .box_w = 11, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 385, .adv_w = 181, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 396, .adv_w = 184, .box_w = 10, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 410, .adv_w = 153, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 420, .adv_w = 184, .box_w = 10, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 435, .adv_w = 154, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 445, .adv_w = 154, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 455, .adv_w = 155, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 466, .adv_w = 179, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 477, .adv_w = 166, .box_w = 10, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 491, .adv_w = 236, .box_w = 14, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 511, .adv_w = 164, .box_w = 10, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 525, .adv_w = 163, .box_w = 9, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 538, .adv_w = 151, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 549, .adv_w = 88, .box_w = 3, .box_h = 13, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 554, .adv_w = 86, .box_w = 5, .box_h = 13, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 563, .adv_w = 88, .box_w = 3, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 568, .adv_w = 113, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 572, .adv_w = 109, .box_w = 7, .box_h = 1, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 573, .adv_w = 77, .box_w = 3, .box_h = 2, .ofs_x = 1, .ofs_y = 9},
    {.bitmap_index = 574, .adv_w = 135, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 580, .adv_w = 147, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 590, .adv_w = 137, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 597, .adv_w = 147, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 607, .adv_w = 140, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 614, .adv_w = 89, .box_w = 5, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 621, .adv_w = 147, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 631, .adv_w = 142, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 640, .adv_w = 58, .box_w = 2, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 643, .adv_w = 58, .box_w = 3, .box_h = 14, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 649, .adv_w = 132, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 659, .adv_w = 58, .box_w = 1, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 661, .adv_w = 210, .box_w = 11, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 672, .adv_w = 142, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 678, .adv_w = 144, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 685, .adv_w = 147, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 695, .adv_w = 147, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 705, .adv_w = 90, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 709, .adv_w = 127, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 715, .adv_w = 79, .box_w = 4, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 720, .adv_w = 142, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 726, .adv_w = 135, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 734, .adv_w = 196, .box_w = 11, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 745, .adv_w = 131, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 753, .adv_w = 135, .box_w = 8, .box_h = 11, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 764, .adv_w = 133, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 770, .adv_w = 102, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 779, .adv_w = 80, .box_w = 1, .box_h = 18, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 782, .adv_w = 102, .box_w = 5, .box_h = 13, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 791, .adv_w = 159, .box_w = 8, .box_h = 3, .ofs_x = 1, .ofs_y = 3}
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

/*-----------------
 *    KERNING
 *----------------*/


/*Pair left and right glyphs for kerning*/
static const uint8_t kern_pair_glyph_ids[] =
{
    3, 7,
    3, 13,
    3, 15,
    3, 21,
    7, 3,
    7, 8,
    7, 61,
    8, 7,
    8, 13,
    8, 15,
    8, 21,
    11, 7,
    11, 13,
    11, 15,
    11, 21,
    11, 33,
    11, 64,
    12, 19,
    12, 20,
    12, 24,
    12, 61,
    13, 3,
    13, 8,
    13, 17,
    13, 18,
    13, 20,
    13, 22,
    13, 23,
    13, 24,
    13, 25,
    13, 26,
    13, 32,
    13, 33,
    14, 19,
    14, 20,
    14, 24,
    14, 61,
    15, 3,
    15, 8,
    15, 17,
    15, 18,
    15, 20,
    15, 22,
    15, 23,
    15, 24,
    15, 25,
    15, 26,
    15, 32,
    15, 33,
    16, 13,
    16, 15,
    17, 13,
    17, 15,
    17, 24,
    17, 61,
    17, 64,
    19, 21,
    20, 11,
    20, 13,
    20, 15,
    20, 63,
    21, 11,
    21, 13,
    21, 15,
    21, 18,
    21, 63,
    22, 13,
    22, 15,
    23, 13,
    23, 15,
    23, 64,
    24, 4,
    24, 7,
    24, 13,
    24, 15,
    24, 17,
    24, 20,
    24, 21,
    24, 22,
    24, 23,
    24, 24,
    24, 25,
    24, 26,
    24, 27,
    24, 28,
    24, 29,
    24, 64,
    25, 11,
    25, 13,
    25, 15,
    25, 63,
    26, 13,
    26, 15,
    26, 24,
    26, 61,
    26, 64,
    27, 61,
    28, 61,
    30, 61,
    31, 24,
    31, 61,
    33, 13,
    33, 15,
    33, 16,
    33, 61,
    33, 64,
    61, 3,
    61, 8,
    61, 11,
    61, 12,
    61, 14,
    61, 16,
    61, 18,
    61, 30,
    61, 32,
    61, 33,
    61, 61,
    61, 63,
    61, 95,
    63, 7,
    63, 13,
    63, 15,
    63, 21,
    63, 33,
    63, 64,
    64, 11,
    64, 17,
    64, 18,
    64, 20,
    64, 21,
    64, 22,
    64, 23,
    64, 25,
    64, 26,
    64, 33,
    64, 61,
    64, 63,
    64, 93,
    95, 19,
    95, 20,
    95, 24,
    95, 61
};

/* Kerning between the respective left and right glyphs
 * 4.4 format which needs to scaled with `kern_scale`*/
static const int8_t kern_pair_values[] =
{
    -8, -20, -20, -15, -8, -8, -15, -8,
    -20, -20, -15, -8, -33, -33, -11, -3,
    -16, -8, -3, -5, -12, -20, -20, -6,
    -21, -6, -3, -6, -3, -5, -2, -21,
    -11, -8, -3, -5, -12, -20, -20, -6,
    -21, -6, -3, -6, -3, -5, -2, -21,
    -11, -9, -9, -6, -6, -5, -2, -11,
    -4, -4, -5, -5, -4, -5, -8, -8,
    -5, -5, -6, -6, -8, -8, -11, -14,
    -11, -30, -30, -4, -4, -14, -2, -4,
    5, -3, -2, -8, -8, -22, -38, -4,
    -5, -5, -4, -6, -6, -5, -2, -11,
    -15, -15, -16, -18, -18, -11, -11, -9,
    -8, -9, -19, -19, -19, -8, -8, 5,
    -8, -12, -15, -8, -12, -19, -8, -8,
    -33, -33, -11, -3, -16, -16, -11, -26,
    -11, -14, -11, -11, -11, -11, -9, -19,
    -16, 8, -8, -3, -5, -12
};

/*Collect the kern pair's data in one place*/
static const lv_font_fmt_txt_kern_pair_t kern_pairs =
{
    .glyph_ids = kern_pair_glyph_ids,
    .values = kern_pair_values,
    .pair_cnt = 142,
    .glyph_ids_size = 0
};

/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = &kern_pairs,
    .kern_scale = 16,
    .cmap_num = 1,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t ui_font_Inter15 = {
#else
lv_font_t ui_font_Inter15 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 18,          /*The maximum line height required by the font*/
    .base_line = 3,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -3,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if UI_FONT_INTER15*/

