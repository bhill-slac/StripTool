/*-----------------------------------------------------------------------------
 * Copyright (c) 1996 Southeastern Universities Research Association,
 *               Continuous Electron Beam Accelerator Facility
 *
 * This software was developed under a United States Government license
 * described in the NOTICE file included as part of this distribution.
 *
 *-----------------------------------------------------------------------------
 */


#include "StripDataBuffer.h"
#include "StripDefines.h"
#include "StripMisc.h"

#define CURVE_DATA(C)	((CurveData *)((StripCurveInfo *)C)->id)

#define SDB_MAX_BINLINES	2048

#define SDB_LTE			0
#define SDB_GTE 		1

#define SDB_DUMP_FIELDWIDTH	30
#define SDB_DUMP_NUMWIDTH	20
#define SDB_DUMP_BADVALUESTR	"???"

typedef struct
{
  StripCurveInfo	*curve;
  double		*val;
  char			*stat;
  size_t		idx_t0, idx_t1;
} CurveData;

typedef struct
{
  size_t		buf_size;
  size_t		cur_idx;
  size_t		idx_t0, idx_t1;
  size_t		count;
  
  struct timeval	*times;
  CurveData		buffers[STRIP_MAX_CURVES];
}
StripDataBufferInfo;


/* ====== Static Function Prototypes ====== */
static int 	StripDataBuffer_resize 	(StripDataBufferInfo 	*sdb,
					 size_t 	     	buf_size);

static long	StripDataBuffer_find_date_idx 	(StripDataBufferInfo	*sdb,
						 struct timeval		*t,
						 int 			mode);
static int	pack_array	(void **, size_t,
				 int, int, int,
				 int, int *, int *);


/* ====== Static Data ====== */
static struct timezone	tz;


/* ====== Functions ====== */

/*
 * StripDataBuffer_init
 */
StripDataBuffer	StripDataBuffer_init	(void)
{
  StripDataBufferInfo	*sdb;
  int			i;

  if ((sdb = (StripDataBufferInfo *)malloc (sizeof(StripDataBufferInfo)))
      != NULL)
    {
      sdb->buf_size 	= 0;
      sdb->cur_idx	= 0;
      sdb->count	= 0;
      sdb->times	= NULL;
      sdb->idx_t0	= 0;
      sdb->idx_t1	= 0;

      for (i = 0; i < STRIP_MAX_CURVES; i++)
	{
	  sdb->buffers[i].curve 	= NULL;
          sdb->buffers[i].val		= NULL;
          sdb->buffers[i].stat		= NULL;
	  sdb->buffers[i].idx_t1	= 0;
	  sdb->buffers[i].idx_t0	= 0;
	}
    }

  return sdb;
}



/*
 * StripDataBuffer_delete
 */
void	StripDataBuffer_delete	(StripDataBuffer the_sdb)
{
  StripDataBufferInfo	*sdb = (StripDataBufferInfo *)the_sdb;
  int			i;

  for (i = 0; i < STRIP_MAX_CURVES; i++)
  {
    if (sdb->buffers[i].val)
      free (sdb->buffers[i].val);
    if (sdb->buffers[i].stat)
      free (sdb->buffers[i].stat);
  }

  free (sdb);
}



/*
 * StripDataBuffer_setattr
 */
int	StripDataBuffer_setattr	(StripDataBuffer the_sdb, ...)
{
  va_list		ap;
  StripDataBufferInfo	*sdb = (StripDataBufferInfo *)the_sdb;
  int			attrib;
  size_t		tmp;
  int			ret_val = 1;

  
  va_start (ap, the_sdb);
  for (attrib = va_arg (ap, SDBAttribute);
       (attrib != 0);
       attrib = va_arg (ap, SDBAttribute))
    {
      if ((ret_val = ((attrib > 0) && (attrib < SDB_LAST_ATTRIBUTE))))
	switch (attrib)
	  {
	  case SDB_NUMSAMPLES:
	    tmp = va_arg (ap, size_t);
	    /*
	    fprintf (stdout, "StripDataBuffer_setattr(): sdb = %p\n", sdb);
	    fprintf (stdout, "StripDataBuffer_setattr(): tmp = %u\n", tmp);
	    */
	    if (tmp > sdb->buf_size) StripDataBuffer_resize (sdb, tmp);
	    break;
	  }
    }

  va_end (ap);
  return ret_val;
}



