/***************************************************************************
 begin       : Thu Apr 15 2010
 copyright   : (C) 2010 by Martin Preuss
 email       : martin@aqbanking.de

 ***************************************************************************
 * This file is part of the project "AqBanking".                           *
 * Please see toplevel file COPYING of that project for license details.   *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif



#include "dlg_editaccount_p.h"
#include "i18n_l.h"

#include <aqbanking/account.h>
#include <aqbanking/banking_be.h>
#include <aqbanking/dlg_selectbankinfo.h>

#include <gwenhywfar/gwenhywfar.h>
#include <gwenhywfar/misc.h>
#include <gwenhywfar/pathmanager.h>
#include <gwenhywfar/debug.h>
#include <gwenhywfar/gui.h>


#define DIALOG_MINWIDTH  400
#define DIALOG_MINHEIGHT 300

#define USER_LIST_MINCOLWIDTH 100



GWEN_INHERIT(GWEN_DIALOG, AB_EDIT_ACCOUNT_DIALOG)




GWEN_DIALOG *AB_EditAccountDialog_new(AB_BANKING *ab, AB_ACCOUNT *a, int doLock) {
  GWEN_DIALOG *dlg;
  AB_EDIT_ACCOUNT_DIALOG *xdlg;
  GWEN_BUFFER *fbuf;
  int rv;

  dlg=GWEN_Dialog_new("ab_edit_account");
  GWEN_NEW_OBJECT(AB_EDIT_ACCOUNT_DIALOG, xdlg);
  GWEN_INHERIT_SETDATA(GWEN_DIALOG, AB_EDIT_ACCOUNT_DIALOG, dlg, xdlg,
		       AB_EditAccountDialog_FreeData);
  GWEN_Dialog_SetSignalHandler(dlg, AB_EditAccountDialog_SignalHandler);

  /* get path of dialog description file */
  fbuf=GWEN_Buffer_new(0, 256, 0, 1);
  rv=GWEN_PathManager_FindFile(GWEN_PM_LIBNAME, GWEN_PM_SYSDATADIR,
			       "aqbanking/dialogs/dlg_editaccount.dlg",
			       fbuf);
  if (rv<0) {
    DBG_INFO(AQBANKING_LOGDOMAIN, "Dialog description file not found (%d).", rv);
    GWEN_Buffer_free(fbuf);
    GWEN_Dialog_free(dlg);
    return NULL;
  }

  /* read dialog from dialog description file */
  rv=GWEN_Dialog_ReadXmlFile(dlg, GWEN_Buffer_GetStart(fbuf));
  if (rv<0) {
    DBG_INFO(AQBANKING_LOGDOMAIN, "here (%d).", rv);
    GWEN_Buffer_free(fbuf);
    GWEN_Dialog_free(dlg);
    return NULL;
  }
  GWEN_Buffer_free(fbuf);

  xdlg->banking=ab;
  xdlg->account=a;
  xdlg->doLock=doLock;

  /* done */
  return dlg;
}



void GWENHYWFAR_CB AB_EditAccountDialog_FreeData(void *bp, void *p) {
  AB_EDIT_ACCOUNT_DIALOG *xdlg;

  xdlg=(AB_EDIT_ACCOUNT_DIALOG*) p;
  AB_User_List2_free(xdlg->selectedUsers);
  GWEN_FREE_OBJECT(xdlg);
}



static int createCountryString(const AB_COUNTRY *c, GWEN_BUFFER *tbuf) {
  const char *s;

  s=AB_Country_GetLocalName(c);
  if (s && *s) {
    GWEN_Buffer_AppendString(tbuf, s);
    s=AB_Country_GetCode(c);
    if (s && *s) {
      GWEN_Buffer_AppendString(tbuf, " (");
      GWEN_Buffer_AppendString(tbuf, s);
      GWEN_Buffer_AppendString(tbuf, ")");
    }
    return 0;
  }
  DBG_INFO(AQBANKING_LOGDOMAIN, "No local name");
  return GWEN_ERROR_NO_DATA;
}



static int createCurrencyString(const AB_COUNTRY *c, GWEN_BUFFER *tbuf) {
  const char *s;

  s=AB_Country_GetLocalCurrencyName(c);
  if (s && *s) {
    GWEN_Buffer_AppendString(tbuf, s);
    s=AB_Country_GetCurrencyCode(c);
    if (s && *s) {
      GWEN_Buffer_AppendString(tbuf, " (");
      GWEN_Buffer_AppendString(tbuf, s);
      GWEN_Buffer_AppendString(tbuf, ")");
    }
    return 0;
  }
  DBG_INFO(AQBANKING_LOGDOMAIN, "No local name");
  return GWEN_ERROR_NO_DATA;
}



