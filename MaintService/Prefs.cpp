#include "stdafx.h"
#include "Defs.h"
#include "Prefs.h"
#include "Utils.h"
#include "Markup.h"
#include "MaintService.h"
#include "Registry.h"

extern BOOL		g_BreakSignal;
extern CUtils g_util;
extern BOOL g_connected;

CPrefs::CPrefs(void)
{

	m_title = _T("");

	m_instancenumber = 0;



	m_bypasspingtest = TRUE;
	m_bypassreconnect = FALSE;
	m_lockcheckmode = LOCKCHECKMODE_READWRITE;


	m_logtodatabase = FALSE;
	m_DBserver = _T("");
	m_Database = _T("PDFHUB");
	m_DBuser = _T("sa");
	m_DBpassword = _T("infra");



	m_scripttimeout = 60;



	m_bSortOnCreateTime = FALSE;

	m_usecurrentuserweb = TRUE;
	m_webFTPuser = _T("");
	m_webFTPpassword = _T("");



	m_emailtimeout = 60;

	m_CustomMessageDateFormat = _T("YYMMDD");
	m_CustomMessageFileName = _T("%P.xml");
	m_CustomMessageReleaseDateFormat = _T("DD.MM.YY");
	m_CustomMessageReleasePubDateFormat = _T("DD.MM.YY");
	//m_CustomMessageReleaseTimeFormat = _T("HH:MM");
}

CPrefs::~CPrefs(void)
{
}


