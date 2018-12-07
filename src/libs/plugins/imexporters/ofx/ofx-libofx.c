/***************************************************************************
 $RCSfile$
 -------------------
 cvs         : $Id: ofx.c 1411 2008-01-06 17:54:41Z martin $
 begin       : Mon Mar 01 2004
 copyright   : (C) 2004 by Martin Preuss
 email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/



#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "ofx-libofx_p.h"
#include <aqbanking/banking.h>
#include <aqbanking/banking_be.h>
#include <aqbanking/imexporter_be.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/gui.h>
#include <gwenhywfar/inherit.h>
#include <gwenhywfar/text.h>
#include <gwenhywfar/directory.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#ifdef OS_WIN32
# define DIRSEP "\\"
#else
# define DIRSEP "/"
#endif



GWEN_INHERIT(AB_IMEXPORTER, AH_IMEXPORTER_OFX);


AB_IMEXPORTER *ofx_factory(AB_BANKING *ab, GWEN_DB_NODE *db){
  AB_IMEXPORTER *ie;
  AH_IMEXPORTER_OFX *ieh;

  ie=AB_ImExporter_new(ab, "ofx");
  GWEN_NEW_OBJECT(AH_IMEXPORTER_OFX, ieh);
  GWEN_INHERIT_SETDATA(AB_IMEXPORTER, AH_IMEXPORTER_OFX, ie, ieh,
		       AH_ImExporterOFX_FreeData);
  ieh->dbData=db;

  AB_ImExporter_SetImportFn(ie, AH_ImExporterOFX_Import);
  AB_ImExporter_SetCheckFileFn(ie, AH_ImExporterOFX_CheckFile);
  return ie;
}



void GWENHYWFAR_CB AH_ImExporterOFX_FreeData(void *bp, void *p){
  AH_IMEXPORTER_OFX *ieh;

  ieh=(AH_IMEXPORTER_OFX*)p;
  GWEN_FREE_OBJECT(ieh);
}



int AH_ImExporterOFX_Import(AB_IMEXPORTER *ie,
			    AB_IMEXPORTER_CONTEXT *ctx,
                            GWEN_IO_LAYER *io,
			    GWEN_DB_NODE *params,
			    uint32_t guiid){
  AH_IMEXPORTER_OFX *ieh;
  LibofxContextPtr ofxctx;
  GWEN_BUFFER *dbuf;
  int rv;
  GWEN_STRINGLIST *paths;

  assert(ie);
  ieh=GWEN_INHERIT_GETDATA(AB_IMEXPORTER, AH_IMEXPORTER_OFX, ie);
  assert(ieh);

  ieh->context=ctx;
  ofxctx=libofx_get_new_context();

  ofx_set_account_cb(ofxctx,
		     AH_ImExporterOFX_AccountCallback_cb,
		     ieh);
  ofx_set_transaction_cb(ofxctx,
			 AH_ImExporterOFX_TransactionCallback_cb,
			 ieh);

  paths=AB_Banking_GetGlobalDataDirs();
  if (paths) {
    GWEN_BUFFER *fbuf;
    int rv;

    fbuf=GWEN_Buffer_new(0, 256, 0, 1);
    rv=GWEN_Directory_FindPathForFile(paths,
				      "libofx"
				      DIRSEP
				      "dtd"
				      DIRSEP
				      "ofx150.dtd",
				      fbuf);
    GWEN_StringList_free(paths);
    if (rv==0) {
      GWEN_Buffer_AppendString(fbuf,
			       DIRSEP
			       "libofx"
			       DIRSEP
			       "dtd"
			       DIRSEP);
#ifdef LIBOFX_GT_0_8_4
      DBG_INFO(AQBANKING_LOGDOMAIN,
	       "Setting path for DTD files to [%s]",
	       GWEN_Buffer_GetStart(fbuf));
      libofx_set_dtd_dir(ofxctx, GWEN_Buffer_GetStart(fbuf));
#else
      DBG_INFO(AQBANKING_LOGDOMAIN,
	       "Cannot set path for DTD files to [%s]: libofx is 0.8.3 or older",
	       GWEN_Buffer_GetStart(fbuf));
#endif
    }
    GWEN_Buffer_free(fbuf);
  }

  /* read whole stream into buffer */
  dbuf=GWEN_Buffer_new(0, 2048, 0, 1);
  GWEN_Buffer_SetStep(dbuf, 4096);
  rv=GWEN_Io_Layer_ReadToBufferUntilEof(io, dbuf, guiid, 2000);
  if (rv<0) {
    DBG_INFO(AQBANKING_LOGDOMAIN, "here (%d)", rv);
    GWEN_Buffer_free(dbuf);
    libofx_free_context(ofxctx);
    return rv;
  }

  /* setup debugging parameters */
  extern int ofx_PARSER_msg;
  extern int ofx_DEBUG_msg;
  extern int ofx_WARNING_msg;
  extern int ofx_ERROR_msg;
  extern int ofx_INFO_msg;
  extern int ofx_STATUS_msg;

  ofx_PARSER_msg=GWEN_DB_GetIntValue(params, "show_parser_msg", 0, 0);
  ofx_DEBUG_msg=GWEN_DB_GetIntValue(params, "show_debug_msg", 0, 0);
  ofx_WARNING_msg=GWEN_DB_GetIntValue(params, "show_warning_msg", 0, 1);
  ofx_ERROR_msg=GWEN_DB_GetIntValue(params, "show_error_msg", 0, 1);
  ofx_INFO_msg=GWEN_DB_GetIntValue(params, "show_info_msg", 0, 1);
  ofx_STATUS_msg=GWEN_DB_GetIntValue(params, "show_status_msg", 0, 1);

  /* now the buffer contains all the data */
  rv=libofx_proc_buffer(ofxctx,
                        GWEN_Buffer_GetStart(dbuf),
                        GWEN_Buffer_GetUsedBytes(dbuf));
  DBG_ERROR(0, "I'm back");
  GWEN_Buffer_free(dbuf);
  libofx_free_context(ofxctx);

  ieh->context=0;
  if (rv)
    return GWEN_ERROR_BAD_DATA;

  DBG_ERROR(0, "Returning");
  return 0;
}