static void createUserString(const AB_USER *u, GWEN_BUFFER *tbuf) {
  const char *s;
  char numbuf[32];
  uint32_t uid;
  
  /* column 1 */
  uid=AB_User_GetUniqueId(u);
  snprintf(numbuf, sizeof(numbuf)-1, "%d", uid);
  numbuf[sizeof(numbuf)-1]=0;
  GWEN_Buffer_AppendString(tbuf, numbuf);
  GWEN_Buffer_AppendString(tbuf, "\t");
  
  /* column 2 */
  s=AB_User_GetUserId(u);
  if (s && *s)
    GWEN_Buffer_AppendString(tbuf, s);
  GWEN_Buffer_AppendString(tbuf, "\t");
  
  /* column 3 */
  s=AB_User_GetCustomerId(u);
  if (s && *s)
    GWEN_Buffer_AppendString(tbuf, s);
  GWEN_Buffer_AppendString(tbuf, "\t");
  
  /* column 4 */
  s=AB_User_GetUserName(u);
  if (s && *s)
    GWEN_Buffer_AppendString(tbuf, s);
}



AB_USER *AB_EditAccountDialog_GetCurrentUser(GWEN_DIALOG *dlg, const char *listBoxName) {
  AB_EDIT_ACCOUNT_DIALOG *xdlg;
  AB_USER_LIST2 *ul;

  assert(dlg);
  xdlg=GWEN_INHERIT_GETDATA(GWEN_DIALOG, AB_EDIT_ACCOUNT_DIALOG, dlg);
  assert(xdlg);

  /* user list */
  ul=AB_Banking_GetUsers(xdlg->banking);
  if (ul) {
    int idx;

    idx=GWEN_Dialog_GetIntProperty(dlg, listBoxName, GWEN_DialogProperty_Value, 0, -1);
    if (idx>=0) {
      const char *currentText;
  
      currentText=GWEN_Dialog_GetCharProperty(dlg, listBoxName, GWEN_DialogProperty_Value, idx, NULL);
      if (currentText && *currentText) {
	AB_USER_LIST2_ITERATOR *it;
    
	it=AB_User_List2_First(ul);
	if (it) {
	  AB_USER *u;
	  GWEN_BUFFER *tbuf;
    
	  tbuf=GWEN_Buffer_new(0, 256, 0, 1);
	  u=AB_User_List2Iterator_Data(it);
	  while(u) {
	    createUserString(u, tbuf);
	    if (strcasecmp(currentText, GWEN_Buffer_GetStart(tbuf))==0) {
	      GWEN_Buffer_free(tbuf);
	      AB_User_List2Iterator_free(it);
	      AB_User_List2_free(ul);
	      return u;
	    }
	    GWEN_Buffer_Reset(tbuf);
	    u=AB_User_List2Iterator_Next(it);
	  }
	  GWEN_Buffer_free(tbuf);

	  AB_User_List2Iterator_free(it);
	}
	AB_User_List2_free(ul);
      }
    }
  }

  return NULL;
}



void AB_EditAccountDialog_SampleUserList(GWEN_DIALOG *dlg,
					 const char *listBoxName,
					 AB_USER_LIST2 *destList) {
  AB_EDIT_ACCOUNT_DIALOG *xdlg;
  AB_USER_LIST2 *ul;

  assert(dlg);
  xdlg=GWEN_INHERIT_GETDATA(GWEN_DIALOG, AB_EDIT_ACCOUNT_DIALOG, dlg);
  assert(xdlg);

  /* user list */
  ul=AB_Banking_GetUsers(xdlg->banking);
  if (ul) {
    int i;
    int num;

    num=GWEN_Dialog_GetIntProperty(dlg, listBoxName, GWEN_DialogProperty_ValueCount, 0, 0);
    for (i=0; i<num; i++) {
      const char *t;
  
      t=GWEN_Dialog_GetCharProperty(dlg, listBoxName, GWEN_DialogProperty_Value, i, NULL);
      if (t && *t) {
	AB_USER_LIST2_ITERATOR *it;
    
	it=AB_User_List2_First(ul);
	if (it) {
	  AB_USER *u;
	  GWEN_BUFFER *tbuf;
    
	  tbuf=GWEN_Buffer_new(0, 256, 0, 1);
	  u=AB_User_List2Iterator_Data(it);
	  while(u) {
	    createUserString(u, tbuf);
	    if (strcasecmp(t, GWEN_Buffer_GetStart(tbuf))==0)
	      AB_User_List2_PushBack(destList, u);
	    GWEN_Buffer_Reset(tbuf);
	    u=AB_User_List2Iterator_Next(it);
	  }
	  GWEN_Buffer_free(tbuf);

	  AB_User_List2Iterator_free(it);
	}
	AB_User_List2_free(ul);
      }
    }
  }
}



