/*-----------------------------------------------------------------------------
 * Copyright (c) 1996 Southeastern Universities Research Association,
 *               Continuous Electron Beam Accelerator Facility
 *
 * This software was developed under a United States Government license
 * described in the NOTICE file included as part of this distribution.
 *
 *-----------------------------------------------------------------------------
 */


#ifndef _StripDataSource
#define _StripDataSource

#include "StripCurve.h"
#include "StripHistory.h"


/* ======= Data Types ======= */

typedef void *  StripDataSource;

typedef enum
{
  SDS_REFRESH_ALL, SDS_JOIN_NEW
}
sdsRenderTechnique;

/* sdsTransform
 *
 *      Function to transform a set of real values into a set
 *      of plottable values.
 *
 *      The first parameter references some client-specific
 *      data used to compute the transform.  The second and
 *      third parameters are, respectively, the input and
 *      output arrays.  The transform routine must guarantee
 *      that these can both point to the same memory without
 *      ill effects (allowing an in-place transformation).
 *      The last parameter indicates the length of the arrays.
 */
typedef void    (*sdsTransform) (void *,        /* transform data */
                                 double *,      /* before */
                                 double *,      /* after */
                                 int);          /* num points */

typedef enum
{
  DATASTAT_PLOTABLE     = 1     /* the point is plotable */
} DataStatus;

/* ======= Attributes ======= */
typedef enum
{
  SDS_NUMSAMPLES = 1,   /* (size_t)     number of samples to keep       rw */
  SDS_LAST_ATTRIBUTE
} SDSAttribute;



/* ======= Functions ======= */

/*
 * StripDataSource_init
 *
 *      Creates a new strip data structure, setting all values to defaults.
 */
StripDataSource         StripDataSource_init    (StripHistory);


/*
 * StripDataSource_delete
 *
 *      Destroys the specified data buffer.
 */
void    StripDataSource_delete  (StripDataSource);


/*
 * StripDataSource_set/getattr
 *
 *      Sets or gets the specified attribute, returning true on success.
 */
int     StripDataSource_setattr (StripDataSource, ...);
int     StripDataSource_getattr (StripDataSource, ...);


/*
 * StripDataSource_addcurve
 *
 *      Tells the DataSource to acquire data for the given curve whenever
 *      a sample is requested.
 */
int     StripDataSource_addcurve        (StripDataSource, StripCurve);


/*
 * StripDataSource_removecurve
 *
 *      Removes the given curve from those the DataSource knows.
 */
int     StripDataSource_removecurve     (StripDataSource, StripCurve);


/*
 * StripDataSource_sample
 *
 *      Tells the buffer to sample the data for all curves it knows about.
 */
void    StripDataSource_sample  (StripDataSource);


/*
 * StripDataSource_init_range
 *
 *      Initializes DataSource for subsequent retrievals.  After this routine
 *      is called, and until it is called again, all requests for data or
 *      time stamps will only return data inside the range specified here.
 *      (The endpoints are included).  Returns true iff some data is available
 *      for plotting.
 *
 *      technique:              refresh all, join new
 *
 *      This specifies the technique to be used in subsequent render calls.
 */
int     StripDataSource_init_range      (StripDataSource,
                                         struct timeval *,      /* begin */
                                         double,                /* bin size */
                                         int,                   /* n bins */
                                         sdsRenderTechnique);
 

/* StripDataSource_render
 *
 *      Bins the data on the current range for the specified curve, given
 *      transform functions for both the x and y axes.
 *
 *      The resulting data is stored as a series of line segments,
 *      the starting address of which will be written into the supplied
 *      pointer location.  The number of generated segments is returned.
 *
 *      Note that the referenced XSegment array is a static buffer,
 *      so its contents are only good until the next call to render(),
 *      at which point they will be overwritten.
 *
 *      If the prevailing technique (as specified in previous call to
 *      init_range()) is JOIN_NEW, then only that data which has
 *      accumulated since the last call will be rendered, and it
 *      will be joined to the previous endpoints if appropriate.
 *
 *      In order to accomplish this, the endpoints from the resulting
 *      (joined) range are remembered at the end of the routine.
 *
 *      Assumes init_range() has already been called.
 */
size_t  StripDataSource_render  (StripDataSource,
                                 StripCurve,
                                 sdsTransform,          /* x transform */
                                 void *,                /* x transform data */
                                 sdsTransform,          /* y transform */
                                 void *,                /* y transform data */
                                 XSegment **);          /* result */


/*
 * StripDataSource_dump
 *
 *      Causes all ring buffer data for the current range to be dumped out to
 *      the specified file.
 */
int     StripDataSource_dump            (StripDataSource, FILE *);


#ifdef USE_SDDS
/*
 * StripDataSource_dump_sdds
 *
 *      Causes all ring buffer data for the current range to be output in SDDS 
 *      format to the specified filename.
 */
int     StripDataSource_dump_sdds            (StripDataSource, char *);
#endif /* SDDS */

#endif
