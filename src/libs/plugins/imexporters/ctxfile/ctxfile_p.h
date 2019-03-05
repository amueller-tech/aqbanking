/***************************************************************************
    begin       : Tue Mar 31 2009
    copyright   : (C) 2018 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef AQHBCI_IMEX_CTXFILE_P_H
#define AQHBCI_IMEX_CTXFILE_P_H

#include "ctxfile.h"

#include <gwenhywfar/dbio.h>
#include <aqbanking/backendsupport/imexporter_be.h>


typedef struct AH_IMEXPORTER_CTXFILE AH_IMEXPORTER_CTXFILE;
struct AH_IMEXPORTER_CTXFILE {
  int dummy;
};


static void GWENHYWFAR_CB AH_ImExporterCtxFile_FreeData(void *bp, void *p);

static int AH_ImExporterCtxFile_Import(AB_IMEXPORTER *ie,
                                       AB_IMEXPORTER_CONTEXT *ctx,
                                       GWEN_SYNCIO *sio,
                                       GWEN_DB_NODE *params);

static int AH_ImExporterCtxFile_Export(AB_IMEXPORTER *ie,
                                       AB_IMEXPORTER_CONTEXT *ctx,
                                       GWEN_SYNCIO *sio,
                                       GWEN_DB_NODE *params);

static int AH_ImExporterCtxFile_CheckFile(AB_IMEXPORTER *ie, const char *fname);


#endif /* AQHBCI_IMEX_CTXFILE_P_H */
