/*-----------------------------------------------------------------------------
 * Copyright (c) 1996 Southeastern Universities Research Association,
 *               Continuous Electron Beam Accelerator Facility
 *
 * This software was developed under a United States Government license
 * described in the NOTICE file included as part of this distribution.
 *
 *-----------------------------------------------------------------------------
 */


#ifndef _StripDefines
#define _StripDefines

#define STRIPGRAPH_TITLE        "StripTool Graph Window"
#define STRIPGRAPH_ICON_NAME    "Graph"
#define STRIPDIALOG_TITLE       "StripTool Controls"
#define STRIPDIALOG_ICON_NAME   "Controls"

/* the maximum number of curves
 *
 * Note: increasing this value will require modifying StripConfig
 */
#if !defined (STRIP_MAX_CURVES)
#  define STRIP_MAX_CURVES      10
#endif

/* user and site application defaults files
 * The site default file is first read, then the user default.
 * Both of these are read after the X-toolkit has finished
 * its own resource merging, so these will both override
 * .Xdefaults files and command-line parameters.
 */
#define STRIP_USER_DEFAULTS_FILE        ".StripToolrc"

#define STRIP_SITE_DEFAULTS_FILE_ENV    "STRIP_SITE_DEFAULTS"
#ifndef STRIP_SITE_DEFAULTS_FILE
#  define STRIP_SITE_DEFAULTS_FILE      "/cs/op/lib/stripTool/.StripToolrc"
#endif


/* maximum number of bytes to use for caching sampled data */
#define STRIP_MAX_CACHE_BYTES           (8L*1024L*1024L)        /* 8 megs */

/* the maximum number of characters in a curve's name string */
#define STRIP_MAX_NAME_CHAR             63

/* the maximum number of characters in a curve's engineering units string */
#define STRIP_MAX_EGU_CHAR              31

/* the maximum number of characters in a curve's comment string */
#define STRIP_MAX_COMMENT_CHAR          255

/* the smallest fraction of a second which is still accurate */
#define STRIP_TIMER_ACCURACY            0.001

/* timeout period for handling Channel Access events */
#define STRIP_CA_PEND_TIMEOUT           0.1 /* Albert 0.001 */

/* timeout period for handling cdev events */
#define STRIP_CDEV_PEND_TIMEOUT         0.005

/* number of seconds to wait for a curve to connect to its data source
 * before taking some action */
#define STRIP_CONNECTION_TIMEOUT        5.0

/* the default fallback font name */
#define STRIP_FALLBACK_FONT_STR         "*fixed-medium-r-normal--10*"

/* the default dimensions (in millimeters) for the graph window */
#define STRIP_GRAPH_WIDTH_MM            250.0 /* 200 Albert */
#define STRIP_GRAPH_HEIGHT_MM           180.0 /* 100 Albert */

/* the default number of seconds of data displayed on plot */
#define STRIP_DEFAULT_TIMESPAN          300

/* the maximum font height (in millimeters) */
#define STRIP_FONT_MAXHEIGHT_MM         8.0 /* 4.0 Albert */

/* the default directory in which to find configuration files */
#ifndef STRIP_CONFIGFILE_DIR
#  define STRIP_CONFIGFILE_DIR          "."
#endif

/* the default configuration filename */
#define STRIP_DEFAULT_FILENAME          "StripTool.config"

/* the search path for finding config files specified on command line */
#define STRIP_FILE_SEARCH_PATH_ENV      "EPICS_DISPLAY_PATH"

/* the default wildcard for finding configuration files */
#ifndef STRIP_CONFIGFILE_PATTERN
#define STRIP_CONFIGFILE_PATTERN        "*"
#endif

#define STRIP_PRINTER_NAME_ENV                  "STRIP_PRINTER_NAME"
#define STRIP_PRINTER_DEVICE_ENV                "STRIP_PRINTER_DEVICE"
#define STRIP_PRINTER_NAME_FALLBACK_ENV         "PSPRINTER"

#define STRIP_PRINTER_DEVICE_FALLBACK           "ps"

/* hard-coded printer info --fallback fallback */
#ifndef STRIP_PRINTER_DEVICE
#define STRIP_PRINTER_DEVICE            "pjetxl"
#endif
#ifndef STRIP_PRINTER_NAME
#define STRIP_PRINTER_NAME              "mcc104c"
#endif

#endif