int AB_EditAccountDialog_FindUserEntry(GWEN_DIALOG *dlg,
				       AB_USER *u,
				       const char *listBoxName) {
  AB_EDIT_ACCOUNT_DIALOG *xdlg;
  GWEN_BUFFER *tbuf;
  int i;
  int num;
  const char *s;

  assert(dlg);
  xdlg=GWEN_INHERIT_GETDATA(GWEN_DIALOG, AB_EDIT_ACCOUNT_DIALOG, dlg);
  assert(xdlg);

  tbuf=GWEN_Buffer_new(0, 256, 0, 1);
  createUserString(u, tbuf);
  s=GWEN_Buffer_GetStart(tbuf);

  /* user list */
  num=GWEN_Dialog_GetIntProperty(dlg, listBoxName, GWEN_DialogProperty_ValueCount, 0, 0);
  for (i=0; i<num; i++) {
    const char *t;

    t=GWEN_Dialog_GetCharProperty(dlg, listBoxName, GWEN_DialogProperty_Value, i, NULL);
    if (t && *t && strcasecmp(s, t)==0) {
      GWEN_Buffer_free(tbuf);
      return i;
    }
  }
  GWEN_Buffer_free(tbuf);

  return -1;
}



void AB_EditAccountDialog_RebuildUserLists(GWEN_DIALOG *dlg) {
  AB_EDIT_ACCOUNT_DIALOG *xdlg;
  AB_USER_LIST2 *users;

  assert(dlg);
  xdlg=GWEN_INHERIT_GETDATA(GWEN_DIALOG, AB_EDIT_ACCOUNT_DIALOG, dlg);
  assert(xdlg);

  GWEN_Dialog_SetIntProperty(dlg, "availUserList", GWEN_DialogProperty_ClearValues, 0, 0, 0);
  GWEN_Dialog_SetIntProperty(dlg, "selectedUserList", GWEN_DialogProperty_ClearValues, 0, 0, 0);

  /* setup lists of available and selected users */
  users=AB_Banking_FindUsers(xdlg->banking,
			     AB_Account_GetBackendName(xdlg->account),
			     "*", "*", "*", "*");
  if (users) {
    AB_USER_LIST2_ITERATOR *it1;
    GWEN_BUFFER *tbuf;

    tbuf=GWEN_Buffer_new(0, 256, 0, 1);
    it1=AB_User_List2_First(users);
    if (it1) {
      AB_USER *u1;

      u1=AB_User_List2Iterator_Data(it1);
      while(u1) {
	int isSelected=0;

	if (xdlg->selectedUsers)
	  isSelected=(AB_User_List2_Contains(xdlg->selectedUsers, u1)!=NULL);

	createUserString(u1, tbuf);
	if (isSelected)
	  GWEN_Dialog_SetCharProperty(dlg,
				      "selectedUserList",
				      GWEN_DialogProperty_AddValue,
				      0,
				      GWEN_Buffer_GetStart(tbuf),
				      0);
	else
	  GWEN_Dialog_SetCharProperty(dlg,
				      "availUserList",
				      GWEN_DialogProperty_AddValue,
				      0,
				      GWEN_Buffer_GetStart(tbuf),
				      0);
	GWEN_Buffer_Reset(tbuf);
	u1=AB_User_List2Iterator_Next(it1);
      }
      AB_User_List2Iterator_free(it1);
    }
    GWEN_Buffer_Reset(tbuf);
  }
  AB_User_List2_free(users);
}



