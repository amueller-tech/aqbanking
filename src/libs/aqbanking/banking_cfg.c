/***************************************************************************
 begin       : Sat Sep 27 2008
 copyright   : (C) 2008 by Martin Preuss
 email       : martin@libchipcard.de

 ***************************************************************************
 * This file is part of the project "AqBanking".                           *
 * Please see toplevel file COPYING of that project for license details.   *
 ***************************************************************************/



int AB_Banking__GetConfigManager(AB_BANKING *ab, const char *dname) {
  GWEN_BUFFER *buf;
  char home[256];

  if (GWEN_Directory_GetHomeDirectory(home, sizeof(home))) {
    DBG_ERROR(AQBANKING_LOGDOMAIN,
	      "Could not determine home directory, aborting.");
    abort();
  }

  buf=GWEN_Buffer_new(0, 256, 0, 1);

  if (dname) {
    /* setup data dir */
    ab->dataDir=strdup(dname);

    /* determine config manager URL */
    GWEN_Buffer_AppendString(buf, "dir://");
    GWEN_Buffer_AppendString(buf, dname);
    GWEN_Buffer_AppendString(buf, DIRSEP);
    GWEN_Buffer_AppendString(buf, "settings");
  }
  else {
    const char *s;
    uint32_t pos;


    GWEN_Buffer_AppendString(buf, "dir://");
    pos=GWEN_Buffer_GetPos(buf);

    /* determine config directory */
    s=getenv("AQBANKING_HOME");
    if (s && !*s)
      s=0;
    if (s)
      GWEN_Buffer_AppendString(buf, s);
    else {
      /* use default */
      GWEN_Buffer_AppendString(buf, home);
      GWEN_Buffer_AppendString(buf, DIRSEP);
      GWEN_Buffer_AppendString(buf, AB_BANKING_USERDATADIR);
    }

    /* as we are at it: store default data dir */
    ab->dataDir=strdup(GWEN_Buffer_GetStart(buf)+pos);

    /* continue with settings folder */
    GWEN_Buffer_AppendString(buf, DIRSEP);
    GWEN_Buffer_AppendString(buf, "settings");

  }

  DBG_INFO(AQBANKING_LOGDOMAIN,
	   "Using data folder [%s]",
	   ab->dataDir);
  DBG_INFO(AQBANKING_LOGDOMAIN,
	   "Using ConfigManager [%s]",
	   GWEN_Buffer_GetStart(buf));

  ab->configMgr=GWEN_ConfigMgr_Factory(GWEN_Buffer_GetStart(buf));
  if (ab->configMgr==NULL) {
    DBG_ERROR(AQBANKING_LOGDOMAIN,
	      "Could not create ConfigMgr[%s]. "
	      "Maybe the gwenhywfar plugins are not installed?",
	      GWEN_Buffer_GetStart(buf));
    GWEN_Buffer_free(buf);
    return GWEN_ERROR_GENERIC;
  }

  /* done */
  GWEN_Buffer_free(buf);
  return 0;
}



int AB_Banking_HasConf4(AB_BANKING *ab) {
  int rv;
  GWEN_DB_NODE *db=NULL;
  int backends=0;
  int users=0;
  int accounts=0;
  GWEN_STRINGLIST *sl;

  /* check for config manager (created by AB_Banking_Init) */
  if (ab->configMgr==NULL) {
    DBG_ERROR(AQBANKING_LOGDOMAIN,
	      "No config manager. Maybe the gwenhywfar plugins are not installed correctly?");
    return GWEN_ERROR_GENERIC;
  }

  /* load main group, check version */
  rv=GWEN_ConfigMgr_GetGroup(ab->configMgr, AB_CFG_GROUP_MAIN, "config", &db);
  if (rv<0) {
    DBG_INFO(AQBANKING_LOGDOMAIN, "Could not load main config group (%d)", rv);
    return rv;
  }
  GWEN_DB_Group_free(db);

  sl=GWEN_StringList_new();

  /* count backends */
  rv=GWEN_ConfigMgr_ListSubGroups(ab->configMgr, AB_CFG_GROUP_BACKENDS, sl);
  if (rv<0) {
    DBG_ERROR(AQBANKING_LOGDOMAIN, "Could not list backend groups (%d)", rv);
    GWEN_StringList_free(sl);
    return rv;
  }
  backends=GWEN_StringList_Count(sl);
  GWEN_StringList_Clear(sl);

  /* count users */
  rv=GWEN_ConfigMgr_ListSubGroups(ab->configMgr, AB_CFG_GROUP_USERS, sl);
  if (rv<0) {
    DBG_ERROR(AQBANKING_LOGDOMAIN, "Could not list user groups (%d)", rv);
    GWEN_StringList_free(sl);
    return rv;
  }
  users=GWEN_StringList_Count(sl);
  GWEN_StringList_Clear(sl);

  /* count accounts */
  rv=GWEN_ConfigMgr_ListSubGroups(ab->configMgr, AB_CFG_GROUP_ACCOUNTS, sl);
  if (rv<0) {
    DBG_ERROR(AQBANKING_LOGDOMAIN, "Could not list account groups (%d)", rv);
    GWEN_StringList_free(sl);
    return rv;
  }
  accounts=GWEN_StringList_Count(sl);

  GWEN_StringList_free(sl);

  if (users)
    return 0;
  if (backends || accounts)
    return GWEN_ERROR_PARTIAL;

  return GWEN_ERROR_NO_DATA;
}



