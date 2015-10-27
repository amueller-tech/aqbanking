/***************************************************************************
 begin       : Wed Jan 15 2014
 copyright   : (C) 2014 by Martin Preuss
 email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef AH_JOBSEPASTANDINGORDERDELETE_L_H  /* --- rw --- 27.9.15 */
#define AH_JOBSEPASTANDINGORDERDELETE_L_H


#include "accountjob_l.h"

                  /* --- rw --- Delete_new   */
AH_JOB *AH_Job_SepaStandingOrderDelete_new(AB_USER *u, AB_ACCOUNT *account);


#endif /* AH_JOBSEPASTANDINGORDERDELETE_L_H  */
