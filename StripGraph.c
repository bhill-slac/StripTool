/*-----------------------------------------------------------------------------
 * Copyright (c) 1996 Southeastern Universities Research Association,
 *               Continuous Electron Beam Accelerator Facility
 *
 * This software was developed under a United States Government license
 * described in the NOTICE file included as part of this distribution.
 *
 *-----------------------------------------------------------------------------
 */


#ifdef QUANTIFY_PRECISE
#  include <quantify.h>
#endif

#include <X11/Xlib.h>
#include <Xm/Xm.h>
#include <Xm/DrawingA.h>
#include <Xm/PushB.h>
#include <Xm/Label.h>
#include <Xm/Form.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/times.h>
#include <unistd.h>

#include "FastLabel.h"
#include "jlAxis.h"
#include "jlLegend.h"

#include "StripGraph.h"
#include "StripDefines.h"
#include "StripMisc.h"

#define SG_DUMP_MATRIX_FIELDWIDTH       30
#define SG_DUMP_MATRIX_NUMWIDTH         20
#define SG_DUMP_MATRIX_BADVALUESTR      "???"
#define LEGEND_OFFSET                   5

static char     *SGComponentStr[SGCOMP_COUNT] =
{
  "X-Axis",
  "Y-Axis",
  "Legend",
  "Data",
  "Title",
  "Grid"
};


/* StripGraphInfo
 *
 *      This is the graph object.
 */
typedef struct
{
  /* === X Stuff === */
  Widget                parent, canvas, msg_lbl, loc_lbl;
  Widget                x_axis, y_axis, legend, title_lbl;
  Display               *display;
  Window                window;
  GC                    gc;
  Pixmap                pixmap;
  Pixmap                plotpix;
  int                   screen;

  /* === time stuff === */
  struct timeval        t0, t1;
  struct timeval        plotted_t0, plotted_t1;

  /* === raster info ==== */
  double                db;             /* bin width */
  double                b_min, b_max;   /* min / max bin start values */
  double                dl;             /* interval width (real) */
  struct timeval        dt;             /* interval width (time) */

  /* === layout info === */
  XRectangle            window_rect;
  
  /* === graph components === */
  StripConfig           *config;
  StripCurveInfo        *curves[STRIP_MAX_CURVES];
  jlaTransformInfo      transforms[STRIP_MAX_CURVES];
  LegendItem            lgitems[STRIP_MAX_CURVES];
  StripCurveInfo        *selected_curve;
  StripDataSource       data;
  XPoint                loc_xy;     /* (x,y) of pointer position */

  struct _grid
  {
    XSegment    h_seg[AXIS_MAX_TICS+1];
    XSegment    v_seg[AXIS_MAX_TICS+1];
  } grid;
  
  char                  *title;
  unsigned              draw_mask;
  unsigned              status;

  void                  *user_data;
}
StripGraphInfo;


/* sgTransformYData / sgTransformXData
 *
 *      Contains useful information for the transform functions
 *      invoked by StripDataSource_render.
 */
typedef struct
{
  jlaTransformInfo      *xform;
  StripGraphInfo        *sgi;
  StripCurveInfo        *curve;
} sgTransformYData;

typedef struct
{
  double                t0;
  double                db;
} sgTransformXData;


/* prototypes for internal static functions */
static void     StripGraph_manage_geometry      (StripGraphInfo *);
static void     StripGraph_plotdata             (StripGraphInfo *);
static void     StripGraph_update_loc_lbl       (StripGraphInfo *sgi);
static void     callback                        (Widget, XtPointer, XtPointer);
static void     crossing_event_handler          (Widget,
                                                 XtPointer,
                                                 XCrossingEvent *,
                                                 Boolean *);
static void     motion_event_handler            (Widget,
                                                 XtPointer,
                                                 XMotionEvent *,
                                                 Boolean *);
static void     y_transform                     (void *,
                                                 double *,
                                                 double *,
                                                 int);
static void     x_transform                     (void *,
                                                 double *,
                                                 double *,
                                                 int);

/* static variables */
static struct timeval   tv;
static struct timeval   *ptv;


/*
 * StripGraph_init
 */