void AB_EditAccountDialog_Init(GWEN_DIALOG *dlg) {
  AB_EDIT_ACCOUNT_DIALOG *xdlg;
  GWEN_DB_NODE *dbPrefs;
  AB_USER_LIST2 *ul;
  int i;
  int j;
  const char *s;
  AB_ACCOUNT_TYPE t;

  assert(dlg);
  xdlg=GWEN_INHERIT_GETDATA(GWEN_DIALOG, AB_EDIT_ACCOUNT_DIALOG, dlg);
  assert(xdlg);

  dbPrefs=GWEN_Dialog_GetPreferences(dlg);

  /* init */
  xdlg->countryList=AB_Banking_ListCountriesByName(xdlg->banking, "*");

  GWEN_Dialog_SetCharProperty(dlg,
			      "",
			      GWEN_DialogProperty_Title,
			      0,
			      I18N("Edit Account"),
			      0);

  /* setup country */
  if (xdlg->countryList) {
    AB_COUNTRY_CONSTLIST2_ITERATOR *it;
    int idx=-1;
    const char *selectedCountry;

    selectedCountry=AB_Account_GetCountry(xdlg->account);
    it=AB_Country_ConstList2_First(xdlg->countryList);
    if (it) {
      const AB_COUNTRY *c;
      GWEN_BUFFER *tbuf;
      GWEN_STRINGLIST *sl;
      GWEN_STRINGLISTENTRY *se;
      int i=0;

      sl=GWEN_StringList_new();
      tbuf=GWEN_Buffer_new(0, 256, 0, 1);
      c=AB_Country_ConstList2Iterator_Data(it);
      while(c) {
	GWEN_Buffer_AppendByte(tbuf, '1');
	if (createCountryString(c, tbuf)==0) {
	  const char *s;

          s=AB_Country_GetCode(c);
	  if (idx==-1 && selectedCountry && s && strcasecmp(s, selectedCountry)==0) {
	    char *p;

	    p=GWEN_Buffer_GetStart(tbuf);
	    if (p)
	      *p='0';
	    idx=i;
	  }
	  GWEN_StringList_AppendString(sl, GWEN_Buffer_GetStart(tbuf), 0, 1);
          i++;
	}
        GWEN_Buffer_Reset(tbuf);
	c=AB_Country_ConstList2Iterator_Next(it);
      }
      GWEN_Buffer_free(tbuf);
      AB_Country_ConstList2Iterator_free(it);

      GWEN_StringList_Sort(sl, 0, GWEN_StringList_SortModeNoCase);
      idx=-1;
      i=0;
      se=GWEN_StringList_FirstEntry(sl);
      while(se) {
	const char *s;

	s=GWEN_StringListEntry_Data(se);
	if (*s=='0')
          idx=i;
	GWEN_Dialog_SetCharProperty(dlg, "countryCombo", GWEN_DialogProperty_AddValue, 0, s+1, 0);
        i++;
	se=GWEN_StringListEntry_Next(se);
      }
      GWEN_StringList_free(sl);
    }
    if (idx>=0)
      /* chooses selected entry in combo box */
      GWEN_Dialog_SetIntProperty(dlg, "countryCombo", GWEN_DialogProperty_Value, 0, idx, 0);
  }

  /* setup currency */
  if (xdlg->countryList) {
    AB_COUNTRY_CONSTLIST2_ITERATOR *it;
    int idx=-1;
    const char *selectedCurrency;

    selectedCurrency=AB_Account_GetCurrency(xdlg->account);
    it=AB_Country_ConstList2_First(xdlg->countryList);
    if (it) {
      const AB_COUNTRY *c;
      GWEN_BUFFER *tbuf;
      GWEN_STRINGLIST *sl;
      GWEN_STRINGLISTENTRY *se;
      int i=0;

      sl=GWEN_StringList_new();
      tbuf=GWEN_Buffer_new(0, 256, 0, 1);
      c=AB_Country_ConstList2Iterator_Data(it);
      while(c) {
	GWEN_Buffer_AppendByte(tbuf, '1');
	if (createCurrencyString(c, tbuf)==0) {
	  const char *s;

          s=AB_Country_GetCurrencyCode(c);
	  if (idx==-1 && selectedCurrency && s && strcasecmp(s, selectedCurrency)==0) {
	    char *p;

	    p=GWEN_Buffer_GetStart(tbuf);
	    if (p)
              *p='0';
	    idx=i;
	  }
	  GWEN_StringList_AppendString(sl, GWEN_Buffer_GetStart(tbuf), 0, 1);
          i++;
	}
        GWEN_Buffer_Reset(tbuf);
	c=AB_Country_ConstList2Iterator_Next(it);
      }
      GWEN_Buffer_free(tbuf);
      AB_Country_ConstList2Iterator_free(it);

      GWEN_StringList_Sort(sl, 0, GWEN_StringList_SortModeNoCase);
      idx=-1;
      i=0;
      se=GWEN_StringList_FirstEntry(sl);
      while(se) {
	const char *s;

	s=GWEN_StringListEntry_Data(se);
	if (*s=='0')
	  idx=i;
	GWEN_Dialog_SetCharProperty(dlg, "currencyCombo", GWEN_DialogProperty_AddValue, 0, s+1, 0);
        i++;
	se=GWEN_StringListEntry_Next(se);
      }
      GWEN_StringList_free(sl);
    }
    if (idx>=0)
      /* chooses selected entry in combo box */
      GWEN_Dialog_SetIntProperty(dlg, "currencyCombo", GWEN_DialogProperty_Value, 0, idx, 0);
  }

  s=AB_Account_GetBankCode(xdlg->account);
  GWEN_Dialog_SetCharProperty(dlg, "bankCodeEdit", GWEN_DialogProperty_Value, 0, s, 0);

  s=AB_Account_GetBankName(xdlg->account);
  GWEN_Dialog_SetCharProperty(dlg, "bankNameEdit", GWEN_DialogProperty_Value, 0, s, 0);

  s=AB_Account_GetBIC(xdlg->account);
  GWEN_Dialog_SetCharProperty(dlg, "bicEdit", GWEN_DialogProperty_Value, 0, s, 0);

  s=AB_Account_GetAccountNumber(xdlg->account);
  GWEN_Dialog_SetCharProperty(dlg, "accountNumberEdit", GWEN_DialogProperty_Value, 0, s, 0);

  s=AB_Account_GetAccountName(xdlg->account);
  GWEN_Dialog_SetCharProperty(dlg, "accountNameEdit", GWEN_DialogProperty_Value, 0, s, 0);

  s=AB_Account_GetIBAN(xdlg->account);
  GWEN_Dialog_SetCharProperty(dlg, "ibanEdit", GWEN_DialogProperty_Value, 0, s, 0);

  s=AB_Account_GetOwnerName(xdlg->account);
  GWEN_Dialog_SetCharProperty(dlg, "ownerNameEdit", GWEN_DialogProperty_Value, 0, s, 0);

  /* setup account type */
  GWEN_Dialog_SetCharProperty(dlg, "accountTypeCombo", GWEN_DialogProperty_AddValue, 0,
			      I18N("unknown"),
			      0);
  GWEN_Dialog_SetCharProperty(dlg, "accountTypeCombo", GWEN_DialogProperty_AddValue, 0,
			      I18N("Bank Account"),
			      0);
  GWEN_Dialog_SetCharProperty(dlg, "accountTypeCombo", GWEN_DialogProperty_AddValue, 0,
			      I18N("Credit Card Account"),
			      0);
  GWEN_Dialog_SetCharProperty(dlg, "accountTypeCombo", GWEN_DialogProperty_AddValue, 0,
			      I18N("Checking Account"),
			      0);
  GWEN_Dialog_SetCharProperty(dlg, "accountTypeCombo", GWEN_DialogProperty_AddValue, 0,
			      I18N("Savings Account"),
			      0);
  GWEN_Dialog_SetCharProperty(dlg, "accountTypeCombo", GWEN_DialogProperty_AddValue, 0,
			      I18N("Investment Account"),
			      0);
  GWEN_Dialog_SetCharProperty(dlg, "accountTypeCombo", GWEN_DialogProperty_AddValue, 0,
			      I18N("Cash Account"),
			      0);
  GWEN_Dialog_SetCharProperty(dlg, "accountTypeCombo", GWEN_DialogProperty_AddValue, 0,
			      I18N("Moneymarket Account"),
			      0);

  t=AB_Account_GetAccountType(xdlg->account);
  if (t<AB_AccountType_MoneyMarket)
    GWEN_Dialog_SetIntProperty(dlg, "accountTypeCombo", GWEN_DialogProperty_Value, 0, t, 0);

  /* available user list */
  GWEN_Dialog_SetCharProperty(dlg,
			      "availUserList",
			      GWEN_DialogProperty_Title,
			      0,
			      I18N("Id\tUser Id\tCustomer Id\tUser Name"),
			      0);
  GWEN_Dialog_SetIntProperty(dlg,
			     "availUserList",
			     GWEN_DialogProperty_SelectionMode,
			     0,
			     GWEN_Dialog_SelectionMode_Single,
			     0);

  /* selected user list */
  GWEN_Dialog_SetCharProperty(dlg,
			      "selectedUserList",
			      GWEN_DialogProperty_Title,
			      0,
			      I18N("Id\tUser Id\tCustomer Id\tUser Name"),
			      0);
  GWEN_Dialog_SetIntProperty(dlg,
			     "selectedUserList",
			     GWEN_DialogProperty_SelectionMode,
			     0,
			     GWEN_Dialog_SelectionMode_Single,
			     0);

  ul=AB_Account_GetSelectedUsers(xdlg->account);
  if (ul) {
    if (xdlg->selectedUsers)
      AB_User_List2_free(xdlg->selectedUsers);
    xdlg->selectedUsers=AB_User_List2_dup(ul);

  }
  AB_EditAccountDialog_RebuildUserLists(dlg);


  /* read width */
  i=GWEN_DB_GetIntValue(dbPrefs, "dialog_width", 0, -1);
  if (i<DIALOG_MINWIDTH)
    i=DIALOG_MINWIDTH;
  GWEN_Dialog_SetIntProperty(dlg, "", GWEN_DialogProperty_Width, 0, i, 0);

  /* read height */
  i=GWEN_DB_GetIntValue(dbPrefs, "dialog_height", 0, -1);
  if (i<DIALOG_MINHEIGHT)
    i=DIALOG_MINHEIGHT;
  GWEN_Dialog_SetIntProperty(dlg, "", GWEN_DialogProperty_Height, 0, i, 0);

  /* read avail user column widths */
  for (i=0; i<4; i++) {
    j=GWEN_DB_GetIntValue(dbPrefs, "avail_user_list_columns", i, -1);
    if (j<USER_LIST_MINCOLWIDTH)
      j=USER_LIST_MINCOLWIDTH;
    GWEN_Dialog_SetIntProperty(dlg, "availUserList", GWEN_DialogProperty_ColumnWidth, i, j, 0);
  }
  /* get sort column */
  i=GWEN_DB_GetIntValue(dbPrefs, "avail_user_list_sortbycolumn", 0, -1);
  j=GWEN_DB_GetIntValue(dbPrefs, "avail_user_list_sortdir", 0, -1);
  if (i>=0 && j>=0)
    GWEN_Dialog_SetIntProperty(dlg, "availUserList", GWEN_DialogProperty_SortDirection, i, j, 0);

  /* read selected user column widths */
  for (i=0; i<4; i++) {
    j=GWEN_DB_GetIntValue(dbPrefs, "sel_user_list_columns", i, -1);
    if (j<USER_LIST_MINCOLWIDTH)
      j=USER_LIST_MINCOLWIDTH;
    GWEN_Dialog_SetIntProperty(dlg, "selectedUserList", GWEN_DialogProperty_ColumnWidth, i, j, 0);
  }
  /* get sort column */
  i=GWEN_DB_GetIntValue(dbPrefs, "sel_user_list_sortbycolumn", 0, -1);
  j=GWEN_DB_GetIntValue(dbPrefs, "sel_user_list_sortdir", 0, -1);
  if (i>=0 && j>=0)
    GWEN_Dialog_SetIntProperty(dlg, "selectedUserList", GWEN_DialogProperty_SortDirection, i, j, 0);

}



