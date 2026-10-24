#ifndef VBA_GBA_CHEAT_TYPES_H
#define VBA_GBA_CHEAT_TYPES_H

#pragma once

// The instruction types the GBA cheat interpreter understands.  They are
// stored in CheatsData::size, so both the engine and the tools that read a
// cheat list back (the cheat explanation window among them) need the same
// numbers.  The list lives here instead of in each core, because V7 and V8
// used to carry their own copy of it.

#define UNKNOWN_CODE                  -1
#define INT_8_BIT_WRITE               0
#define INT_16_BIT_WRITE              1
#define INT_32_BIT_WRITE              2
#define GSA_16_BIT_ROM_PATCH          3
#define GSA_8_BIT_GS_WRITE            4
#define GSA_16_BIT_GS_WRITE           5
#define GSA_32_BIT_GS_WRITE           6
#define CBA_IF_KEYS_PRESSED           7
#define CBA_IF_TRUE                   8
#define CBA_SLIDE_CODE                9
#define CBA_IF_FALSE                  10
#define CBA_AND                       11
#define GSA_8_BIT_GS_WRITE2           12
#define GSA_16_BIT_GS_WRITE2          13
#define GSA_32_BIT_GS_WRITE2          14
#define GSA_16_BIT_ROM_PATCH2C        15
#define GSA_8_BIT_SLIDE               16
#define GSA_16_BIT_SLIDE              17
#define GSA_32_BIT_SLIDE              18
#define GSA_8_BIT_IF_TRUE             19
#define GSA_32_BIT_IF_TRUE            20
#define GSA_8_BIT_IF_FALSE            21
#define GSA_32_BIT_IF_FALSE           22
#define GSA_8_BIT_FILL                23
#define GSA_16_BIT_FILL               24
#define GSA_8_BIT_IF_TRUE2            25
#define GSA_16_BIT_IF_TRUE2           26
#define GSA_32_BIT_IF_TRUE2           27
#define GSA_8_BIT_IF_FALSE2           28
#define GSA_16_BIT_IF_FALSE2          29
#define GSA_32_BIT_IF_FALSE2          30
#define GSA_SLOWDOWN                  31
#define CBA_ADD                       32
#define CBA_OR                        33
#define CBA_LT                        34
#define CBA_GT                        35
#define CBA_SUPER                     36
#define GSA_8_BIT_POINTER             37
#define GSA_16_BIT_POINTER            38
#define GSA_32_BIT_POINTER            39
#define GSA_8_BIT_ADD                 40
#define GSA_16_BIT_ADD                41
#define GSA_32_BIT_ADD                42
#define GSA_8_BIT_IF_LOWER_U          43
#define GSA_16_BIT_IF_LOWER_U         44
#define GSA_32_BIT_IF_LOWER_U         45
#define GSA_8_BIT_IF_HIGHER_U         46
#define GSA_16_BIT_IF_HIGHER_U        47
#define GSA_32_BIT_IF_HIGHER_U        48
#define GSA_8_BIT_IF_AND              49
#define GSA_16_BIT_IF_AND             50
#define GSA_32_BIT_IF_AND             51
#define GSA_8_BIT_IF_LOWER_U2         52
#define GSA_16_BIT_IF_LOWER_U2        53
#define GSA_32_BIT_IF_LOWER_U2        54
#define GSA_8_BIT_IF_HIGHER_U2        55
#define GSA_16_BIT_IF_HIGHER_U2       56
#define GSA_32_BIT_IF_HIGHER_U2       57
#define GSA_8_BIT_IF_AND2             58
#define GSA_16_BIT_IF_AND2            59
#define GSA_32_BIT_IF_AND2            60
#define GSA_ALWAYS                    61
#define GSA_ALWAYS2                   62
#define GSA_8_BIT_IF_LOWER_S          63
#define GSA_16_BIT_IF_LOWER_S         64
#define GSA_32_BIT_IF_LOWER_S         65
#define GSA_8_BIT_IF_HIGHER_S         66
#define GSA_16_BIT_IF_HIGHER_S        67
#define GSA_32_BIT_IF_HIGHER_S        68
#define GSA_8_BIT_IF_LOWER_S2         69
#define GSA_16_BIT_IF_LOWER_S2        70
#define GSA_32_BIT_IF_LOWER_S2        71
#define GSA_8_BIT_IF_HIGHER_S2        72
#define GSA_16_BIT_IF_HIGHER_S2       73
#define GSA_32_BIT_IF_HIGHER_S2       74
#define GSA_16_BIT_WRITE_IOREGS       75
#define GSA_32_BIT_WRITE_IOREGS       76
#define GSA_CODES_ON                  77
#define GSA_8_BIT_IF_TRUE3            78
#define GSA_16_BIT_IF_TRUE3           79
#define GSA_32_BIT_IF_TRUE3           80
#define GSA_8_BIT_IF_FALSE3           81
#define GSA_16_BIT_IF_FALSE3          82
#define GSA_32_BIT_IF_FALSE3          83
#define GSA_8_BIT_IF_LOWER_S3         84
#define GSA_16_BIT_IF_LOWER_S3        85
#define GSA_32_BIT_IF_LOWER_S3        86
#define GSA_8_BIT_IF_HIGHER_S3        87
#define GSA_16_BIT_IF_HIGHER_S3       88
#define GSA_32_BIT_IF_HIGHER_S3       89
#define GSA_8_BIT_IF_LOWER_U3         90
#define GSA_16_BIT_IF_LOWER_U3        91
#define GSA_32_BIT_IF_LOWER_U3        92
#define GSA_8_BIT_IF_HIGHER_U3        93
#define GSA_16_BIT_IF_HIGHER_U3       94
#define GSA_32_BIT_IF_HIGHER_U3       95
#define GSA_8_BIT_IF_AND3             96
#define GSA_16_BIT_IF_AND3            97
#define GSA_32_BIT_IF_AND3            98
#define GSA_ALWAYS3                   99
#define GSA_16_BIT_ROM_PATCH2D        100
#define GSA_16_BIT_ROM_PATCH2E        101
#define GSA_16_BIT_ROM_PATCH2F        102
#define GSA_GROUP_WRITE               103
#define GSA_32_BIT_ADD2               104
#define GSA_32_BIT_SUB2               105
#define GSA_16_BIT_IF_LOWER_OR_EQ_U   106
#define GSA_16_BIT_IF_HIGHER_OR_EQ_U  107
#define GSA_16_BIT_MIF_TRUE           108
#define GSA_16_BIT_MIF_FALSE          109
#define GSA_16_BIT_MIF_LOWER_OR_EQ_U  110
#define GSA_16_BIT_MIF_HIGHER_OR_EQ_U 111
#define MASTER_CODE                   112
#define CHEATS_16_BIT_WRITE           114
#define CHEATS_32_BIT_WRITE           115

// V7 spelled type 15 without the trailing C.
#ifndef GSA_16_BIT_ROM_PATCH2
#define GSA_16_BIT_ROM_PATCH2 GSA_16_BIT_ROM_PATCH2C
#endif

#endif // VBA_GBA_CHEAT_TYPES_H