int AH_ImExporterOFX_CheckFile(AB_IMEXPORTER *ie, const char *fname, uint32_t guiid){
  int fd;
  GWEN_BUFFEREDIO *bio;

  assert(ie);
  assert(fname);

  fd=open(fname, O_RDONLY);
  if (fd==-1) {
    /* error */
    DBG_ERROR(AQBANKING_LOGDOMAIN,
              "open(%s): %s", fname, strerror(errno));
    return GWEN_ERROR_NOT_FOUND;
  }

  bio=GWEN_BufferedIO_File_new(fd);
  GWEN_BufferedIO_SetReadBuffer(bio, 0, 256);

  while(!GWEN_BufferedIO_CheckEOF(bio)) {
    char lbuffer[256];
    int err;

    err=GWEN_BufferedIO_ReadLine(bio, lbuffer, sizeof(lbuffer));
    if (err) {
      DBG_INFO(AQBANKING_LOGDOMAIN,
	       "File \"%s\" is not supported by this plugin",
               fname);
      GWEN_BufferedIO_Close(bio);
      GWEN_BufferedIO_free(bio);
      return GWEN_ERROR_BAD_DATA;
    }
    if (-1!=GWEN_Text_ComparePattern(lbuffer, "*<OFX>*", 0) ||
        -1!=GWEN_Text_ComparePattern(lbuffer, "*<OFC>*", 0)) {
      /* match */
      DBG_INFO(AQBANKING_LOGDOMAIN,
               "File \"%s\" is supported by this plugin",
               fname);
      GWEN_BufferedIO_Close(bio);
      GWEN_BufferedIO_free(bio);
      return 0;
    }
  } /* while */

  GWEN_BufferedIO_Close(bio);
  GWEN_BufferedIO_free(bio);
  return GWEN_ERROR_BAD_DATA;
}





int AH_ImExporterOFX_StatusCallback_cb(const struct OfxStatusData data,
                                       void *user_data){
  AH_IMEXPORTER_OFX *ieh;

  DBG_INFO(AQBANKING_LOGDOMAIN,
           "Status callback");
  ieh=(AH_IMEXPORTER_OFX*)user_data;

  return 0;
}