StripGraph StripGraph_init (Widget      parent,
                            StripConfig *cfg)
{
  StripGraphInfo        *sgi;
  XWindowAttributes     win_attrib;
  int                   i;

  if ((sgi = (StripGraphInfo *)malloc (sizeof(StripGraphInfo))) != NULL)
  {
    /* create the widgets */
    sgi->y_axis = XtVaCreateManagedWidget
      ("yAxis",
       axisWidgetClass,                 parent,
       XmNbackground,                   cfg->Color.background.xcolor.pixel,
       XmNforeground,                   cfg->Color.foreground.xcolor.pixel,
       XjNtextColor,                    cfg->Color.foreground.xcolor.pixel,
       XmNtopAttachment,                XmATTACH_FORM,
       XmNleftAttachment,               XmATTACH_FORM,
       XmNrightAttachment,              XmATTACH_NONE,
       XmNbottomAttachment,             XmATTACH_FORM,
       0);
    
    sgi->legend = XtVaCreateManagedWidget
      ("legend",
       legendWidgetClass,               parent,
       XmNbackground,                   cfg->Color.background.xcolor.pixel,
       XmNforeground,                   cfg->Color.foreground.xcolor.pixel,
       XmNtopAttachment,                XmATTACH_FORM,
       XmNleftAttachment,               XmATTACH_NONE,
       XmNrightAttachment,              XmATTACH_FORM,
       XmNbottomAttachment,             XmATTACH_FORM,
       0);
    XtAddCallback (sgi->legend, XjNchooseCallback, callback, sgi);

    sgi->title_lbl = XtVaCreateManagedWidget
      ("titleLabel",
       xmLabelWidgetClass,              parent,
       XmNbackground,                   cfg->Color.background.xcolor.pixel,
       XmNforeground,                   cfg->Color.foreground.xcolor.pixel,
       XmNtopAttachment,                XmATTACH_FORM,
       XmNleftAttachment,               XmATTACH_WIDGET,
       XmNleftWidget,                   sgi->y_axis,
       XmNrightAttachment,              XmATTACH_WIDGET,
       XmNrightWidget,                  sgi->legend,
       XmNbottomAttachment,             XmATTACH_NONE,
       0);

    sgi->x_axis = XtVaCreateManagedWidget
      ("xAxis",
       axisWidgetClass,                 parent,
       XmNbackground,                   cfg->Color.background.xcolor.pixel,
       XmNforeground,                   cfg->Color.foreground.xcolor.pixel,
       XjNtextColor,                    cfg->Color.foreground.xcolor.pixel,
       XmNtopAttachment,                XmATTACH_NONE,
       XmNleftAttachment,               XmATTACH_WIDGET,
       XmNleftWidget,                   sgi->y_axis,
       XmNrightAttachment,              XmATTACH_WIDGET,
       XmNrightWidget,                  sgi->legend,
       XmNrightOffset,                  LEGEND_OFFSET,
       XmNbottomAttachment,             XmATTACH_FORM,
       0);

    sgi->canvas = XtVaCreateManagedWidget
      ("canvas",
       xmDrawingAreaWidgetClass,        parent,
       XmNtopAttachment,                XmATTACH_WIDGET,
       XmNtopWidget,                    sgi->title_lbl,
       XmNleftAttachment,               XmATTACH_WIDGET,
       XmNleftWidget,                   sgi->y_axis,
       XmNrightAttachment,              XmATTACH_WIDGET,
       XmNrightWidget,                  sgi->legend,
       XmNrightOffset,                  LEGEND_OFFSET,
       XmNbottomAttachment,             XmATTACH_WIDGET,
       XmNbottomWidget,                 sgi->x_axis,
       0);

    sgi->loc_lbl = XtVaCreateManagedWidget
      ("locationLabel",
       xgFastLabelWidgetClass,          parent,
       XmNmappedWhenManaged,            False,
       XmNbackground,                   cfg->Color.background.xcolor.pixel,
       XmNforeground,                   cfg->Color.foreground.xcolor.pixel,
       XmNtopAttachment,                XmATTACH_NONE,
       XmNleftAttachment,               XmATTACH_FORM,
       XmNrightAttachment,              XmATTACH_NONE,
       XmNbottomAttachment,             XmATTACH_FORM,
       0);

    sgi->msg_lbl = XtVaCreateWidget     /* unmanaged */
      ("messageLabel",
       xmLabelWidgetClass,              parent,
       XmNtopAttachment,                XmATTACH_WIDGET,
       XmNtopWidget,                    sgi->title_lbl,
       XmNleftAttachment,               XmATTACH_WIDGET,
       XmNleftWidget,                   sgi->y_axis,
       XmNrightAttachment,              XmATTACH_WIDGET,
       XmNrightWidget,                  sgi->legend,
       XmNrightOffset,                  LEGEND_OFFSET,
       XmNbottomAttachment,             XmATTACH_WIDGET,
       XmNbottomWidget,                 sgi->x_axis,
       0);

    XtAddCallback (sgi->canvas, XmNresizeCallback, callback, sgi);
    XtAddCallback (sgi->canvas, XmNexposeCallback, callback, sgi);

    /* add event handlers to track pointer */
    XtAddEventHandler
      (sgi->canvas, EnterWindowMask | LeaveWindowMask, False,
       (XtEventHandler)crossing_event_handler,
       (XtPointer)sgi);
    XtAddEventHandler
      (sgi->canvas, PointerMotionMask, False,
       (XtEventHandler)motion_event_handler,
       (XtPointer)sgi);
       

    /* initializations */
    sgi->parent         = parent;
    sgi->display        = XtDisplay (parent);
    sgi->window         = XtWindow (sgi->canvas);
    sgi->gc             = XCreateGC (sgi->display, sgi->window, 0, 0);
    sgi->screen         = DefaultScreen (sgi->display);

    /* zero out these fields which are determined by calling Strip_resize */
    sgi->pixmap         = 0;
    sgi->plotpix        = 0;
  
    /* default values */
    sgi->title          = 0;

    /* other initializations */
    sgi->data           = 0;
    sgi->config         = cfg;

    for (i = 0; i < STRIP_MAX_CURVES; i++) sgi->curves[i] = 0;
    sgi->selected_curve = 0;

    get_current_time (&sgi->t1);
    dbl2time (&tv, sgi->config->Time.timespan);
    subtract_times (&sgi->t0, &tv, &sgi->t1);
    sgi->plotted_t0.tv_sec      = 0;
    sgi->plotted_t0.tv_usec     = 0;
    sgi->plotted_t1.tv_sec      = 0;
    sgi->plotted_t1.tv_usec     = 0;
      
    /* default area is entire window */
    sgi->window_rect.x = 0;
    sgi->window_rect.y = 0;

    sgi->draw_mask = SGCOMPMASK_ALL;
    sgi->status = SGSTAT_GRAPH_REFRESH;

    sgi->user_data = NULL;
  }
  
  return (StripGraph)sgi;
}


