/*-----------------------------------------------------------------------------
 * Copyright (c) 1996 Southeastern Universities Research Association,
 *               Continuous Electron Beam Accelerator Facility
 *
 * This software was developed under a United States Government license
 * described in the NOTICE file included as part of this distribution.
 *
 *-----------------------------------------------------------------------------
 */


#include "StripCurve.h"

StripCurve	StripCurve_init (StripConfig *scfg)
{
  StripCurveInfo	*sc;

  if ((sc = (StripCurveInfo *)malloc (sizeof (StripCurveInfo))) != NULL)
  {
    sc->scfg		= scfg;
    sc->id		= 0;
    sc->details 	= NULL;
    sc->func_data	= NULL;
    sc->get_value	= NULL;
    sc->get_history	= NULL;
    sc->history.time	= NULL;
    sc->history.data	= NULL;
    sc->history.n	= 0;
    sc->status	= 0;
  }

  return (StripCurve)sc;
}


void		StripCurve_delete	(StripCurve the_sc)
{
  free (the_sc);
}


int		StripCurve_setattr	(StripCurve the_sc, ...)
{
  va_list		ap;
  int			attrib;
  StripCurveInfo	*sc = (StripCurveInfo *)the_sc;
  int			ret_val = 1;

  va_start (ap, the_sc);
  for (attrib = va_arg (ap, StripCurveAttribute);
       (attrib != 0);
       attrib = va_arg (ap, StripCurveAttribute))
  {
    if ((ret_val = ((attrib > 0) &&
                    (attrib < STRIPCURVE_LAST_ATTRIBUTE))))
      switch (attrib)
      {
	  case STRIPCURVE_NAME:
	    strcpy (sc->details->name, va_arg (ap, char *));
            StripConfigMask_set
              (&sc->details->update_mask, SCFGMASK_CURVE_NAME);
	    StripConfigMask_set
              (&sc->scfg->UpdateInfo.update_mask, SCFGMASK_CURVE_NAME);
	    break;
            
	  case STRIPCURVE_EGU:
	    strcpy (sc->details->egu, va_arg (ap, char *));
            StripConfigMask_set
              (&sc->details->update_mask, SCFGMASK_CURVE_EGU);
	    StripConfigMask_set
              (&sc->scfg->UpdateInfo.update_mask, SCFGMASK_CURVE_EGU);
	    StripCurve_setstat (sc, STRIPCURVE_EGU_SET);
	    break;
            
	  case STRIPCURVE_COMMENT:
	    strcpy (sc->details->comment, va_arg (ap, char *));
            StripConfigMask_set
              (&sc->details->update_mask, SCFGMASK_CURVE_COMMENT);
	    StripConfigMask_set
              (&sc->scfg->UpdateInfo.update_mask, SCFGMASK_CURVE_COMMENT);
	    StripCurve_setstat (sc, STRIPCURVE_COMMENT);
	    break;
            
	  case STRIPCURVE_PRECISION:
	    sc->details->precision = va_arg (ap, int);
	    sc->details->precision =
	      max ( sc->details->precision, STRIPMIN_CURVE_PRECISION);
	    sc->details->precision =
	      min ( sc->details->precision, STRIPMAX_CURVE_PRECISION);
            StripConfigMask_set
              (&sc->details->update_mask, SCFGMASK_CURVE_PRECISION);
	    StripConfigMask_set
              (&sc->scfg->UpdateInfo.update_mask, SCFGMASK_CURVE_PRECISION);
	    StripCurve_setstat (sc, STRIPCURVE_PRECISION_SET);
	    break;
            
	  case STRIPCURVE_MIN:
	    sc->details->min = va_arg (ap, double);
            StripConfigMask_set
              (&sc->details->update_mask, SCFGMASK_CURVE_MIN);
	    StripConfigMask_set
              (&sc->scfg->UpdateInfo.update_mask, SCFGMASK_CURVE_MIN);
	    StripCurve_setstat (sc, STRIPCURVE_MIN_SET);
	    break;
            
	  case STRIPCURVE_MAX:
	    sc->details->max = va_arg (ap, double);
            StripConfigMask_set
              (&sc->details->update_mask, SCFGMASK_CURVE_MAX);
	    StripConfigMask_set
              (&sc->scfg->UpdateInfo.update_mask, SCFGMASK_CURVE_MAX);
	    StripCurve_setstat (sc, STRIPCURVE_MAX_SET);
	    break;
            
	  case STRIPCURVE_PENSTAT:
	    sc->details->penstat = va_arg (ap, int);
            StripConfigMask_set
              (&sc->details->update_mask, SCFGMASK_CURVE_PENSTAT);
	    StripConfigMask_set
              (&sc->scfg->UpdateInfo.update_mask, SCFGMASK_CURVE_PENSTAT);
	    break;
            
	  case STRIPCURVE_PLOTSTAT:
	    sc->details->plotstat = va_arg (ap, int);
            StripConfigMask_set
              (&sc->details->update_mask, SCFGMASK_CURVE_PLOTSTAT);
	    StripConfigMask_set
              (&sc->scfg->UpdateInfo.update_mask, SCFGMASK_CURVE_PLOTSTAT);
	    break;

          case STRIPCURVE_COLOR:
            sc->details->color = va_arg (ap, cColor *);
            break;
            
            
	  case STRIPCURVE_SAMPLEFUNC:
	    sc->get_value = va_arg (ap, StripCurveSampleFunc);
	    break;
            
	  case STRIPCURVE_FUNCDATA:
	    sc->func_data = va_arg (ap, void *);
	    break;
      }
    else break;
  }

  va_end (ap);
  return ret_val;
}