int AB_Banking_LoadAppConfig(AB_BANKING *ab, GWEN_DB_NODE **pDb) {
  assert(ab);
  assert(ab->appName);
  if (ab->appName) {
    int rv;

    rv=GWEN_ConfigMgr_GetGroup(ab->configMgr, AB_CFG_GROUP_APPS, ab->appName, pDb);
    if (rv<0) {
      DBG_ERROR(AQBANKING_LOGDOMAIN,
		"Could not load app group [%s] (%d)",
		ab->appName, rv);
      return rv;
    }
    return 0;
  }
  else {
    DBG_INFO(AQBANKING_LOGDOMAIN, "No application name");
    return GWEN_ERROR_GENERIC;
  }
}



int AB_Banking_SaveAppConfig(AB_BANKING *ab, GWEN_DB_NODE *db) {
  assert(ab);
  assert(ab->appName);
  if (ab->appName) {
    int rv;

    rv=GWEN_ConfigMgr_SetGroup(ab->configMgr, AB_CFG_GROUP_APPS, ab->appName, db);
    if (rv<0) {
      DBG_ERROR(AQBANKING_LOGDOMAIN,
		"Could not save app group [%s] (%d)",
		ab->appName, rv);
      return rv;
    }
    return 0;
  }
  else {
    DBG_INFO(AQBANKING_LOGDOMAIN, "No application name");
    return GWEN_ERROR_GENERIC;
  }
}



int AB_Banking_LockAppConfig(AB_BANKING *ab) {
  assert(ab);
  assert(ab->appName);
  if (ab->appName) {
    int rv;

    rv=GWEN_ConfigMgr_LockGroup(ab->configMgr, AB_CFG_GROUP_APPS, ab->appName);
    if (rv<0) {
      DBG_ERROR(AQBANKING_LOGDOMAIN,
		"Could not lock app group [%s] (%d)",
		ab->appName, rv);
      return rv;
    }
    return 0;
  }
  else {
    DBG_INFO(AQBANKING_LOGDOMAIN, "No application name");
    return GWEN_ERROR_GENERIC;
  }
}



int AB_Banking_UnlockAppConfig(AB_BANKING *ab) {
  assert(ab);
  assert(ab->appName);
  if (ab->appName) {
    int rv;

    rv=GWEN_ConfigMgr_UnlockGroup(ab->configMgr, AB_CFG_GROUP_APPS, ab->appName);
    if (rv<0) {
      DBG_ERROR(AQBANKING_LOGDOMAIN,
		"Could not unlock app group [%s] (%d)",
		ab->appName, rv);
      return rv;
    }
    return 0;
  }
  else {
    DBG_INFO(AQBANKING_LOGDOMAIN, "No application name");
    return GWEN_ERROR_GENERIC;
  }
}



int AB_Banking_LoadSharedConfig(AB_BANKING *ab, const char *name, GWEN_DB_NODE **pDb) {
  assert(ab);
  assert(name);
  if (name) {
    int rv;

    rv=GWEN_ConfigMgr_GetGroup(ab->configMgr, AB_CFG_GROUP_SHARED, name, pDb);
    if (rv<0) {
      DBG_ERROR(AQBANKING_LOGDOMAIN,
		"Could not load shared group [%s] (%d)",
		name, rv);
      return rv;
    }
    return 0;
  }
  else {
    DBG_INFO(AQBANKING_LOGDOMAIN, "Name of shared group missing");
    return GWEN_ERROR_GENERIC;
  }
}



