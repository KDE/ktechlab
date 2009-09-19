/***************************************************************************
 *   Copyright (C) 2005 by David Saxton                                    *
 *   david@bluehaze.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "libraryitem.h"
#include "logic.h"
#include "matrixdisplaydriver.h"

#include <klocale.h>
#include <qpainter.h>
#include <qstring.h>

#include <cassert>

// Thank you Scott Dattalo!
// http://www.dattalo.com/gnupic/lcdfont.inc

static char characterMap[256][5] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //0
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //1
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //2
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //3
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //4
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //5
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //6
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //7
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //8
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //9
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //10
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //11
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //12
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //13
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //14
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //15
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //16
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //17
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //18
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //19
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //20
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //21
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //22
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //23
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //24
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //25
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //26
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //27
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //28
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //29
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //30
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //31
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //32
    { 0x00, 0x00, 0x7d, 0x00, 0x00 }, //33
    { 0x00, 0x70, 0x00, 0x70, 0x00 }, //34
    { 0x14, 0x7f, 0x14, 0x7f, 0x14 }, //35
    { 0x12, 0x2a, 0x7f, 0x2a, 0x24 }, //36
    { 0x62, 0x64, 0x08, 0x13, 0x23 }, //37
    { 0x36, 0x49, 0x55, 0x22, 0x05 }, //38
    { 0x00, 0x50, 0x60, 0x00, 0x00 }, //39
    { 0x00, 0x1c, 0x22, 0x41, 0x00 }, //40
    { 0x00, 0x41, 0x22, 0x1c, 0x00 }, //41
    { 0x14, 0x08, 0x3e, 0x08, 0x14 }, //42
    { 0x08, 0x08, 0x3e, 0x08, 0x08 }, //43
    { 0x00, 0x05, 0x06, 0x00, 0x00 }, //44
    { 0x08, 0x08, 0x08, 0x08, 0x08 }, //45
    { 0x00, 0x03, 0x03, 0x00, 0x00 }, //46
    { 0x02, 0x04, 0x08, 0x10, 0x20 }, //47
    { 0x3e, 0x45, 0x49, 0x51, 0x3e }, //48
    { 0x00, 0x21, 0x7f, 0x01, 0x00 }, //49
    { 0x21, 0x43, 0x45, 0x49, 0x31 }, //50
    { 0x42, 0x41, 0x51, 0x69, 0x46 }, //51
    { 0x0c, 0x14, 0x24, 0x7f, 0x04 }, //52
    { 0x72, 0x51, 0x51, 0x51, 0x4e }, //53
    { 0x1e, 0x29, 0x49, 0x49, 0x06 }, //54
    { 0x40, 0x47, 0x48, 0x50, 0x60 }, //55
    { 0x36, 0x49, 0x49, 0x49, 0x36 }, //56
    { 0x30, 0x49, 0x49, 0x4a, 0x3c }, //57
    { 0x00, 0x36, 0x36, 0x00, 0x00 }, //58
    { 0x00, 0x35, 0x36, 0x00, 0x00 }, //59
    { 0x08, 0x14, 0x22, 0x41, 0x00 }, //60
    { 0x14, 0x14, 0x14, 0x14, 0x14 }, //61
    { 0x41, 0x22, 0x14, 0x08, 0x00 }, //62
    { 0x20, 0x40, 0x45, 0x48, 0x30 }, //63
    { 0x26, 0x49, 0x4f, 0x41, 0x3e }, //64
    { 0x3f, 0x44, 0x44, 0x44, 0x3f }, //65
    { 0x7f, 0x49, 0x49, 0x49, 0x36 }, //66
    { 0x3e, 0x41, 0x41, 0x41, 0x22 }, //67
    { 0x7f, 0x41, 0x41, 0x41, 0x3e }, //68
    { 0x7f, 0x49, 0x49, 0x49, 0x41 }, //69
    { 0x7f, 0x48, 0x48, 0x48, 0x40 }, //70
    { 0x3e, 0x41, 0x49, 0x49, 0x2f }, //71
    { 0x7f, 0x08, 0x08, 0x08, 0x7f }, //72
    { 0x00, 0x41, 0x7f, 0x41, 0x00 }, //73
    { 0x02, 0x01, 0x41, 0x7e, 0x40 }, //74
    { 0x7f, 0x08, 0x14, 0x22, 0x41 }, //75
    { 0x7f, 0x01, 0x01, 0x01, 0x01 }, //76
    { 0x7f, 0x40, 0x20, 0x40, 0x7f }, //77
    { 0x7f, 0x10, 0x08, 0x04, 0x7f }, //78
    { 0x3e, 0x41, 0x41, 0x41, 0x3e }, //79
    { 0x7f, 0x48, 0x48, 0x48, 0x30 }, //80
    { 0x3e, 0x41, 0x45, 0x42, 0x3d }, //81
    { 0x7f, 0x48, 0x4c, 0x4a, 0x31 }, //82
    { 0x31, 0x49, 0x49, 0x49, 0x46 }, //83
    { 0x40, 0x40, 0x7f, 0x40, 0x40 }, //84
    { 0x7e, 0x01, 0x01, 0x01, 0x7e }, //85
    { 0x7c, 0x02, 0x01, 0x02, 0x7c }, //86
    { 0x7e, 0x01, 0x0e, 0x01, 0x7e }, //87
    { 0x63, 0x14, 0x08, 0x14, 0x63 }, //88
    { 0x70, 0x08, 0x07, 0x08, 0x70 }, //89
    { 0x43, 0x45, 0x49, 0x51, 0x61 }, //90
    { 0x00, 0x7f, 0x41, 0x41, 0x00 }, //91
    { 0x54, 0x34, 0x1f, 0x34, 0x54 }, //92
    { 0x00, 0x41, 0x41, 0x7f, 0x00 }, //93
    { 0x10, 0x20, 0x40, 0x20, 0x10 }, //94
    { 0x01, 0x01, 0x01, 0x01, 0x01 }, //95
    { 0x00, 0x40, 0x20, 0x10, 0x00 }, //96
    { 0x02, 0x15, 0x15, 0x15, 0x0f }, //97
    { 0x7f, 0x09, 0x11, 0x11, 0x0e }, //98
    { 0x0e, 0x11, 0x11, 0x11, 0x02 }, //99
    { 0x0e, 0x11, 0x11, 0x09, 0x7f }, //100
    { 0x0e, 0x15, 0x15, 0x15, 0x0c }, //101
    { 0x08, 0x3f, 0x48, 0x40, 0x20 }, //102
    { 0x30, 0x49, 0x49, 0x49, 0x7e }, //103
    { 0x7f, 0x08, 0x10, 0x10, 0x0f }, //104
    { 0x00, 0x11, 0x5f, 0x01, 0x00 }, //105
    { 0x02, 0x01, 0x21, 0x7e, 0x00 }, //106
    { 0x7f, 0x04, 0x0a, 0x11, 0x00 }, //107
    { 0x00, 0x41, 0x7f, 0x01, 0x00 }, //108
    { 0x1f, 0x10, 0x0c, 0x10, 0x0f }, //109
    { 0x1f, 0x08, 0x10, 0x10, 0x0f }, //110
    { 0x0e, 0x11, 0x11, 0x11, 0x0e }, //111
    { 0x1f, 0x14, 0x14, 0x14, 0x08 }, //112
    { 0x08, 0x14, 0x14, 0x0c, 0x1f }, //113
    { 0x1f, 0x08, 0x10, 0x10, 0x08 }, //114
    { 0x09, 0x15, 0x15, 0x15, 0x12 }, //115
    { 0x20, 0x7e, 0x21, 0x01, 0x02 }, //116
    { 0x1e, 0x01, 0x01, 0x02, 0x1f }, //117
    { 0x1c, 0x02, 0x01, 0x02, 0x1c }, //118
    { 0x1e, 0x01, 0x06, 0x01, 0x1e }, //119
    { 0x11, 0x0a, 0x04, 0x0a, 0x11 }, //120
    { 0x18, 0x05, 0x05, 0x05, 0x1e }, //121
    { 0x11, 0x13, 0x15, 0x19, 0x11 }, //122
    { 0x00, 0x08, 0x36, 0x41, 0x00 }, //123
    { 0x00, 0x00, 0x7f, 0x00, 0x00 }, //124
    { 0x00, 0x41, 0x36, 0x08, 0x00 }, //125
    { 0x08, 0x08, 0x2a, 0x1c, 0x08 }, //126
    { 0x08, 0x1c, 0x2a, 0x08, 0x08 }, //127
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //128
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //129
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //130
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //131
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //132
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //133
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //134
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //135
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //136
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //137
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //138
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //139
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //140
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //141
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //142
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //143
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //144
    { 0x07, 0x05, 0x07, 0x00, 0x00 }, //145
    { 0x00, 0x00, 0x78, 0x40, 0x40 }, //146
    { 0x01, 0x01, 0x0f, 0x00, 0x00 }, //147
    { 0x04, 0x02, 0x01, 0x00, 0x00 }, //148
    { 0x00, 0x0c, 0x0c, 0x00, 0x00 }, //149
    { 0x28, 0x28, 0x29, 0x2a, 0x3c }, //150
    { 0x10, 0x11, 0x16, 0x14, 0x18 }, //151
    { 0x02, 0x04, 0x0f, 0x10, 0x00 }, //152
    { 0x0c, 0x08, 0x19, 0x09, 0x0e }, //153
    { 0x09, 0x09, 0x0f, 0x09, 0x09 }, //154
    { 0x09, 0x0a, 0x0c, 0x1f, 0x08 }, //155
    { 0x08, 0x1f, 0x08, 0x0a, 0x0c }, //156
    { 0x01, 0x09, 0x09, 0x0f, 0x01 }, //157
    { 0x15, 0x15, 0x15, 0x1f, 0x00 }, //158
    { 0x0c, 0x00, 0x0d, 0x01, 0x0e }, //159
    { 0x04, 0x04, 0x04, 0x04, 0x04 }, //160
    { 0x40, 0x41, 0x5e, 0x48, 0x70 }, //161
    { 0x04, 0x08, 0x1f, 0x20, 0x40 }, //162
    { 0x38, 0x20, 0x61, 0x22, 0x3c }, //163
    { 0x11, 0x11, 0x1f, 0x11, 0x11 }, //164
    { 0x22, 0x24, 0x28, 0x7f, 0x20 }, //165
    { 0x21, 0x7e, 0x20, 0x21, 0x3e }, //166
    { 0x28, 0x28, 0x7f, 0x28, 0x28 }, //167
    { 0x08, 0x31, 0x21, 0x22, 0x3c }, //168
    { 0x10, 0x60, 0x21, 0x3e, 0x20 }, //169
    { 0x21, 0x21, 0x21, 0x21, 0x3f }, //170
    { 0x20, 0x79, 0x22, 0x7c, 0x20 }, //171
    { 0x29, 0x29, 0x01, 0x02, 0x1c }, //172
    { 0x21, 0x22, 0x24, 0x2a, 0x31 }, //173
    { 0x20, 0x7e, 0x21, 0x29, 0x31 }, //174
    { 0x30, 0x09, 0x01, 0x02, 0x3c }, //175
    { 0x08, 0x31, 0x29, 0x26, 0x3c }, //176
    { 0x28, 0x29, 0x3e, 0x48, 0x08 }, //177
    { 0x30, 0x00, 0x31, 0x02, 0x3c }, //178
    { 0x10, 0x51, 0x5e, 0x50, 0x10 }, //179
    { 0x00, 0x7f, 0x08, 0x04, 0x00 }, //180
    { 0x11, 0x12, 0x7c, 0x10, 0x10 }, //181
    { 0x01, 0x21, 0x21, 0x21, 0x01 }, //182
    { 0x21, 0x2a, 0x24, 0x2a, 0x30 }, //183
    { 0x22, 0x24, 0x6f, 0x34, 0x22 }, //184
    { 0x00, 0x01, 0x02, 0x7c, 0x00 }, //185
    { 0x0f, 0x00, 0x20, 0x10, 0x0f }, //186
    { 0x7e, 0x11, 0x11, 0x11, 0x11 }, //187
    { 0x20, 0x21, 0x21, 0x22, 0x3c }, //188
    { 0x10, 0x20, 0x10, 0x08, 0x06 }, //189
    { 0x26, 0x20, 0x7f, 0x20, 0x26 }, //190
    { 0x20, 0x24, 0x22, 0x25, 0x38 }, //191
    { 0x00, 0x2a, 0x2a, 0x2a, 0x01 }, //192
    { 0x0e, 0x12, 0x22, 0x02, 0x07 }, //193
    { 0x01, 0x0a, 0x04, 0x0a, 0x30 }, //194
    { 0x28, 0x3e, 0x29, 0x29, 0x29 }, //195
    { 0x10, 0x7f, 0x10, 0x14, 0x18 }, //196
    { 0x01, 0x21, 0x21, 0x3f, 0x01 }, //197
    { 0x29, 0x29, 0x29, 0x29, 0x3f }, //198
    { 0x10, 0x50, 0x51, 0x52, 0x1c }, //199
    { 0x78, 0x01, 0x02, 0x7c, 0x00 }, //200
    { 0x1f, 0x00, 0x3f, 0x01, 0x06 }, //201
    { 0x3f, 0x01, 0x02, 0x04, 0x08 }, //202
    { 0x3f, 0x21, 0x21, 0x21, 0x3f }, //203
    { 0x38, 0x20, 0x21, 0x22, 0x3c }, //204
    { 0x21, 0x21, 0x01, 0x02, 0x0c }, //205
    { 0x20, 0x10, 0x40, 0x20, 0x00 }, //206
    { 0x70, 0x50, 0x70, 0x00, 0x00 }, //207
    { 0x0e, 0x11, 0x09, 0x06, 0x19 }, //208
    { 0x02, 0x55, 0x15, 0x55, 0x0f }, //209
    { 0x1f, 0x2a, 0x2a, 0x2a, 0x14 }, //210
    { 0x0a, 0x15, 0x15, 0x11, 0x02 }, //211
    { 0x3f, 0x02, 0x02, 0x04, 0x3e }, //212
    { 0x0e, 0x11, 0x19, 0x15, 0x12 }, //213
    { 0x0f, 0x12, 0x22, 0x22, 0x1c }, //214
    { 0x1c, 0x22, 0x22, 0x22, 0x3f }, //215
    { 0x02, 0x01, 0x1e, 0x10, 0x10 }, //216
    { 0x20, 0x20, 0x00, 0x70, 0x00 }, //217
    { 0x00, 0x00, 0x10, 0x5f, 0x00 }, //218
    { 0x28, 0x10, 0x28, 0x00, 0x00 }, //219
    { 0x18, 0x24, 0x7e, 0x24, 0x08 }, //220
    { 0x14, 0x7f, 0x15, 0x01, 0x01 }, //221
    { 0x1f, 0x48, 0x50, 0x50, 0x0f }, //222
    { 0x0e, 0x51, 0x11, 0x51, 0x0e }, //223
    { 0x3f, 0x12, 0x22, 0x22, 0x1c }, //224
    { 0x1c, 0x22, 0x22, 0x12, 0x3f }, //225
    { 0x3c, 0x52, 0x52, 0x52, 0x3c }, //226
    { 0x03, 0x05, 0x02, 0x05, 0x06 }, //227
    { 0x1a, 0x26, 0x20, 0x26, 0x1a }, //228
    { 0x1e, 0x41, 0x01, 0x42, 0x1f }, //229
    { 0x63, 0x55, 0x49, 0x41, 0x41 }, //230
    { 0x22, 0x3c, 0x20, 0x3e, 0x22 }, //231
    { 0x51, 0x4a, 0x44, 0x4a, 0x51 }, //232
    { 0x3c, 0x02, 0x02, 0x02, 0x3f }, //233
    { 0x28, 0x28, 0x3e, 0x28, 0x48 }, //234
    { 0x22, 0x3c, 0x28, 0x28, 0x2e }, //235
    { 0x3e, 0x28, 0x38, 0x28, 0x3e }, //236
    { 0x04, 0x04, 0x15, 0x04, 0x04 }, //237
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //238
    { 0x7f, 0x7f, 0x7f, 0x7f, 0x7f }, //239
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //240
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //241
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //242
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //243
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //244
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //245
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //246
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //247
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //248
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //249
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //250
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //251
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //252
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //253
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //254
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, //255
};

Item* MatrixDisplayDriver::construct(ItemDocument *itemDocument, bool newItem, const char *id) {
    return new MatrixDisplayDriver((ICNDocument*)itemDocument, newItem, id);
}

LibraryItem * MatrixDisplayDriver::libraryItem() {
    return new LibraryItem(
               "ec/matrix_display_driver",
               i18n("Matrix Display Driver"),
               i18n("Integrated Circuits"),
               "ic2.png",
               LibraryItem::lit_component,
               MatrixDisplayDriver::construct);
}

MatrixDisplayDriver::MatrixDisplayDriver(ICNDocument *icnDocument, bool newItem, const char *id)
        : DIPComponent(icnDocument, newItem, id ? id : "Matrix Display Driver") {
    m_name = i18n("Matrix Display Driver");

    m_Col = 4;

// FIXME: functionality not implemented! =(
    createProperty("diode-configuration", Variant::Type::Select);
    property("diode-configuration")->setCaption(i18n("Configuration"));
    QStringMap allowed;
    allowed["Row Cathode"] = i18n("Row Cathode");
    allowed["Column Cathode"] = i18n("Column Cathode");
    property("diode-configuration")->setAllowed(allowed);
    property("diode-configuration")->setValue("Row Cathode");
    property("diode-configuration")->setAdvanced(true);

    QStringList pins = QStringList::split(',', "D0,D1,D2,D3,D4,D5,D6,D7,,,,,,C4,C3,C2,C1,C0,,R0,R1,R2,R3,R4,R5,R6", true);
    initDIPSymbol(pins, 64);
    initDIP(pins);

    m_pValueLogic.resize(8, 0);

    for(unsigned i = 0; i < 8; ++i) {
        m_pValueLogic[i] = new LogicIn(LogicIn::getConfig());
        setup1pinElement(m_pValueLogic[i], ecNodeWithID("D" + QString::number(i))->pin());
    }

    m_pRowLogic.resize(7, 0);

    for(unsigned i = 0; i < 7; ++i) {
        m_pRowLogic[i] = new LogicOut(LogicIn::getConfig(), false);
        setup1pinElement(m_pRowLogic[i], ecNodeWithID("R" + QString::number(i))->pin());

        m_pRowLogic[i]->setOutputLowConductance(1.0);
        m_pRowLogic[i]->setOutputHighVoltage(5.0);
    }

    m_pColLogic.resize(5, 0);

    for(unsigned i = 0; i < 5; ++i) {
        m_pColLogic[i] = new LogicOut(LogicIn::getConfig(), false);
        setup1pinElement(m_pColLogic[i], ecNodeWithID("C" + QString::number(i))->pin());

        m_pColLogic[i]->setOutputHighVoltage(5.0);
    }
}

MatrixDisplayDriver::~MatrixDisplayDriver() {
}

void MatrixDisplayDriver::stepNonLogic() {

    m_pColLogic[m_Col]->setHigh(true);
    m_Col = ++m_Col % 5;
    m_pColLogic[m_Col]->setHigh(false); // enable drain. ??? 

    char value = 0;
    for (unsigned i = 0; i < 8; ++i)
        value |= (m_pValueLogic[i]->isHigh()) ? (1 << i) : 0;

	char display_byte = characterMap[value][m_Col];
	for (unsigned row = 0; row < 7; row++) {
        	m_pRowLogic[row]->setHigh(display_byte & 1);
		display_byte = display_byte >> 1;
	}
}

