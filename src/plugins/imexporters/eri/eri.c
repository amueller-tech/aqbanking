/***************************************************************************
 $RCSfile$
                             -------------------
    cvs         : $Id$
    begin       : Thu 21-07-2005
    copyright   : (C) 2005 by Peter de Vrijer
    email       : pdevrijer@home.nl

 ***************************************************************************
 *    Please see the file COPYING in this directory for license details    *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "eri_p.h"
#include <gwenhywfar/debug.h>
#include <gwenhywfar/text.h>
#include <gwenhywfar/waitcallback.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

GWEN_INHERIT(AB_IMEXPORTER, AH_IMEXPORTER_ERI);

// varstrcut cuts strings of length no and startig at position start from
// the source string and copies them to the destination string
// start is decreaseb by one, because column 1 is at position 0 in string
void varstrcut(char *dest, char *src, int start, int n) {
  int i;

  start--;

  // go to start char
  /*  for (i = 0; i < start; ++i) {
    *src++;
    } */

  src += start;  // will this work on every machine and compiler?

  // copy wanted length of string and add \0
  for (i = 0; i < n; ++i) {
    *dest++ = *src++;
  }
  *dest = 0;
  return;
}


// stripPzero strips leading zeroes in accountNumber strings 
// and the P for Postgiro accounts
void stripPzero(char *dest, char *src) {

  while ((*src == 'P') || (*src == '0')) { 
    src++;
  }

  // if string was all zeroes, the result is an empty string
  if (!*src) {
    *dest = 0;
    return;
  }

  // copy the remaining string
  while (*src) {
    *dest++ = *src++;
  }

  *dest = 0;
  return;
}

void stripTrailSpaces(char *buffer) {
  char *p;

  p = buffer;

  // find trailing zero
  while (*p) p++;

  // check for empty strings
  if (p == buffer) return;

  // Go back one to last char of string
  p--;

  // Strip trailing spaces (beware of strings containing all spaces
  while ((p >= buffer) && (*p == '\x20')) p--;

  // Go forward one char and add trailing \0
  *++p = 0;
}
// My Own Simple Error Codes for GWEN
int mySimpleCode(GWEN_ERRORCODE err) {
  int serr;

  serr = (int)(err & 0xFF);
  return serr;
}

// For debugging a function to print GWEN_BUFFEREDIO Errors
void printGWEN_BufferedIO_Errors(GWEN_ERRORCODE err) {
  char es[128];
  int serr;


  GWEN_Error_ToString(err, es, 128);
  printf("Gwen Error Code is %s\n", es);

  serr = mySimpleCode(err);
  printf("Simplified Error Code is %u\n", serr);
  return;
}

int eriReadRecord(GWEN_BUFFEREDIO *bio,
			   char *buffer) {
  GWEN_ERRORCODE gwerr;
  int serr, count, *cnt = &count;
  char c;

  // check if there are no CR and or LF in the buffer
  while (((c = GWEN_BufferedIO_PeekChar(bio)) == '\n') || (c == '\r')) 
                     c = GWEN_BufferedIO_ReadChar(bio);

  *cnt = REC_LENGTH;
  gwerr = GWEN_BufferedIO_ReadRaw(bio, buffer, cnt);

  if (gwerr) {
    // printf("Bytes read is %d\n", *cnt);
    printGWEN_BufferedIO_Errors(gwerr);
  }

  // When buffer was not filled enough not all cnt char are read,
  // So in that case we do a read for the rest.
  if (*cnt != REC_LENGTH) {
    buffer += *cnt;                   // Set start pointer to right point
    *cnt = REC_LENGTH - *cnt;         // Calculate char to do
    gwerr = GWEN_BufferedIO_ReadRaw(bio, buffer, cnt);

    if (gwerr) {
      // printf("Bytes read is %d\n", *cnt);
      printGWEN_BufferedIO_Errors(gwerr);
    }
  }
  serr = mySimpleCode(gwerr);
  return serr;
}