void CPrefs::LoadIniFile(CString sIniFile)
{
	TCHAR Tmp[MAX_PATH];
	TCHAR Tmp2[MAX_PATH];


	
	::GetPrivateProfileString("System", "AlwaysUseCurrentUser", "1", Tmp, 255, sIniFile);
	m_alwaysusecurrentuser = _tstoi(Tmp);	

	::GetPrivateProfileString("System", "CcdataFilenameMode", "1", Tmp, 255, sIniFile);
	m_ccdatafilenamemode = _tstoi(Tmp);

	::GetPrivateProfileString("System", "ScriptTimeOut", "60", Tmp, _MAX_PATH, sIniFile);
	m_scripttimeout = _tstoi(Tmp);

	GetPrivateProfileString("Setup", "LockCheckMode", "2", Tmp, 255, sIniFile);
	m_lockcheckmode = _tstoi(Tmp);

	GetPrivateProfileString("Setup", "IgnoreLockCheck", "0", Tmp, 255, sIniFile);
	if (_tstoi(Tmp) > 0)
		m_lockcheckmode = 0;

	::GetPrivateProfileString("System", "MinMBfreeCCDATA", "0", Tmp, _MAX_PATH, sIniFile);
	m_minMBfreeCCDATA = _tstoi(Tmp);




	GetPrivateProfileString("Setup", "BypassPingTest", "1", Tmp, 255, sIniFile);
	m_bypasspingtest = _tstoi(Tmp);

	GetPrivateProfileString("Setup", "BypassReconnect", "0", Tmp, 255, sIniFile);
	m_bypassreconnect = _tstoi(Tmp);

	GetPrivateProfileString("Setup", "BypassDiskFreeTest", "0", Tmp, 255, sIniFile);
	m_bypassdiskfreetest = _tstoi(Tmp);




	::GetPrivateProfileString("Setup", "MailOnFileError", "0", Tmp, _MAX_PATH, sIniFile);
	m_emailonfileerror = _tstoi(Tmp);

	::GetPrivateProfileString("Setup", "MailOnFolderError", "0", Tmp, _MAX_PATH, sIniFile);
	m_emailonfoldererror = _tstoi(Tmp);

	::GetPrivateProfileString("Setup", "MailSmtpServer", "", Tmp, _MAX_PATH, sIniFile);
	m_emailsmtpserver = Tmp;
	::GetPrivateProfileString("Setup", "MailSmtpPort", "25", Tmp, _MAX_PATH, sIniFile);
	m_mailsmtpport = _tstoi(Tmp);

	::GetPrivateProfileString("Setup", "MailSmtpUsername", "", Tmp, _MAX_PATH, sIniFile);
	m_mailsmtpserverusername = Tmp;

	::GetPrivateProfileString("Setup", "MailSmtpPassword", "", Tmp, _MAX_PATH, sIniFile);
	m_mailsmtpserverpassword = Tmp;

	::GetPrivateProfileString("Setup", "MailUseSSL", "0", Tmp, _MAX_PATH, sIniFile);
	m_mailusessl = _tstoi(Tmp);

	::GetPrivateProfileString("Setup", "MailFrom", "", Tmp, _MAX_PATH, sIniFile);
	m_emailfrom = Tmp;
	::GetPrivateProfileString("Setup", "MailTo", "", Tmp, _MAX_PATH, sIniFile);
	m_emailto = Tmp;
	::GetPrivateProfileString("Setup", "MailCc", "", Tmp, _MAX_PATH, sIniFile);
	m_emailcc = Tmp;
	::GetPrivateProfileString("Setup", "MailSubject", "", Tmp, _MAX_PATH, sIniFile);
	m_emailsubject = Tmp;

	::GetPrivateProfileString("Setup", "MailPreventFlooding", "0", Tmp, _MAX_PATH, sIniFile);
	m_emailpreventflooding = _tstoi(Tmp);

	::GetPrivateProfileString("Setup", "MainPreventFloodingDelay", "10", Tmp, _MAX_PATH, sIniFile);
	m_emailpreventfloodingdelay = _tstoi(Tmp);

	::GetPrivateProfileString("Setup", "MaxLogFileSize", "10485760", Tmp, _MAX_PATH, sIniFile);
	m_maxlogfilesize = _tstoi(Tmp);




	sprintf(Tmp, "%s\\alivelogs", (LPCSTR)g_util.GetFilePath(m_logFilePath));
	::GetPrivateProfileString("System", "AliveLogFolder", Tmp, m_alivelogFilePath.GetBuffer(_MAX_PATH), _MAX_PATH, sIniFile);
	m_alivelogFilePath.ReleaseBuffer();
	::CreateDirectory(m_alivelogFilePath, NULL);

	::GetPrivateProfileString("System", "EnableAutoDelete", "1", Tmp, _MAX_PATH, sIniFile);
	m_enableautodelete = atoi(Tmp);

	::GetPrivateProfileString("System", "AutoDeleteHoursBegin", "5", Tmp, _MAX_PATH, sIniFile);
	m_autopurgehoursbegin = atoi(Tmp);
	
	::GetPrivateProfileString("System", "AutoDeleteHoursEnd", "6", Tmp, _MAX_PATH, sIniFile);
	m_autopurgehoursend = atoi(Tmp);

	::GetPrivateProfileString("System", "OverruleKeepDays", "7", Tmp, _MAX_PATH, sIniFile);
	m_overrulekeepdays = atoi(Tmp);

	::GetPrivateProfileString("System", "KeepDaysLog", "7", Tmp, _MAX_PATH, sIniFile);
	m_keepdayslog = atoi(Tmp);

	::GetPrivateProfileString("System", "AutoRetry", "1", Tmp, _MAX_PATH, sIniFile);
	m_autoretry = atoi(Tmp);

	::GetPrivateProfileString("System", "EnableDeleteUnrelatedFiles", "0", Tmp, _MAX_PATH, sIniFile);
	m_enabledeleteunrelatedfiles = atoi(Tmp);

	::GetPrivateProfileString("System", "EnableDeleteUnrelatedFilesWeekly", "0", Tmp, _MAX_PATH, sIniFile);
	m_EnableDeleteUnrelatedFilesWeekly = atoi(Tmp);


	::GetPrivateProfileString("System", "EnableDeleteUnrelatedFilesWeeklyWeekday", "0", Tmp, _MAX_PATH, sIniFile);
	m_weekday = atoi(Tmp);

	::GetPrivateProfileString("System", "AutoPurgeUnrelatedHoursBegin", "5", Tmp, _MAX_PATH, sIniFile);
	m_autopurgeunrelatedhoursbegin = atoi(Tmp);
	
	::GetPrivateProfileString("System", "AutoPurgeUnrelatedHoursEnd", "6", Tmp, _MAX_PATH, sIniFile);
	m_autopurgeunrelatedhoursend = atoi(Tmp);


	::GetPrivateProfileString("System", "CleanDirty", "1", Tmp, _MAX_PATH, sIniFile);
	m_cleandirty = atoi(Tmp);

	::GetPrivateProfileString("System", "CleanDirtyHoursBegin", "6", Tmp, _MAX_PATH, sIniFile);
	m_cleandirtyhoursbegin = atoi(Tmp);

	::GetPrivateProfileString("System", "CleanDirtyHoursEnd", "7", Tmp, _MAX_PATH, sIniFile);
	m_cleandirtyhoursend = atoi(Tmp);

	::GetPrivateProfileString("System", "MaxAgeUnknownFilesHours", "72", Tmp, _MAX_PATH, sIniFile);
	m_maxageunknownfileshours = atoi(Tmp);

	::GetPrivateProfileString("System", "PlanLock", "1", Tmp, _MAX_PATH, sIniFile);
	m_planlock = atoi(Tmp);

	::GetPrivateProfileString("System", "EnableCustomScript", "1", Tmp, _MAX_PATH, sIniFile);
	m_enablecustomscript = atoi(Tmp);

	::GetPrivateProfileString("System", "CustomScriptHoursBegin", "3", Tmp, _MAX_PATH, sIniFile);
	m_customscripthourbegin = atoi(Tmp);

	::GetPrivateProfileString("System", "CustomScriptHoursEnd", "4", Tmp, _MAX_PATH, sIniFile);
	m_customscripthourend = atoi(Tmp);

	::GetPrivateProfileString("System", "EnableContinousCustomScript", "1", Tmp, _MAX_PATH, sIniFile);
	m_enablecontinouscustomscript = atoi(Tmp);
	
	::GetPrivateProfileString("System", "ContinousCustomScript", "", Tmp, _MAX_PATH, sIniFile);
	m_continouscustomscript = Tmp;
	
	::GetPrivateProfileString("System", "ExportPlans", "1", Tmp, _MAX_PATH, sIniFile);
	m_doexportplans = atoi(Tmp);	

	::GetPrivateProfileString("System", "PlanExportFolder", "", Tmp, _MAX_PATH, sIniFile);
	m_PlanExportFolder = Tmp;

	::GetPrivateProfileString("System", "VerifyCRC", "1", Tmp, _MAX_PATH, sIniFile);
	m_verifyCRC = atoi(Tmp);
	

	::GetPrivateProfileString("System", "NumberOfWatchedFolder", "0", Tmp, _MAX_PATH, sIniFile);
	int nFolders = atoi(Tmp);

	for (int i = 0; i < nFolders; i++) {

		sprintf(Tmp2, "Folder%d", i + 1);
		::GetPrivateProfileString("System", Tmp2, "", Tmp, _MAX_PATH, sIniFile);
		CString sFolder(Tmp);

		sprintf(Tmp2, "Mask%d", i + 1);
		::GetPrivateProfileString("System", Tmp2, "*.*", Tmp, _MAX_PATH, sIniFile);
		CString sMask(Tmp);

		sprintf(Tmp2, "Days%d", i + 1);
		::GetPrivateProfileString("System", Tmp2, "0", Tmp, _MAX_PATH, sIniFile);
		int nDays = atoi(Tmp);

		sprintf(Tmp2, "Hours%d", i + 1);
		::GetPrivateProfileString("System", Tmp2, "0", Tmp, _MAX_PATH, sIniFile);
		int nHours = atoi(Tmp);

		sprintf(Tmp2, "Minutes%d", i + 1);
		::GetPrivateProfileString("System", Tmp2, "0", Tmp, _MAX_PATH, sIniFile);
		int nMinutes = atoi(Tmp);

		sprintf(Tmp2, "DeleteEmptyFolder%d", i + 1);
		::GetPrivateProfileString("System", Tmp2, "0", Tmp, _MAX_PATH, sIniFile);
		BOOL bDeleteEmpty = atoi(Tmp);

		sprintf(Tmp2, "SearchSubfolders%d", i + 1);
		::GetPrivateProfileString("System", Tmp2, "0", Tmp, _MAX_PATH, sIniFile);
		BOOL bsearchsubfolders = atoi(Tmp);

		if (sFolder != "" && sMask != "" && (nDays > 0 || nHours > 0 || nMinutes > 0)) {
			OLDFOLDERSTRUCT *pItem = new OLDFOLDERSTRUCT();
			pItem->m_folder = sFolder;
			pItem->m_searchmask = sMask;
			pItem->m_days = nDays;
			pItem->m_hours = nHours;
			pItem->m_minutes = nMinutes;
			pItem->m_deleteEmptyFolders = bDeleteEmpty;
			pItem->m_searchsubfolders = bsearchsubfolders;
			m_OldFolderList.Add(*pItem);
		}
	}



	::GetPrivateProfileString("System", "NumberOfNotificationFolders", "0", Tmp, _MAX_PATH, sIniFile);
	nFolders = atoi(Tmp);

	m_FolderNotificationList.RemoveAll();
	for (int i = 0; i < nFolders; i++) {

		sprintf(Tmp2, "NotificationFolder%d", i + 1);
		::GetPrivateProfileString("System", Tmp2, "", Tmp, _MAX_PATH, sIniFile);
		CString sFolder(Tmp);

		sprintf(Tmp2, "NotificationMask%d", i + 1);
		::GetPrivateProfileString("System", Tmp2, "*.*", Tmp, _MAX_PATH, sIniFile);
		CString sMask(Tmp);


		sprintf(Tmp2, "NotificationHours%d", i + 1);
		::GetPrivateProfileString("System", Tmp2, "0", Tmp, _MAX_PATH, sIniFile);
		int nHours = atoi(Tmp);

		sprintf(Tmp2, "NotificationMinutes%d", i + 1);
		::GetPrivateProfileString("System", Tmp2, "0", Tmp, _MAX_PATH, sIniFile);
		int nMinutes = atoi(Tmp);


		sprintf(Tmp2, "NotificationReceiver%d", i + 1);
		::GetPrivateProfileString("System", Tmp2, "", Tmp, _MAX_PATH, sIniFile);
		CString sReceiver(Tmp);


		sprintf(Tmp2, "NotificationSubject%d", i + 1);
		::GetPrivateProfileString("System", Tmp2, "Notification: File(s) stuck in folder", Tmp, _MAX_PATH, sIniFile);
		CString sSubject(Tmp);

		sprintf(Tmp2, "NotificationMoveOnError%d", i + 1);
		::GetPrivateProfileString("System", Tmp2, "0", Tmp, _MAX_PATH, sIniFile);
		BOOL bMoveOnError = atoi(Tmp);

		sprintf(Tmp2, "NotificationErrorFolder%d", i + 1);
		::GetPrivateProfileString("System", Tmp2, "", Tmp, _MAX_PATH, sIniFile);
		CString sErrorFolder(Tmp);

		sprintf(Tmp2, "NotificationErrorFolderUseWriteTime%d", i + 1);
		::GetPrivateProfileString("System", Tmp2, "1", Tmp, _MAX_PATH, sIniFile);
		BOOL bUseWriteTime = atoi(Tmp);

		sprintf(Tmp2, "NotificationErrorFolderMaxFiles%d", i + 1);
		::GetPrivateProfileString("System", Tmp2, "1000000", Tmp, _MAX_PATH, sIniFile);
		int nMaxFiles = atoi(Tmp);


		if (sFolder != ""  && sReceiver != "") {
			FOLDERNOTIFICATIONSTRUCT *pItem = new FOLDERNOTIFICATIONSTRUCT();
			pItem->m_folder = sFolder;
			pItem->m_searchmask = sMask;
			pItem->m_hours = nHours;
			pItem->m_minutes = nMinutes;
			pItem->m_sMailReceiver = sReceiver;
			pItem->m_sMailSubject = sSubject;
			pItem->m_moveOnError = bMoveOnError;
			pItem->m_errorfolder = sErrorFolder;
			pItem->m_lastNotificationEvent = CTime(1975, 1, 1, 0, 0, 0);
			pItem->m_usewritetime = bUseWriteTime;
			pItem->m_maxfiles = nMaxFiles;
			m_FolderNotificationList.Add(*pItem);
		}
	}


	::GetPrivateProfileString("System", "CustomMessageFileName", "%P.xml", Tmp, _MAX_PATH, sIniFile);
	m_CustomMessageFileName = Tmp;

	::GetPrivateProfileString("System", "CustomMessageReleaseDateFormat", "DD.MM.YY", Tmp, _MAX_PATH, sIniFile);
	m_CustomMessageReleaseDateFormat = Tmp;

	::GetPrivateProfileString("System", "CustomMessageReleasePubDateFormat", "DD.MM.YY", Tmp, _MAX_PATH, sIniFile);
	m_CustomMessageReleasePubDateFormat = Tmp;


}