/*
 * StripDataBuffer_getattr
 */
int	StripDataBuffer_getattr	(StripDataBuffer the_sdb, ...)
{
  va_list		ap;
  StripDataBufferInfo	*sdb = (StripDataBufferInfo *)the_sdb;
  int			attrib;
  int			ret_val = 1;

  
  va_start (ap, the_sdb);
  for (attrib = va_arg (ap, SDBAttribute);
       (attrib != 0);
       attrib = va_arg (ap, SDBAttribute))
    {
      if ((ret_val = ((attrib > 0) && (attrib < SDB_LAST_ATTRIBUTE))))
	switch (attrib)
	  {
	  case SDB_NUMSAMPLES:
	    *(va_arg (ap, size_t *)) = sdb->buf_size;
	    break;
	  }
    }

  va_end (ap);
  return ret_val;
}



/*
 * StripDataBuffer_addcurve
 */
int StripDataBuffer_addcurve	(StripDataBuffer 	the_sdb,
				 StripCurve 		the_curve)
{
  StripDataBufferInfo	*sdb = (StripDataBufferInfo *)the_sdb;
  int 			i;
  int			ret = 0;
  
  for (i = 0; i < STRIP_MAX_CURVES; i++)
    if (sdb->buffers[i].curve == NULL)		/* it's available */
      break;

  if (i < STRIP_MAX_CURVES)
  {
    sdb->buffers[i].val = (double *)malloc (sdb->buf_size * sizeof (double));
    sdb->buffers[i].stat = (char *)calloc (sdb->buf_size, sizeof (char));
    if (sdb->buffers[i].val && sdb->buffers[i].stat)
    {
      sdb->buffers[i].curve 	= (StripCurveInfo *)the_curve;
      sdb->buffers[i].idx_t0	= 0;
      sdb->buffers[i].idx_t1	= 0;
      
      /* use the id field of the strip curve to reference the buffer */
      ((StripCurveInfo *)the_curve)->id = &sdb->buffers[i];
      ret = 1;
    }
    else
    {
      if (sdb->buffers[i].val) free (sdb->buffers[i].val);
      if (sdb->buffers[i].stat) free (sdb->buffers[i].stat);
    }
  }
  
  return ret;
}



/*
 * StripDataBuffer_removecurve
 */
int StripDataBuffer_removecurve	(StripDataBuffer the_sdb, StripCurve the_curve)
{
  StripDataBufferInfo	*sdb = (StripDataBufferInfo *)the_sdb;
  CurveData		*cd;
  int			ret_val = 1;

  if ((cd = CURVE_DATA(the_curve)) != NULL)
    {
      cd->curve = NULL;
      free (cd->val);
      free (cd->stat);
      cd->val = NULL;
      cd->stat = NULL;
      ((StripCurveInfo *)the_curve)->id = NULL;
    }

  return ret_val;
}


/*
 * StripDataBuffer_sample
 */
void	StripDataBuffer_sample	(StripDataBuffer the_sdb)
{
  StripDataBufferInfo	*sdb = (StripDataBufferInfo *)the_sdb;
  StripCurveInfo	*c;
  int			i;
  int			need_time = 1;

  for (i = 0; i < STRIP_MAX_CURVES; i++)
  {
    if ((c = sdb->buffers[i].curve) != NULL)
    {
      if (need_time)
      {
        sdb->cur_idx = (sdb->cur_idx + 1) % sdb->buf_size;
        gettimeofday (&sdb->times[sdb->cur_idx], &tz);
        sdb->count = min ((sdb->count+1), sdb->buf_size);
        need_time = 0;
      }
      if ((c->status & STRIPCURVE_CONNECTED) &&
          (!(c->status & STRIPCURVE_WAITING) &&
           (c->details->penstat == STRIPCURVE_PENDOWN)))
      {
        sdb->buffers[i].val[sdb->cur_idx] =
          c->get_value (c->func_data);
        sdb->buffers[i].stat[sdb->cur_idx] = DATASTAT_PLOTABLE;
      }
      else sdb->buffers[i].stat[sdb->cur_idx] = ~DATASTAT_PLOTABLE;
    }
  }
}