int AB_Banking_SaveSharedConfig(AB_BANKING *ab, const char *name, GWEN_DB_NODE *db) {
  assert(ab);
  assert(name);
  if (name) {
    int rv;

    rv=GWEN_ConfigMgr_SetGroup(ab->configMgr, AB_CFG_GROUP_SHARED, name, db);
    if (rv<0) {
      DBG_ERROR(AQBANKING_LOGDOMAIN,
		"Could not save shared group [%s] (%d)",
		name, rv);
      return rv;
    }
    return 0;
  }
  else {
    DBG_INFO(AQBANKING_LOGDOMAIN, "Name of shared group missing");
    return GWEN_ERROR_GENERIC;
  }
}



int AB_Banking_LockSharedConfig(AB_BANKING *ab, const char *name) {
  assert(ab);
  assert(name);
  if (name) {
    int rv;

    rv=GWEN_ConfigMgr_LockGroup(ab->configMgr, AB_CFG_GROUP_SHARED, name);
    if (rv<0) {
      DBG_ERROR(AQBANKING_LOGDOMAIN,
		"Could not lock shared group [%s] (%d)",
		name, rv);
      return rv;
    }
    return 0;
  }
  else {
    DBG_INFO(AQBANKING_LOGDOMAIN, "Name of shared group missing");
    return GWEN_ERROR_GENERIC;
  }
}



int AB_Banking_UnlockSharedConfig(AB_BANKING *ab, const char *name) {
  assert(ab);
  assert(name);
  if (name) {
    int rv;

    rv=GWEN_ConfigMgr_UnlockGroup(ab->configMgr, AB_CFG_GROUP_SHARED, name);
    if (rv<0) {
      DBG_ERROR(AQBANKING_LOGDOMAIN,
		"Could not unlock shared group [%s] (%d)",
		name, rv);
      return rv;
    }
    return 0;
  }
  else {
    DBG_INFO(AQBANKING_LOGDOMAIN, "Name of shared group missing");
    return GWEN_ERROR_GENERIC;
  }
}



int AB_Banking_LoadPluginConfig(AB_BANKING *ab,
				const char *pluginName,
				const char *name,
				GWEN_DB_NODE **pDb) {
  assert(ab);
  assert(pluginName);
  assert(name);
  if (pluginName && name) {
    int rv;

    rv=GWEN_ConfigMgr_GetGroup(ab->configMgr, pluginName, name, pDb);
    if (rv<0) {
      DBG_ERROR(AQBANKING_LOGDOMAIN,
		"Could not load plugin group [%s] (%d)",
		name, rv);
      return rv;
    }
    return 0;
  }
  else {
    DBG_INFO(AQBANKING_LOGDOMAIN, "Name of plugin group missing");
    return GWEN_ERROR_GENERIC;
  }
}



int AB_Banking_SavePluginConfig(AB_BANKING *ab,
				const char *pluginName,
				const char *name,
				GWEN_DB_NODE *db) {
  assert(ab);
  assert(pluginName);
  assert(name);
  if (pluginName && name) {
    int rv;

    rv=GWEN_ConfigMgr_SetGroup(ab->configMgr, pluginName, name, db);
    if (rv<0) {
      DBG_ERROR(AQBANKING_LOGDOMAIN,
		"Could not save plugin group [%s] (%d)",
		name, rv);
      return rv;
    }
    return 0;
  }
  else {
    DBG_INFO(AQBANKING_LOGDOMAIN, "Name of plugin group missing");
    return GWEN_ERROR_GENERIC;
  }
}



int AB_Banking_LockPluginConfig(AB_BANKING *ab,
				const char *pluginName,
				const char *name) {
  assert(ab);
  assert(pluginName);
  assert(name);
  if (pluginName && name) {
    int rv;

    /* check for config manager (created by AB_Banking_Init) */
    if (ab->configMgr==NULL) {
      DBG_ERROR(AQBANKING_LOGDOMAIN,
                "No config manager. Maybe the gwenhywfar plugins are not installed correctly?");
      return GWEN_ERROR_GENERIC;
    }

    rv=GWEN_ConfigMgr_LockGroup(ab->configMgr, pluginName, name);
    if (rv<0) {
      DBG_ERROR(AQBANKING_LOGDOMAIN,
		"Could not lock plugin group [%s] (%d)",
		name, rv);
      return rv;
    }
    return 0;
  }
  else {
    DBG_INFO(AQBANKING_LOGDOMAIN, "Name of plugin group missing");
    return GWEN_ERROR_GENERIC;
  }
}