BOOL CPrefs::ConnectToDB(BOOL bLoadDBNames, CString &sErrorMessage)
{

	g_connected = m_DBmaint.InitDB(m_DBserver, m_Database, m_DBuser, m_DBpassword, m_IntegratedSecurity, sErrorMessage);

	if (g_connected) {
		if (m_DBmaint.UpdateService(1,"","",sErrorMessage) == FALSE) {
			g_util.Logprintf("ERROR: RegisterService() returned - %s", sErrorMessage);
		}
	}
	else
		g_util.Logprintf("ERROR: InitDB() returned - %s", sErrorMessage);

	if (bLoadDBNames) {
		if (m_DBmaint.LoadAllPrefs(sErrorMessage) == FALSE)
			g_util.Logprintf("ERROR: LoadAllPrefs() returned - %s", sErrorMessage);

	}

	return g_connected;
}


CString CPrefs::GetPublicationName(int nID)
{
	return GetPublicationName(m_DBmaint, nID);
}

CString CPrefs::GetPublicationName(CDatabaseManager &DB, int nID)
{
	CString sErrorMessage;

	if (nID == 0)
		return _T("");

	for (int i = 0; i < m_PublicationList.GetCount(); i++) {
		if (m_PublicationList[i].m_ID == nID)
			return m_PublicationList[i].m_name;
	}

	CString sPubName = DB.LoadNewPublicationName(nID, sErrorMessage);

	if (sPubName != "")
		DB.LoadSpecificAlias("Publication", sPubName, sErrorMessage);

	return sPubName;
}

