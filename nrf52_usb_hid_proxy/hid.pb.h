/* Automatically generated nanopb header */
/* Generated by nanopb-0.3.9 at Sun Mar 03 21:55:21 2019. */

#ifndef PB_HID_PB_H_INCLUDED
#define PB_HID_PB_H_INCLUDED
#include <pb.h>

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Enum definitions */
typedef enum _MouseUpdate_Type {
    MouseUpdate_Type_LEFT = 0,
    MouseUpdate_Type_MIDDLE = 1,
    MouseUpdate_Type_RIGHT = 2,
    MouseUpdate_Type_XY = 3
} MouseUpdate_Type;
#define _MouseUpdate_Type_MIN MouseUpdate_Type_LEFT
#define _MouseUpdate_Type_MAX MouseUpdate_Type_XY
#define _MouseUpdate_Type_ARRAYSIZE ((MouseUpdate_Type)(MouseUpdate_Type_XY+1))

/* Struct definitions */
typedef struct _MouseUpdate {
    MouseUpdate_Type type;
    int16_t x;
    int16_t y;
/* @@protoc_insertion_point(struct:MouseUpdate) */
} MouseUpdate;

/* Default values for struct fields */

/* Initializer values for message structs */
#define MouseUpdate_init_default                 {(MouseUpdate_Type)0, 0, 0}
#define MouseUpdate_init_zero                    {(MouseUpdate_Type)0, 0, 0}

/* Field tags (for use in manual encoding/decoding) */
#define MouseUpdate_type_tag                     1
#define MouseUpdate_x_tag                        2
#define MouseUpdate_y_tag                        3

/* Struct field encoding specification for nanopb */
extern const pb_field_t MouseUpdate_fields[4];

/* Maximum encoded size of messages (where known) */
#define MouseUpdate_size                         24

/* Message IDs (where set with "msgid" option) */
#ifdef PB_MSGID

#define HID_MESSAGES \


#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
/* @@protoc_insertion_point(eof) */

#endif