int parseFirstRecord(char *recbuf, ERI_TRANSACTION *current) {
  char varbuf[MAXVARLEN], s[MAXVARLEN];

  // Sanity check, is this a ERI file??
  varstrcut(varbuf, recbuf, 11, 17);
  if (strcmp(varbuf, "EUR99999999992000") != 0) {
    printf("This is not an ERI file!!\n");
    return REC_BAD;
  }

  // first local account number
  varstrcut(varbuf, recbuf, 1, 10);
  stripPzero(s, varbuf);
  strcpy(current->localAccountNumber, s);

  // remote account number
  varstrcut(varbuf, recbuf, 39, 10);
  stripPzero(s, varbuf);
  strcpy(current->remoteAccountNumber, s);

  // Name payee
  varstrcut(varbuf, recbuf, 49, 24);
  stripTrailSpaces(varbuf);
  strcpy(current->namePayee, varbuf);

  // Amount of transaction
  varstrcut(varbuf, recbuf, 74, 13);
  current->amount = strtod(varbuf, (char**)NULL)/100;

  // Sign of transaction C is plus, D is minus
  varstrcut(varbuf, recbuf, 87, 1);
  if (*varbuf == 'D') {
    current->amount *= -1;
  }

  // Transaction date next, Is simple string, No changes needed
  varstrcut(current->date, recbuf, 88, 6);

  // Valuta date same
  varstrcut(current->valutaDate, recbuf, 94, 6);

  // Transaction Id, only valid when BETALINGSKENM. (see ERI description)
  varstrcut(varbuf, recbuf, 109, 16);
  stripTrailSpaces(varbuf);
  strcpy(current->transactionId, varbuf);

  return REC_OK;
}

int parseSecondRecord(char *recbuf, ERI_TRANSACTION *current) {
  char varbuf[MAXVARLEN];

  // Sanity check, is this record type 3?
  varstrcut(varbuf, recbuf, 11, 14);
  if (strcmp(varbuf, "EUR99999999993") != 0) {
    printf("Second record of transaction is not of type 3!\n");
    return REC_BAD;
  }

  // Check if theres is a transaction Id
  varstrcut(varbuf, recbuf, 25, 14);
  if (strcmp(varbuf, "BETALINGSKENM.") == 0) {
    current->transactionIdValid = TRUE;
  }

  // Purpose line 1 
  varstrcut(varbuf, recbuf, 57, 32);
  stripTrailSpaces(varbuf);
  strcpy(current->purpose1, varbuf);

  // Purpose line 2
  varstrcut(varbuf, recbuf, 89, 32);
  stripTrailSpaces(varbuf);
  strcpy(current->purpose2, varbuf);

  return REC_OK;
}

int parseThirdRecord(char *recbuf, ERI_TRANSACTION *current) {
  char varbuf[MAXVARLEN];

  // Sanity check, is this record type 4?
  varstrcut(varbuf, recbuf, 11, 14);
  if (strcmp(varbuf, "EUR99999999994") != 0) {
    printf("Third record of transaction is not of type 4!\n");
    return REC_BAD;
  }

  // Purpose line 3 
  varstrcut(varbuf, recbuf, 25, 32);
  stripTrailSpaces(varbuf);
  strcpy(current->purpose3, varbuf);

  // Purpose line 4 
  varstrcut(varbuf, recbuf, 57, 32);
  stripTrailSpaces(varbuf);
  strcpy(current->purpose4, varbuf);

   // Purpose line 5
  varstrcut(varbuf, recbuf, 89, 32);
  stripTrailSpaces(varbuf);
  strcpy(current->purpose5, varbuf);

  // Check if theres is a transaction Id
  varstrcut(varbuf, recbuf, 25, 14);
  if (strcmp(varbuf, "BETALINGSKENM.") == 0) {
    current->transactionIdValid = TRUE;
    *current->purpose3 = 0;
  }

  return REC_OK;
}