/*
 * StripGraph_delete
 */
void StripGraph_delete (StripGraph the_graph)
{
  StripGraphInfo        *sgi = (StripGraphInfo *)the_graph;
  
  if (sgi->plotpix) XFreePixmap (sgi->display, sgi->plotpix);
  if (sgi->pixmap) XFreePixmap (sgi->display, sgi->pixmap);
  if (sgi->gc) XFreeGC (sgi->display, sgi->gc);
  
  free (sgi);
}


/*
 * StripGraph_setattr
 */
int     StripGraph_setattr      (StripGraph the_sgi, ...)
{
  va_list               ap;
  StripGraphInfo        *sgi = (StripGraphInfo *)the_sgi;
  int                   attrib;
  int                   ret_val = 1;
  int                   tmp_int;
  XmString              xstr;

  
  va_start (ap, the_sgi);
  for (attrib = va_arg (ap, StripGraphAttribute);
       (attrib != 0);
       attrib = va_arg (ap, StripGraphAttribute))
  {
    if ((ret_val = ((attrib > 0) && (attrib < STRIPGRAPH_LAST_ATTRIBUTE))))
      switch (attrib)
      {
          case STRIPGRAPH_HEADING:
            sgi->title = va_arg (ap, char *);
            xstr = XmStringCreateLocalized (sgi->title? sgi->title : "");
            XtVaSetValues (sgi->title_lbl, XmNlabelString, xstr, 0);
            XmStringFree (xstr);
            break;

          case STRIPGRAPH_DATA_SOURCE:
            sgi->data = va_arg (ap, StripDataSource);
            break;

          case STRIPGRAPH_BEGIN_TIME:
            ptv = va_arg (ap, struct timeval *);
            sgi->t0.tv_sec = ptv->tv_sec;
            sgi->t0.tv_usec = ptv->tv_usec;
            break;

          case STRIPGRAPH_END_TIME:
            ptv = va_arg (ap, struct timeval *);
            sgi->t1.tv_sec = ptv->tv_sec;
            sgi->t1.tv_usec = ptv->tv_usec;
            break;

          case STRIPGRAPH_USER_DATA:
            sgi->user_data = va_arg (ap, char *);
            break;
      }
  }

  va_end (ap);
  return ret_val;
}


/*
 * StripGraph_getattr
 */
int     StripGraph_getattr      (StripGraph the_sgi, ...)
{
  va_list               ap;
  StripGraphInfo        *sgi = (StripGraphInfo *)the_sgi;
  int                   attrib;
  int                   ret_val = 1;

  
  va_start (ap, the_sgi);
  for (attrib = va_arg (ap, StripGraphAttribute);
       (attrib != 0);
       attrib = va_arg (ap, StripGraphAttribute))
  {
    if ((ret_val = ((attrib > 0) && (attrib < STRIPGRAPH_LAST_ATTRIBUTE))))
      switch (attrib)
      {
          case STRIPGRAPH_WIDGET:
            *(va_arg (ap, Widget *)) = sgi->canvas;
            break;

          case STRIPGRAPH_HEADING:
            *(va_arg (ap, char **)) = sgi->title;
            break;

          case STRIPGRAPH_DATA_SOURCE:
            *(va_arg (ap, StripDataSource *)) = sgi->data;
            break;

          case STRIPGRAPH_BEGIN_TIME:
            *(va_arg (ap, struct timeval *)) = sgi->t0;
            break;

          case STRIPGRAPH_END_TIME:
            *(va_arg (ap, struct timeval *)) = sgi->t1;
            break;

          case STRIPGRAPH_USER_DATA:
            *(va_arg (ap, char **)) = (char *)sgi->user_data;
            break;
      }
  }

  va_end (ap);
  return ret_val;
}


static void StripGraph_manage_geometry (StripGraphInfo *sgi)
{
  AxisEndpointPosition  minpos, maxpos;
  double                pixels_per_mm;
  double                num_pixels;
  Dimension             w, h;
  Position              x, y, xx, yy;

  /* get dimensions and location of graph window */ 
  XtVaGetValues
    (sgi->canvas,
     XmNwidth,          &w,
     XmNheight,         &h,
     XmNx,              &x,
     XmNy,              &y,
     0);

  sgi->window_rect.width = w;
  sgi->window_rect.height = h;

  /* reset x axis endpoints */
  XtVaGetValues
    (sgi->x_axis,
     XmNx,                      &xx,
     XmNwidth,                  &w,
     0);
  minpos = x + sgi->window_rect.x - xx;
  maxpos = minpos + sgi->window_rect.width - 1;
  
  XtVaSetValues
    (sgi->x_axis,
     XjNminPos,                 minpos,
     XjNmaxPos,                 maxpos,
     0);

  /* reset y axis endpoints */
  XtVaGetValues
    (sgi->y_axis,
     XmNy,                      &yy,
     XmNheight,                 &h,
     0);
  minpos = (yy + h) - (y + sgi->window_rect.y + sgi->window_rect.height);
  maxpos = minpos + sgi->window_rect.height - 1;

  XtVaSetValues
    (sgi->y_axis,
     XjNminPos,                 minpos,
     XjNmaxPos,                 maxpos,
     0);

  /* plot area pixmap and "double buffer" pixmap */
  if (sgi->plotpix) XFreePixmap (sgi->display, sgi->plotpix);
  if (sgi->pixmap) XFreePixmap (sgi->display, sgi->pixmap);

  /* need to catch failure */
  XSynchronize (sgi->display, True);

  Strip_x_error_code = Success;
  sgi->plotpix = XCreatePixmap
    (sgi->display, sgi->window,
     sgi->window_rect.width, sgi->window_rect.height,
     sgi->config->xvi.depth);
  if (Strip_x_error_code != Success) sgi->plotpix = 0;
  
  Strip_x_error_code = Success;
  sgi->pixmap = XCreatePixmap
    (sgi->display, sgi->window,
     sgi->window_rect.width, sgi->window_rect.height,
     sgi->config->xvi.depth);
  if (Strip_x_error_code != Success) sgi->pixmap = 0;
  
  XSynchronize (sgi->display, False);

  /* clear plot pixmap */
  if (sgi->plotpix)
  {
    XSetForeground
      (sgi->display, sgi->gc, sgi->config->Color.background.xcolor.pixel);
    XFillRectangle
      (sgi->display, sgi->plotpix, sgi->gc, 
       0, 0, sgi->window_rect.width+1, sgi->window_rect.height+1);
  }

  /* if couldn't get a pixmap, then map the error message label */
  if (!sgi->plotpix || !sgi->pixmap)
    XtManageChild (sgi->msg_lbl);
  else if (XtIsManaged (sgi->msg_lbl))
    XtUnmanageChild (sgi->msg_lbl);

  sgi->draw_mask = SGCOMPMASK_ALL;
  StripGraph_setstat (sgi, SGSTAT_GRAPH_REFRESH);
}