void AB_EditAccountDialog_Fini(GWEN_DIALOG *dlg) {
  AB_EDIT_ACCOUNT_DIALOG *xdlg;
  int i;
  GWEN_DB_NODE *dbPrefs;

  assert(dlg);
  xdlg=GWEN_INHERIT_GETDATA(GWEN_DIALOG, AB_EDIT_ACCOUNT_DIALOG, dlg);
  assert(xdlg);

  dbPrefs=GWEN_Dialog_GetPreferences(dlg);

  /* fromGui */




  /* store dialog width */
  i=GWEN_Dialog_GetIntProperty(dlg, "", GWEN_DialogProperty_Width, 0, -1);
  if (i<DIALOG_MINWIDTH)
    i=DIALOG_MINWIDTH;
  GWEN_DB_SetIntValue(dbPrefs,
		      GWEN_DB_FLAGS_OVERWRITE_VARS,
		      "dialog_width",
		      i);

  /* store dialog height */
  i=GWEN_Dialog_GetIntProperty(dlg, "", GWEN_DialogProperty_Height, 0, -1);
  if (i<DIALOG_MINHEIGHT)
    i=DIALOG_MINHEIGHT;
  GWEN_DB_SetIntValue(dbPrefs,
		      GWEN_DB_FLAGS_OVERWRITE_VARS,
		      "dialog_height",
		      i);

  /* store column widths of user list */
  GWEN_DB_DeleteVar(dbPrefs, "avail_user_list_columns");
  for (i=0; i<4; i++) {
    int j;

    j=GWEN_Dialog_GetIntProperty(dlg, "availUserList", GWEN_DialogProperty_ColumnWidth, i, -1);
    if (j<USER_LIST_MINCOLWIDTH)
      j=USER_LIST_MINCOLWIDTH;
    GWEN_DB_SetIntValue(dbPrefs,
			GWEN_DB_FLAGS_DEFAULT,
			"avail_user_list_columns",
			j);
  }
  /* store column sorting of user list */
  GWEN_DB_SetIntValue(dbPrefs,
		      GWEN_DB_FLAGS_OVERWRITE_VARS,
		      "avail_user_list_sortbycolumn",
		      -1);
  for (i=0; i<4; i++) {
    int j;

    j=GWEN_Dialog_GetIntProperty(dlg, "availUserList", GWEN_DialogProperty_SortDirection, i,
				 GWEN_DialogSortDirection_None);
    if (j!=GWEN_DialogSortDirection_None) {
      GWEN_DB_SetIntValue(dbPrefs,
			  GWEN_DB_FLAGS_OVERWRITE_VARS,
			  "avail_user_list_sortbycolumn",
			  i);
      GWEN_DB_SetIntValue(dbPrefs,
			  GWEN_DB_FLAGS_OVERWRITE_VARS,
			  "avail_user_list_sortdir",
			  (j==GWEN_DialogSortDirection_Up)?1:0);
      break;
    }
  }

  /* store column widths of user list */
  GWEN_DB_DeleteVar(dbPrefs, "sel_user_list_columns");
  for (i=0; i<4; i++) {
    int j;

    j=GWEN_Dialog_GetIntProperty(dlg, "selectedUserList", GWEN_DialogProperty_ColumnWidth, i, -1);
    if (j<USER_LIST_MINCOLWIDTH)
      j=USER_LIST_MINCOLWIDTH;
    GWEN_DB_SetIntValue(dbPrefs,
			GWEN_DB_FLAGS_DEFAULT,
			"sel_user_list_columns",
			j);
  }
  /* store column sorting of user list */
  GWEN_DB_SetIntValue(dbPrefs,
		      GWEN_DB_FLAGS_OVERWRITE_VARS,
		      "sel_user_list_sortbycolumn",
		      -1);
  for (i=0; i<4; i++) {
    int j;

    j=GWEN_Dialog_GetIntProperty(dlg, "selectedUserList", GWEN_DialogProperty_SortDirection, i,
				 GWEN_DialogSortDirection_None);
    if (j!=GWEN_DialogSortDirection_None) {
      GWEN_DB_SetIntValue(dbPrefs,
			  GWEN_DB_FLAGS_OVERWRITE_VARS,
			  "sel_user_list_sortbycolumn",
			  i);
      GWEN_DB_SetIntValue(dbPrefs,
			  GWEN_DB_FLAGS_OVERWRITE_VARS,
			  "sel_user_list_sortdir",
			  (j==GWEN_DialogSortDirection_Up)?1:0);
      break;
    }
  }

}



