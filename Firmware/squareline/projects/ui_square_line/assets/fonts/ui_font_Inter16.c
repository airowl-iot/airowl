/*******************************************************************************
 * Size: 16 px
 * Bpp: 1
 * Opts: --bpp 1 --size 16 --font E:/airowl_3.0/squareline/projects/ui_square_line/assets/fonts/Inter-VariableFont_opsz,wght.ttf -o E:/airowl_3.0/squareline/projects/ui_square_line/assets/fonts\ui_font_Inter16.c --format lvgl -r 0x20-0x7f --no-compress --no-prefilter
 ******************************************************************************/

#include "../ui.h"

#ifndef UI_FONT_INTER16
#define UI_FONT_INTER16 1
#endif

#if UI_FONT_INTER16

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0x55, 0x55, 0xf,

    /* U+0022 "\"" */
    0x99, 0x99,

    /* U+0023 "#" */
    0x11, 0x84, 0x41, 0x11, 0xff, 0x11, 0x4, 0x43,
    0x30, 0x88, 0xff, 0x88, 0x82, 0x20, 0x88,

    /* U+0024 "$" */
    0x10, 0x10, 0x3c, 0x53, 0x91, 0x90, 0xd0, 0x78,
    0x1e, 0x13, 0x11, 0x91, 0xd2, 0x7c, 0x10, 0x10,

    /* U+0025 "%" */
    0x60, 0x49, 0xc, 0x90, 0x89, 0x10, 0x63, 0x0,
    0x20, 0x4, 0x0, 0xc6, 0x8, 0x91, 0x9, 0x20,
    0x96, 0x6,

    /* U+0026 "&" */
    0x38, 0x22, 0x11, 0x9, 0x83, 0x81, 0x81, 0xc1,
    0x92, 0x85, 0x43, 0x31, 0xc7, 0x20,

    /* U+0027 "'" */
    0xf0,

    /* U+0028 "(" */
    0x69, 0x69, 0x24, 0x92, 0x64, 0x98,

    /* U+0029 ")" */
    0xc9, 0x32, 0x49, 0x24, 0xb4, 0xb0,

    /* U+002A "*" */
    0x10, 0xa9, 0xe1, 0xc5, 0x42, 0x0,

    /* U+002B "+" */
    0x10, 0x20, 0x40, 0x8f, 0xe2, 0x4, 0x8,

    /* U+002C "," */
    0x7e,

    /* U+002D "-" */
    0xf8,

    /* U+002E "." */
    0xf0,

    /* U+002F "/" */
    0x8, 0x46, 0x21, 0x8, 0xc4, 0x21, 0x18, 0x84,
    0x60,

    /* U+0030 "0" */
    0x3c, 0x66, 0x42, 0x81, 0x81, 0x81, 0x81, 0x81,
    0x81, 0x42, 0x66, 0x3c,

    /* U+0031 "1" */
    0x37, 0x91, 0x11, 0x11, 0x11, 0x11,

    /* U+0032 "2" */
    0x38, 0x8a, 0x8, 0x10, 0x20, 0x83, 0xc, 0x30,
    0xc1, 0x7, 0xf0,

    /* U+0033 "3" */
    0x3c, 0x46, 0xc2, 0x2, 0x6, 0x1c, 0x2, 0x1,
    0x1, 0xc1, 0x42, 0x3c,

    /* U+0034 "4" */
    0x6, 0x3, 0x3, 0x83, 0x41, 0x21, 0x90, 0x88,
    0x84, 0xff, 0x81, 0x0, 0x80, 0x40,

    /* U+0035 "5" */
    0x7e, 0x81, 0x6, 0xf, 0x98, 0x80, 0x81, 0x3,
    0x87, 0x11, 0xc0,

    /* U+0036 "6" */
    0x3c, 0x62, 0x41, 0x80, 0xbc, 0xc2, 0x81, 0x81,
    0x81, 0x81, 0x42, 0x3c,

    /* U+0037 "7" */
    0xff, 0x3, 0x2, 0x6, 0x4, 0xc, 0x8, 0x18,
    0x10, 0x30, 0x20, 0x60,

    /* U+0038 "8" */
    0x3c, 0x66, 0x42, 0x42, 0x66, 0x3c, 0x42, 0x81,
    0x81, 0x81, 0x42, 0x3c,

    /* U+0039 "9" */
    0x3c, 0x42, 0x82, 0x81, 0x81, 0x81, 0x43, 0x3d,
    0x1, 0x82, 0x46, 0x3c,

    /* U+003A ":" */
    0xf0, 0x3, 0xc0,

    /* U+003B ";" */
    0xf0, 0x3, 0xa8,

    /* U+003C "<" */
    0x1, 0x7, 0x1c, 0x70, 0xc0, 0x70, 0x1c, 0x7,
    0x1,

    /* U+003D "=" */
    0xfe, 0x0, 0x0, 0xf, 0xe0,

    /* U+003E ">" */
    0x80, 0xe0, 0x38, 0xe, 0x7, 0x1c, 0x70, 0xc0,
    0x0,

    /* U+003F "?" */
    0x7d, 0x8f, 0x8, 0x10, 0x61, 0x86, 0x8, 0x0,
    0x0, 0xc1, 0x80,

    /* U+0040 "@" */
    0xf, 0x81, 0x83, 0x18, 0xc, 0x80, 0x28, 0x74,
    0xc4, 0x66, 0x41, 0x32, 0x9, 0x90, 0x4c, 0x82,
    0x62, 0x32, 0x8e, 0xe6, 0x0, 0x18, 0x0, 0x3f,
    0x0,

    /* U+0041 "A" */
    0xc, 0x3, 0x1, 0xe0, 0x48, 0x12, 0xc, 0xc2,
    0x11, 0x86, 0x7f, 0x90, 0x2c, 0xf, 0x3,

    /* U+0042 "B" */
    0xfc, 0x86, 0x82, 0x82, 0x86, 0xfc, 0x82, 0x81,
    0x81, 0x81, 0x83, 0xfc,

    /* U+0043 "C" */
    0x1e, 0x31, 0x90, 0x70, 0x18, 0x4, 0x2, 0x1,
    0x0, 0x80, 0xa0, 0xd8, 0xc3, 0xc0,

    /* U+0044 "D" */
    0xfc, 0x41, 0x20, 0x50, 0x18, 0xc, 0x6, 0x3,
    0x1, 0x80, 0xc0, 0xa0, 0x9f, 0x80,

    /* U+0045 "E" */
    0xff, 0x2, 0x4, 0x8, 0x10, 0x3f, 0x40, 0x81,
    0x2, 0x7, 0xf0,

    /* U+0046 "F" */
    0xff, 0x2, 0x4, 0x8, 0x10, 0x3f, 0x40, 0x81,
    0x2, 0x4, 0x0,

    /* U+0047 "G" */
    0x1e, 0x8, 0x64, 0xe, 0x1, 0x80, 0x20, 0x8,
    0x3e, 0x1, 0x80, 0x50, 0x32, 0x18, 0x78,

    /* U+0048 "H" */
    0x80, 0xc0, 0x60, 0x30, 0x18, 0xf, 0xfe, 0x3,
    0x1, 0x80, 0xc0, 0x60, 0x30, 0x10,

    /* U+0049 "I" */
    0xff, 0xf0,

    /* U+004A "J" */
    0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x81, 0x83,
    0x7, 0x13, 0xe0,

    /* U+004B "K" */
    0x83, 0x86, 0x8c, 0x88, 0x98, 0xb0, 0xf8, 0xc8,
    0x8c, 0x86, 0x82, 0x83,

    /* U+004C "L" */
    0x82, 0x8, 0x20, 0x82, 0x8, 0x20, 0x82, 0x8,
    0x3f,

    /* U+004D "M" */
    0xc0, 0x78, 0xf, 0x83, 0xd0, 0x5a, 0xb, 0x63,
    0x64, 0x4c, 0xd9, 0x9b, 0x31, 0x46, 0x38, 0xc2,
    0x10,

    /* U+004E "N" */
    0xc0, 0xe0, 0x78, 0x36, 0x1b, 0x8c, 0xc6, 0x33,
    0x1d, 0x86, 0xc1, 0xe0, 0x70, 0x30,

    /* U+004F "O" */
    0x1e, 0x8, 0x44, 0xa, 0x1, 0x80, 0x60, 0x18,
    0x6, 0x1, 0x80, 0x50, 0x26, 0x18, 0x78,

    /* U+0050 "P" */
    0xfc, 0x82, 0x81, 0x81, 0x81, 0x81, 0x82, 0xfc,
    0x80, 0x80, 0x80, 0x80,

    /* U+0051 "Q" */
    0x1e, 0x8, 0x44, 0xa, 0x1, 0x80, 0x60, 0x18,
    0x6, 0x1, 0x84, 0x51, 0xa6, 0x38, 0x7e, 0x0,
    0x80,

    /* U+0052 "R" */
    0xfc, 0x82, 0x81, 0x81, 0x81, 0x82, 0xfc, 0x8c,
    0x86, 0x82, 0x83, 0x81,

    /* U+0053 "S" */
    0x3c, 0x43, 0x81, 0x80, 0xc0, 0x78, 0x1e, 0x3,
    0x1, 0x81, 0xc2, 0x7c,

    /* U+0054 "T" */
    0xff, 0x84, 0x2, 0x1, 0x0, 0x80, 0x40, 0x20,
    0x10, 0x8, 0x4, 0x2, 0x1, 0x0,

    /* U+0055 "U" */
    0x80, 0xc0, 0x60, 0x30, 0x18, 0xc, 0x6, 0x3,
    0x1, 0x80, 0xc0, 0x50, 0x47, 0xc0,

    /* U+0056 "V" */
    0x40, 0x48, 0x9, 0x83, 0x10, 0x43, 0x8, 0x63,
    0x4, 0x40, 0xc8, 0x1b, 0x1, 0x40, 0x28, 0x3,
    0x0,

    /* U+0057 "W" */
    0x41, 0x82, 0x83, 0xd, 0x8e, 0x1b, 0x1e, 0x22,
    0x2c, 0x44, 0x49, 0x8d, 0x93, 0x1b, 0x34, 0x14,
    0x28, 0x28, 0x70, 0x70, 0xe0, 0xc1, 0x80,

    /* U+0058 "X" */
    0x60, 0xc4, 0x10, 0xc6, 0xd, 0x80, 0xa0, 0x1c,
    0x3, 0x80, 0xd8, 0x1b, 0x6, 0x31, 0x83, 0x20,
    0x60,

    /* U+0059 "Y" */
    0xc1, 0xa0, 0x98, 0xc4, 0x43, 0x60, 0xa0, 0x20,
    0x10, 0x8, 0x4, 0x2, 0x1, 0x0,

    /* U+005A "Z" */
    0xff, 0x3, 0x6, 0x4, 0xc, 0x8, 0x10, 0x30,
    0x20, 0x60, 0xc0, 0xff,

    /* U+005B "[" */
    0xf2, 0x49, 0x24, 0x92, 0x49, 0x38,

    /* U+005C "\\" */
    0xc2, 0x10, 0xc2, 0x10, 0x86, 0x10, 0x84, 0x30,
    0x84,

    /* U+005D "]" */
    0xe4, 0x92, 0x49, 0x24, 0x92, 0x78,

    /* U+005E "^" */
    0x31, 0xc5, 0x92, 0xca, 0x10,

    /* U+005F "_" */
    0xfe,

    /* U+0060 "`" */
    0x4c,

    /* U+0061 "a" */
    0x7d, 0x8c, 0x18, 0x77, 0xf8, 0x60, 0xc3, 0x7a,

    /* U+0062 "b" */
    0x80, 0x80, 0x80, 0xbc, 0xc2, 0x81, 0x81, 0x81,
    0x81, 0x81, 0xc2, 0xbc,

    /* U+0063 "c" */
    0x3c, 0x8e, 0x4, 0x8, 0x10, 0x20, 0xa3, 0x3c,

    /* U+0064 "d" */
    0x1, 0x1, 0x1, 0x3d, 0x43, 0x81, 0x81, 0x81,
    0x81, 0x81, 0x43, 0x3d,

    /* U+0065 "e" */
    0x3c, 0x42, 0x81, 0x81, 0xff, 0x80, 0x80, 0x43,
    0x3e,

    /* U+0066 "f" */
    0x19, 0x9, 0xf2, 0x10, 0x84, 0x21, 0x8, 0x40,

    /* U+0067 "g" */
    0x3d, 0x43, 0x81, 0x81, 0x81, 0x81, 0x81, 0x43,
    0x3d, 0x1, 0xc3, 0x7c,

    /* U+0068 "h" */
    0x81, 0x2, 0x5, 0xec, 0x70, 0x60, 0xc1, 0x83,
    0x6, 0xc, 0x10,

    /* U+0069 "i" */
    0xf2, 0xaa, 0xaa,

    /* U+006A "j" */
    0x6c, 0x24, 0x92, 0x49, 0x24, 0xa0,

    /* U+006B "k" */
    0xc1, 0x83, 0x6, 0x3c, 0xdb, 0x3c, 0x78, 0xd9,
    0x9b, 0x16, 0x30,

    /* U+006C "l" */
    0xff, 0xf0,

    /* U+006D "m" */
    0xb9, 0xd9, 0xce, 0x10, 0xc2, 0x18, 0x43, 0x8,
    0x61, 0xc, 0x21, 0x84, 0x20,

    /* U+006E "n" */
    0xbd, 0x8e, 0xc, 0x18, 0x30, 0x60, 0xc1, 0x82,

    /* U+006F "o" */
    0x3c, 0x42, 0x81, 0x81, 0x81, 0x81, 0x81, 0x42,
    0x3c,

    /* U+0070 "p" */
    0xbc, 0xc2, 0x81, 0x81, 0x81, 0x81, 0x81, 0xc2,
    0xbc, 0x80, 0x80, 0x80,

    /* U+0071 "q" */
    0x3d, 0x43, 0x81, 0x81, 0x81, 0x81, 0x81, 0x43,
    0x3d, 0x1, 0x1, 0x1,

    /* U+0072 "r" */
    0xbc, 0x88, 0x88, 0x88, 0x80,

    /* U+0073 "s" */
    0x7a, 0x38, 0x30, 0x78, 0x30, 0x61, 0x78,

    /* U+0074 "t" */
    0x44, 0xf4, 0x44, 0x44, 0x44, 0x30,

    /* U+0075 "u" */
    0x83, 0x6, 0xc, 0x18, 0x30, 0x60, 0xe3, 0x7a,

    /* U+0076 "v" */
    0x41, 0x21, 0x98, 0xc4, 0x43, 0x61, 0xb0, 0x50,
    0x28, 0x8, 0x0,

    /* U+0077 "w" */
    0x84, 0x28, 0xe2, 0xca, 0x64, 0xa6, 0x4a, 0x47,
    0x94, 0x71, 0xc3, 0x18, 0x31, 0x80,

    /* U+0078 "x" */
    0xc6, 0x89, 0xb1, 0xc1, 0x5, 0x1b, 0x22, 0xc6,

    /* U+0079 "y" */
    0x41, 0x31, 0x98, 0xc4, 0x43, 0x60, 0xa0, 0x50,
    0x38, 0x8, 0xc, 0x6, 0xe, 0x0,

    /* U+007A "z" */
    0xfe, 0xc, 0x30, 0xc1, 0x6, 0x18, 0x20, 0xfe,

    /* U+007B "{" */
    0x19, 0x8, 0x42, 0x10, 0x98, 0x21, 0x8, 0x42,
    0xc,

    /* U+007C "|" */
    0xff, 0xff, 0xf0,

    /* U+007D "}" */
    0xc1, 0x8, 0x42, 0x10, 0x83, 0x21, 0x8, 0x42,
    0x60,

    /* U+007E "~" */
    0x71, 0x99, 0x8e
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 72, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 74, .box_w = 2, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 4, .adv_w = 119, .box_w = 4, .box_h = 4, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 6, .adv_w = 162, .box_w = 10, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 21, .adv_w = 164, .box_w = 8, .box_h = 16, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 37, .adv_w = 251, .box_w = 12, .box_h = 12, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 55, .adv_w = 165, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 69, .adv_w = 77, .box_w = 1, .box_h = 4, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 70, .adv_w = 93, .box_w = 3, .box_h = 15, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 76, .adv_w = 93, .box_w = 3, .box_h = 15, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 82, .adv_w = 128, .box_w = 7, .box_h = 6, .ofs_x = 0, .ofs_y = 6},
    {.bitmap_index = 88, .adv_w = 169, .box_w = 7, .box_h = 8, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 95, .adv_w = 74, .box_w = 2, .box_h = 4, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 96, .adv_w = 118, .box_w = 5, .box_h = 1, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 97, .adv_w = 74, .box_w = 2, .box_h = 2, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 98, .adv_w = 92, .box_w = 5, .box_h = 14, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 107, .adv_w = 162, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 119, .adv_w = 104, .box_w = 4, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 125, .adv_w = 156, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 136, .adv_w = 158, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 148, .adv_w = 165, .box_w = 9, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 162, .adv_w = 152, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 173, .adv_w = 159, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 185, .adv_w = 145, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 197, .adv_w = 158, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 209, .adv_w = 159, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 221, .adv_w = 74, .box_w = 2, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 224, .adv_w = 77, .box_w = 2, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 227, .adv_w = 169, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 236, .adv_w = 169, .box_w = 7, .box_h = 5, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 241, .adv_w = 169, .box_w = 8, .box_h = 9, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 250, .adv_w = 131, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 261, .adv_w = 247, .box_w = 13, .box_h = 15, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 286, .adv_w = 177, .box_w = 10, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 301, .adv_w = 168, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 313, .adv_w = 187, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 327, .adv_w = 185, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 341, .adv_w = 154, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 352, .adv_w = 151, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 363, .adv_w = 191, .box_w = 10, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 378, .adv_w = 190, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 392, .adv_w = 69, .box_w = 1, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 394, .adv_w = 146, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 405, .adv_w = 172, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 417, .adv_w = 145, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 426, .adv_w = 231, .box_w = 11, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 443, .adv_w = 193, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 457, .adv_w = 196, .box_w = 10, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 472, .adv_w = 164, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 484, .adv_w = 196, .box_w = 10, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 501, .adv_w = 165, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 513, .adv_w = 164, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 525, .adv_w = 165, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 539, .adv_w = 191, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 553, .adv_w = 177, .box_w = 11, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 570, .adv_w = 252, .box_w = 15, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 593, .adv_w = 175, .box_w = 11, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 610, .adv_w = 174, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 624, .adv_w = 161, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 636, .adv_w = 93, .box_w = 3, .box_h = 15, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 642, .adv_w = 92, .box_w = 5, .box_h = 14, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 651, .adv_w = 93, .box_w = 3, .box_h = 15, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 657, .adv_w = 121, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 662, .adv_w = 117, .box_w = 7, .box_h = 1, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 663, .adv_w = 83, .box_w = 3, .box_h = 2, .ofs_x = 1, .ofs_y = 10},
    {.bitmap_index = 664, .adv_w = 144, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 672, .adv_w = 157, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 684, .adv_w = 146, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 692, .adv_w = 157, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 704, .adv_w = 149, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 713, .adv_w = 95, .box_w = 5, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 721, .adv_w = 157, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 733, .adv_w = 151, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 744, .adv_w = 62, .box_w = 2, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 747, .adv_w = 62, .box_w = 3, .box_h = 15, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 753, .adv_w = 141, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 764, .adv_w = 62, .box_w = 1, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 766, .adv_w = 224, .box_w = 11, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 779, .adv_w = 151, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 787, .adv_w = 154, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 796, .adv_w = 157, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 808, .adv_w = 157, .box_w = 8, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 820, .adv_w = 96, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 825, .adv_w = 135, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 832, .adv_w = 84, .box_w = 4, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 838, .adv_w = 151, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 846, .adv_w = 144, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 857, .adv_w = 210, .box_w = 12, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 871, .adv_w = 140, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 879, .adv_w = 144, .box_w = 9, .box_h = 12, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 893, .adv_w = 141, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 901, .adv_w = 109, .box_w = 5, .box_h = 14, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 910, .adv_w = 85, .box_w = 1, .box_h = 20, .ofs_x = 2, .ofs_y = -4},
    {.bitmap_index = 913, .adv_w = 109, .box_w = 5, .box_h = 14, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 922, .adv_w = 169, .box_w = 8, .box_h = 3, .ofs_x = 1, .ofs_y = 3}
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
    -9, -21, -21, -16, -9, -9, -16, -9,
    -21, -21, -16, -9, -35, -35, -12, -3,
    -17, -9, -3, -6, -13, -21, -21, -6,
    -22, -7, -4, -6, -4, -6, -2, -22,
    -11, -9, -3, -6, -13, -21, -21, -6,
    -22, -7, -4, -6, -4, -6, -2, -22,
    -11, -10, -10, -6, -6, -5, -2, -12,
    -4, -4, -6, -6, -4, -5, -9, -9,
    -5, -5, -7, -7, -9, -9, -12, -14,
    -12, -32, -32, -4, -4, -15, -2, -4,
    5, -4, -2, -9, -9, -23, -41, -4,
    -6, -6, -4, -6, -6, -5, -2, -12,
    -16, -16, -17, -19, -19, -11, -11, -10,
    -9, -10, -20, -20, -20, -9, -9, 5,
    -9, -13, -16, -9, -12, -20, -9, -9,
    -35, -35, -12, -3, -17, -17, -12, -28,
    -12, -14, -12, -12, -12, -12, -10, -20,
    -17, 9, -9, -3, -6, -13
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
const lv_font_t ui_font_Inter16 = {
#else
lv_font_t ui_font_Inter16 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 20,          /*The maximum line height required by the font*/
    .base_line = 4,             /*Baseline measured from the bottom of the line*/
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



#endif /*#if UI_FONT_INTER16*/