/*
 * StripGraph_draw
 */ 
void StripGraph_draw    (StripGraph     the_graph,
                         unsigned       components,
                         Region         *area)
{
  StripGraphInfo        *sgi = (StripGraphInfo *)the_graph;
  Pixel                 text_color;
  int                   i, n, w, pos;
  int                   do_it;
  int                   update_loc_lbl = 0;
  Region                clip_region;
  XFontStruct           *font;
  double                dbl_min, dbl_max;
  double                log_epsilon;
  AxisTransform         transform;
  int                   tic_offsets[2*(AXIS_MAX_TICS+1)];
  char                  buf[256];

  /* draw components specified as well as those which need to be drawn */
  sgi->draw_mask |= components;

  /* only draw if the window is viewable */
  if (!window_isviewable (sgi->display, sgi->window) ||
      !window_ismapped (sgi->display, sgi->window))
    return;

  /* if either pixmap is bad, write a message to the window
   * and return */
  if (!sgi->pixmap || !sgi->plotpix)
  {
    /* it'd be nice to give some error indication here */
    return;
  }

  if (sgi->draw_mask & SGCOMPMASK_TITLE)
  {
    XtVaSetValues
      (sgi->title_lbl,
       XmNforeground,   sgi->config->Color.foreground.xcolor.pixel,
       XmNbackground,   sgi->config->Color.background.xcolor.pixel,
       0);
    sgi->draw_mask &= ~SGCOMPMASK_TITLE;
  }

  /* ====== x axis ====== */
  if (sgi->draw_mask & SGCOMPMASK_XAXIS)
  {
    dbl_min = time2dbl (&sgi->t0);
    dbl_max = time2dbl (&sgi->t1);
    XtVaSetValues
      (sgi->x_axis,
       XjNminVal,       &dbl_min,
       XjNmaxVal,       &dbl_max,
       XmNforeground,   sgi->config->Color.foreground.xcolor.pixel,
       XjNtextColor,    sgi->config->Color.foreground.xcolor.pixel,
       XmNbackground,   sgi->config->Color.background.xcolor.pixel,
       0);
    sgi->draw_mask &= ~SGCOMPMASK_XAXIS;
    update_loc_lbl = 1;
  }
      
  /* ====== y axis ====== */
  if (sgi->draw_mask & SGCOMPMASK_YAXIS)
  {
    if (!sgi->selected_curve)
    {
      for (i = 0; i < STRIP_MAX_CURVES; i++)
        if (sgi->curves[i])
        {
          sgi->selected_curve = sgi->curves[i];
          break;
        }
    }
    
    text_color = sgi->config->Color.foreground.xcolor.pixel;
    if (sgi->selected_curve)
    {
      if (sgi->config->Option.axis_ycolorstat)
        text_color = sgi->selected_curve->details->color->xcolor.pixel;

      log_epsilon = - sgi->selected_curve->details->precision;
      transform = sgi->selected_curve->details->scale == STRIPSCALE_LOG_10?
        XjAXIS_LOG10 : XjAXIS_LINEAR;
      
      XtVaSetValues
        (sgi->y_axis,
         XjNminVal,     &sgi->selected_curve->details->min,
         XjNmaxVal,     &sgi->selected_curve->details->max,
         XmNforeground, sgi->config->Color.foreground.xcolor.pixel,
         XmNbackground, sgi->config->Color.background.xcolor.pixel,
         XjNtextColor,  text_color,
         XjNunitString, sgi->selected_curve->details->egu,
         XjNlogEpsilon, &log_epsilon,
         XjNtransform,  transform,
         0);
    }
    else
    {
      dbl_min = 0;
      dbl_max = 1;
      XtVaSetValues
        (sgi->y_axis,
         XjNminVal,     &dbl_min,
         XjNmaxVal,     &dbl_max,
         XmNforeground, sgi->config->Color.foreground.xcolor.pixel,
         XmNbackground, sgi->config->Color.background.xcolor.pixel,
         XjNtextColor,  text_color,
         XjNunitString, 0,
         XjNtransform,  XjAXIS_LINEAR,
         0);
    }
    
    sgi->draw_mask &= ~SGCOMPMASK_YAXIS;
    update_loc_lbl = 1;
  }


  /* ====== location label ====== */
  /*
   * If either of the X or Y axes has changed, then must update the
   * location label to reflect change in mapping from raster location
   * to axis value.
   */
  if (update_loc_lbl)
  {
    XtVaGetValues (sgi->y_axis, XjNtextColor, &text_color, 0);
    XtVaSetValues
      (sgi->loc_lbl,
       XmNforeground,   text_color,
       XmNbackground,   sgi->config->Color.background.xcolor.pixel,
       0);
    StripGraph_update_loc_lbl (sgi);
  }

  /* ====== legend ====== */
  if ((sgi->draw_mask & SGCOMPMASK_LEGEND) &&
      StripGraph_getstat ((StripGraph)sgi, SGSTAT_LEGEND_REFRESH))
  {
    /* make sure the legend info is up to date */
    for (i = 0; i < STRIP_MAX_CURVES; i++)
      if (sgi->curves[i])
      {
        XtVaSetValues
          (sgi->legend,
           XmNforeground,       sgi->config->Color.foreground.xcolor.pixel,
           XmNbackground,       sgi->config->Color.background.xcolor.pixel,
           0);
        sprintf
          (buf,
           sgi->curves[i]->details->scale == STRIPSCALE_LOG_10?
           "log10 (%g, %g)" : "(%g, %g)",
           sgi->curves[i]->details->min, sgi->curves[i]->details->max);
        XjLegendUpdateItem
          (sgi->legend,
           sgi->lgitems[i],
           sgi->curves[i]->details->name,
           buf,
           strcmp (sgi->curves[i]->details->egu, STRIPDEF_CURVE_EGU)?
           sgi->curves[i]->details->egu : 0,
           sgi->curves[i]->details->comment,
           sgi->curves[i]->details->color->xcolor.pixel);
      }
    XjLegendResize (sgi->legend);
    sgi->draw_mask &= ~SGCOMPMASK_LEGEND;
    StripGraph_clearstat (sgi, SGSTAT_LEGEND_REFRESH);
  }
  
  /* ====== plot pixmap ====== */
  if (sgi->draw_mask & SGCOMPMASK_DATA)
  {
    StripGraph_plotdata (sgi);
    sgi->draw_mask &= ~SGCOMPMASK_DATA;
  }

  /* make copy of plot on which we can overlay the grid */
  XCopyArea
    (sgi->display, sgi->plotpix, sgi->pixmap, sgi->gc,
     0, 0,
     sgi->window_rect.width, sgi->window_rect.height,
     sgi->window_rect.x, sgi->window_rect.y);

  /* draw the grid lines, updating if necessary */
  if (sgi->config->Option.grid_xon || sgi->config->Option.grid_yon)
  {
    XSetLineAttributes
      (sgi->display, sgi->gc, 1, LineOnOffDash, CapButt, JoinMiter);
    XSetForeground
      (sgi->display, sgi->gc, sgi->config->Color.grid.xcolor.pixel);

    /* x */
    if (sgi->config->Option.grid_xon == STRIPGRID_SOME ||
        sgi->config->Option.grid_xon == STRIPGRID_ALL)
      n = XjAxisGetMajorTicOffsets (sgi->x_axis, tic_offsets, AXIS_MAX_TICS+1);
    if (sgi->config->Option.grid_xon == STRIPGRID_ALL)
      n += XjAxisGetMinorTicOffsets
        (sgi->x_axis, tic_offsets + n, AXIS_MAX_TICS+1);
    if (sgi->config->Option.grid_xon != STRIPGRID_NONE)
    {
      for (i = 0; i < n; i++)
      {
        pos = sgi->window_rect.x + tic_offsets[i];
        sgi->grid.v_seg[i].x1 = sgi->grid.v_seg[i].x2 = pos;
        sgi->grid.v_seg[i].y1 = sgi->window_rect.y;
        sgi->grid.v_seg[i].y2 =
          sgi->window_rect.y + sgi->window_rect.height - 1;
      }
      XDrawSegments (sgi->display, sgi->pixmap, sgi->gc, sgi->grid.v_seg, n);
    }
    
    /* y */
    if (sgi->config->Option.grid_yon == STRIPGRID_SOME ||
        sgi->config->Option.grid_yon == STRIPGRID_ALL)
      n = XjAxisGetMajorTicOffsets (sgi->y_axis, tic_offsets, AXIS_MAX_TICS+1);
    if (sgi->config->Option.grid_yon == STRIPGRID_ALL)
      n += XjAxisGetMinorTicOffsets
        (sgi->y_axis, tic_offsets + n, AXIS_MAX_TICS+1);
    if (sgi->config->Option.grid_yon)
    {
      for (i = 0; i < n; i++)
      {
        pos = sgi->window_rect.y + sgi->window_rect.height - 1;
        pos -= tic_offsets[i];
        sgi->grid.h_seg[i].y1 = sgi->grid.h_seg[i].y2 = pos;
        sgi->grid.h_seg[i].x1 = sgi->window_rect.x;
        sgi->grid.h_seg[i].x2 =
          sgi->window_rect.x + sgi->window_rect.width - 1;
      }
      XDrawSegments (sgi->display, sgi->pixmap, sgi->gc, sgi->grid.h_seg, n);
    }
  }

  /* copy pixmap to window */
  if (area) XSetRegion (sgi->display, sgi->gc, *area);
  XCopyArea
    (sgi->display, sgi->pixmap, sgi->window, sgi->gc,
     0, 0,
     sgi->window_rect.width, sgi->window_rect.height,
     sgi->window_rect.x, sgi->window_rect.y);
  if (area) XSetClipMask (sgi->display, sgi->gc, None);
  XFlush(sgi->display);
}


