/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id$
    begin       : Mon Mar 01 2004
    copyright   : (C) 2004 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#undef BUILDING_AQOFXCONNECT_DLL
#undef BUILDING_DLL

#include "provider.h"
#include "aqofxconnect_l.h"


#include <gwenhywfar/plugin.h>



static AB_PROVIDER *AB_Plugin_ProviderOFX_Factory(GWEN_PLUGIN *pl, AB_BANKING *ab){
  return AO_Provider_new(ab);
}


/* interface to gwens plugin loader */
AQBANKING_EXPORT GWEN_PLUGIN *provider_aqofxconnect_factory(GWEN_PLUGIN_MANAGER *pm,
							    const char *name,
							    const char *fileName) {
  GWEN_PLUGIN *pl;

  pl=AB_Plugin_Provider_new(pm, name, fileName);
  AB_Plugin_Provider_SetFactoryFn(pl, AB_Plugin_ProviderOFX_Factory);
  return pl;
}


