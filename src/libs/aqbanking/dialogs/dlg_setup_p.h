/***************************************************************************
 begin       : Wed Apr 14 2010
 copyright   : (C) 2018 by Martin Preuss
 email       : martin@aqbanking.de

 ***************************************************************************
 * This file is part of the project "AqBanking".                           *
 * Please see toplevel file COPYING of that project for license details.   *
 ***************************************************************************/

#ifndef AB_DLG_SETUP_P_H
#define AB_DLG_SETUP_P_H


#include "dlg_setup.h"

#include <aqbanking/user.h>
#include <aqbanking/account.h>



typedef struct AB_SETUP_DIALOG AB_SETUP_DIALOG;
struct AB_SETUP_DIALOG {
  AB_BANKING *banking;

  AB_PROVIDER_LIST2 *providersInUse;

  AB_USER_LIST *currentUserList;
  AB_ACCOUNT_LIST *currentAccountList;
};


static void GWENHYWFAR_CB AB_SetupDialog_FreeData(void *bp, void *p);

static int GWENHYWFAR_CB AB_SetupDialog_SignalHandler(GWEN_DIALOG *dlg,
                                                      GWEN_DIALOG_EVENTTYPE t,
                                                      const char *sender);





#endif