int parseFourthRecord(char *recbuf, ERI_TRANSACTION *current) {
  char varbuf[MAXVARLEN];

  // Sanity check, is this record type 4?
  varstrcut(varbuf, recbuf, 11, 14);
  if (strcmp(varbuf, "EUR99999999994") != 0) {
    printf("Fourth record of transaction is not of type 4!\n");
    return REC_BAD;
  }

  // Purpose line 6 
  varstrcut(varbuf, recbuf, 25, 96);
  stripTrailSpaces(varbuf);
  strcpy(current->purpose6, varbuf);

  // Check if theres is a transaction Id
  varstrcut(varbuf, recbuf, 25, 14);
  if (strcmp(varbuf, "BETALINGSKENM.") == 0) {
    current->transactionIdValid = TRUE;
    *current->purpose6 = 0;
  }

  return REC_OK;
}

void eriAddPurpose(AB_TRANSACTION *t, char *purpose) {

  if (strlen(purpose) > 0) {
    AB_Transaction_AddPurpose(t, purpose, 0);
  }
}

int eriAddTransaction(AB_IMEXPORTER_CONTEXT *ctx,
		      ERI_TRANSACTION *current) {
  AB_IMEXPORTER_ACCOUNTINFO *iea = 0;
  AB_TRANSACTION *t = 0;
  AB_VALUE *vAmount = 0;
  GWEN_TIME *ti = 0;
  char *defaultTime = "12000020", dateTime[15];

  // Search if account number is already in context
  // If so add transaction there, else make new account number in context.
  iea = AB_ImExporterContext_GetFirstAccountInfo(ctx);
  while(iea) {
    if (strcmp(AB_ImExporterAccountInfo_GetAccountNumber(iea), current->localAccountNumber) == 0) 
      break;
    iea = AB_ImExporterContext_GetNextAccountInfo(ctx);
  }

  if (!iea) {
    // Not found, add it
    iea = AB_ImExporterAccountInfo_new();
    AB_ImExporterContext_AddAccountInfo(ctx, iea);
    AB_ImExporterAccountInfo_SetType(iea, AB_AccountType_Bank);
    AB_ImExporterAccountInfo_SetBankName(iea, "Rabobank");
    AB_ImExporterAccountInfo_SetAccountNumber(iea, current->localAccountNumber);
  }

  // Now create AB Transaction and start filling it with what we know
  t = AB_Transaction_new();

  // remoteAccountNumber
  AB_Transaction_SetRemoteAccountNumber(t, current->remoteAccountNumber);

  // namePayee
  AB_Transaction_AddRemoteName(t, current->namePayee, 0);

  // amount
  vAmount = AB_Value_new(current->amount, "EUR");
  AB_Transaction_SetValue(t, vAmount);
  AB_Value_free(vAmount);

  // date
  // Transaction time, we take noon
  strcpy(dateTime, defaultTime);
  strcat(dateTime, current->date);
  ti = GWEN_Time_fromString(dateTime, "hhmmssYYYYMMDD");
  AB_Transaction_SetDate(t, ti);
  GWEN_Time_free(ti);

  // Same for valuta date
  strcpy(dateTime, defaultTime);
  strcat(dateTime, current->valutaDate);
  ti = GWEN_Time_fromString(dateTime, "hhmmssYYYYMMDD");
  AB_Transaction_SetValutaDate(t, ti);
  GWEN_Time_free(ti);

  // transactionId if there
  if (current->transactionIdValid) {
    AB_Transaction_SetCustomerReference(t, current->transactionId);
  }

  // Now add all the purpose descriptions if there
  eriAddPurpose(t, current->purpose1);
  eriAddPurpose(t, current->purpose2);
  eriAddPurpose(t, current->purpose3);
  eriAddPurpose(t, current->purpose4);
  eriAddPurpose(t, current->purpose5);
  eriAddPurpose(t, current->purpose6);

  // Add it to the AccountInfo List
  AB_ImExporterAccountInfo_AddTransaction(iea, t);

  return TRANS_OK;
}