int AB_EditAccountDialog_HandleActivatedToRight(GWEN_DIALOG *dlg) {
  AB_EDIT_ACCOUNT_DIALOG *xdlg;
  AB_USER *u;

  assert(dlg);
  xdlg=GWEN_INHERIT_GETDATA(GWEN_DIALOG, AB_EDIT_ACCOUNT_DIALOG, dlg);
  assert(xdlg);

  u=AB_EditAccountDialog_GetCurrentUser(dlg, "availUserList");
  if (u) {
    AB_User_List2_PushBack(xdlg->selectedUsers, u);
    AB_EditAccountDialog_RebuildUserLists(dlg);
  }
  return GWEN_DialogEvent_ResultHandled;
}



int AB_EditAccountDialog_HandleActivatedToLeft(GWEN_DIALOG *dlg) {
  AB_EDIT_ACCOUNT_DIALOG *xdlg;
  AB_USER *u;

  assert(dlg);
  xdlg=GWEN_INHERIT_GETDATA(GWEN_DIALOG, AB_EDIT_ACCOUNT_DIALOG, dlg);
  assert(xdlg);

  u=AB_EditAccountDialog_GetCurrentUser(dlg, "selectedUserList");
  if (u) {
    AB_User_List2_Remove(xdlg->selectedUsers, u);
    AB_EditAccountDialog_RebuildUserLists(dlg);
  }
  return GWEN_DialogEvent_ResultHandled;
}