int AH_ImExporterOFX_AccountCallback_cb(const struct OfxAccountData data,
                                        void *user_data){
  AH_IMEXPORTER_OFX *ieh;
  AB_IMEXPORTER_ACCOUNTINFO *ai;

  DBG_INFO(AQBANKING_LOGDOMAIN,
	   "Account callback");
  ieh=(AH_IMEXPORTER_OFX*)user_data;

  ai=AB_ImExporterAccountInfo_new();
#ifdef HAVE_OFX_WITH_CONNECT
  if (data.account_number_valid) {
    AB_ImExporterAccountInfo_SetAccountNumber(ai, data.account_number);
    if (*(data.account_name))
      AB_ImExporterAccountInfo_SetAccountName(ai, data.account_name);
  }
  else if (data.account_id_valid)
    AB_ImExporterAccountInfo_SetAccountNumber(ai, data.account_id);
  else
    AB_ImExporterAccountInfo_SetAccountNumber(ai, "----");

  if (data.bank_id_valid)
    AB_ImExporterAccountInfo_SetBankCode(ai, data.bank_id);
#else
  if (data.account_id_valid) {
    AB_ImExporterAccountInfo_SetAccountNumber(ai, data.account_id);
    if (*(data.account_name))
      AB_ImExporterAccountInfo_SetAccountName(ai, data.account_name);
  }
  else {
    AB_ImExporterAccountInfo_SetAccountNumber(ai, "----");
  }
#endif

  if (data.account_type_valid) {
    AB_ACCOUNT_TYPE at;

    switch(data.account_type) {
    case OFX_CHECKING:   at=AB_AccountType_Checking; break;
    case OFX_SAVINGS:    at=AB_AccountType_Savings; break;
    case OFX_MONEYMRKT:  at=AB_AccountType_Investment; break;
    case OFX_CREDITLINE: at=AB_AccountType_Bank; break;
    case OFX_CMA:        at=AB_AccountType_Cash; break;
    case OFX_CREDITCARD: at=AB_AccountType_CreditCard; break;
    case OFX_INVESTMENT: at=AB_AccountType_Investment; break;
    default:
      at=AB_AccountType_Bank;
      break;
    }
    AB_ImExporterAccountInfo_SetType(ai, at);
  }
  else {
    AB_ImExporterAccountInfo_SetType(ai, AB_AccountType_Bank);
  }

  AB_ImExporterContext_AddAccountInfo(ieh->context, ai);
  ieh->lastAccountInfo=ai;
  return 0;
}



int AH_ImExporterOFX_SecurityCallback_cb(const struct OfxSecurityData data,
                                         void *user_data){
  AH_IMEXPORTER_OFX *ieh;

  DBG_INFO(AQBANKING_LOGDOMAIN,
	   "Security callback");
  ieh=(AH_IMEXPORTER_OFX*)user_data;

  return 0;
}



