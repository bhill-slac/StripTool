/*-----------------------------------------------------------------------------
 * Copyright (c) 1996 Southeastern Universities Research Association,
 *               Continuous Electron Beam Accelerator Facility
 *
 * This software was developed under a United States Government license
 * described in the NOTICE file included as part of this distribution.
 *
 *-----------------------------------------------------------------------------
 */


#ifndef _StripCurve
#define _StripCurve

#include "StripDefines.h" 
#include "StripConfig.h"

#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

#define STRIPCURVE_PENDOWN	1
#define STRIPCURVE_PLOTTED	1

/* ======= Data Types ======= */
typedef struct
{
  struct timeval	req_t0, req_t1;	/* the requested (begin, end) times */
  struct timeval	*time;		/* the corresponding time stamps */
  double		*data;		/* the corresponding data */
  int			n;		/* (n=0) --> no data; (n<0) --> status */
}
StripCurveHistory;


typedef void *		StripCurve;

typedef double 		(*StripCurveSampleFunc)		(void *);
typedef int		(*StripCurveHistoryFunc)	(void *,
                                                         struct timeval *,
                                                         struct timeval *,
                                                         StripCurveHistory *);

/* ======= Attributes ======= */
typedef enum
{
  STRIPCURVE_NAME = 1,		/* (char *)				rw */
  STRIPCURVE_EGU,		/* (char *)				rw */
  STRIPCURVE_COMMENT,		/* (char *)				rw */
  STRIPCURVE_PRECISION,		/* (int)				rw */
  STRIPCURVE_MIN,		/* (double)				rw */
  STRIPCURVE_MAX,		/* (double)				rw */
  STRIPCURVE_PENSTAT,		/* (int)   plot new data?		rw */
  STRIPCURVE_PLOTSTAT,		/* (int)   curve data plotted?		rw */
  STRIPCURVE_WAITSTAT,		/* (int)   waiting for connection?	rw */
  STRIPCURVE_CONNECTSTAT,	/* (int)   curve connected?		rw */
  STRIPCURVE_COLOR,		/* (cColor *)			r  */
  STRIPCURVE_FUNCDATA,		/* (void *)				rw */
  STRIPCURVE_SAMPLEFUNC,	/* (StripCurveSampleFunc)		rw */
  STRIPCURVE_HISTORYFUNC,	/* (StripCurveSampleFunc)		rw */
  STRIPCURVE_LAST_ATTRIBUTE
}
StripCurveAttribute;

enum	_scidx
{
  SCIDX_WAITING	= 0,
  SCIDX_CHECK_CONNECT,
  SCIDX_CONNECTED,
  SCIDX_EGU_SET,
  SCIDX_COMMENT_SET,
  SCIDX_PRECISION_SET,
  SCIDX_MIN_SET,
  SCIDX_MAX_SET
};


typedef enum
{
  STRIPCURVE_WAITING 		= (1 << SCIDX_WAITING),
  STRIPCURVE_CHECK_CONNECT	= (1 << SCIDX_CHECK_CONNECT),
  STRIPCURVE_CONNECTED		= (1 << SCIDX_CONNECTED),
  STRIPCURVE_EGU_SET		= (1 << SCIDX_EGU_SET),
  STRIPCURVE_COMMENT_SET	= (1 << SCIDX_COMMENT_SET),
  STRIPCURVE_PRECISION_SET	= (1 << SCIDX_PRECISION_SET),
  STRIPCURVE_MIN_SET		= (1 << SCIDX_MIN_SET),
  STRIPCURVE_MAX_SET		= (1 << SCIDX_MAX_SET)
}
StripCurveStatus;



/* ======= Functions ======= */
/* StripCurve_set/getattr
 *
 *	Sets or gets the specified attributes, returning true on success,
 *	false otherwise.
 */
int	StripCurve_setattr	(StripCurve, ...);
int	StripCurve_getattr	(StripCurve, ...);


/*
 * StripCurve_update
 *
 *	Causes outstanding modifications to be propagated via the StripConfig
 *	component.
 */
void	StripCurve_update	(StripCurve);


/*
 * StripCurve_set/getattr_val
 *
 *	Gets the specified attribute, returning a void pointer to it (must
 *	be casted before being accessed).
 */
void	*StripCurve_getattr_val	(StripCurve, StripCurveAttribute);


/*
 * StripCurve_set/get/clearstat
 *
 *	Set:	sets the specified status bit high.
 *	Get: 	returns true iff the specified status bit is high.
 *	Clear: 	sets the specified status bit low.
 */
void			StripCurve_setstat	(StripCurve, unsigned);
StripCurveStatus	StripCurve_getstat	(StripCurve, unsigned);
void			StripCurve_clearstat	(StripCurve, unsigned);



/* ======= Private Data (not for client program use) ======= */
typedef struct
{
  StripConfig		*scfg;
  StripCurveDetail	*details;
  void			*id;
  struct timeval	connect_request;
  void			*func_data;
  StripCurveSampleFunc	get_value;	/* must pass func_data when calling */
  StripCurveHistoryFunc	get_history;	/* must pass func_data when calling */
  StripCurveHistory	history;
  unsigned		status;
}
StripCurveInfo;

#endif