CString CPrefs::GetPublicationNameNoReload(int nID)
{
	CString sErrorMessage;

	if (nID == 0)
		return _T("");

	for (int i = 0; i < m_PublicationList.GetCount(); i++) {
		if (m_PublicationList[i].m_ID == nID)
			return m_PublicationList[i].m_name;
	}


	return _T("");
}


void CPrefs::FlushPublicationName(CString sName)
{
	for (int i = 0; i < m_PublicationList.GetCount(); i++) {
		if (sName.CompareNoCase(m_PublicationList[i].m_name) == 0) {
			m_PublicationList.RemoveAt(i);
			return;
		}
	}
}

PUBLICATIONSTRUCT *CPrefs::GetPublicationStruct(int nID)
{
	return GetPublicationStruct(m_DBmaint, nID);
}

PUBLICATIONSTRUCT *CPrefs::GetPublicationStructNoDbLookup(int nID)
{
	for (int i = 0; i < m_PublicationList.GetCount(); i++) {
		if (m_PublicationList[i].m_ID == nID)
			return &m_PublicationList[i];
	}

	return NULL;
}

PUBLICATIONSTRUCT *CPrefs::GetPublicationStruct(CDatabaseManager &DB, int nID)
{
	if (nID == 0)
		return NULL;
	for (int i = 0; i < m_PublicationList.GetCount(); i++) {
		if (m_PublicationList[i].m_ID == nID)
			return &m_PublicationList[i];
	}

	CString sErrorMessage;

	CString sPubName = DB.LoadNewPublicationName(nID, sErrorMessage);

	if (sPubName != "")
		DB.LoadSpecificAlias("Publication", sPubName, sErrorMessage);

	for (int i = 0; i < m_PublicationList.GetCount(); i++) {
		if (m_PublicationList[i].m_ID == nID)
			return &m_PublicationList[i];
	}

	return NULL;
}