int AB_Banking_UnlockPluginConfig(AB_BANKING *ab,
				  const char *pluginName,
				  const char *name) {
  assert(ab);
  assert(pluginName);
  assert(name);
  if (pluginName && name) {
    int rv;

    rv=GWEN_ConfigMgr_UnlockGroup(ab->configMgr, pluginName, name);
    if (rv<0) {
      DBG_ERROR(AQBANKING_LOGDOMAIN,
		"Could not unlock plugin group [%s] (%d)",
		name, rv);
      return rv;
    }
    return 0;
  }
  else {
    DBG_INFO(AQBANKING_LOGDOMAIN, "Name of plugin group missing");
    return GWEN_ERROR_GENERIC;
  }
}



int AB_Banking_GetUserDataDir(const AB_BANKING *ab, GWEN_BUFFER *buf){
  if (ab->dataDir) {
    GWEN_Buffer_AppendString(buf, ab->dataDir);
    return 0;
  }
  else {
    DBG_ERROR(AQBANKING_LOGDOMAIN, "No data dir (not init?)");
    return GWEN_ERROR_GENERIC;
  }
}



int AB_Banking_GetSharedDataDir(const AB_BANKING *ab,
                                const char *name,
                                GWEN_BUFFER *buf){
  assert(ab);
  if (ab->dataDir) {
    GWEN_Buffer_AppendString(buf, ab->dataDir);
    GWEN_Buffer_AppendString(buf, DIRSEP "shared" DIRSEP);
    if (GWEN_Text_EscapeToBufferTolerant(name, buf)) {
      DBG_ERROR(AQBANKING_LOGDOMAIN,
		"Bad share name, aborting.");
      abort();
    }
    else {
      char *s;
  
      s=GWEN_Buffer_GetStart(buf);
      while(*s) {
	*s=tolower(*s);
	s++;
      }
    }
    return 0;
  }
  else {
    DBG_ERROR(AQBANKING_LOGDOMAIN, "No data dir (not init?)");
    return GWEN_ERROR_GENERIC;
  }
}



int AB_Banking_GetAppUserDataDir(const AB_BANKING *ab, GWEN_BUFFER *buf){
  int rv;

  assert(ab->appEscName);
  rv=AB_Banking_GetUserDataDir(ab, buf);
  if (rv<0) {
    DBG_INFO(AQBANKING_LOGDOMAIN, "here (%d)", rv);
    return rv;
  }
  GWEN_Buffer_AppendString(buf, DIRSEP "apps" DIRSEP);
  GWEN_Buffer_AppendString(buf, ab->appEscName);
  GWEN_Buffer_AppendString(buf, DIRSEP "data");

  return 0;
}



int AB_Banking_GetProviderUserDataDir(const AB_BANKING *ab,
				      const char *name,
				      GWEN_BUFFER *buf){
  int rv;

  rv=AB_Banking_GetUserDataDir(ab, buf);
  if (rv)
    return rv;
  GWEN_Buffer_AppendString(buf, DIRSEP "backends" DIRSEP);
  GWEN_Buffer_AppendString(buf, name);
  GWEN_Buffer_AppendString(buf, DIRSEP "data");
  return 0;
}