/*
 * StripGraph_plotdata
 *
 * This routine updates the graph pixmap, plotting all data on {t0, t1} to
 * the points along the width of the pixmap.
 */
static void StripGraph_plotdata (StripGraphInfo *sgi)
{
  StripCurveInfo        *curve;
  XSegment              *segs;
  register XSegment     *pseg;
  struct timeval        dt_new, dt_cur; /* interval width (time) */
  double                dl_new, dl_cur; /* interval width (real) */
  double                db;             /* bin width (real) */
  double                dl;             /* interval width (real) */
  double                l_min, l_max;   /* min, max (real) */
  int                   b_min, b_max;   /* min, max (quantized) */
  int                   n_shift;
  int                   n, m;
  struct timeval        t;
  double                r;
  sdsRenderTechnique    method;
  sgTransformYData      y_data;
  sgTransformXData      x_data;
  Boolean               need_xform;
  Boolean               ok;

  /* new and current interval widths, in real and time types */
  dl_new = subtract_times (&dt_new, &sgi->t0, &sgi->t1);
  dl_cur = subtract_times (&dt_cur, &sgi->plotted_t0, &sgi->plotted_t1);
    
  if (!StripGraph_getstat (sgi, SGSTAT_GRAPH_REFRESH))
  {
    /* current bin width */
    db = dl_cur / (sgi->window_rect.width - 1);
    
    /* translate new min and values, using current interval */
    l_min = subtract_times (&t, &sgi->plotted_t0, &sgi->t0);
    b_min = (int)(l_min / db);
    l_max = subtract_times (&t, &sgi->plotted_t0, &sgi->t1);
    b_max = (int)(l_max / db);
    
    /* n_shift <-- number of bins which need to be shifted */
    n_shift = b_min;

    /* need to redraw if any of the following is true:
     *
     *  o  interval width has changed
     *  o  new range doesn't intersect previous
     *  o  range intersects, but shift is in negative direction
     */
    if ((ABS(dl_new - dl_cur) > DBL_EPSILON) ||
        (b_min >= sgi->window_rect.width - 1) ||
        (b_max <= 0) ||
        (n_shift < 0))
      StripGraph_setstat (sgi, SGSTAT_GRAPH_REFRESH);
  }
  
  /* if everything needs to be re-plotted, erase the whole pixmap */
  if (StripGraph_getstat (sgi, SGSTAT_GRAPH_REFRESH))
  {
    /* clear the pixmap */
    XSetForeground
      (sgi->display, sgi->gc, sgi->config->Color.background.xcolor.pixel);
    XFillRectangle
      (sgi->display, sgi->plotpix, sgi->gc,
       0, 0, sgi->window_rect.width+1, sgi->window_rect.height+1);

    sgi->plotted_t0 = sgi->t0;
    sgi->plotted_t1 = sgi->t1;
    db = dl_new / (sgi->window_rect.width - 1);
    dl = dl_new;
    method = SDS_REFRESH_ALL;
  }

  /* if only a portion needs to be plotted, re-arrange the displayed data
   * to reflect the new time range and clear the vacated portion of the
   * pixmap */
  else
  {
    XCopyArea
      (sgi->display,
       sgi->plotpix, sgi->plotpix,
       sgi->gc,
       n_shift, 0,
       sgi->window_rect.width - n_shift,
       sgi->window_rect.height,
       0, 0);

    /* clear the vacated area */
    XSetForeground
      (sgi->display, sgi->gc, sgi->config->Color.background.xcolor.pixel);
    XFillRectangle
      (sgi->display, sgi->plotpix, sgi->gc,
       sgi->window_rect.width - n_shift, 0,
       n_shift, sgi->window_rect.height + 1);

    /* update the endpoints by shifting in bin-sized increments */
    r = time2dbl (&sgi->plotted_t0);
    dbl2time (&sgi->plotted_t0, r + (n_shift * db));
    r = time2dbl (&sgi->plotted_t1);
    dbl2time (&sgi->plotted_t1, r + (n_shift * db));

    dl = dl_cur;
    method = SDS_JOIN_NEW;
  }

  XSetLineAttributes
    (sgi->display, sgi->gc,
     sgi->config->Option.graph_linewidth,
     LineSolid, CapButt, JoinMiter);

  /* initialize data source for time interval */
  if (StripDataSource_init_range
      (sgi->data, &sgi->plotted_t0, db, sgi->window_rect.width, method) > 0)
  {
#ifdef QUANTIFY_PRECISE
    if (method == SDS_REFRESH_ALL)
      quantify_start_recording_data();
#endif
    /* for each plotted curve ... */
    for (m = 0; m < STRIP_MAX_CURVES; m++)
    {
      n = sgi->config->Curves.plot_order[m];
      curve = sgi->curves[n];
      if (!curve) continue;
      if (curve->details->plotstat != STRIPCURVE_PLOTTED) continue;

      /* if this is the selected curve, get its transform info from
       * the axis */
      if (curve == sgi->selected_curve)
        XjAxisGetTransform (sgi->y_axis, &sgi->transforms[n]);

      /* otherwise, verify that the current transform info is valid */
      else
      {
        if (curve->details->scale == STRIPSCALE_LOG_10)
          need_xform = (sgi->transforms[n].transform != XjAXIS_LOG10);
        else need_xform = (sgi->transforms[n].transform != XjAXIS_LINEAR);
        
        need_xform |= (sgi->transforms[n].min_pos == 0);
        need_xform |=
          (sgi->transforms[n].max_pos == sgi->window_rect.height - 1);
        need_xform |= (sgi->transforms[n].min_val != curve->details->min);
        need_xform |= (sgi->transforms[n].max_val != curve->details->max);
        need_xform |=
          (sgi->transforms[n].log_epsilon != curve->details->precision);
        
        if (need_xform)
        {
          ok = jlaBuildTransform
            (&sgi->transforms[n],
             curve->details->scale == STRIPSCALE_LOG_10?
             XjAXIS_LOG10 : XjAXIS_LINEAR,
             XjAXIS_REAL,
             (AxisEndpointPosition)0,
             (AxisEndpointPosition)(sgi->window_rect.height - 1),
             curve->details->min,
             curve->details->max,
             -curve->details->precision);
          
          if (!ok) continue;
        }
      }
  
      y_data.xform = &sgi->transforms[n];
      y_data.sgi = sgi;
      y_data.curve = curve;

      x_data.t0 = time2dbl (&sgi->plotted_t0);
      x_data.db = db;

      /* transform data into axis coordinates */
      n = StripDataSource_render
        (sgi->data, curve,
         (sdsTransform)x_transform, &x_data,
         (sdsTransform)y_transform, &y_data,
         &segs);

      /* draw the segments */
      if (n > 0)
      {
        XSetForeground
          (sgi->display, sgi->gc, curve->details->color->xcolor.pixel);
        XDrawSegments (sgi->display, sgi->plotpix, sgi->gc, segs, n);
        StripGraph_clearstat (sgi, SGSTAT_GRAPH_REFRESH);
      }
    }
#ifdef QUANTIFY_PRECISE
    quantify_stop_recording_data();
#endif
    
  }
}


