/*-----------------------------------------------------------------------------
 * Copyright (c) 1996 Southeastern Universities Research Association,
 *               Continuous Electron Beam Accelerator Facility
 *
 * This software was developed under a United States Government license
 * described in the NOTICE file included as part of this distribution.
 *
 *-----------------------------------------------------------------------------
 */


#ifndef _StripMisc
#define _StripMisc

#include <X11/Intrinsic.h>
#include <X11/Xlib.h>
#include <time.h>

#define STRIPCURVE_PENDOWN	1
#define STRIPCURVE_PLOTTED	1

#define ONE_MILLION		1000000.0
#define ONE_THOUSAND		1000.0

#define TIME_CHARACTERS		"0123456789:"
#define REALNUM_CHARACTERS	"+-0123456789.e"


#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef min
#  define min(a,b) ((a)<(b)?(a):(b))
#endif

typedef enum
{
  STRIPCHARSET_ALL,	/* all characters in the font */
  STRIPCHARSET_TIME,	/* TIME_CHARACTERS */
  STRIPCHARSET_REALNUM,	/* REALNUM_CHARACTERS */
  STRIPCHARSET_COUNT
}
StripCharSet;


/* ====== Various Font Data and Functions ====== */

/*
 * get_font
 *
 *	Returns a pointer to the XFontStruct which most closely matches
 *	the requirements.
 *
 *	The height and width specify a bounding box into which the text must
 *	fit.  If text is NULL, then it and the width field are ignored and
 *	a font is selected on the basis of height alone, unless num_characters
 *	is non-zero in which case a font is chosen such that any string
 *	composed of num_characters characters from the specified character
 *	set is less than or equal to width pixels wide.
 */
XFontStruct	*get_font	(Display *,
				 int,		/* height */
				 char *,	/* text */
				 int,		/* width */
				 int,		/* num characters */
				 StripCharSet);	/* indicates a character set */



/* ====== Various Time Functions ====== */

struct timeval 	*add_times	(struct timeval *,      /* result */
                                 struct timeval *,      /* operand 1 */
                                 struct timeval *);     /* operand 2 */

struct timeval 	*dbl2time	(struct timeval *, double);

double          subtract_times  (struct timeval *,      /* result */
                                 struct timeval *,      /* right operand */
                                 struct timeval *);     /* left operand */

int             compare_times   (struct timeval *,      /* left operand */
                                 struct timeval *);     /* right operand */

char		*time_str 	(struct timeval *);


/* ====== Various Window Functions ====== */

int	window_isviewable	(Display *, Window);
int	window_ismapped		(Display *, Window);
void	window_map		(Display *, Window);
void	window_unmap		(Display *, Window);



/* ====== Miscellaneous ====== */

void	sec2hms	(unsigned sec, int *h, int *m, int *s);

char	*dbl2str	(double,	/* value */
			 int,		/* precision: num digits after radix */
			 char[],	/* result buffer */
			 int);		/* max length of converted string */
#endif