CString CPrefs::GetExtendedAlias(int nPublicationID)
{
	for (int i = 0; i < m_PublicationList.GetCount(); i++) {
		if (m_PublicationList[i].m_ID == nPublicationID)
			return m_PublicationList[i].m_extendedalias;
	}

	return GetPublicationName(nPublicationID);
}


CString CPrefs::GetEditionName(int nID)
{
	return GetEditionName(m_DBmaint, nID);
}

CString CPrefs::GetEditionName(CDatabaseManager &DB, int nID)
{
	CString sErrorMessage;

	if (nID == 0)
		return _T("");

	for (int i = 0; i < m_EditionList.GetCount(); i++) {
		if (m_EditionList[i].m_ID == nID)
			return m_EditionList[i].m_name;
	}

	CString sEdName = DB.LoadNewEditionName(nID, sErrorMessage);

	if (sEdName != "")
		DB.LoadSpecificAlias("Edition", sEdName, sErrorMessage);

	return sEdName;
}

int CPrefs::GetEditionID(CString s)
{
	return GetEditionID(m_DBmaint, s);
}

int CPrefs::GetEditionID(CDatabaseManager &DB, CString s)
{
	CString sErrorMessage;

	if (s.Trim() == "")
		return 0;

	for (int i = 0; i < m_EditionList.GetCount(); i++) {
		if (s.CompareNoCase(m_EditionList[i].m_name) == 0)
			return m_EditionList[i].m_ID;
	}
	int nID = DB.LoadNewEditionID(s, sErrorMessage);

	if (nID > 0)
		DB.LoadSpecificAlias("Edition", s, sErrorMessage);

	return nID;
}