/*
 * StripGraph_addcurve
 */
int     StripGraph_addcurve     (StripGraph the_sgi, StripCurve curve)
{
  StripGraphInfo        *sgi = (StripGraphInfo *)the_sgi;
  StripCurveInfo        *c = (StripCurveInfo *)curve;
  int                   i;
  int                   ok;
  char                  buf[256];

  for (i = 0; i < STRIP_MAX_CURVES; i++)
    if (!sgi->curves[i]) break;

  if (ok = (i < STRIP_MAX_CURVES))
  {
    sgi->curves[i] = c;

    /* build transform for this curve */
    ok = jlaBuildTransform
      (&sgi->transforms[i],
       c->details->scale == STRIPSCALE_LOG_10? XjAXIS_LOG10 : XjAXIS_LINEAR,
       XjAXIS_REAL,
       (AxisEndpointPosition)0,
       (AxisEndpointPosition)(sgi->window_rect.height - 1),
       c->details->min,
       c->details->max,
       -c->details->precision);

    if (!ok) {
      fprintf (stderr, "unable to build transform for curve\n");
      sgi->curves[i] = 0;
      return 0;
    }
       
    sprintf
      (buf, "(%g, %g)",
       sgi->curves[i]->details->min, sgi->curves[i]->details->max);
    sgi->lgitems[i] = XjLegendNewItem
      (sgi->legend,
       sgi->curves[i]->details->name,
       buf,
       sgi->curves[i]->details->egu,
       sgi->curves[i]->details->comment,
       sgi->curves[i]->details->color->xcolor.pixel);
       
    StripGraph_setstat (sgi, SGSTAT_GRAPH_REFRESH | SGSTAT_LEGEND_REFRESH);
  }

  return ok;
}