int parseTransaction(AB_IMEXPORTER_CONTEXT *ctx,
		     GWEN_BUFFEREDIO *bio) {

  ERI_TRANSACTION trans, *current = &trans;
  int rerr, terr, aerr, translen;
  char recbuf[REC_LENGTH+1];
  recbuf[REC_LENGTH] = 0;
  current->transactionIdValid = FALSE;

  GWEN_BufferedIO_SetReadBuffer(bio, 0, REC_LENGTH);

  // Read the first record of the transaction
  rerr = eriReadRecord(bio, recbuf);

  if (rerr == GWEN_BUFFEREDIO_ERROR_READ) {
    // When Error on Read occurs here, buffer was empty, normal EOF
    return TRANS_EOF;
  } else if (rerr == GWEN_BUFFEREDIO_ERROR_EOF) {
    // With Error met EOF, EOF occured in middle of record
    printf("Bad first record in Transaction\n");
    return TRANS_BAD;
  }

  // Get the info from the first record of the transaction and place them in the struct
  terr = parseFirstRecord(recbuf, current);

  if (terr == REC_BAD) {
    return TRANS_BAD;
  }

  printf("l %s, r %s, n %s, a %f, d %s, v %s, i %s.\n", current->localAccountNumber, 
	 current->remoteAccountNumber, current->namePayee, current->amount,
	 current->date, current->valutaDate, current->transactionId);

  // Read the second record into recbuf
  rerr = eriReadRecord(bio, recbuf);

  // End of File should not happen here!
  if ((rerr == GWEN_BUFFEREDIO_ERROR_READ) || (rerr == GWEN_BUFFEREDIO_ERROR_EOF)) {
    printf("Transaction not complete, bad second record!\n");
    return TRANS_BAD;
  }

  // check how many records in transaction
  switch (recbuf[121-1]) {
  case '0':
    translen = LINES2;
    break;
  case '1':
    translen = LINES3;
    break;
  case '2':
    translen = LINES4;
    break;
  }

  // get the info from the second record and place them in transaction struct
  terr = parseSecondRecord(recbuf, current);

  if (terr == REC_BAD) {
    return TRANS_BAD;
  }

  printf("p1 %s, p2 %s.\n", current->purpose1, current->purpose2);

  // Clear all purpose strings of line 3 and 4. They may contain rubbish when lines are
  // not there
  *current->purpose3 = 0;
  *current->purpose4 = 0;
  *current->purpose5 = 0;
  *current->purpose6 = 0;

  // If 1 or 2 type 4 records (3 lines or 4 lines transaction), read and parse them
  if (translen >= LINES3) {
    // Read third record in recbuf
    rerr = eriReadRecord(bio, recbuf);

    // End of File should not happen here!
    if ((rerr == GWEN_BUFFEREDIO_ERROR_READ) || (rerr == GWEN_BUFFEREDIO_ERROR_EOF)) {
      printf("Transaction not complete, bad third record!\n");
      return TRANS_BAD;
    }

    // get the info from the third record and place them in transaction struct
    terr = parseThirdRecord(recbuf, current);

    if (terr == REC_BAD) {
      return TRANS_BAD;
    }

    printf("p3 %s, p4 %s, p5 %s.\n", current->purpose3, current->purpose4, 
	   current->purpose5);

    // If 4 line is present in transaction, read and parse it
    if (translen == LINES4) {
      // Read fourth record in buffer
      rerr = eriReadRecord(bio, recbuf);

      // End of File should not happen here!
      if ((rerr == GWEN_BUFFEREDIO_ERROR_READ) || (rerr == GWEN_BUFFEREDIO_ERROR_EOF)) {
        printf("Transaction not complete, bad fourth record!\n");
        return TRANS_BAD;
      }

      // get the info from the fourth record and place them in transaction struct
      terr = parseFourthRecord(recbuf, current);

      if (terr == REC_BAD) {
        return TRANS_BAD;
      }

      printf("p6 %s.\n", current->purpose6);

    }

  }

  if (current->transactionIdValid) {
    printf("t %s.\n", current->transactionId);
  }

  aerr = eriAddTransaction(ctx, current);

  return TRANS_OK;
}