int		StripCurve_getattr	(StripCurve the_sc, ...)
{
  va_list		ap;
  int			attrib;
  StripCurveInfo	*sc = (StripCurveInfo *)the_sc;
  int			ret_val = 1;

  va_start (ap, the_sc);
  for (attrib = va_arg (ap, StripCurveAttribute);
       (attrib != 0);
       attrib = va_arg (ap, StripCurveAttribute))
  {
    if ((ret_val = ((attrib > 0) && (attrib < STRIPCURVE_LAST_ATTRIBUTE))))
      switch (attrib)
      {
	  case STRIPCURVE_NAME:
	    *(va_arg (ap, char **)) = sc->details->name;
	    break;
	  case STRIPCURVE_EGU:
	    *(va_arg (ap, char **)) = sc->details->egu;
	    break;
	  case STRIPCURVE_COMMENT:
	    *(va_arg (ap, char **)) = sc->details->comment;
	    break;
	  case STRIPCURVE_PRECISION:
	    *(va_arg (ap, int *)) = sc->details->precision;
	    break;
	  case STRIPCURVE_MIN:
	    *(va_arg (ap, double *)) = sc->details->min;
	    break;
	  case STRIPCURVE_MAX:
	    *(va_arg (ap, double *)) = sc->details->max;
	    break;
	  case STRIPCURVE_PENSTAT:
	    *(va_arg (ap, int *)) = sc->details->penstat;
	    break;
	  case STRIPCURVE_PLOTSTAT:
	    *(va_arg (ap, int *)) = sc->details->plotstat;
	    break;
	  case STRIPCURVE_COLOR:
	    *(va_arg (ap, cColor **)) = sc->details->color;
	    break;
	  case STRIPCURVE_FUNCDATA:
	    *(va_arg (ap, void **)) = sc->func_data;
	    break;
	  case STRIPCURVE_SAMPLEFUNC:
	    *(va_arg (ap, StripCurveSampleFunc *)) = sc->get_value;
	    break;
	  case STRIPCURVE_HISTORYFUNC:
	    *(va_arg (ap, StripCurveHistoryFunc *)) = sc->get_history;
	    break;
      }
    else break;
  }
  va_end (ap);
  return ret_val;
}


void	StripCurve_update	(StripCurve the_sc)
{
  StripCurveInfo	*sc = (StripCurveInfo *)the_sc;

  StripConfig_update (sc->scfg, SCFGMASK_CURVE);
}  


/*
 * StripCurve_set/getattr_val
 */
void	*StripCurve_getattr_val	(StripCurve the_sc, StripCurveAttribute attrib)
{
  StripCurveInfo	*sc = (StripCurveInfo *)the_sc;
  
  switch (attrib)
  {
      case STRIPCURVE_NAME:
        return (void *)sc->details->name;
      case STRIPCURVE_EGU:
        return (void *)sc->details->egu;
      case STRIPCURVE_COMMENT:
        return (void *)sc->details->comment;
      case STRIPCURVE_PRECISION:
        return (void *)&sc->details->precision;
      case STRIPCURVE_MIN:
        return (void *)&sc->details->min;
      case STRIPCURVE_MAX:
        return (void *)&sc->details->max;
      case STRIPCURVE_PENSTAT:
        return (void *)&sc->details->penstat;
      case STRIPCURVE_PLOTSTAT:
        return (void *)&sc->details->plotstat;
      case STRIPCURVE_COLOR:
        return (void *)sc->details->color;
      case STRIPCURVE_FUNCDATA:
        return (void *)sc->func_data;
      case STRIPCURVE_SAMPLEFUNC:
        return (void *)sc->get_value;
      case STRIPCURVE_HISTORYFUNC:
        return (void *)sc->get_history;
  }
}


/*
 * StripCurve_setstat
 */
void		StripCurve_setstat	(StripCurve	the_sc,
                                         unsigned	stat)
{
  StripCurveInfo	*sc = (StripCurveInfo *)the_sc;
  int			i;

  /* translate status bits into StripConfigMaskElement values and
   * update set_mask appropriately */
  for (i = 0; (i + SCIDX_EGU_SET) <= SCIDX_MAX_SET; i++)
    if (stat & (1 << (i + SCIDX_EGU_SET)))
      StripConfigMask_set (&sc->details->set_mask, SCFGMASK_CURVE_EGU + i);

  /* update the status */
  sc->status |= stat;
}

/*
 * StripCurve_getstat
 */
StripCurveStatus	StripCurve_getstat	(StripCurve	the_sc,
						 unsigned	stat)
{
  unsigned long		ret;
  StripCurveInfo	*sc = (StripCurveInfo *)the_sc;
  int			i;
  

  /* need to translate StripConfigMaskElement values from the
   * set_mask into StripCurveStatus bits to combine with the
   * existing status bits to produce the return value. */
  ret = sc->status;
  for (i = 0; (i + SCIDX_EGU_SET) <= SCIDX_MAX_SET; i++)
    if (StripConfigMask_stat (&sc->details->set_mask, SCFGMASK_CURVE_EGU + i))
      ret |= (1 << (SCIDX_EGU_SET + i));
  
  return (StripCurveStatus)(ret & stat);
}

/*
 * StripCurve_clearstat
 */
void		StripCurve_clearstat	(StripCurve	the_sc,
                                         unsigned	stat)
{
  StripCurveInfo	*sc = (StripCurveInfo *)the_sc;
  int			i;

  /* translate status bits into StripConfigMaskElement values and
   * update set_mask appropriately */
  for (i = 0; (i + SCIDX_EGU_SET) <= SCIDX_MAX_SET; i++)
    if (stat & (1 << (i + SCIDX_EGU_SET)))
      StripConfigMask_unset (&sc->details->set_mask, SCFGMASK_CURVE_EGU + i);

  /* update the status */
  sc->status &= ~stat;
}
