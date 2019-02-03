/***************************************************************************
    begin       : Mon May 03 2010
    copyright   : (C) 2018 by Martin Preuss
    email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef AQHBCI_IMEX_Q43_P_H
#define AQHBCI_IMEX_Q43_P_H


#include "q43.h"

#include <aqbanking/imexporter_be.h>


typedef struct AH_IMEXPORTER_Q43 AH_IMEXPORTER_Q43;
struct AH_IMEXPORTER_Q43 {
  int dummy;
};


static void GWENHYWFAR_CB AH_ImExporterQ43_FreeData(void *bp, void *p);

static const char *AH_ImExporterQ43_GetCurrencyCode(int code);
static int AH_ImExporterQ43_ReadInt(const char *p, int len);

static int AH_ImExporterQ43_ReadDocument(AB_IMEXPORTER *ie,
                                         AB_IMEXPORTER_CONTEXT *ctx,
                                         GWEN_FAST_BUFFER *fb,
                                         GWEN_DB_NODE *params);

static int AH_ImExporterQ43_Import(AB_IMEXPORTER *ie,
                                   AB_IMEXPORTER_CONTEXT *ctx,
                                   GWEN_SYNCIO *sio,
                                   GWEN_DB_NODE *params);

static int AH_ImExporterQ43_Export(AB_IMEXPORTER *ie,
                                   AB_IMEXPORTER_CONTEXT *ctx,
                                   GWEN_SYNCIO *sio,
                                   GWEN_DB_NODE *params);

static int AH_ImExporterQ43_CheckFile(AB_IMEXPORTER *ie, const char *fname);


#endif /* AQHBCI_IMEX_CTXFILE_P_H */