int
AH_ImExporterOFX_TransactionCallback_cb(const struct OfxTransactionData data,
                                        void *user_data){
  AH_IMEXPORTER_OFX *ieh;

  DBG_INFO(AQBANKING_LOGDOMAIN,
           "Transaction callback");
  ieh=(AH_IMEXPORTER_OFX*)user_data;

  if (!ieh->lastAccountInfo) {
    DBG_ERROR(AQBANKING_LOGDOMAIN, "Transaction but no account. Ignoring");
    return -1;
  }
  else {
    AB_TRANSACTION *t;

    t=AB_Transaction_new();
    if (data.account_id_valid)
      AB_Transaction_SetLocalAccountNumber(t, data.account_id);
    else
      AB_Transaction_SetLocalAccountNumber(t, "----");

    if (data.date_posted_valid) {
      GWEN_TIME *ti;

      ti=GWEN_Time_fromSeconds(data.date_posted);
      AB_Transaction_SetValutaDate(t, ti);
      GWEN_Time_free(ti);
    }

    if (data.date_initiated_valid) {
      GWEN_TIME *ti;

      ti=GWEN_Time_fromSeconds(data.date_initiated);
      AB_Transaction_SetDate(t, ti);
      GWEN_Time_free(ti);
    }

    if (data.fi_id_valid)
      AB_Transaction_SetFiId(t, data.fi_id);

    if (data.name_valid)
      AB_Transaction_AddRemoteName(t, data.name, 0);
    if (data.memo_valid)
      AB_Transaction_AddPurpose(t, data.memo, 0);

    if (data.amount_valid) {
      AB_VALUE *val;
      const char *cur;

      cur=0;
      if (data.account_ptr)
        if (data.account_ptr->currency_valid)
	  cur=data.account_ptr->currency;
      val=AB_Value_fromDouble(data.amount);
      assert(val);
      AB_Value_SetCurrency(val, cur);
      if (data.invtransactiontype_valid)
	/* negate for investment transaction type (hack, see KMyMoney) */
	AB_Value_Negate(val);
      AB_Transaction_SetValue(t, val);
      AB_Value_free(val);
    }
    else {
      DBG_ERROR(AQBANKING_LOGDOMAIN,
                "No amount in transaction");
    }

    if (data.transactiontype_valid){
      switch(data.transactiontype){
      case OFX_CHECK:
        AB_Transaction_SetTransactionKey(t, "CHK");
        AB_Transaction_SetTransactionText(t, "Check");
        break;
      case OFX_INT:
        AB_Transaction_SetTransactionKey(t, "INT");
        AB_Transaction_SetTransactionText(t, "Interest");
        break;
      case OFX_DIV:
        AB_Transaction_SetTransactionKey(t, "DIV");
        AB_Transaction_SetTransactionText(t, "Dividend");
        break;
      case OFX_SRVCHG:
        AB_Transaction_SetTransactionKey(t, "CHG");
        AB_Transaction_SetTransactionText(t, "Service charge");
        break;
      case OFX_FEE:
        AB_Transaction_SetTransactionKey(t, "BRF");
        AB_Transaction_SetTransactionText(t, "Fee");
        break;
      case OFX_DEP:
        AB_Transaction_SetTransactionKey(t, "LDP"); /* FIXME: not sure */
        AB_Transaction_SetTransactionText(t, "Deposit");
        break;
      case OFX_ATM:
        AB_Transaction_SetTransactionKey(t, "MSC"); /* misc */
        AB_Transaction_SetTransactionText(t, "Cash dispenser");
        break;
      case OFX_POS:
        AB_Transaction_SetTransactionKey(t, "MSC"); /* misc */
        AB_Transaction_SetTransactionText(t, "Point of sale");
        break;
      case OFX_XFER:
        AB_Transaction_SetTransactionKey(t, "TRF");
        AB_Transaction_SetTransactionText(t, "Transfer");
        break;
      case OFX_PAYMENT:
        AB_Transaction_SetTransactionKey(t, "TRF"); /* FIXME: not sure */
        AB_Transaction_SetTransactionText(t, "Electronic payment");
        break;
      case OFX_CASH:
        AB_Transaction_SetTransactionKey(t, "MSC"); /* FIXME: not sure */
        AB_Transaction_SetTransactionText(t, "Cash");
        break;
      case OFX_DIRECTDEP:
        AB_Transaction_SetTransactionKey(t, "LDP"); /* FIXME: not sure */
        AB_Transaction_SetTransactionText(t, "Direct deposit");
        break;
      case OFX_DIRECTDEBIT:
        AB_Transaction_SetTransactionKey(t, "MSC"); /* FIXME: not sure */
        AB_Transaction_SetTransactionText(t, "Merchant initiated debit");
        break;
      case OFX_REPEATPMT:
        AB_Transaction_SetTransactionKey(t, "STO");
        AB_Transaction_SetTransactionText(t, "Standing order");
        break;
      case OFX_DEBIT:
      case OFX_CREDIT:
      case OFX_OTHER:
        AB_Transaction_SetTransactionKey(t, "MSC"); /* FIXME: not sure */
        break;
      }
    } /* if transaction type is valid */
    else if (data.invtransactiontype_valid) {
      switch (data.invtransactiontype){
      case OFX_BUYDEBT:
      case OFX_BUYMF:
      case OFX_BUYOPT:
      case OFX_BUYOTHER:
      case OFX_BUYSTOCK:
        AB_Transaction_SetTransactionKey(t, "BUY");
        AB_Transaction_SetTransactionText(t, "Buy stocks or alike");
        AB_Transaction_SetSubType(t, AB_Transaction_SubTypeBuy);
        break;
      case OFX_REINVEST:
        AB_Transaction_SetTransactionKey(t, "REINV");
        AB_Transaction_SetTransactionText(t, "Reinvestment");
        AB_Transaction_SetSubType(t, AB_Transaction_SubTypeReinvest);
        break;
      case OFX_SELLDEBT:
      case OFX_SELLMF:
      case OFX_SELLOPT:
      case OFX_SELLOTHER:
      case OFX_SELLSTOCK:
        AB_Transaction_SetTransactionKey(t, "BUY");
        AB_Transaction_SetTransactionText(t, "Buy stocks or alike");
        AB_Transaction_SetSubType(t, AB_Transaction_SubTypeSell);
        break;
      case OFX_INCOME:
        AB_Transaction_SetTransactionKey(t, "DIV");
        AB_Transaction_SetTransactionText(t, "Dividend");
        AB_Transaction_SetSubType(t, AB_Transaction_SubTypeDividend);
        break;
        /* rest is unhandled */
      case OFX_CLOSUREOPT:
        AB_Transaction_SetTransactionText(t, "XCLOSUREOPT");
        break;
      case OFX_INVEXPENSE:
        AB_Transaction_SetTransactionText(t, "XINVEXPENSE");
        break;
      case OFX_JRNLFUND:
        AB_Transaction_SetTransactionText(t, "XJRNLFUND");
        break;
      case OFX_MARGININTEREST:
        AB_Transaction_SetTransactionText(t, "XMARGININTEREST");
        break;
      case OFX_RETOFCAP:
        AB_Transaction_SetTransactionText(t, "XRETOFCAP");
        break;
      case OFX_SPLIT:
        AB_Transaction_SetTransactionText(t, "XSPLIT");
        break;
      case OFX_TRANSFER:
        AB_Transaction_SetTransactionText(t, "XTRANSFER");
        break;
      default:
        break;
      }

#ifdef HAVE_OFX_0_8_PLUS
      if (data.fees_valid || data.commission_valid) {
        AB_VALUE *vFees;

	vFees=AB_Value_new();
        if (data.fees_valid) {
          AB_VALUE *v;

	  v=AB_Value_fromDouble(data.fees);
	  AB_Value_AddValue(vFees, v);
	  AB_Value_free(v);
        }
        if (data.commission_valid) {
          AB_VALUE *v;

	  v=AB_Value_fromDouble(data.commission);
          AB_Value_AddValue(vFees, v);
          AB_Value_free(v);
        }
        AB_Transaction_SetFees(t, vFees);
        AB_Value_free(vFees);
      }
#endif

      if (data.units_valid)
        AB_Transaction_SetUnits(t, data.units);
#ifdef HAVE_OFX_0_8_PLUS
      if (data.unitprice_valid && data.fees_valid) {
        AB_VALUE *v;

	v=AB_Value_fromDouble(data.fees); /* TODO: add currency */
        AB_Transaction_SetUnitPrice(t, v);
        AB_Value_free(v);
      }
#endif
    }
    else {
      DBG_NOTICE(AQBANKING_LOGDOMAIN, "No transaction type");
    }

    if (data.server_transaction_id_valid)
      AB_Transaction_SetBankReference(t, data.server_transaction_id);

    if (data.check_number_valid)
      AB_Transaction_SetCustomerReference(t, data.check_number);
    else if (data.reference_number_valid)
      AB_Transaction_SetCustomerReference(t, data.reference_number);

    DBG_INFO(0, "Adding transaction");
    AB_ImExporterAccountInfo_AddTransaction(ieh->lastAccountInfo, t);
  }

  return 0;

}



int AH_ImExporterOFX_StatementCallback_cb(const struct OfxStatementData data,
                                          void *user_data){
  AH_IMEXPORTER_OFX *ieh;

  DBG_INFO(AQBANKING_LOGDOMAIN,
	   "Statement callback");
  ieh=(AH_IMEXPORTER_OFX*)user_data;

  return 0;
}
