int AB_Banking_BeginExclUseAccount(AB_BANKING *ab,
				   AB_ACCOUNT *a) {
  GWEN_DB_NODE *db=NULL;
  GWEN_DB_NODE *dbP;
  int rv;

  assert(ab);

  /* check for config manager (created by AB_Banking_Init) */
  if (ab->configMgr==NULL) {
    DBG_ERROR(AQBANKING_LOGDOMAIN,
	      "No config manager (maybe the gwenhywfar plugins are not installed?");
    return GWEN_ERROR_GENERIC;
  }

  /* lock group */
  rv=GWEN_ConfigMgr_LockGroup(ab->configMgr,
			      AB_CFG_GROUP_ACCOUNTS,
			      AB_Account_GetDbId(a));
  if (rv<0) {
    DBG_ERROR(AQBANKING_LOGDOMAIN, "Unable to lock account config group (%d)", rv);
    return rv;
  }

  /* load group (is locked now) */
  rv=GWEN_ConfigMgr_GetGroup(ab->configMgr,
			     AB_CFG_GROUP_ACCOUNTS,
			     AB_Account_GetDbId(a),
			     &db);
  if (rv<0) {
    DBG_ERROR(AQBANKING_LOGDOMAIN, "Could not load account group (%d)", rv);
    GWEN_ConfigMgr_UnlockGroup(ab->configMgr,
			       AB_CFG_GROUP_ACCOUNTS,
			       AB_Account_GetDbId(a));
    return rv;
  }

  AB_Account_ReadDb(a, db);
  /* let provider also reload account data */
  dbP=GWEN_DB_GetGroup(db, GWEN_DB_FLAGS_DEFAULT, "data/backend");
  rv=AB_Provider_ExtendAccount(AB_Account_GetProvider(a), a, AB_ProviderExtendMode_Reload, dbP);
  if (rv<0) {
    DBG_WARN(AQBANKING_LOGDOMAIN, "Could not extend account [%s/%s] (%d)",
	     AB_Account_GetBankCode(a), AB_Account_GetAccountNumber(a), rv);
    GWEN_ConfigMgr_UnlockGroup(ab->configMgr,
			       AB_CFG_GROUP_ACCOUNTS,
			       AB_Account_GetDbId(a));
    GWEN_DB_Group_free(db);
    return rv;
  }

  GWEN_DB_Group_free(db);

  return 0;
}



int AB_Banking_EndExclUseAccount(AB_BANKING *ab,
				 AB_ACCOUNT *a,
				 int abandon) {
  int rv;

  if (!abandon) {
    GWEN_DB_NODE *db=GWEN_DB_Group_new("account");
    GWEN_DB_NODE *dbP;

    AB_Account_toDb(a, db);
    dbP=GWEN_DB_GetGroup(db, GWEN_DB_FLAGS_DEFAULT, "data/backend");
    rv=AB_Provider_ExtendAccount(AB_Account_GetProvider(a), a,
				 AB_ProviderExtendMode_Save,
				 dbP);
    if (rv) {
      DBG_INFO(AQBANKING_LOGDOMAIN, "here (%d)", rv);
      GWEN_DB_Group_free(db);
      return rv;
    }

    /* save group (still locked) */
    rv=GWEN_ConfigMgr_SetGroup(ab->configMgr,
			       AB_CFG_GROUP_ACCOUNTS,
			       AB_Account_GetDbId(a),
			       db);
    if (rv<0) {
      DBG_ERROR(AQBANKING_LOGDOMAIN, "Could not save account group (%d)", rv);
      GWEN_ConfigMgr_UnlockGroup(ab->configMgr,
				 AB_CFG_GROUP_ACCOUNTS,
				 AB_Account_GetDbId(a));
      GWEN_DB_Group_free(db);
      return rv;
    }
    GWEN_DB_Group_free(db);
  }

  /* unlock group */
  rv=GWEN_ConfigMgr_UnlockGroup(ab->configMgr,
				AB_CFG_GROUP_ACCOUNTS,
				AB_Account_GetDbId(a));
  if (rv<0) {
    DBG_ERROR(AQBANKING_LOGDOMAIN, "Could not unlock account group (%d)", rv);
    return rv;
  }

  return 0;
}



int AB_Banking_BeginExclUseUser(AB_BANKING *ab, AB_USER *u) {
  GWEN_DB_NODE *db=NULL;
  GWEN_DB_NODE *dbP;
  int rv;

  assert(ab);

  /* check for config manager (created by AB_Banking_Init) */
  if (ab->configMgr==NULL) {
    DBG_ERROR(AQBANKING_LOGDOMAIN,
	      "No config manager (maybe the gwenhywfar plugins are not installed?");
    return GWEN_ERROR_GENERIC;
  }

  /* lock group */
  rv=GWEN_ConfigMgr_LockGroup(ab->configMgr,
			      AB_CFG_GROUP_USERS,
			      AB_User_GetDbId(u));
  if (rv<0) {
    DBG_ERROR(AQBANKING_LOGDOMAIN, "Unable to lock account config group (%d)", rv);
    return rv;
  }

  /* load group (is locked now) */
  rv=GWEN_ConfigMgr_GetGroup(ab->configMgr,
			     AB_CFG_GROUP_USERS,
			     AB_User_GetDbId(u),
			     &db);
  if (rv<0) {
    DBG_ERROR(AQBANKING_LOGDOMAIN, "Could not load account group (%d)", rv);
    GWEN_ConfigMgr_UnlockGroup(ab->configMgr,
			       AB_CFG_GROUP_USERS,
			       AB_User_GetDbId(u));
    return rv;
  }

  /* reload user from DB */
  AB_User_ReadDb(u, db);
  dbP=GWEN_DB_GetGroup(db, GWEN_DB_FLAGS_DEFAULT, "data/backend");
  AB_User_toDb(u, db);
  rv=AB_Provider_ExtendUser(AB_User_GetProvider(u), u,
                            AB_ProviderExtendMode_Reload,
			    dbP);
  if (rv) {
    DBG_INFO(AQBANKING_LOGDOMAIN, "here (%d)", rv);
    GWEN_DB_Group_free(db);
    return rv;
  }

  GWEN_DB_Group_free(db);

  return 0;
}