/*
 * StripDataBuffer_init_range
 */
size_t		StripDataBuffer_init_range	(StripDataBuffer the_sdb,
						 struct timeval  *t0,
						 struct timeval  *t1)
{
  StripDataBufferInfo	*sdb = (StripDataBufferInfo *)the_sdb;
  long			r1, r2;
  int			i;

  r1 = ((sdb->count > 0)?
	StripDataBuffer_find_date_idx (sdb, t0, SDB_GTE) : -1);

  if (r1 >= 0)	/* look for last date only if the first one was ok */
    r2 = StripDataBuffer_find_date_idx (sdb, t1, SDB_LTE);

  if ((r1 < 0) || (r2 < 0))
    sdb->idx_t0 = sdb->idx_t1;
  else {
    sdb->idx_t0 = (size_t)r1;
    sdb->idx_t1 = (size_t)r2;
  }

  if (sdb->idx_t0 == sdb->idx_t1)
    r1 = 0;
  else if (sdb->idx_t0 < sdb->idx_t1)
    r1 = sdb->idx_t1 - sdb->idx_t0 + 1;
  else r1 = sdb->buf_size - sdb->idx_t0 + sdb->idx_t1 + 1;

  for (i = 0; i < STRIP_MAX_CURVES; i++)
  {
    sdb->buffers[i].idx_t0 = sdb->idx_t0;
    sdb->buffers[i].idx_t1 = sdb->idx_t1;
  }

  return (size_t)r1;
}


/*
 * StripDataBuffer_get_times
 */
size_t	StripDataBuffer_get_times	(StripDataBuffer	the_sdb,
					 struct timeval		**times)
{
  StripDataBufferInfo	*sdb = (StripDataBufferInfo *)the_sdb;
  size_t		ret_val = 0;

  if (sdb->idx_t0 != sdb->idx_t1)
    {
      *times = &sdb->times[sdb->idx_t0];

      if (sdb->idx_t0 < sdb->idx_t1)
	{
	  ret_val = sdb->idx_t1 - sdb->idx_t0 + 1;
	  sdb->idx_t0 = sdb->idx_t1;
	}
      else
	{
	  ret_val = sdb->buf_size - sdb->idx_t0;
	  sdb->idx_t0 = 0;
	}
    }
  else *times = NULL;
  
  return ret_val;
}

  
/*
 * StripDataBuffer_get_data
 */
size_t	StripDataBuffer_get_data	(StripDataBuffer	the_sdb,
					 StripCurve		the_curve,
                                         double			**val,
                                         char			**stat)
{
  StripDataBufferInfo	*sdb = (StripDataBufferInfo *)the_sdb;
  CurveData		*cd = CURVE_DATA(the_curve);

  size_t		ret_val = 0;
  
  if (cd->idx_t0 != cd->idx_t1)
  {
    *val = &cd->val[cd->idx_t0];
    *stat = &cd->stat[cd->idx_t0];
    
    if (cd->idx_t0 < cd->idx_t1)
    {
      ret_val = cd->idx_t1 - cd->idx_t0 + 1;
      cd->idx_t0 = cd->idx_t1;
    }
    else
    {
      ret_val = sdb->buf_size - cd->idx_t0;
      cd->idx_t0 = 0;
    }
  }
  else *val = NULL;
  
  return ret_val;
}


/*
 * StripDataBuffer_dump
 */