CString CPrefs::GetSectionName(int nID)
{
	return GetSectionName(m_DBmaint, nID);
}

CString CPrefs::GetSectionName(CDatabaseManager &DB, int nID)
{
	CString sErrorMessage;

	if (nID == 0)
		return _T("");

	for (int i = 0; i < m_SectionList.GetCount(); i++) {
		if (m_SectionList[i].m_ID == nID)
			return m_SectionList[i].m_name;
	}

	CString sSecName = DB.LoadNewSectionName(nID, sErrorMessage);

	if (sSecName != "")
		DB.LoadSpecificAlias("Section", sSecName, sErrorMessage);

	return sSecName;
}

int CPrefs::GetSectionID(CString sName)
{
	return GetSectionID(m_DBmaint, sName);
}

int CPrefs::GetSectionID(CDatabaseManager &DB, CString s)
{
	CString sErrorMessage;

	if (s.Trim() == "")
		return 0;

	for (int i = 0; i < m_SectionList.GetCount(); i++) {
		if (s.CompareNoCase(m_SectionList[i].m_name) == 0)
			return m_SectionList[i].m_ID;
	}

	int nID = DB.LoadNewSectionID(s, sErrorMessage);

	if (nID > 0)
		DB.LoadSpecificAlias("Section", s, sErrorMessage);

	return nID;
}

CString CPrefs::GetPublisherName(int nID)
{
	return GetPublisherName(m_DBmaint, nID);
}