int AB_Banking_SaveUser(AB_BANKING *ab, AB_USER *u) {
  GWEN_DB_NODE *db;
  GWEN_DB_NODE *dbP;
  int rv;

  db=GWEN_DB_Group_new("user");
  AB_User_toDb(u, db);
  dbP=GWEN_DB_GetGroup(db, GWEN_DB_FLAGS_DEFAULT,
		       "data/backend");
  rv=AB_Provider_ExtendUser(AB_User_GetProvider(u), u,
			    AB_ProviderExtendMode_Save,
			    dbP);
  if (rv) {
    DBG_INFO(AQBANKING_LOGDOMAIN, "here (%d)", rv);
    GWEN_DB_Group_free(db);
    return rv;
  }

  /* save group (still locked) */
  rv=GWEN_ConfigMgr_SetGroup(ab->configMgr,
			     AB_CFG_GROUP_USERS,
			     AB_User_GetDbId(u),
			     db);
  if (rv<0) {
    DBG_ERROR(AQBANKING_LOGDOMAIN, "Could not save user group (%d)", rv);
    GWEN_DB_Group_free(db);
    return rv;
  }
  GWEN_DB_Group_free(db);

  return 0;
}



int AB_Banking_EndExclUseUser(AB_BANKING *ab,
			      AB_USER *u,
			      int abandon) {
  int rv;

  if (!abandon) {
    /* save group (still locked) */
    rv=AB_Banking_SaveUser(ab, u);
    if (rv<0) {
      DBG_ERROR(AQBANKING_LOGDOMAIN, "Could not save user group (%d)", rv);
      GWEN_ConfigMgr_UnlockGroup(ab->configMgr,
				 AB_CFG_GROUP_USERS,
				 AB_User_GetDbId(u));
      return rv;
    }
  }

  /* unlock group */
  rv=GWEN_ConfigMgr_UnlockGroup(ab->configMgr,
				AB_CFG_GROUP_USERS,
				AB_User_GetDbId(u));
  if (rv<0) {
    DBG_ERROR(AQBANKING_LOGDOMAIN, "Could not unlock user group (%d)", rv);
    return rv;
  }

  return 0;
}



