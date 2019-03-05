/***************************************************************************
 begin       : Mon Apr 12 2010
 copyright   : (C) 2018 by Martin Preuss
 email       : martin@aqbanking.de

 ***************************************************************************
 * This file is part of the project "AqBanking".                           *
 * Please see toplevel file COPYING of that project for license details.   *
 ***************************************************************************/

#ifndef AQHBCI_DLG_PINTAN_H
#define AQHBCI_DLG_PINTAN_H


#include <aqhbci/aqhbci.h>

#include <aqbanking/banking.h>
#include <aqbanking/backendsupport/user.h>

#include <gwenhywfar/dialog.h>
#include <gwenhywfar/db.h>


#ifdef __cplusplus
extern "C" {
#endif



GWEN_DIALOG *AH_PinTanDialog_new(AB_PROVIDER *pro);

const char *AH_PinTanDialog_GetBankCode(const GWEN_DIALOG *dlg);
void AH_PinTanDialog_SetBankCode(GWEN_DIALOG *dlg, const char *s);

const char *AH_PinTanDialog_GetBankName(const GWEN_DIALOG *dlg);
void AH_PinTanDialog_SetBankName(GWEN_DIALOG *dlg, const char *s);

const char *AH_PinTanDialog_GetUserName(const GWEN_DIALOG *dlg);
void AH_PinTanDialog_SetUserName(GWEN_DIALOG *dlg, const char *s);

const char *AH_PinTanDialog_GetUserId(const GWEN_DIALOG *dlg);
void AH_PinTanDialog_SetUserId(GWEN_DIALOG *dlg, const char *s);

const char *AH_PinTanDialog_GetCustomerId(const GWEN_DIALOG *dlg);
void AH_PinTanDialog_SetCustomerId(GWEN_DIALOG *dlg, const char *s);

const char *AH_PinTanDialog_GetUrl(const GWEN_DIALOG *dlg);
void AH_PinTanDialog_SetUrl(GWEN_DIALOG *dlg, const char *s);

int AH_PinTanDialog_GetHttpVMajor(const GWEN_DIALOG *dlg);
int AH_PinTanDialog_GetHttpVMinor(const GWEN_DIALOG *dlg);
void AH_PinTanDialog_SetHttpVersion(GWEN_DIALOG *dlg, int vmajor, int vminor);

int AH_PinTanDialog_GetHbciVersion(const GWEN_DIALOG *dlg);
void AH_PinTanDialog_SetHbciVersion(GWEN_DIALOG *dlg, int i);

uint32_t AH_PinTanDialog_GetFlags(const GWEN_DIALOG *dlg);
void AH_PinTanDialog_SetFlags(GWEN_DIALOG *dlg, uint32_t fl);
void AH_PinTanDialog_AddFlags(GWEN_DIALOG *dlg, uint32_t fl);
void AH_PinTanDialog_SubFlags(GWEN_DIALOG *dlg, uint32_t fl);

const char *AH_PinTanDialog_GetTanMediumId(const GWEN_DIALOG *dlg);
void AH_PinTanDialog_SetTanMediumId(GWEN_DIALOG *dlg, const char *s);


AB_USER *AH_PinTanDialog_GetUser(const GWEN_DIALOG *dlg);


#ifdef __cplusplus
}
#endif



#endif