int	StripDataBuffer_dump	(StripDataBuffer	the_sdb,
				 StripCurve		curves[],
				 struct timeval 	*begin,
				 struct timeval 	*end,	
				 FILE 			*outfile)
{
  StripDataBufferInfo	*sdb = (StripDataBufferInfo *)the_sdb;
  struct timeval	*times;
  int			n_curves;
  int			n, i, j;
  int			msec;
  time_t		tt;
  char			buf[SDB_DUMP_FIELDWIDTH+1];
  int			ret_val = 0;
  struct		_cv
  {
    CurveData	*cd;
    double	*val;
    char	*stat;
  }
  cv[STRIP_MAX_CURVES];

  /* first build the array of curves to dump */
  if (curves[0])
  {
    /* get curves from array */
    for (n = 0; curves[n]; n++)
      if ((cv[n].cd = CURVE_DATA(curves[n])) == NULL)
      {
        fprintf (stderr, "StripDataBuffer_dump(): bad curve descriptor\n");
        return 0;
      }
  }
  else for (i = 0, n = 0; i < STRIP_MAX_CURVES; i++)	/* all curves */
  {
    if (sdb->buffers[i].curve != NULL)
    {
      cv[n].cd = &sdb->buffers[i];
      n++;
    }
  }
  n_curves = n;
  
  /* now set the begin, end times */
  if (!begin) begin = &sdb->times[sdb->idx_t0];
  if (!end) end = &sdb->times[sdb->idx_t1];

  /* now get the data */
  if (StripDataBuffer_init_range (the_sdb, begin, end) > 0)
  {
    /* first print out the curve names along the top */
    fprintf (outfile, "%-*s", SDB_DUMP_FIELDWIDTH, "Sample Time");
    for (i = 0; i < n_curves; i++)
      fprintf
        (outfile, "%*s", SDB_DUMP_FIELDWIDTH,
         cv[i].cd->curve->details->name);
    fprintf (outfile, "\n");
    
    while ((n = StripDataBuffer_get_times (the_sdb, &times)) > 0)
    {
      /* get the value array for each curve */
      for (i = 0; i < n_curves; i++)
        if (StripDataBuffer_get_data
            (the_sdb, (StripCurve)cv[i].cd->curve, &cv[i].val, &cv[i].stat)
            != n)
        {
          fprintf
            (stderr, "StripDataBuffer_dump(): unexpected data count!\n");
          return 0;
        }
      
      /* write out the samples */
      for (i = 0; i < n; i++)
      {
        /* sample time */
        tt = (time_t)times[i].tv_sec;
        msec = (int)(times[i].tv_usec / ONE_THOUSAND);
        strftime
          (buf, SDB_DUMP_FIELDWIDTH, "%m/%d/%Y %H:%M:%S",
           localtime (&tt));
        j = strlen (buf);
        buf[j++] = '.';
        int2str (msec, &buf[j], 3);
        fprintf (outfile, "%-*s", SDB_DUMP_FIELDWIDTH, buf);
        
        /* sampled values */
        for (j = 0; j < n_curves; j++)
        {
          if (cv[j].stat[i] & DATASTAT_PLOTABLE)
            dbl2str
              (cv[j].val[i], cv[j].cd->curve->details->precision,
               buf, SDB_DUMP_NUMWIDTH);
          else strcpy (buf, SDB_DUMP_BADVALUESTR);
          fprintf (outfile, "%*s", SDB_DUMP_FIELDWIDTH, buf);
        }
        
        /* finally, the end-line */
        fprintf (outfile, "\n");
      }
    }
    
    fflush (outfile);
  }
  
  return ret_val;
}