int AB_Banking_SaveAccountConfig(AB_BANKING *ab, AB_ACCOUNT *a, int doLock) {
  GWEN_DB_NODE *db;
  GWEN_DB_NODE *dbP;
  int rv;

  assert(ab);

  /* check for config manager (created by AB_Banking_Init) */
  if (ab->configMgr==NULL) {
    DBG_ERROR(AQBANKING_LOGDOMAIN,
	      "No config manager (maybe the gwenhywfar plugins are not installed?");
    return GWEN_ERROR_GENERIC;
  }

  if (doLock) {
    /* lock group */
    rv=GWEN_ConfigMgr_LockGroup(ab->configMgr,
				AB_CFG_GROUP_ACCOUNTS,
				AB_Account_GetDbId(a));
    if (rv<0) {
      DBG_ERROR(AQBANKING_LOGDOMAIN, "Unable to lock account config group (%d)", rv);
      return rv;
    }
  }

  db=GWEN_DB_Group_new("account");
  AB_Account_toDb(a, db);

  /* let backend save its data */
  dbP=GWEN_DB_GetGroup(db, GWEN_DB_FLAGS_DEFAULT, "data/backend");
  rv=AB_Provider_ExtendAccount(AB_Account_GetProvider(a), a, AB_ProviderExtendMode_Save, dbP);
  if (rv<0) {
    DBG_WARN(AQBANKING_LOGDOMAIN, "Could not extend account [%s/%s] (%d)",
	     AB_Account_GetBankCode(a), AB_Account_GetAccountNumber(a), rv);
    GWEN_DB_Group_free(db);
    if (doLock)
      GWEN_ConfigMgr_UnlockGroup(ab->configMgr,
				 AB_CFG_GROUP_ACCOUNTS,
				 AB_Account_GetDbId(a));
    return rv;
  }


  /* save group (still locked) */
  rv=GWEN_ConfigMgr_SetGroup(ab->configMgr,
			     AB_CFG_GROUP_ACCOUNTS,
			     AB_Account_GetDbId(a),
			     db);
  if (rv<0) {
    DBG_ERROR(AQBANKING_LOGDOMAIN, "Could not save account group (%d)", rv);
    if (doLock) {
      GWEN_ConfigMgr_UnlockGroup(ab->configMgr,
				 AB_CFG_GROUP_ACCOUNTS,
				 AB_Account_GetDbId(a));
    }
    GWEN_DB_Group_free(db);
    return rv;
  }
  GWEN_DB_Group_free(db);

  if (doLock) {
    /* unlock group */
    rv=GWEN_ConfigMgr_UnlockGroup(ab->configMgr,
				  AB_CFG_GROUP_ACCOUNTS,
				  AB_Account_GetDbId(a));
    if (rv<0) {
      DBG_ERROR(AQBANKING_LOGDOMAIN, "Could not unlock account group (%d)", rv);
      return rv;
    }
  }

  return 0;
}



int AB_Banking_SaveUserConfig(AB_BANKING *ab, AB_USER *u, int doLock) {
  GWEN_DB_NODE *db;
  GWEN_DB_NODE *dbP;
  int rv;

  assert(ab);

  /* check for config manager (created by AB_Banking_Init) */
  if (ab->configMgr==NULL) {
    DBG_ERROR(AQBANKING_LOGDOMAIN,
	      "No config manager (maybe the gwenhywfar plugins are not installed?");
    return GWEN_ERROR_GENERIC;
  }

  if (doLock) {
    /* lock group */
    rv=GWEN_ConfigMgr_LockGroup(ab->configMgr,
				AB_CFG_GROUP_USERS,
				AB_User_GetDbId(u));
    if (rv<0) {
      DBG_ERROR(AQBANKING_LOGDOMAIN, "Unable to lock user config group (%d)", rv);
      return rv;
    }
  }

  db=GWEN_DB_Group_new("user");
  AB_User_toDb(u, db);

  /* let the backend save its data */
  dbP=GWEN_DB_GetGroup(db, GWEN_DB_FLAGS_DEFAULT,
		       "data/backend");
  rv=AB_Provider_ExtendUser(AB_User_GetProvider(u), u,
			    AB_ProviderExtendMode_Save,
			    dbP);
  if (rv) {
    DBG_INFO(AQBANKING_LOGDOMAIN, "here (%d)", rv);
    GWEN_DB_Group_free(db);
    if (doLock) {
      GWEN_ConfigMgr_UnlockGroup(ab->configMgr,
				 AB_CFG_GROUP_USERS,
				 AB_User_GetDbId(u));
    }
    return rv;
  }

  /* save group (still locked) */
  rv=GWEN_ConfigMgr_SetGroup(ab->configMgr,
			     AB_CFG_GROUP_USERS,
			     AB_User_GetDbId(u),
			     db);
  if (rv<0) {
    DBG_ERROR(AQBANKING_LOGDOMAIN, "Could not save user group (%d)", rv);
    if (doLock) {
      GWEN_ConfigMgr_UnlockGroup(ab->configMgr,
				 AB_CFG_GROUP_USERS,
				 AB_User_GetDbId(u));
    }
    GWEN_DB_Group_free(db);
    return rv;
  }
  GWEN_DB_Group_free(db);

  if (doLock) {
    /* unlock group */
    rv=GWEN_ConfigMgr_UnlockGroup(ab->configMgr,
				  AB_CFG_GROUP_USERS,
				  AB_User_GetDbId(u));
    if (rv<0) {
      DBG_ERROR(AQBANKING_LOGDOMAIN, "Could not unlock user group (%d)", rv);
      return rv;
    }
  }

  return 0;
}