CString CPrefs::GetPublisherName(CDatabaseManager &DB, int nID)
{
	CString sErrorMessage;

	if (nID == 0)
		return _T("");

	for (int i = 0; i < m_PublisherList.GetCount(); i++) {
		if (m_PublisherList[i].m_ID == nID)
			return m_PublisherList[i].m_name;
	}

	CString sPublisherName = DB.LoadNewPublisherName(nID, sErrorMessage);

	return sPublisherName;
}



CString CPrefs::GetChannelName(int nID)
{
	for (int i = 0; i < m_ChannelList.GetCount(); i++) {
		if (m_ChannelList[i].m_channelID == nID) {
			return m_ChannelList[i].m_channelname;
		}
	}

	CHANNELSTRUCT *p = GetChannelStruct(nID);
	if (p != NULL)
		return p->m_channelname;

	return _T("");
}

CHANNELSTRUCT	*CPrefs::GetChannelStruct(int nID)
{
	return GetChannelStruct(m_DBmaint, nID);
}

CHANNELSTRUCT	*CPrefs::GetChannelStruct(CDatabaseManager &DB, int nID)
{
	CString sErrorMessage = _T("");
	int nIndex = 0;
	for (int i = 0; i < m_ChannelList.GetCount(); i++) {
		if (m_ChannelList[i].m_channelID == nID) {
			return &m_ChannelList[i];
		}
	}


	DB.LoadNewChannel(nID, sErrorMessage);

	for (int i = 0; i < m_ChannelList.GetCount(); i++) {
		if (m_ChannelList[i].m_channelID == nID)
			return &m_ChannelList[i];
	}

	return NULL;
}

CString CPrefs::LookupAbbreviationFromName(CString sType, CString sLongName)
{


	for (int i = 0; i < m_AliasList.GetCount(); i++) {
		ALIASSTRUCT a = m_AliasList[i];
		if (a.sType.CompareNoCase(sType) == 0 && a.sLongName.CompareNoCase(sLongName) == 0)
			return a.sShortName;
	}

	return sLongName;
}

CString CPrefs::LookupNameFromAbbreviation(CString sType, CString sShortName)
{
	for (int i = 0; i < m_AliasList.GetCount(); i++) {
		ALIASSTRUCT a = m_AliasList[i];
		if (a.sType.CompareNoCase(sType) == 0 && a.sShortName.CompareNoCase(sShortName) == 0)
			return a.sLongName;
	}

	return sShortName;
}

int CPrefs::GetPageFormatIDFromPublication(int nPublicationID)
{
	return GetPageFormatIDFromPublication(m_DBmaint, nPublicationID, false);
}

int CPrefs::GetPageFormatIDFromPublication(CDatabaseManager &DB, int nPublicationID, BOOL bForceReload)
{
	CString sErrorMessage;

	if (nPublicationID == 0)
		return 0;

	if (bForceReload == FALSE) {

		for (int i = 0; i < m_PublicationList.GetCount(); i++) {
			if (nPublicationID == m_PublicationList[i].m_ID) {
				return m_PublicationList[i].m_PageFormatID;
			}
		}
	}

	

	return 0;
}