/* ====== Static Functions ====== */
static long	StripDataBuffer_find_date_idx 	(StripDataBufferInfo	*sdb,
						 struct timeval		*t,
						 int 			mode)
{
  long	a, b, i;
  long	x;


  x = ((long)sdb->cur_idx - (long)sdb->count + 1);
  if (x < 0) x += sdb->buf_size;

  a = x;
  b = (long)sdb->cur_idx;

  /* first check boundary conditions */
  if (mode == SDB_LTE)
    {
      x = compare_times (&sdb->times[a], t);
      if (x > 0)
	return -1;
      else if (x == 0)	/* hey, we found it! */
	return a;
    }
  else if (mode == SDB_GTE)
    {
      x = compare_times (&sdb->times[b], t);
      if (x < 0)
	return -1;
      else if (x == 0)	/* hey, we found it! */
	return b;
    }

  /* break the buffer into two parts if the begin index is greater than
   * the end index */
  if (a > b)
    {
      /* the buffer wraps around --determine whether t lies btw a and n,
       * or 0 and b */
      x = compare_times (t, &sdb->times[sdb->buf_size-1]);
      if (x < 0)
	b = (long)sdb->buf_size-1;
      else if (x > 0)
	a = 0;
      else 	/* hey, we found it! */
	return (long)sdb->buf_size-1;
    }
/*
  fprintf (stdout, "*** SDB: beginning search ***\n");
*/
  
  /* now do a binary search */
  do {
    i = a + ((b-a)/2);
/*
    fprintf (stdout, "SDB: a = %0.3d, b = %0.3d, i = %0.3d\n", a, b, i);
*/    
    x = compare_times (&sdb->times[i], t);

    if (x > 0)
      b = i-1;
    else if (x < 0)
      a = i+1;
    else	/* found it */
      return i;
  } while (a <= b);
/*  
  fprintf (stdout, "*** SDB: finished search ***\n");
*/  
  if (x < 0)
    {
      if (mode == SDB_LTE)
	return i;
      if (mode == SDB_GTE)
	return i+1;
    }
  else if (x > 0)
    {
      if (mode == SDB_LTE)
	return i-1;
      if (mode == SDB_GTE)
	return i;
    }

  return -1;	/* never executed */
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * change data buffer size routine
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
static int StripDataBuffer_resize (StripDataBufferInfo *sdb, size_t buf_size)
{
  int			i;
  int			new_cur_idx = sdb->cur_idx;
  int			new_count = sdb->count;
  int 			ret_val = 0;
  
  int			new_index;

  if (sdb->buf_size == 0)
  {
    
    sdb->times = (struct timeval *)malloc
      (buf_size * sizeof(struct timeval));
    new_index = new_count = 0;
    ret_val = (sdb->times != NULL);
  }
  else
  {
    ret_val = pack_array
      ((void **)&sdb->times, sizeof(struct timeval),
       sdb->buf_size, sdb->cur_idx, sdb->count,
       buf_size, &new_index, &new_count);
    if (ret_val)
      for (i = 0; i < STRIP_MAX_CURVES; i++)
        if (sdb->buffers[i].val)
        {
          ret_val = pack_array
            ((void **)&sdb->buffers[i].val, sizeof(double),
             sdb->buf_size, sdb->cur_idx, sdb->count,
             buf_size, 0, 0);
          if (ret_val)
            ret_val = pack_array
              ((void **)&sdb->buffers[i].stat, sizeof (char),
               sdb->buf_size, sdb->cur_idx, sdb->count,
               buf_size, 0, 0);
          if (!ret_val) break;
        }
  }
  if (ret_val)
  {
    sdb->buf_size = buf_size;
    sdb->cur_idx = new_index;
    sdb->count = new_count;
  }
  return ret_val;
}



static int	pack_array	(void 	**p,	/* address of pointer */
				 size_t	nbytes,	/* array element size */
				 int	n0,	/* old size */
				 int	i0,	/* old index */
				 int	s0,	/* old count */
				 int	n1,	/* new size */
				 int	*i1,	/* new index */
				 int	*s1)	/* new count */
{
  int	x, y;
  char	*q;

  if ((q = (char *)*p) != NULL)
    {
      if (n1 > n0)	/* new size is greater than old */
	{
	  if (q = (char *)realloc (q, n1*nbytes))
	    {
	      /* how many to push to the end? */
	      x = s0-i0-1;
	      if (x > 0)
		memmove (q+((n1-x)*nbytes), q+((n0-x)*nbytes), x*nbytes);
	      if (i1) *i1 = i0;
	      if (s1) *s1 = s0;
	    }
	}
      else		/* new size is less than old */
	{
	  x = (i0+1) - n1; /* num at front of old array which won't now fit */

	  if (x > 0)	/* not all elements on [0, i0] will fit on [0, n1] */
	    {
	      memmove (q, q+(x*nbytes), n1*nbytes);
	      if (i1) *i1 = i0 - x;
	      if (s1) *s1 = n1;
	    }
	  else	/* x <= 0 */
	    {
	      y = i0 + 1;		/* number at front of array */
	      x = min (-x, s0-y);	/* x <-- num elem. to pack at end */
	      
	      if (x > 0)
		memmove (q+((n1-x)*nbytes), q+((n0-x)*nbytes), x*nbytes);
	      
	      if (i1) *i1 = i0;
	      if (s1) *s1 = y + x;
	    }
	  
	  q = (char *)realloc (q, n1*nbytes);
	}
    }

  return ((*p = q) != NULL);
}