/*
 * StripGraph_removecurve
 */
int     StripGraph_removecurve  (StripGraph the_sgi, StripCurve curve)
{
  StripGraphInfo        *sgi = (StripGraphInfo *)the_sgi;
  int                   i;
  int                   ret_val;

  for (i = 0; i < STRIP_MAX_CURVES; i++)
    if (sgi->curves[i] == (StripCurveInfo *)curve)
      break;

  if (ret_val = (i < STRIP_MAX_CURVES))
  {
    XjLegendDeleteItem (sgi->legend, sgi->lgitems[i]);
    if (sgi->selected_curve == (StripCurveInfo *)curve)
      sgi->selected_curve = NULL;
    sgi->curves[i] = 0;
    sgi->lgitems[i] = 0;
    StripGraph_setstat (sgi, SGSTAT_GRAPH_REFRESH | SGSTAT_LEGEND_REFRESH);
  }

  return ret_val;
}


/*
 * StripGraph_dumpdata
 */
int     StripGraph_dumpdata     (StripGraph the_sgi, FILE *f)
{
  StripGraphInfo        *sgi = (StripGraphInfo *)the_sgi;
  
  return StripDataSource_dump (sgi->data, f);
}


/*
 * StripGraph_print
 */
void StripGraph_print (StripGraph the_sgi)
{
  StripGraphInfo        *sgi = (StripGraphInfo *)the_sgi;

  fprintf (stdout, "StripGraph_print() is under construction :)\n");
}


/*
 * StripGraph_setstat
 */
unsigned        StripGraph_setstat      (StripGraph     the_sgi,
                                         unsigned       stat)
{
  StripGraphInfo        *sgi = (StripGraphInfo *)the_sgi;
  
  return (sgi->status |= stat);
}

/*
 * StripGraph_getstat
 */
unsigned        StripGraph_getstat      (StripGraph     the_sgi,
                                         unsigned       stat)
{
  StripGraphInfo        *sgi = (StripGraphInfo *)the_sgi;
  
  return (sgi->status & stat);
}