AB_IMEXPORTER *eri_factory(AB_BANKING *ab, GWEN_DB_NODE *db) {
  AB_IMEXPORTER *ie;
  AH_IMEXPORTER_ERI *ieh;

  ie = AB_ImExporter_new(ab, "eri");
  GWEN_NEW_OBJECT(AH_IMEXPORTER_ERI, ieh);
  GWEN_INHERIT_SETDATA(AB_IMEXPORTER, AH_IMEXPORTER_ERI, ie, ieh,
		       AH_ImExporterERI_FreeData);

  ieh->dbData = db;

  AB_ImExporter_SetImportFn(ie, AH_ImExporterERI_Import);
  AB_ImExporter_SetExportFn(ie, AH_ImExporterERI_Export);
  AB_ImExporter_SetCheckFileFn(ie, AH_ImExporterERI_CheckFile);

  return ie;
}



void AH_ImExporterERI_FreeData(void *bp, void *p) {
  AH_IMEXPORTER_ERI *ieh;

  ieh = (AH_IMEXPORTER_ERI*) p;
  GWEN_FREE_OBJECT(ieh);
}

int AH_ImExporterERI_Import(AB_IMEXPORTER *ie,
			    AB_IMEXPORTER_CONTEXT *ctx,
			    GWEN_BUFFEREDIO *bio,
			    GWEN_DB_NODE *params) {
  int err;

  // check buffered IO is in place
  assert(bio);

  // Now start reading and parsing transactions until EOF or error
  while (!(err = parseTransaction(ctx, bio)));

  if (err == TRANS_EOF) return 0;  // EOF everything Ok


  if (err == TRANS_BAD) return AB_ERROR_BAD_DATA;  // Something is wrong with the transactions

  return AB_ERROR_UNKNOWN;

}

int AH_ImExporterERI_Export(AB_IMEXPORTER *ie,
			    AB_IMEXPORTER_CONTEXT *ctx,
			    GWEN_BUFFEREDIO *bio,
			    GWEN_DB_NODE *params) {
  return AB_ERROR_NOT_SUPPORTED;
}

int AH_ImExporterERI_CheckFile(AB_IMEXPORTER *ie, const char *fname) {
  int fd;
  char lbuffer[CHECKBUF_LENGTH];
  GWEN_BUFFEREDIO *bio;
  GWEN_ERRORCODE err;

  assert(ie);
  assert(fname);

  fd = open(fname, O_RDONLY);
  if (fd == -1) {
    /* error */
    DBG_ERROR(AQBANKING_LOGDOMAIN,
	      "open(%s): %s", fname, strerror(errno));
    return AB_ERROR_NOT_FOUND;
  }

  bio = GWEN_BufferedIO_File_new(fd);
  GWEN_BufferedIO_SetReadBuffer(bio, 0, CHECKBUF_LENGTH);

  err = GWEN_BufferedIO_ReadLine(bio, lbuffer, CHECKBUF_LENGTH);
  if (!GWEN_Error_IsOk(err)) {
    DBG_INFO(AQBANKING_LOGDOMAIN,
	     "File \"%s\" is not supported by this plugin",
	     fname);
    GWEN_BufferedIO_Close(bio);
    GWEN_BufferedIO_free(bio);
    return AB_ERROR_BAD_DATA;
  }

  if ( -1 != GWEN_Text_ComparePattern(lbuffer, "*EUR99999999992000*", 0)) {
    /* match */
    DBG_INFO(AQBANKING_LOGDOMAIN,
	     "File \"%s\" is supported by this plugin",
	     fname);
    GWEN_BufferedIO_Close(bio);
    GWEN_BufferedIO_free(bio);
    return 0;
  }

  GWEN_BufferedIO_Close(bio);
  GWEN_BufferedIO_free(bio);
  return AB_ERROR_BAD_DATA;
}