BOOL CPrefs::LoadPreferencesFromRegistry()
{
	CRegistry pReg;

	// Set defaults
	m_logFilePath = _T("c:\\Temp");
	m_DBserver = _T(".");
	m_Database = _T("PDFHUB");
	m_DBuser = _T("sa");
	m_DBpassword = _T("Infra2Logic");
	m_IntegratedSecurity = FALSE;

	m_databaselogintimeout = 20;
	m_databasequerytimeout = 10;
	m_nQueryRetries = 20;
	m_QueryBackoffTime = 500;
	m_instancenumber = 1;

	if (pReg.OpenKey(CRegistry::localMachine, "Software\\InfraLogic\\MaintService\\Parameters")) {
		CString sVal = _T("");
		DWORD nValue;

		if (pReg.GetValue("InstanceNumber", nValue))
			m_instancenumber = nValue;

		if (pReg.GetValue("IntegratedSecurity", nValue))
			m_IntegratedSecurity = nValue;

		if (pReg.GetValue("DBServer", sVal))
			m_DBserver = sVal;

		if (pReg.GetValue("Database", sVal))
			m_Database = sVal;

		if (pReg.GetValue("DBUser", sVal))
			m_DBuser = sVal;

		if (pReg.GetValue("DBpassword", sVal))
			m_DBpassword = sVal;

		if (pReg.GetValue("DBLoginTimeout", nValue))
			m_databaselogintimeout = nValue;

		if (pReg.GetValue("DBQueryTimeout", nValue))
			m_databasequerytimeout = nValue;

		if (pReg.GetValue("DBQueryRetries", nValue))
			m_nQueryRetries = nValue > 0 ? nValue : 5;

		if (pReg.GetValue("DBQueryBackoffTime", nValue))
			m_QueryBackoffTime = nValue >= 500 ? nValue : 500;

		if (pReg.GetValue("Logging", nValue))
			m_logging = nValue;

		if (pReg.GetValue("LogFileFolder", sVal))
			m_logFilePath = sVal;

		pReg.CloseKey();

		return TRUE;
	}

	return FALSE;
}

CString CPrefs::FormCCFilesName(int nPDFtype, int nMasterCopySeparationSet, CString sFileName)
{
	sFileName = g_util.GetFileName(sFileName, TRUE); // Shave off extension

	CString sPath = _T("");
	if (nPDFtype == POLLINPUTTYPE_LOWRESPDF) {
		if (m_ccdatafilenamemode && sFileName != _T(""))
			sPath.Format("%s\\%s#%d.PDF", m_lowresPDFPath, sFileName, nMasterCopySeparationSet);
		else
			sPath.Format("%s%d.PDF", m_lowresPDFPath, nMasterCopySeparationSet);
	}
	else if (nPDFtype == POLLINPUTTYPE_HIRESPDF)
	{
		if (m_ccdatafilenamemode && sFileName != _T(""))
			sPath.Format("%s\\%s#%d.PDF", m_hiresPDFPath, sFileName, nMasterCopySeparationSet);
		else
			sPath.Format("%s%d.PDF", m_hiresPDFPath, nMasterCopySeparationSet);
	}
	else
	{
		if (m_ccdatafilenamemode && sFileName != _T(""))
			sPath.Format("%s\\%s#%d.PDF", m_printPDFPath, sFileName, nMasterCopySeparationSet);
		else
			sPath.Format("%s%d.PDF", m_printPDFPath, nMasterCopySeparationSet);
	}

	return sPath;
}


CString CPrefs::FormCcdataFileNameLowres(CString sCCFileName, int nMasterCopySeparationSet)
{
	CString sDestPath;
	if (m_ccdatafilenamemode && sCCFileName != _T(""))
		sDestPath.Format("%s\\%s_%d.PDF", m_lowresPDFPath, sCCFileName, nMasterCopySeparationSet);
	else
		sDestPath.Format("%s\\%d.PDF", m_lowresPDFPath, nMasterCopySeparationSet);

	return sDestPath;
}

CString CPrefs::FormCcdataFileNameHires(CString sCCFileName, int nMasterCopySeparationSet)
{
	CString sDestPath;
	if (m_ccdatafilenamemode && sCCFileName != _T(""))
		sDestPath.Format("%s\\%s_%d.PDF", m_hiresPDFPath, sCCFileName, nMasterCopySeparationSet);
	else
		sDestPath.Format("%s\\%d.PDF", m_hiresPDFPath, nMasterCopySeparationSet);

	return sDestPath;
}

CString CPrefs::FormCcdataFileNamePrint(CString sCCFileName, int nMasterCopySeparationSet)
{
	CString sDestPath;
	if (m_ccdatafilenamemode && sCCFileName != _T(""))
		sDestPath.Format("%s\\%s_%d.PDF", m_printPDFPath, sCCFileName, nMasterCopySeparationSet);
	else
		sDestPath.Format("%s\\%d.PDF", m_printPDFPath, nMasterCopySeparationSet);

	return sDestPath;
}