/*
 * StripGraph_clearstat
 */
unsigned        StripGraph_clearstat    (StripGraph     the_sgi,
                                         unsigned        stat)
{
  StripGraphInfo        *sgi = (StripGraphInfo *)the_sgi;
  
  return (sgi->status &= ~stat);
}


/*
 * StripGraph_update_loc_lbl
 */
static void     StripGraph_update_loc_lbl (StripGraphInfo *sgi)
{
  double    x = sgi->loc_xy.x;
  double    y = sgi->window_rect.height - 1 - sgi->loc_xy.y;
  time_t    tt;
  char      buf[256];
  char      *p;
  XmString  xstr;
  
  /* Need to translate (x,y) raster location into (time, value). */
  XjAxisUntransformRasterizedValues (sgi->x_axis, &x, &x, 1);
  XjAxisUntransformRasterizedValues (sgi->y_axis, &y, &y, 1);

  tt = (time_t)x;
  buf[0] = '(';
  strftime (buf+1, 254, "%H:%M:%S", localtime (&tt));
  p = buf; while (*p) p++;
  sprintf (p, ", %g)", y);
  XtVaSetValues (sgi->loc_lbl, XmNstring, buf, 0);
}


static void     callback        (Widget w, XtPointer client, XtPointer call)
{
  static Region                 expose_region = (Region)0;

  XmDrawingAreaCallbackStruct   *cbs;
  XEvent                        *event;
  StripGraphInfo                *sgi;
  Dimension                     width, height;
  XRectangle                    r;
  int                           i;

  sgi = (StripGraphInfo *)client;

  if (w == sgi->canvas)
  {
    XmDrawingAreaCallbackStruct *cbs = (XmDrawingAreaCallbackStruct *)call;
    event = cbs->event;

    /* expose or resize */
    sgi->draw_mask |= SGCOMPMASK_DATA | SGCOMPMASK_GRID;

    /* resize (or first expose --use pixmap as flag)? */
    if ((cbs->reason == XmCR_RESIZE) || !sgi->pixmap)
      StripGraph_manage_geometry (sgi);

    /* expose? */
    else if (cbs->reason == XmCR_EXPOSE)
    {
      if (!expose_region) expose_region = XCreateRegion();
      
      r.x = event->xexpose.x;
      r.y = event->xexpose.y;
      r.width = event->xexpose.width;
      r.height = event->xexpose.height;
      
      XUnionRectWithRegion (&r, expose_region, expose_region);
      
      if (event->xexpose.count == 0)
      {
        StripGraph_draw ((StripGraph)sgi, 0, &expose_region);
        XDestroyRegion (expose_region);
        expose_region = (Region)0;
      }
    }
  }

  else if (w == sgi->legend)
  {
    XjLegendCallbackStruct *cbs = (XjLegendCallbackStruct *)call;
    
    for (i = 0; i < STRIP_MAX_CURVES; i++)
      if (sgi->lgitems[i] == cbs->item)
      {
        sgi->selected_curve = sgi->curves[i];
        StripGraph_draw
          (sgi, SGCOMPMASK_YAXIS | SGCOMPMASK_GRID, (Region *)0);
        break;
      }
  }
}


static void     crossing_event_handler    (Widget           w,
                                           XtPointer        data,
                                           XCrossingEvent   *event,
                                           Boolean          *BOGUS(dispatch))
{
  StripGraphInfo  *sgi = (StripGraphInfo *)data;

  if (event->type == EnterNotify)
  {
    sgi->loc_xy.x = event->x;
    sgi->loc_xy.y = event->y;
    StripGraph_update_loc_lbl (sgi);
    XtMapWidget (sgi->loc_lbl);
  }
  else if (event->type == LeaveNotify)
  {
    XtUnmapWidget (sgi->loc_lbl);
  }
}


static void     motion_event_handler      (Widget           w,
                                           XtPointer        data,
                                           XMotionEvent     *event,
                                           Boolean          *BOGUS(dispatch))
{
  StripGraphInfo  *sgi = (StripGraphInfo *)data;
#if 0
  Bool            status = False;
  XEvent          last;

  /* let's discard all but the most recent motion event */
  last.xmotion = *event;
  while (status)
    status = XCheckTypedWindowEvent
      (event->display, event->window, MotionNotify, &last);
  
  sgi->loc_xy.x = last.xmotion.x;
  sgi->loc_xy.y = last.xmotion.y;
#else
  sgi->loc_xy.x = event->x;
  sgi->loc_xy.y = event->y;
#endif
    
  StripGraph_update_loc_lbl (sgi);
}


static void     y_transform     (void                   *arg,
                                 register double        *in,
                                 register double        *out,
                                 register int           n)
{
  sgTransformYData      *data = (sgTransformYData *)arg;
  StripGraphInfo        *sgi = data->sgi;
  StripCurveInfo        *cv = data->curve;
  register double       height = sgi->window_rect.height;
  register double       delta;

  /* transform the points, then translate to graph coordinates */
  jlaTransformValuesRasterized (data->xform, in, out, n);
  while (--n >= 0)
  {
    *out = height - 1 - *out;
    out++;
  }
}


static void     x_transform     (void                   *arg,
                                 register double        *in,
                                 register double        *out,
                                 register int           n)
{
  sgTransformXData      *data = (sgTransformXData *)arg;
  register double       t0 = data->t0;
  register double       db = data->db;

  while (--n >= 0) *out++ = (*in++ - t0) / db;
}