int AB_EditAccountDialog_HandleActivatedBankCode(GWEN_DIALOG *dlg) {
  AB_EDIT_ACCOUNT_DIALOG *xdlg;
  GWEN_DIALOG *dlg2;
  int rv;

  assert(dlg);
  xdlg=GWEN_INHERIT_GETDATA(GWEN_DIALOG, AB_EDIT_ACCOUNT_DIALOG, dlg);
  assert(xdlg);

  dlg2=AB_SelectBankInfoDialog_new(xdlg->banking, "de", NULL);
  if (dlg2==NULL) {
    DBG_ERROR(AQBANKING_LOGDOMAIN, "Could not create dialog");
    return GWEN_DialogEvent_ResultHandled;
  }

  rv=GWEN_Gui_ExecDialog(dlg2, 0);
  if (rv==0) {
    /* rejected */
    GWEN_Dialog_free(dlg2);
    return GWEN_DialogEvent_ResultHandled;
  }
  else {
    const AB_BANKINFO *bi;

    bi=AB_SelectBankInfoDialog_GetSelectedBankInfo(dlg2);
    if (bi) {
      const char *s;

      s=AB_BankInfo_GetBankId(bi);
      GWEN_Dialog_SetCharProperty(dlg,
				  "bankCodeEdit",
				  GWEN_DialogProperty_Value,
				  0,
				  (s && *s)?s:"",
				  0);

      s=AB_BankInfo_GetBankName(bi);
      GWEN_Dialog_SetCharProperty(dlg,
				  "bankNameEdit",
				  GWEN_DialogProperty_Value,
				  0,
				  (s && *s)?s:"",
				  0);
    }
  }
  GWEN_Dialog_free(dlg2);

  return GWEN_DialogEvent_ResultHandled;
}



