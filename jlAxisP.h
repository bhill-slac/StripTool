#ifndef _jlAxisP
#define _jlAxisP

#include <Xm/PrimitiveP.h>   
#include "jlAxis.h"

#define AXIS_MAX_DIGITS       4
#define AXIS_MAX_LABEL        32
#define AXIS_TIC_LENGTH       7
#define AXIS_LABEL_MAX_SKEW   2
#define AXIS_LABEL_PAD        2

/* ====== New fields for the Axis widget class record ====== */
typedef struct _AxisClassPart
{
  int   make_compiler_happy;    /* keep compiler happy */
}
AxisClassPart;


/* ====== Full class record declaration ====== */
typedef struct _AxisClassRec
{
  CoreClassPart         core_class;
  XmPrimitiveClassPart  primitive_class;
  AxisClassPart         axis_class;
}
AxisClassRec;


extern AxisClassRec     axisClassRec;


/* ====== New fields for the Axis widget record ====== */
typedef struct _AxisPart
{
  /* resources */
  XFontStruct           *font;
  Pixel                 text_color;
  String                      unit_string;
  AxisDirection         direction;
  AxisTransform         transform;
  AxisValueType         value_type;
  double                      min_val, max_val;
  AxisEndpointPosition  min_pos, max_pos;
  Dimension                   min_margin;
  double                      log_epsilon;
  Boolean                     use_pixmap;
  
  
  /* private state */
  GC                          draw_gc;
  GC                          erase_gc;
  GC                          text_gc;
  Pixmap                      pixmap;
  Boolean                     need_refresh;
  
  short               n_tics;
  double                      tic_offsets[AXIS_MAX_TICS+1];     /* normalized */
  short               tic_lengths[AXIS_MAX_TICS+1];
  char                tic_labels[AXIS_MAX_TICS+1][AXIS_MAX_LABEL+1];
  char                tic_scale[AXIS_MAX_LABEL+1];
  
  short               log_n_powers;
  short               log_epsilon_index;
  double                      log_epsilon_offset;
  double                      log_delta;
}
AxisPart;

/* ====== Full instance record declaration ====== */
typedef struct _AxisRec
{
  CorePart              core;
  XmPrimitivePart       primitive;
  AxisPart              axis;
}
AxisRec;

#endif
