/*******************************************************************************
 * Size: 12 px
 * Bpp: 1
 * Opts: --bpp 1 --size 12 --font E:/airowl_3.0/squareline/projects/ui_square_line/assets/fonts/Inter-VariableFont_opsz,wght.ttf -o E:/airowl_3.0/squareline/projects/ui_square_line/assets/fonts\ui_font_Inter12.c --format lvgl -r 0x20-0x7f --no-compress --no-prefilter
 ******************************************************************************/

#include "../ui.h"

#ifndef UI_FONT_INTER12
#define UI_FONT_INTER12 1
#endif

#if UI_FONT_INTER12

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xfc, 0x80,

    /* U+0022 "\"" */
    0xb6, 0x80,

    /* U+0023 "#" */
    0x24, 0x49, 0xf9, 0x22, 0x5f, 0xd2, 0x24, 0x48,

    /* U+0024 "$" */
    0x21, 0xea, 0xe8, 0xe0, 0xe2, 0xc9, 0xad, 0xe2,
    0x0,

    /* U+0025 "%" */
    0xe1, 0x51, 0x29, 0x1d, 0x0, 0x80, 0x9c, 0x8a,
    0x45, 0x43, 0x80,

    /* U+0026 "&" */
    0x30, 0x91, 0x23, 0x86, 0x1a, 0xa3, 0x44, 0x74,

    /* U+0027 "'" */
    0xe0,

    /* U+0028 "(" */
    0x56, 0xaa, 0xa5,

    /* U+0029 ")" */
    0xa9, 0x55, 0x7a,

    /* U+002A "*" */
    0x25, 0x5d, 0xf2, 0x0,

    /* U+002B "+" */
    0x21, 0x9, 0xf2, 0x10, 0x80,

    /* U+002C "," */
    0xe0,

    /* U+002D "-" */
    0xf0,

    /* U+002E "." */
    0x80,

    /* U+002F "/" */
    0x11, 0x22, 0x22, 0x44, 0x44, 0x80,

    /* U+0030 "0" */
    0x79, 0x28, 0x61, 0x86, 0x18, 0x52, 0x78,

    /* U+0031 "1" */
    0x74, 0x92, 0x49, 0x20,

    /* U+0032 "2" */
    0x74, 0x62, 0x11, 0x19, 0x88, 0xf8,

    /* U+0033 "3" */
    0x39, 0x10, 0x41, 0x38, 0x10, 0x71, 0x78,

    /* U+0034 "4" */
    0x18, 0x62, 0x8a, 0x4a, 0x2f, 0xc2, 0x8,

    /* U+0035 "5" */
    0x7d, 0x4, 0x1e, 0xc, 0x10, 0x53, 0x78,

    /* U+0036 "6" */
    0x39, 0x38, 0x2e, 0xce, 0x18, 0x53, 0x78,

    /* U+0037 "7" */
    0xfc, 0x10, 0x86, 0x10, 0xc2, 0x18, 0x40,

    /* U+0038 "8" */
    0x7a, 0x18, 0x61, 0x7b, 0x38, 0x61, 0x78,

    /* U+0039 "9" */
    0x7b, 0x28, 0x61, 0xcd, 0xd0, 0x72, 0x70,

    /* U+003A ":" */
    0x84,

    /* U+003B ";" */
    0x87,

    /* U+003C "<" */
    0x4, 0x66, 0x20, 0x60, 0x60, 0x40,

    /* U+003D "=" */
    0xf8, 0x1, 0xf0,

    /* U+003E ">" */
    0x83, 0x83, 0x83, 0x33, 0x0, 0x0,

    /* U+003F "?" */
    0x72, 0x72, 0x11, 0x10, 0x80, 0x20,

    /* U+0040 "@" */
    0x1f, 0x18, 0x65, 0xee, 0x89, 0xa2, 0x68, 0x9a,
    0x26, 0x7e, 0x40, 0x18, 0x1, 0xe0,

    /* U+0041 "A" */
    0x18, 0x18, 0x18, 0x24, 0x24, 0x3c, 0x42, 0x42,
    0x42,

    /* U+0042 "B" */
    0xfa, 0x18, 0x61, 0xfa, 0x38, 0x61, 0xf8,

    /* U+0043 "C" */
    0x3c, 0x8e, 0xc, 0x8, 0x10, 0x20, 0xa3, 0x3c,

    /* U+0044 "D" */
    0xf9, 0xa, 0xc, 0x18, 0x30, 0x60, 0xc2, 0xf8,

    /* U+0045 "E" */
    0xfc, 0x21, 0xf, 0xc2, 0x10, 0xf8,

    /* U+0046 "F" */
    0xfc, 0x21, 0xf, 0xc2, 0x10, 0x80,

    /* U+0047 "G" */
    0x38, 0x8e, 0xc, 0x8, 0xf0, 0x60, 0xa2, 0x38,

    /* U+0048 "H" */
    0x83, 0x6, 0xc, 0x1f, 0xf0, 0x60, 0xc1, 0x82,

    /* U+0049 "I" */
    0xff, 0x80,

    /* U+004A "J" */
    0x8, 0x42, 0x10, 0x86, 0x31, 0x70,

    /* U+004B "K" */
    0x85, 0x12, 0x45, 0xe, 0x1a, 0x22, 0x46, 0x84,

    /* U+004C "L" */
    0x84, 0x21, 0x8, 0x42, 0x10, 0xf8,

    /* U+004D "M" */
    0xc1, 0xe0, 0xf8, 0xf4, 0x5a, 0x2c, 0xa6, 0x53,
    0x29, 0x88, 0x80,

    /* U+004E "N" */
    0xc3, 0x86, 0x8d, 0x99, 0x33, 0x62, 0xc3, 0x86,

    /* U+004F "O" */
    0x3c, 0x42, 0x81, 0x81, 0x81, 0x81, 0x81, 0x42,
    0x3c,

    /* U+0050 "P" */
    0xfa, 0x38, 0x61, 0x8f, 0xe8, 0x20, 0x80,

    /* U+0051 "Q" */
    0x3c, 0x42, 0x81, 0x81, 0x81, 0x81, 0x89, 0x46,
    0x3e, 0x3,

    /* U+0052 "R" */
    0xfa, 0x18, 0x61, 0xfa, 0x68, 0xa3, 0x84,

    /* U+0053 "S" */
    0x7a, 0x38, 0x30, 0x38, 0x30, 0x63, 0x78,

    /* U+0054 "T" */
    0xfe, 0x20, 0x40, 0x81, 0x2, 0x4, 0x8, 0x10,

    /* U+0055 "U" */
    0x83, 0x6, 0xc, 0x18, 0x30, 0x60, 0xe3, 0x3c,

    /* U+0056 "V" */
    0xc1, 0x42, 0x42, 0x22, 0x24, 0x24, 0x14, 0x18,
    0x18,

    /* U+0057 "W" */
    0xc6, 0x24, 0x62, 0x46, 0x24, 0xa6, 0x29, 0x42,
    0x94, 0x29, 0x43, 0x14, 0x10, 0x80,

    /* U+0058 "X" */
    0x42, 0x26, 0x24, 0x18, 0x18, 0x1c, 0x24, 0x62,
    0x43,

    /* U+0059 "Y" */
    0x82, 0x89, 0xb1, 0x43, 0x82, 0x4, 0x8, 0x10,

    /* U+005A "Z" */
    0xfc, 0x30, 0x84, 0x30, 0x84, 0x30, 0xfc,

    /* U+005B "[" */
    0xea, 0xaa, 0xab,

    /* U+005C "\\" */
    0x84, 0x44, 0x42, 0x22, 0x21, 0x10,

    /* U+005D "]" */
    0xd5, 0x55, 0x57,

    /* U+005E "^" */
    0x21, 0x9c, 0x94, 0x80,

    /* U+005F "_" */
    0xf8,

    /* U+0060 "`" */
    0x90,

    /* U+0061 "a" */
    0x74, 0x42, 0xf8, 0xc5, 0xe0,

    /* U+0062 "b" */
    0x82, 0xb, 0xb3, 0x86, 0x18, 0x73, 0xb8,

    /* U+0063 "c" */
    0x7b, 0x28, 0x20, 0x83, 0x27, 0x80,

    /* U+0064 "d" */
    0x4, 0x17, 0x73, 0x86, 0x18, 0x73, 0x74,

    /* U+0065 "e" */
    0x7b, 0x38, 0x7f, 0x83, 0x17, 0x80,

    /* U+0066 "f" */
    0x74, 0xf4, 0x44, 0x44, 0x40,

    /* U+0067 "g" */
    0x77, 0x38, 0x61, 0x87, 0x37, 0x41, 0xc5, 0xe0,

    /* U+0068 "h" */
    0x84, 0x2d, 0x98, 0xc6, 0x31, 0x88,

    /* U+0069 "i" */
    0xbf, 0x80,

    /* U+006A "j" */
    0x45, 0x55, 0x56,

    /* U+006B "k" */
    0x84, 0x27, 0x2a, 0x72, 0xd2, 0x88,

    /* U+006C "l" */
    0xff, 0x80,

    /* U+006D "m" */
    0xb7, 0x64, 0x62, 0x31, 0x18, 0x8c, 0x46, 0x22,

    /* U+006E "n" */
    0xf4, 0x63, 0x18, 0xc6, 0x20,

    /* U+006F "o" */
    0x7b, 0x38, 0x61, 0x87, 0x37, 0x80,

    /* U+0070 "p" */
    0xbb, 0x38, 0x61, 0x87, 0x3b, 0xa0, 0x82, 0x0,

    /* U+0071 "q" */
    0x77, 0x38, 0x61, 0x87, 0x37, 0x41, 0x4, 0x10,

    /* U+0072 "r" */
    0xf2, 0x49, 0x20,

    /* U+0073 "s" */
    0x74, 0x60, 0xe0, 0xc5, 0xc0,

    /* U+0074 "t" */
    0x44, 0xe4, 0x44, 0x44, 0x60,

    /* U+0075 "u" */
    0x8c, 0x63, 0x18, 0xc5, 0xe0,

    /* U+0076 "v" */
    0xc5, 0x14, 0xca, 0x28, 0xc1, 0x0,

    /* U+0077 "w" */
    0x4c, 0xa6, 0x53, 0x2a, 0xa3, 0x31, 0x98, 0xcc,

    /* U+0078 "x" */
    0x45, 0xa3, 0x84, 0x39, 0xa4, 0x40,

    /* U+0079 "y" */
    0xc5, 0x14, 0xca, 0x28, 0xa1, 0x4, 0x21, 0x80,

    /* U+007A "z" */
    0xf8, 0xc4, 0x46, 0x63, 0xe0,

    /* U+007B "{" */
    0x29, 0x25, 0x92, 0x49, 0x10,

    /* U+007C "|" */
    0xff, 0xfe,

    /* U+007D "}" */
    0x89, 0x24, 0xd2, 0x49, 0x40,

    /* U+007E "~" */
    0xe6, 0x70
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 54, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 55, .box_w = 1, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 3, .adv_w = 89, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 5, .adv_w = 122, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 13, .adv_w = 123, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 22, .adv_w = 189, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 33, .adv_w = 124, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 41, .adv_w = 58, .box_w = 1, .box_h = 3, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 42, .adv_w = 70, .box_w = 2, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 45, .adv_w = 70, .box_w = 2, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 48, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 52, .adv_w = 127, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 57, .adv_w = 55, .box_w = 1, .box_h = 3, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 58, .adv_w = 88, .box_w = 4, .box_h = 1, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 59, .adv_w = 55, .box_w = 1, .box_h = 1, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 60, .adv_w = 69, .box_w = 4, .box_h = 11, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 66, .adv_w = 121, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 73, .adv_w = 78, .box_w = 3, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 77, .adv_w = 117, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 83, .adv_w = 119, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 90, .adv_w = 124, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 97, .adv_w = 114, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 104, .adv_w = 119, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 111, .adv_w = 109, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 118, .adv_w = 119, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 125, .adv_w = 119, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 132, .adv_w = 55, .box_w = 1, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 133, .adv_w = 58, .box_w = 1, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 134, .adv_w = 127, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 140, .adv_w = 127, .box_w = 5, .box_h = 4, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 143, .adv_w = 127, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 149, .adv_w = 98, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 155, .adv_w = 185, .box_w = 10, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 169, .adv_w = 132, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 178, .adv_w = 126, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 185, .adv_w = 140, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 193, .adv_w = 139, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 201, .adv_w = 115, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 207, .adv_w = 113, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 213, .adv_w = 143, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 221, .adv_w = 143, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 229, .adv_w = 52, .box_w = 1, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 231, .adv_w = 110, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 237, .adv_w = 129, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 245, .adv_w = 109, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 251, .adv_w = 173, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 262, .adv_w = 145, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 270, .adv_w = 147, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 279, .adv_w = 123, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 286, .adv_w = 147, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 296, .adv_w = 124, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 303, .adv_w = 123, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 310, .adv_w = 124, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 318, .adv_w = 143, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 326, .adv_w = 132, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 335, .adv_w = 189, .box_w = 12, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 349, .adv_w = 131, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 358, .adv_w = 130, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 366, .adv_w = 121, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 373, .adv_w = 70, .box_w = 2, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 376, .adv_w = 69, .box_w = 4, .box_h = 11, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 382, .adv_w = 70, .box_w = 2, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 385, .adv_w = 90, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 389, .adv_w = 88, .box_w = 5, .box_h = 1, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 390, .adv_w = 62, .box_w = 2, .box_h = 2, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 391, .adv_w = 108, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 396, .adv_w = 118, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 403, .adv_w = 110, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 409, .adv_w = 118, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 416, .adv_w = 112, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 422, .adv_w = 71, .box_w = 4, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 427, .adv_w = 118, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 435, .adv_w = 114, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 441, .adv_w = 47, .box_w = 1, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 443, .adv_w = 47, .box_w = 2, .box_h = 12, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 446, .adv_w = 105, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 452, .adv_w = 47, .box_w = 1, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 454, .adv_w = 168, .box_w = 9, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 462, .adv_w = 113, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 467, .adv_w = 115, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 473, .adv_w = 118, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 481, .adv_w = 118, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 489, .adv_w = 72, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 492, .adv_w = 101, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 497, .adv_w = 63, .box_w = 4, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 502, .adv_w = 114, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 507, .adv_w = 108, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 513, .adv_w = 157, .box_w = 9, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 521, .adv_w = 105, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 527, .adv_w = 108, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 535, .adv_w = 106, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 540, .adv_w = 82, .box_w = 3, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 545, .adv_w = 64, .box_w = 1, .box_h = 15, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 547, .adv_w = 82, .box_w = 3, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 552, .adv_w = 127, .box_w = 6, .box_h = 2, .ofs_x = 1, .ofs_y = 2}
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
    -6, -16, -16, -12, -6, -6, -12, -6,
    -16, -16, -12, -6, -26, -26, -9, -2,
    -13, -6, -2, -4, -10, -16, -16, -5,
    -17, -5, -3, -5, -3, -4, -1, -17,
    -8, -6, -2, -4, -10, -16, -16, -5,
    -17, -5, -3, -5, -3, -4, -1, -17,
    -8, -8, -8, -5, -5, -4, -2, -9,
    -3, -3, -4, -4, -3, -4, -6, -6,
    -4, -4, -5, -5, -6, -6, -9, -11,
    -9, -24, -24, -3, -3, -11, -2, -3,
    4, -3, -2, -6, -6, -17, -30, -3,
    -4, -4, -3, -5, -5, -4, -2, -9,
    -12, -12, -13, -14, -14, -8, -8, -8,
    -6, -8, -15, -15, -15, -6, -6, 4,
    -6, -10, -12, -7, -9, -15, -6, -6,
    -26, -26, -9, -2, -13, -13, -9, -21,
    -9, -11, -9, -9, -9, -9, -8, -15,
    -13, 6, -6, -2, -4, -10
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
const lv_font_t ui_font_Inter12 = {
#else
lv_font_t ui_font_Inter12 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 15,          /*The maximum line height required by the font*/
    .base_line = 3,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -2,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if UI_FONT_INTER12*/