int AB_EditAccountDialog_HandleActivated(GWEN_DIALOG *dlg, const char *sender) {
  if (strcasecmp(sender, "toRightButton")==0)
    return AB_EditAccountDialog_HandleActivatedToRight(dlg);
  else if (strcasecmp(sender, "toLeftButton")==0)
    return AB_EditAccountDialog_HandleActivatedToLeft(dlg);
  else if (strcasecmp(sender, "bankCodeButton")==0)
    return AB_EditAccountDialog_HandleActivatedBankCode(dlg);
  else if (strcasecmp(sender, "okButton")==0)
    return GWEN_DialogEvent_ResultAccept;
  else if (strcasecmp(sender, "abortButton")==0)
    return GWEN_DialogEvent_ResultReject;
  else if (strcasecmp(sender, "helpButton")==0) {
    /* TODO: open a help dialog */
  }

  return GWEN_DialogEvent_ResultNotHandled;
}



int GWENHYWFAR_CB AB_EditAccountDialog_SignalHandler(GWEN_DIALOG *dlg,
						     GWEN_DIALOG_EVENTTYPE t,
						     const char *sender) {
  AB_EDIT_ACCOUNT_DIALOG *xdlg;

  assert(dlg);
  xdlg=GWEN_INHERIT_GETDATA(GWEN_DIALOG, AB_EDIT_ACCOUNT_DIALOG, dlg);
  assert(xdlg);

  switch(t) {
  case GWEN_DialogEvent_TypeInit:
    AB_EditAccountDialog_Init(dlg);
    return GWEN_DialogEvent_ResultHandled;;

  case GWEN_DialogEvent_TypeFini:
    AB_EditAccountDialog_Fini(dlg);
    return GWEN_DialogEvent_ResultHandled;;

  case GWEN_DialogEvent_TypeValueChanged:
    DBG_ERROR(0, "ValueChanged: %s", sender);
    return GWEN_DialogEvent_ResultHandled;;

  case GWEN_DialogEvent_TypeActivated:
    return AB_EditAccountDialog_HandleActivated(dlg, sender);

  case GWEN_DialogEvent_TypeEnabled:
  case GWEN_DialogEvent_TypeDisabled:
  case GWEN_DialogEvent_TypeClose:

  case GWEN_DialogEvent_TypeLast:
    return GWEN_DialogEvent_ResultNotHandled;

  }

  return GWEN_DialogEvent_ResultNotHandled;
}





