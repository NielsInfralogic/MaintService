#include "StdAfx.h"
#include "Defs.h"
#include "DatabaseManager.h"
#include "Utils.h"
#include "Prefs.h"
#include "PlanExport.h"

extern CPrefs g_prefs;
extern CUtils g_util;
extern bool g_pollthreadrunning;

CDatabaseManager::CDatabaseManager(void)
{
	m_DBopen = FALSE;
	m_pDB = NULL;

	m_DBserver = _T(".");
	m_Database = _T("ControlCenter");
	m_DBuser = "saxxxxxxxxx";
	m_DBpassword = "xxxxxx";
	m_IntegratedSecurity = FALSE;
	m_PersistentConnection = FALSE;

}

CDatabaseManager::~CDatabaseManager(void)
{
	ExitDB();
	if (m_pDB != NULL)
		delete m_pDB;
}

BOOL CDatabaseManager::InitDB(CString sDBserver, CString sDatabase, CString sDBuser, CString sDBpassword, BOOL bIntegratedSecurity, CString &sErrorMessage)
{
	m_DBserver = sDBserver;
	m_Database = sDatabase;
	m_DBuser = sDBuser;
	m_DBpassword = sDBpassword;
	m_IntegratedSecurity = bIntegratedSecurity;

	return InitDB(sErrorMessage);
}


int CDatabaseManager::InitDB(CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (m_pDB) {
		if (m_pDB->IsOpen() == FALSE) {
			try {
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
		}
	}

	if (!m_PersistentConnection)
		ExitDB();

	if (m_DBopen)
		return TRUE;

	if (m_DBserver == _T("") || m_Database == _T("") || m_DBuser == _T("")) {
		sErrorMessage = _T("Empty server, database or username not allowed");
		return FALSE;
	}

	if (m_pDB == NULL)
		m_pDB = new CDatabase;

	m_pDB->SetLoginTimeout(g_prefs.m_databaselogintimeout);
	m_pDB->SetQueryTimeout(g_prefs.m_databasequerytimeout);

	CString sConnectStr = _T("Driver={SQL Server}; Server=") + m_DBserver + _T("; ") +
		_T("Database=") + m_Database + _T("; ");

	if (m_IntegratedSecurity)
		sConnectStr += _T(" Integrated Security=True;");
	else
		sConnectStr += _T("USER=") + m_DBuser + _T("; PASSWORD=") + m_DBpassword + _T(";");

	try {
		if (!m_pDB->OpenEx((LPCTSTR)sConnectStr, CDatabase::noOdbcDialog)) {
			sErrorMessage.Format(_T("Error connecting to database with connection string '%s'"), (LPCSTR)sConnectStr);
			return FALSE;
		}
	}
	catch (CDBException* e) {
		sErrorMessage.Format(_T("Error connecting to database - %s (%s)"), (LPCSTR)e->m_strError, (LPCSTR)sConnectStr);
		e->Delete();
		return FALSE;
	}

	m_DBopen = TRUE;
	return TRUE;
}

void CDatabaseManager::ExitDB()
{
	if (!m_DBopen)
		return;

	if (m_pDB)
		m_pDB->Close();

	m_DBopen = FALSE;

	return;
}

BOOL CDatabaseManager::IsOpen()
{
	return m_DBopen;
}

//
// SERVICE RELATED METHODS
//

BOOL CDatabaseManager::RunCheckDB(CString &sResult, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	sResult = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return -1;

	CRecordset Rs(m_pDB);
	CString sSQL, s;
	sSQL.Format("{CALL usp_check_dbcccheckdbresults }");
	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format(_T("Query failed - %s"), (LPCSTR)sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {

			sResult = "CHECKDB result: ";
			CDBVariant DBvar;

			Rs.GetFieldValue((short)0, DBvar, SQL_C_TIMESTAMP);
			CTime t(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
			sResult += " " + g_util.Time2String(t);
			Rs.GetFieldValue((short)1, s);
			sResult += " " + s;
			Rs.GetFieldValue((short)2, s);
			sResult += " Errors found: " + s;

			Rs.GetFieldValue((short)3, s);
			sResult += " Repairs done: " + s;

		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format(_T("ERROR (DATABASEMGR): Query failed - %s"), (LPCSTR)e->m_strError);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	return TRUE;
	}
BOOL CDatabaseManager::LoadMaintParamters(int nInstanceNumber, int &nDaysToKeep, int &nDaysToKeepLogData, 
	CString &sVisioLinkTemplateFile, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	nDaysToKeep = 0;
	nDaysToKeepLogData = 0;
	sVisioLinkTemplateFile = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return -1;

	CRecordset Rs(m_pDB);
	CString sSQL, s;
	sSQL.Format("SELECT DaysToKeepProducts,DaysToKeepLogdata,VisioLinkTemplateFile FROM MaintConfigurations WHERE InstanceNumber=%d", nInstanceNumber);
	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format(_T("Query failed - %s"), (LPCSTR)sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {
			Rs.GetFieldValue((short)0, s);
			nDaysToKeep = atoi(s);
			Rs.GetFieldValue((short)1, s);
			nDaysToKeepLogData = atoi(s);

			Rs.GetFieldValue((short)2, sVisioLinkTemplateFile);



		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format(_T("ERROR (DATABASEMGR): Query failed - %s"), (LPCSTR)e->m_strError);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	return TRUE;
}

BOOL CDatabaseManager::LoadConfigIniFile(int nInstanceNumber, CString &sFileName, CString &sFileName2, CString &sFileName3, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return -1;

	CRecordset Rs(m_pDB);
	CString sSQL, s;
	sSQL.Format("SELECT ConfigData,ConfigData2,ConfigData3 FROM ServiceConfigurations WHERE ServiceName='MaintService'", nInstanceNumber);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format(_T("Query failed - %s"), (LPCSTR)sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {
			Rs.GetFieldValue((short)0, s);
			sFileName = s;
			Rs.GetFieldValue((short)1, s);
			sFileName2  = s;
			
			Rs.GetFieldValue((short)2, s);
			sFileName3 = s;
			
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format(_T("ERROR (DATABASEMGR): Query failed - %s"), (LPCSTR)e->m_strError);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	return TRUE;

}


BOOL CDatabaseManager::RegisterService(CString &sErrorMessage)
{
	CString sSQL, s;
	sErrorMessage = _T("");

	if (InitDB(sErrorMessage) == FALSE)
		return -1;
	
	CRecordset Rs(m_pDB);

	CString sComputer = g_util.GetComputerName();

	sSQL.Format("{CALL spRegisterService ('MaintService', 1, 7, '%s',-1,'','','')}", sComputer);

	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Insert failed - %s", (LPCSTR)e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	return TRUE;
}

BOOL CDatabaseManager::InsertLogEntry(int nEvent, CString sSource,  CString sFileName, CString sMessage, int nMasterCopySeparationSet, int nVersion, int nMiscInt, CString sMiscString, CString &sErrorMessage)
{
	CString sSQL;
	
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;
	
	if (sMessage == "")
		sMessage = "Ok";

	sSQL.Format("{CALL [spAddServiceLogEntry] ('%s',1,%d, '%s','%s', '%s',%d,%d,%d,'%s')}",
		_T("MaintService"), nEvent, sSource, sFileName, sMessage, nMasterCopySeparationSet, nVersion, nMiscInt, sMiscString);

	try {
		m_pDB->ExecuteSQL( sSQL );
	}
	catch( CDBException* e ) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Insert failed - %s", (LPCSTR)e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch( CDBException* e ) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::UpdateService(int nCurrentState, CString sCurrentJob, CString sLastError, CString &sErrorMessage)
{
	return UpdateService(nCurrentState, sCurrentJob, sLastError, _T(""), sErrorMessage);
}

BOOL CDatabaseManager::UpdateService(int nCurrentState, CString sCurrentJob, CString sLastError, CString sAddedLogData, CString &sErrorMessage)
{
	CString sSQL;

	CString sComputer = g_util.GetComputerName();
	sErrorMessage = _T("");

	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;
	sSQL.Format("{CALL spRegisterService ('MaintService', 1, 7, '%s',%d,'%s','%s','%s')}", sComputer, nCurrentState, sCurrentJob, sLastError, sAddedLogData);
//	g_util.Logprintf("DEBUG: %s", sSQL);
	try {
		m_pDB->ExecuteSQL( sSQL );
	}
	catch( CDBException* e ) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch( CDBException* e ) {
			// So what..! Go on!;
		}
		return FALSE;
	}
 
	return TRUE;
}


BOOL CDatabaseManager::UpdateServiceDB(int nCurrentState, CString sCurrentJob, CString sLastError, CString sAddedLogData, CString &sErrorMessage)
{
	CString sSQL;
	CString sComputer = g_util.GetComputerName();

	
	sErrorMessage = _T("");

	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;
	sSQL.Format("{CALL spRegisterService ('Database', 1, 5, '%s',%d,'%s','%s','%s')}", sComputer, nCurrentState, sCurrentJob, sLastError, sAddedLogData);

	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}


BOOL CDatabaseManager::UpdateServiceFileServer(CString sServer, int nCurrentState, CString sCurrentJob, CString sLastError, CString sAddedLogData, CString &sErrorMessage)
{
	CString sSQL;

	DWORD buflen = MAX_PATH;

	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;
	sSQL.Format("{CALL spRegisterService ('Fileserver', 1, 6, '%s',%d,'%s','%s','%s')}", sServer, nCurrentState, sCurrentJob, sLastError, sAddedLogData);

	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::LoadSTMPSetup(CString &sErrorMessage)
{
	sErrorMessage = _T("");

	CString sSQL = _T("SELECT TOP 1 SMTPServer,SMTPPort, SMTPUserName,SMTPPassword,UseSSL,SMTPConnectionTimeout,SMTPFrom,SMTPCC,SMTPTo,SMTPSubject FROM SMTPPreferences");
	CString s;
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);

	try {
		if (!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {
			CString s;
			int fld = 0;
			Rs.GetFieldValue((short)fld++, g_prefs.m_emailsmtpserver);
			Rs.GetFieldValue((short)fld++, s);
			g_prefs.m_mailsmtpport = atoi(s);

			Rs.GetFieldValue((short)fld++, g_prefs.m_mailsmtpserverusername);
			Rs.GetFieldValue((short)fld++, g_prefs.m_mailsmtpserverpassword);
			Rs.GetFieldValue((short)fld++, s);
			g_prefs.m_mailusessl = atoi(s);
			Rs.GetFieldValue((short)fld++, s);
			g_prefs.m_emailtimeout = atoi(s);

			Rs.GetFieldValue((short)fld++, g_prefs.m_emailfrom);
			Rs.GetFieldValue((short)fld++, g_prefs.m_emailcc);
			Rs.GetFieldValue((short)fld++, g_prefs.m_emailto);
			Rs.GetFieldValue((short)fld++, g_prefs.m_emailsubject);

		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("Query failed - %s (%s)", e->m_strError, sSQL);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
			return FALSE;
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;

}

BOOL CDatabaseManager::LoadAllPrefs(CString &sErrorMessage)
{
	if (LoadGeneralPreferences(sErrorMessage) <= 0)
		return FALSE;

	LoadSTMPSetup(sErrorMessage);

	if (LoadPublicationList(sErrorMessage) == FALSE)
		return FALSE;

	if (LoadSectionNameList(sErrorMessage) == FALSE)
		return FALSE;

	if (LoadEditionNameList(sErrorMessage) == FALSE)
		return FALSE;


	if (LoadFileServerList(sErrorMessage) == FALSE)
		return FALSE;


	if (LoadAliasList(sErrorMessage) == FALSE)
		return FALSE;


	g_prefs.m_StatusList.RemoveAll();
	if (LoadStatusList(_T("StatusCodes"), g_prefs.m_StatusList, sErrorMessage) == FALSE)
		return FALSE;

	g_prefs.m_ExternalStatusList.RemoveAll();
	if (LoadStatusList(_T("ExternalStatusCodes"), g_prefs.m_ExternalStatusList, sErrorMessage) == FALSE)
		return FALSE;

	if (LoadPublisherList(sErrorMessage) == -1)
		return FALSE;

	
	if (LoadChannelList(sErrorMessage) == -1)
		return FALSE;



	return TRUE;
}


// Returns : -1 error
//			  1 found data
//            0 No data (fatal)
int	CDatabaseManager::LoadGeneralPreferences(CString &sErrorMessage)
{
	CUtils util;
	BOOL foundData = FALSE;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return -1;

	CRecordset Rs(m_pDB);

	CString sSQL = _T("SELECT TOP 1 ServerName,ServerShare,ServerFilePath,ServerPreviewPath,ServerThumbnailPath,ServerLogPath,ServerConfigPath,ServerErrorPath,CopyProofToWeb,WebProofPath,ServerUseCurrentUser,ServerUserName,ServerPassword FROM GeneralPreferences");
	try {
		if (!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return -1;
		}

		if (!Rs.IsEOF()) {
			CString s;
			int fld = 0;
			Rs.GetFieldValue((short)fld++, g_prefs.m_serverName);
			Rs.GetFieldValue((short)fld++, g_prefs.m_serverShare);

			Rs.GetFieldValue((short)fld++, g_prefs.m_hiresPDFPath);
			if (g_prefs.m_hiresPDFPath.Right(1) != "\\")
				g_prefs.m_hiresPDFPath += _T("\\");

			g_prefs.m_lowresPDFPath = g_prefs.m_hiresPDFPath;
			g_prefs.m_lowresPDFPath.MakeUpper();
			g_prefs.m_lowresPDFPath.Replace("CCFILESHIRES", "CCFILESLOWRES");

			g_prefs.m_printPDFPath = g_prefs.m_hiresPDFPath;
			g_prefs.m_printPDFPath.MakeUpper();
			g_prefs.m_printPDFPath.Replace("CCFILESHIRES", "CCFILESPRINT");


			Rs.GetFieldValue((short)fld++, g_prefs.m_previewPath);
			if (g_prefs.m_previewPath.Right(1) != "\\")
				g_prefs.m_previewPath += _T("\\");

			g_prefs.m_readviewpreviewPath = g_prefs.m_previewPath;
			g_prefs.m_readviewpreviewPath.MakeUpper();
			g_prefs.m_readviewpreviewPath.Replace("CCPREVIEWS", "CCreadviewpreviews");



			Rs.GetFieldValue((short)fld++, g_prefs.m_thumbnailPath);
			if (g_prefs.m_thumbnailPath.Right(1) != "\\")
				g_prefs.m_thumbnailPath += _T("\\");
			Rs.GetFieldValue((short)fld++, s);


			Rs.GetFieldValue((short)fld++, g_prefs.m_configPath);
			if (g_prefs.m_configPath.Right(1) != "\\")
				g_prefs.m_configPath += _T("\\");
			Rs.GetFieldValue((short)fld++, g_prefs.m_errorPath);
			if (g_prefs.m_errorPath.Right(1) != "\\")
				g_prefs.m_errorPath += _T("\\");


			Rs.GetFieldValue((short)fld++, s);
			g_prefs.m_copyToWeb = atoi(s);

			Rs.GetFieldValue((short)fld++, g_prefs.m_webPath);
			if (g_prefs.m_webPath.Right(1) != "\\")
				g_prefs.m_webPath += _T("\\");

			Rs.GetFieldValue((short)fld++, s);
			g_prefs.m_mainserverusecussrentuser = atoi(s);

			Rs.GetFieldValue((short)fld++, g_prefs.m_mainserverusername);
			Rs.GetFieldValue((short)fld++, g_prefs.m_mainserverpassword);



			foundData = TRUE;
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("Query failed - %s (%s)", e->m_strError, sSQL);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
			return -1;
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return -1;
	}

	if (foundData == 0)
		sErrorMessage = _T("FATAL ERROR: No data found in GeneralPrefences table");
	return foundData;
}


BOOL CDatabaseManager::LoadPublicationList(CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);
	CString sSQL, s;
	g_prefs.m_PublicationList.RemoveAll();

	sSQL.Format("SELECT DISTINCT PublicationID,[Name],PageFormatID,TrimToFormat,LatestHour,DefaultProofID,DefaultApprove,DefaultPriority,CustomerID,AutoPurgeKeepDays,EmailRecipient,EmailCC,EmailSubject,EmailBody,UploadFolder,Deadline,AnnumText,AllowUnplanned,ReleaseDays,ReleaseTime,PubdateMove,PubdateMoveDays,InputAlias,OutputALias,ExtendedAlias,PublisherID FROM PublicationNames ORDER BY [Name]");

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			LogprintfDB("Query failed : %s", sSQL);
			return FALSE;
		}

		while (!Rs.IsEOF()) {
			int f = 0;
			PUBLICATIONSTRUCT *pItem = new PUBLICATIONSTRUCT;
			Rs.GetFieldValue((short)f++, s);
			pItem->m_ID = atoi(s);

			Rs.GetFieldValue((short)f++, pItem->m_name);
			Rs.GetFieldValue((short)f++, s);
			pItem->m_PageFormatID = atoi(s);
			Rs.GetFieldValue((short)f++, s);
			pItem->m_TrimToFormat = atoi(s);
			Rs.GetFieldValue((short)f++, s);
			pItem->m_LatestHour = atof(s);

			Rs.GetFieldValue((short)f++, s);
			pItem->m_ProofID = atoi(s);

			Rs.GetFieldValue((short)f++, s);
			pItem->m_Approve = atoi(s);
			Rs.GetFieldValue((short)f++, s);
			pItem->m_priority = atoi(s);

			Rs.GetFieldValue((short)f++, s);
			pItem->m_customerID = atoi(s);

			Rs.GetFieldValue((short)f++, s);
			pItem->m_autoPurgeKeepDays = atoi(s);

			Rs.GetFieldValue((short)f++, pItem->m_emailrecipient);
			Rs.GetFieldValue((short)f++, pItem->m_emailCC);
			Rs.GetFieldValue((short)f++, pItem->m_emailSubject);
			Rs.GetFieldValue((short)f++, pItem->m_emailBody);
			Rs.GetFieldValue((short)f++, pItem->m_uploadfolder);

			pItem->m_deadline = CTime(1975, 1, 1);

			try {
				CDBVariant DBvar;
				Rs.GetFieldValue((short)f++, DBvar, SQL_C_TIMESTAMP);
				CTime t(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
				pItem->m_deadline = t;
			}
			catch (...)
			{
			}

			Rs.GetFieldValue((short)f++, pItem->m_annumtext);
			Rs.GetFieldValue((short)f++, s);
			pItem->m_allowunplanned = atoi(s);

			Rs.GetFieldValue((short)f++, s);
			pItem->m_releasedays = atoi(s);

			Rs.GetFieldValue((short)f++, s);
			int n = atoi(s);
			pItem->m_releasetimehour = n / 100;
			pItem->m_releasetimeminute = n - pItem->m_releasetimehour;

			Rs.GetFieldValue((short)f++, s);
			pItem->m_pubdatemove = atoi(s) > 0;

			Rs.GetFieldValue((short)f++, s);
			pItem->m_pubdatemovedays = atoi(s);

			Rs.GetFieldValue((short)f++, pItem->m_alias);
			Rs.GetFieldValue((short)f++, pItem->m_outputalias);
			Rs.GetFieldValue((short)f++, pItem->m_extendedalias);

			Rs.GetFieldValue((short)f++, s);
			pItem->m_publisherID = atoi(s);

			g_prefs.m_PublicationList.Add(*pItem);

			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}



	for (int i = 0; i < g_prefs.m_PublicationList.GetCount(); i++)
		if (LoadPublicationChannels(g_prefs.m_PublicationList[i].m_ID, sErrorMessage) == FALSE)
			return FALSE;

	return TRUE;
}

BOOL CDatabaseManager::LoadNewChannel(int nChannelID, CString &sErrorMessage)
{

	sErrorMessage = _T("");

	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CHANNELSTRUCT *pItem = NULL;

	BOOL bNewChannel = TRUE;
	for (int i = 0; i < g_prefs.m_ChannelList.GetCount(); i++) {
		if (g_prefs.m_ChannelList[i].m_channelID == nChannelID) {
			pItem = &g_prefs.m_ChannelList[i];
			bNewChannel = FALSE;
			break;
		}
	}

	if (bNewChannel) {
		pItem = new CHANNELSTRUCT();
		pItem->m_channelID = nChannelID;
	}

	CRecordset Rs(m_pDB);
	CString sSQL, s;
	CDBVariant DBvar;

	sSQL.Format("SELECT ChannelID,Name,Enabled,OwnerInstance,UseReleaseTime,ReleaseTime,TransmitNameFormat,TransmitNameDateFormat,TransmitNameUseAbbr,TransmitNameOptions,OutputType,FTPServer,FTPPort,FTPUserName,FTPPassword,FTPfolder,FTPEncryption,FTPPasv,FTPXCRC,FTPTimeout,FTPBlockSize,FTPUseTmpFolder,FTPPostCheck,EmailServer,EmailPort,EmailUserName,EmailPassword,EmailFrom,EmailTo,EmailCC,EmailUseSSL,EmailSubject,EmailBody,EmailHTML,EmailTimeout,OutputFolder,UseSpecificUser,UserName,Password,SubFolderNamingConvension,ChannelNameAlias,PDFType,MergedPDF,EditionsToGenerate,SendCommonPages,MiscInt,MiscString,ConfigChangeTime,PDFProcessID,TriggerMode,TriggerEmail,DeleteOldOutputFilesDays,UsePackageNames FROM ChannelNames WHERE ChannelID=%d", nChannelID);
	try {
		if (!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {

			int fld = 0;

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_channelID = atoi(s);

			Rs.GetFieldValue((short)fld++, pItem->m_channelname);

			Rs.GetFieldValue((short)fld++, s); // Enabled
			pItem->m_enabled = atoi(s);

			Rs.GetFieldValue((short)fld++, s); // OwnerInstance
			pItem->m_ownerinstance = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_usereleasetime = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			int n = atoi(s);
			pItem->m_releasetimehour = n / 100;
			pItem->m_releasetimeminute = n - pItem->m_releasetimehour;

			//Rs.GetFieldValue((short)fld++, s);
			//pItem->m_channelgroupID = atoi(s);

			//Rs.GetFieldValue((short)fld++, s);
			//pItem->m_publisherID = atoi(s);

			Rs.GetFieldValue((short)fld++, pItem->m_transmitnameconv);

			Rs.GetFieldValue((short)fld++, pItem->m_transmitdateformat);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_transmitnameuseabbr = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_transmitnameoptions = atoi(s);

			Rs.GetFieldValue((short)fld++, s); // OutputType
			pItem->m_outputuseftp = atoi(s);
			Rs.GetFieldValue((short)fld++, pItem->m_outputftpserver);
			Rs.GetFieldValue((short)fld++, s); // FTPPort
			pItem->m_outputftpport = atoi(s);
			Rs.GetFieldValue((short)fld++, pItem->m_outputftpuser);
			Rs.GetFieldValue((short)fld++, pItem->m_outputftppw);
			Rs.GetFieldValue((short)fld++, pItem->m_outputftpfolder);
			Rs.GetFieldValue((short)fld++, s); // FTPEncryption
			pItem->m_outputfpttls = atoi(s);
			Rs.GetFieldValue((short)fld++, s); // FTPPasv
			pItem->m_outputftppasv = atoi(s);
			Rs.GetFieldValue((short)fld++, s); // FTPXCRC
			pItem->m_outputftpXCRC = atoi(s);
			Rs.GetFieldValue((short)fld++, s); // FTPTimeout
			pItem->m_ftptimeout = atoi(s);
			Rs.GetFieldValue((short)fld++, s); // FTPBlockSize
			pItem->m_outputftpbuffersize = atoi(s);
			Rs.GetFieldValue((short)fld++, pItem->m_outputftptempfolder);
			Rs.GetFieldValue((short)fld++, s); // FTPPostcheck
			pItem->m_FTPpostcheck = atoi(s);
			Rs.GetFieldValue((short)fld++, pItem->m_emailsmtpserver);
			Rs.GetFieldValue((short)fld++, s); // EmailPort
			pItem->m_emailsmtpport = atoi(s);
			Rs.GetFieldValue((short)fld++, pItem->m_emailsmtpserverusername);
			Rs.GetFieldValue((short)fld++, pItem->m_emailsmtpserverpassword);
			Rs.GetFieldValue((short)fld++, pItem->m_emailfrom);
			Rs.GetFieldValue((short)fld++, pItem->m_emailto);
			Rs.GetFieldValue((short)fld++, pItem->m_emailcc);
			Rs.GetFieldValue((short)fld++, s); // EmailUseSSL
			pItem->m_emailusessl = atoi(s);
			Rs.GetFieldValue((short)fld++, pItem->m_emailsubject);
			Rs.GetFieldValue((short)fld++, pItem->m_emailbody);
			Rs.GetFieldValue((short)fld++, s); // EmailHTML
			pItem->m_emailusehtml = atoi(s);
			Rs.GetFieldValue((short)fld++, s); // EmailHTML
			pItem->m_emailtimeout = atoi(s);

			Rs.GetFieldValue((short)fld++, pItem->m_outputfolder);
			Rs.GetFieldValue((short)fld++, s); // EmailHTML
			pItem->m_outputusecurrentuser = (atoi(s) ? FALSE : TRUE);
			Rs.GetFieldValue((short)fld++, pItem->m_outputusername);
			Rs.GetFieldValue((short)fld++, pItem->m_outputpassword);

			Rs.GetFieldValue((short)fld++, pItem->m_subfoldernameconv);
			Rs.GetFieldValue((short)fld++, pItem->m_channelnamealias);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_pdftype = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_editionstogenerate = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_sendCommonPages = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_mergePDF = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_miscint = atoi(s);

			Rs.GetFieldValue((short)fld++, pItem->m_miscstring);

			pItem->m_configchangetime = CTime(1975, 1, 1);

			try {
				Rs.GetFieldValue((short)fld++, DBvar, SQL_C_TIMESTAMP);
				CTime t(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
				pItem->m_configchangetime = t;
			}
			catch (...)
			{
			}

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_pdfprocessID = atoi(s); // PDFProcessID



			Rs.GetFieldValue((short)fld++, s);
			pItem->m_triggermode = atoi(s);

			Rs.GetFieldValue((short)fld++, pItem->m_triggeremail);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_deleteoldoutputfilesdays = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_usepackagenames = atoi(s);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		sErrorMessage.Format("Query failed - %s (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
			return -1;
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	if (bNewChannel)
		g_prefs.m_ChannelList.Add(*pItem);

	return TRUE;
}

BOOL CDatabaseManager::LoadChannelList(CString &sErrorMessage)
{

	int foundJob = 0;
	sErrorMessage = _T("");

	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	g_prefs.m_ChannelList.RemoveAll();
	CRecordset Rs(m_pDB);
	CDBVariant DBvar;

	CString sSQL = _T("SELECT ChannelID,Name,Enabled,OwnerInstance,UseReleaseTime,ReleaseTime,TransmitNameFormat,TransmitNameDateFormat,TransmitNameUseAbbr,TransmitNameOptions,OutputType,FTPServer,FTPPort,FTPUserName,FTPPassword,FTPfolder,FTPEncryption,FTPPasv,FTPXCRC,FTPTimeout,FTPBlockSize,FTPUseTmpFolder,FTPPostCheck,EmailServer,EmailPort,EmailUserName,EmailPassword,EmailFrom,EmailTo,EmailCC,EmailUseSSL,EmailSubject,EmailBody,EmailHTML,EmailTimeout,OutputFolder,UseSpecificUser,UserName,Password,SubFolderNamingConvension,ChannelNameAlias,PDFType,MergedPDF,EditionsToGenerate,SendCommonPages,MiscInt,MiscString,ConfigChangeTime,PDFProcessID,TriggerMode,TriggerEmail,DeleteOldOutputFilesDays,UsePackageNames FROM ChannelNames WHERE ChannelID>0 AND Name<>''  ORDER BY Name");
	try {
		if (!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			return -1;
		}

		while (!Rs.IsEOF()) {
			CString s;
			int fld = 0;
			CHANNELSTRUCT *pItem = new CHANNELSTRUCT();

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_channelID = atoi(s);

			Rs.GetFieldValue((short)fld++, pItem->m_channelname);

			Rs.GetFieldValue((short)fld++, s); // Enabled
			pItem->m_enabled = atoi(s);

			Rs.GetFieldValue((short)fld++, s); // OwnerInstance
			pItem->m_ownerinstance = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_usereleasetime = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			int n = atoi(s);
			pItem->m_releasetimehour = n / 100;
			pItem->m_releasetimeminute = n - pItem->m_releasetimehour;

			//	Rs.GetFieldValue((short)fld++, s);
		//		pItem->m_channelgroupID = atoi(s);

			//	Rs.GetFieldValue((short)fld++, s);
			//	pItem->m_publisherID = atoi(s);

			Rs.GetFieldValue((short)fld++, pItem->m_transmitnameconv);

			Rs.GetFieldValue((short)fld++, pItem->m_transmitdateformat);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_transmitnameuseabbr = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_transmitnameoptions = atoi(s);

			Rs.GetFieldValue((short)fld++, s); // OutputType
			pItem->m_outputuseftp = atoi(s);
			Rs.GetFieldValue((short)fld++, pItem->m_outputftpserver);
			Rs.GetFieldValue((short)fld++, s); // FTPPort
			pItem->m_outputftpport = atoi(s);
			Rs.GetFieldValue((short)fld++, pItem->m_outputftpuser);
			Rs.GetFieldValue((short)fld++, pItem->m_outputftppw);
			Rs.GetFieldValue((short)fld++, pItem->m_outputftpfolder);
			Rs.GetFieldValue((short)fld++, s); // FTPEncryption
			pItem->m_outputfpttls = atoi(s);
			Rs.GetFieldValue((short)fld++, s); // FTPPasv
			pItem->m_outputftppasv = atoi(s);
			Rs.GetFieldValue((short)fld++, s); // FTPXCRC
			pItem->m_outputftpXCRC = atoi(s);
			Rs.GetFieldValue((short)fld++, s); // FTPTimeout
			pItem->m_ftptimeout = atoi(s);
			Rs.GetFieldValue((short)fld++, s); // FTPBlockSize
			pItem->m_outputftpbuffersize = atoi(s);
			Rs.GetFieldValue((short)fld++, pItem->m_outputftptempfolder);
			Rs.GetFieldValue((short)fld++, s); // FTPPostcheck
			pItem->m_FTPpostcheck = atoi(s);
			Rs.GetFieldValue((short)fld++, pItem->m_emailsmtpserver);
			Rs.GetFieldValue((short)fld++, s); // EmailPort
			pItem->m_emailsmtpport = atoi(s);
			Rs.GetFieldValue((short)fld++, pItem->m_emailsmtpserverusername);
			Rs.GetFieldValue((short)fld++, pItem->m_emailsmtpserverpassword);
			Rs.GetFieldValue((short)fld++, pItem->m_emailfrom);
			Rs.GetFieldValue((short)fld++, pItem->m_emailto);
			Rs.GetFieldValue((short)fld++, pItem->m_emailcc);
			Rs.GetFieldValue((short)fld++, s); // EmailUseSSL
			pItem->m_emailusessl = atoi(s);
			Rs.GetFieldValue((short)fld++, pItem->m_emailsubject);
			Rs.GetFieldValue((short)fld++, pItem->m_emailbody);
			Rs.GetFieldValue((short)fld++, s); // EmailHTML
			pItem->m_emailusehtml = atoi(s);
			Rs.GetFieldValue((short)fld++, s); // emailtimeout
			pItem->m_emailtimeout = atoi(s);
			Rs.GetFieldValue((short)fld++, pItem->m_outputfolder);
			Rs.GetFieldValue((short)fld++, s); // EmailHTML
			pItem->m_outputusecurrentuser = (atoi(s) ? FALSE : TRUE);
			Rs.GetFieldValue((short)fld++, pItem->m_outputusername);
			Rs.GetFieldValue((short)fld++, pItem->m_outputpassword);

			Rs.GetFieldValue((short)fld++, pItem->m_subfoldernameconv);
			Rs.GetFieldValue((short)fld++, pItem->m_channelnamealias);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_pdftype = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_mergePDF = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_editionstogenerate = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_sendCommonPages = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_miscint = atoi(s);
			Rs.GetFieldValue((short)fld++, pItem->m_miscstring);

			pItem->m_configchangetime = CTime(1975, 1, 1);

			try {
				Rs.GetFieldValue((short)fld++, DBvar, SQL_C_TIMESTAMP);
				CTime t(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
				pItem->m_configchangetime = t;
			}
			catch (...)
			{
			}

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_pdfprocessID = atoi(s);

			

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_triggermode = atoi(s);

			Rs.GetFieldValue((short)fld++, pItem->m_triggeremail);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_deleteoldoutputfilesdays = atoi(s);


			Rs.GetFieldValue((short)fld++, s);
			pItem->m_usepackagenames = atoi(s);

			g_prefs.m_ChannelList.Add(*pItem);

			foundJob++;

			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		sErrorMessage.Format("Query failed - %s (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
			return FALSE;
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	
	return TRUE;
}




int CDatabaseManager::LoadPublisherList(CString &sErrorMessage)
{
	int foundJob = 0;

	//if (g_prefs.m_DBcapabilities_ChannelGroupNames == FALSE)
	//	return 0;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return -1;

	g_prefs.m_PublisherList.RemoveAll();
	CRecordset Rs(m_pDB);

	CString sSQL = _T("SELECT PublisherID,PublisherName FROM PublisherNames WHERE PublisherID>0 AND PublisherName<>''  ORDER BY PublisherName");
	try {
		if (!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return -1;
		}

		while (!Rs.IsEOF()) {
			CString s;
			int fld = 0;
			ITEMSTRUCT *pItem = new ITEMSTRUCT();

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_ID = atoi(s);

			Rs.GetFieldValue((short)fld++, pItem->m_name);

			g_prefs.m_PublisherList.Add(*pItem);
			foundJob++;

			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		sErrorMessage.Format("Query failed - %s (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
			return -1;
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return -1;
	}

	return foundJob;
}


BOOL CDatabaseManager::LoadPublicationChannels(int nID, CString &sErrorMessage)
{
	sErrorMessage = _T("");

	PUBLICATIONSTRUCT *pItem = g_prefs.GetPublicationStruct(nID);
	if (pItem == NULL)
		return FALSE;

	pItem->m_numberOfChannels = 0;

	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);

	CString sSQL, s;

	CString sList = _T("");
	sSQL.Format("SELECT DISTINCT ChannelID,PushTrigger,PubDateMoveDays,ReleaseDelay FROM PublicationChannels WHERE PublicationID=%d", nID);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return FALSE;
		}

		while (!Rs.IsEOF()) {

			Rs.GetFieldValue((short)0, s);
			pItem->m_channels[pItem->m_numberOfChannels].m_channelID = atoi(s);
			Rs.GetFieldValue((short)1, s);
			pItem->m_channels[pItem->m_numberOfChannels].m_pushtrigger = atoi(s);
			Rs.GetFieldValue((short)2, s);
			pItem->m_channels[pItem->m_numberOfChannels].m_pubdatemovedays = atoi(s);
			Rs.GetFieldValue((short)3, s);
			pItem->m_channels[pItem->m_numberOfChannels].m_releasedelay = atoi(s);
			Rs.MoveNext();
		}

		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}



CString CDatabaseManager::LoadNewPublicationName(int nID, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return 0;

	CRecordset Rs(m_pDB);

	CString sSQL, s;
	CString sName = _T("");

	PUBLICATIONSTRUCT *pItem = g_prefs.GetPublicationStructNoDbLookup(nID);
	BOOL bNewEntry = pItem == NULL;

	sSQL.Format("SELECT DISTINCT [Name], PageFormatID, TrimToFormat, LatestHour, DefaultProofID, DefaultApprove, DefaultPriority, CustomerID, AutoPurgeKeepDays, EmailRecipient, EmailCC, EmailSubject, EmailBody, UploadFolder, Deadline, AnnumText, AllowUnplanned, ReleaseDays, ReleaseTime, PubdateMove, PubdateMoveDays, InputAlias, OutputALias, ExtendedAlias, PublisherID FROM PublicationNames WHERE PublicationID=%d", nID);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return _T("");
		}

		if (!Rs.IsEOF()) {
			int f = 0;
			if (bNewEntry)
				pItem = new PUBLICATIONSTRUCT();
			pItem->m_ID = nID;

			Rs.GetFieldValue((short)f++, pItem->m_name);
			Rs.GetFieldValue((short)f++, s);
			pItem->m_PageFormatID = atoi(s);
			Rs.GetFieldValue((short)f++, s);
			pItem->m_TrimToFormat = atoi(s);
			Rs.GetFieldValue((short)f++, s);
			pItem->m_LatestHour = atof(s);

			Rs.GetFieldValue((short)f++, s);
			pItem->m_ProofID = atoi(s);

			Rs.GetFieldValue((short)f++, s);
			pItem->m_Approve = atoi(s);
			Rs.GetFieldValue((short)f++, s);
			pItem->m_priority = atoi(s);

			Rs.GetFieldValue((short)f++, s);
			pItem->m_customerID = atoi(s);

			Rs.GetFieldValue((short)f++, s);
			pItem->m_autoPurgeKeepDays = atoi(s);

			Rs.GetFieldValue((short)f++, pItem->m_emailrecipient);
			Rs.GetFieldValue((short)f++, pItem->m_emailCC);
			Rs.GetFieldValue((short)f++, pItem->m_emailSubject);
			Rs.GetFieldValue((short)f++, pItem->m_emailBody);
			Rs.GetFieldValue((short)f++, pItem->m_uploadfolder);

			pItem->m_deadline = CTime(1975, 1, 1);

			try {
				CDBVariant DBvar;
				Rs.GetFieldValue((short)f++, DBvar, SQL_C_TIMESTAMP);
				CTime t(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
				pItem->m_deadline = t;
			}
			catch (...)
			{
			}

			Rs.GetFieldValue((short)f++, pItem->m_annumtext);
			Rs.GetFieldValue((short)f++, s);
			pItem->m_allowunplanned = atoi(s);

			Rs.GetFieldValue((short)f++, s);
			pItem->m_releasedays = atoi(s);

			Rs.GetFieldValue((short)f++, s);
			int n = atoi(s);
			pItem->m_releasetimehour = n / 100;
			pItem->m_releasetimeminute = n - pItem->m_releasetimehour;

			Rs.GetFieldValue((short)f++, s);
			pItem->m_pubdatemove = atoi(s) > 0;

			Rs.GetFieldValue((short)f++, s);
			pItem->m_pubdatemovedays = atoi(s);

			Rs.GetFieldValue((short)f++, pItem->m_alias);
			Rs.GetFieldValue((short)f++, pItem->m_outputalias);
			Rs.GetFieldValue((short)f++, pItem->m_extendedalias);

			Rs.GetFieldValue((short)f++, s);
			pItem->m_publisherID = atoi(s);
			if (bNewEntry)
				g_prefs.m_PublicationList.Add(*pItem);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return _T("");
	}

	LoadPublicationChannels(nID, sErrorMessage);

	return sName;
}


CString CDatabaseManager::LoadNewEditionName(int nID, CString &sErrorMessage)
{
	sErrorMessage = _T( "");
	if (InitDB(sErrorMessage) == FALSE)
		return "";

	CRecordset Rs(m_pDB);

	CString sSQL;
	CString sEd = "";
	sSQL.Format("SELECT Name FROM EditionNames WHERE EditionID=%d", nID);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return "";
		}

		if (!Rs.IsEOF()) {
			ITEMSTRUCT *pItem = new ITEMSTRUCT();
			Rs.GetFieldValue((short)0, sEd);
			pItem->m_ID = nID;
			pItem->m_name = sEd;		
			g_prefs.m_EditionList.Add(*pItem);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return "";
	}

	return sEd;
}

int CDatabaseManager::LoadNewEditionID(CString sName, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return 0;

	CRecordset Rs(m_pDB);

	CString sSQL, s;
	int nID = 0;
	sSQL.Format("SELECT EditionID FROM EditionNames WHERE Name='%s'", sName);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return 0;
		}

		if (!Rs.IsEOF()) {
			ITEMSTRUCT *pItem = new ITEMSTRUCT();
			Rs.GetFieldValue((short)0, s);
			nID = atoi(s);
			pItem->m_ID = nID;
			pItem->m_name = sName;
			g_prefs.m_EditionList.Add(*pItem);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return 0;
	}

	return nID;
}


CString CDatabaseManager::LoadNewSectionName(int nID, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return "";

	CRecordset Rs(m_pDB);

	CString sSQL;
	CString sEd = "";
	sSQL.Format("SELECT Name FROM SectionNames WHERE SectionID=%d", nID);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return "";
		}

		if (!Rs.IsEOF()) {
			ITEMSTRUCT *pItem = new ITEMSTRUCT();
			Rs.GetFieldValue((short)0, sEd);
			pItem->m_ID = nID;
			pItem->m_name = sEd;
			g_prefs.m_SectionList.Add(*pItem);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return "";
	}

	return sEd;
}

int CDatabaseManager::LoadNewSectionID(CString sName, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return 0;

	CRecordset Rs(m_pDB);

	CString sSQL,s;
	int nID = 0;
	sSQL.Format("SELECT SectionID FROM SectionNames WHERE Name='%s'", sName);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return 0;
		}

		if (!Rs.IsEOF()) {
			ITEMSTRUCT *pItem = new ITEMSTRUCT();
			Rs.GetFieldValue((short)0, s);
			nID = atoi(s);
			pItem->m_ID = nID;
			pItem->m_name = sName;
			g_prefs.m_SectionList.Add(*pItem);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return 0;
	}

	return nID;
}

CString CDatabaseManager::LoadNewPublisherName(int nID, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return "";

	CRecordset Rs(m_pDB);

	CString sSQL;
	CString sName = "";
	sSQL.Format("SELECT PublisherName FROM PublisherNames WHERE PublisherID=%d", nID);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return "";
		}

		if (!Rs.IsEOF()) {
			ITEMSTRUCT *pItem = new ITEMSTRUCT();
			Rs.GetFieldValue((short)0, sName);
			pItem->m_ID = nID;
			pItem->m_name = sName;
			g_prefs.m_PublisherList.Add(*pItem);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return "";
	}

	return sName;
}


BOOL CDatabaseManager::LoadEditionNameList(CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);

	g_prefs.m_EditionList.RemoveAll();
	CString sSQL;
	CString s;

	sSQL = _T("SELECT EditionID,Name FROM EditionNames");

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return FALSE;
		}

		while (!Rs.IsEOF()) {
			ITEMSTRUCT *pItem = new ITEMSTRUCT;
			Rs.GetFieldValue((short)0, s);
			pItem->m_ID = atoi(s);
			Rs.GetFieldValue((short)1, pItem->m_name);
			g_prefs.m_EditionList.Add(*pItem);
			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	return TRUE;
}

BOOL CDatabaseManager::LoadSectionNameList(CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);

	g_prefs.m_SectionList.RemoveAll();
	CString sSQL;
	CString s;

	sSQL = _T("SELECT SectionID,Name FROM SectionNames");

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return FALSE;
		}

		while (!Rs.IsEOF()) {
			ITEMSTRUCT *pItem = new ITEMSTRUCT;
			Rs.GetFieldValue((short)0, s);
			pItem->m_ID = atoi(s);
			Rs.GetFieldValue((short)1, pItem->m_name);
			g_prefs.m_SectionList.Add(*pItem);
			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	return TRUE;
}


BOOL CDatabaseManager::LoadFileServerList(CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);

	CString sSQL, s;

	sSQL.Format("SELECT [Name],ServerType,CCdatashare,Username,password,IP,Locationid,uselocaluser FROM FileServers ORDER BY ServerType");

	g_prefs.m_FileServerList.RemoveAll();
	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			return FALSE;
		}

		while (!Rs.IsEOF()) {
			FILESERVERSTRUCT *pItem = new FILESERVERSTRUCT();

			Rs.GetFieldValue((short)0, pItem->m_servername);

			Rs.GetFieldValue((short)1, s);
			pItem->m_servertype = atoi(s);

			Rs.GetFieldValue((short)2, pItem->m_sharename);

			Rs.GetFieldValue((short)3, pItem->m_username);

			Rs.GetFieldValue((short)4, pItem->m_password);

			Rs.GetFieldValue((short)5, pItem->m_IP);

			pItem->m_IP.Trim();

			Rs.GetFieldValue((short)6, s);
			pItem->m_locationID = atoi(s);

			Rs.GetFieldValue((short)7, s);
			pItem->m_uselocaluser = atoi(s);

			g_prefs.m_FileServerList.Add(*pItem);
			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	return TRUE;
}


BOOL CDatabaseManager::LoadStatusList(CString sIDtable, STATUSLIST &v, CString &sErrorMessage)
{
	CString sSQL;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);
	sSQL.Format("SELECT * FROM %s", sIDtable);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			return FALSE;
		}

		while (!Rs.IsEOF()) {
			STATUSSTRUCT *pItem = new STATUSSTRUCT;
			CString s;
			Rs.GetFieldValue((short)0, s);
			pItem->m_statusnumber = atoi(s);
			Rs.GetFieldValue((short)1, pItem->m_statusname);
			Rs.GetFieldValue((short)2, pItem->m_statuscolor);
			v.Add(*pItem);
			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	return TRUE;
}


BOOL CDatabaseManager::LoadAliasList(CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);

	g_prefs.m_AliasList.RemoveAll();

	CString sSQL;
	sSQL = _T("SELECT Type,Longname,Shortname FROM InputAliases");

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			return FALSE;
		}

		while (!Rs.IsEOF()) {
			ALIASSTRUCT *pItem = new ALIASSTRUCT();
			CString s;
			Rs.GetFieldValue((short)0, s);
			pItem->sType = s;
			Rs.GetFieldValue((short)1, s);
			pItem->sLongName = s;
			Rs.GetFieldValue((short)2, s);
			pItem->sShortName = s;
			g_prefs.m_AliasList.Add(*pItem);
			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	return TRUE;
}

CString CDatabaseManager::LoadSpecificAlias(CString sType, CString sLongName, CString &sErrorMessage)
{

	CString sShortName = sLongName;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return sShortName;

	CRecordset Rs(m_pDB);

	CString sSQL;
	sSQL.Format("SELECT Shortname FROM InputAliases WHERE Type='%s' AND LongName='%s'", sType, sLongName);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {
			Rs.GetFieldValue((short)0, sShortName);

			ALIASSTRUCT *pItem = new ALIASSTRUCT();
			pItem->sType = sType;
			pItem->sLongName = sLongName;
			pItem->sShortName = sShortName;
		
			g_prefs.m_AliasList.Add(*pItem);
			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return sShortName;
	}
	return sShortName;
}


CTime CDatabaseManager::GetDatabaseTime(CString &sErrorMessage)
{
	CString sSQL;
	CTime tm(1975, 1, 1, 0, 0, 0);
	CDBVariant DBvar;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return tm;

	CRecordset Rs(m_pDB);

	sSQL.Format("SELECT GETDATE()");

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {
			CDBVariant DBvar;
			Rs.GetFieldValue((short)0, DBvar, SQL_C_TIMESTAMP);
			CTime tPubTime(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
			tm = tPubTime;
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return tm;
	}
	return tm;
}

BOOL  CDatabaseManager::GetChannelsForMasterSet(int nMasterCopySeparationSet, int nPDFType, CUIntArray &aChannelIDList, CString &sErrorMessage)
{
	CString sSQL, s;

	sErrorMessage = _T("");

	aChannelIDList.RemoveAll();
	CRecordset *Rs = NULL;

	sSQL.Format("SELECT DISTINCT CS.ChannelID FROM ChannelStatus CS WITH (NOLOCK) INNER JOIN ChannelNames C WITH (NOLOCK) ON C.ChannelID=CS.ChannelID INNER JOIN ChannelGroupNames CG WITH (NOLOCK) ON CG.ChannelGroupID=C.ChannelGroupID WHERE CS.MasterCopySeparationSet=%d AND CG.PDFType=%d ", nMasterCopySeparationSet, nPDFType);

	BOOL bSuccess = FALSE;
	int nRetries = g_prefs.m_nQueryRetries;

	while (!bSuccess && nRetries-- > 0) {

		try {

			if (InitDB(sErrorMessage) == FALSE) {
				Sleep(g_prefs.m_QueryBackoffTime);
				continue;
			}

			Rs = new CRecordset(m_pDB);


			if (!Rs->Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
				sErrorMessage.Format("Query failed - %s", sSQL);

				Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
				delete Rs;
				Rs = NULL;
				continue;
			}

			while (!Rs->IsEOF()) {
				Rs->GetFieldValue((short)0, s);
				aChannelIDList.Add(atoi(s));

				Rs->MoveNext();
			}

			Rs->Close();
			bSuccess = TRUE;
			break;
		}

		catch (CDBException* e) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
			LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
			e->Delete();
			try {
				if (Rs->IsOpen())
					Rs->Close();
				delete Rs;
				Rs = NULL;
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			::Sleep(g_prefs.m_QueryBackoffTime);
		}
	}

	if (Rs != NULL)
		delete Rs;


	return bSuccess;
}


/*

BOOL CDatabaseManager::GetJobDetails(CJobAttributes *pJob, int nCopySeparationSet,  CString &sErrorMessage)
{
	CUtils util;
	CString sSQL, s;

	sSQL.Format("SELECT TOP 1 PublicationID,PubDate,EditionID,SectionID,PageName,Pagination,PageIndex,ColorID,Version,Comment,MiscString1,MiscString2,MiscString3,FileName,LocationID,ProductionID FROM PageTable WITH (NOLOCK) WHERE CopySeparationSet=%d AND Dirty=0 ORDER BY PageIndex", nCopySeparationSet);


	BOOL bSuccess = FALSE;
	CRecordset *Rs = NULL;

	pJob->m_copyflatseparationset = nCopySeparationSet;

	sErrorMessage = _T("");
	int nRetries = g_prefs.m_nQueryRetries;

	while (bSuccess == FALSE && nRetries-- > 0) {
		try {
			if (InitDB(sErrorMessage) == FALSE) {
				Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
				continue;
			}

			Rs = new CRecordset(m_pDB);

			if (!Rs->Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
				sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
				Sleep(g_prefs.m_QueryBackoffTime);
				delete Rs;
				Rs = NULL;
				continue;
			}

			int nFields = Rs->GetODBCFieldCount();

			if (!Rs->IsEOF()) {
				int fld = 0;
				Rs->GetFieldValue((short)fld++, s);
				pJob->m_publicationID = atoi(s);


				CDBVariant DBvar;
				try {
					Rs->GetFieldValue((short)fld++, DBvar, SQL_C_TIMESTAMP);
					CTime t(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
					pJob->m_pubdate = t;
				}
				catch (CException *ex)
				{
				}

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_editionID = atoi(s);

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_sectionID = atoi(s);

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_pagename = s;

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_pagination = atoi(s);

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_pageindex = atoi(s);

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_colorname = _T("PDF");		// HARDCODED

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_version = atoi(s);


				Rs->GetFieldValue((short)fld++, s);
				pJob->m_comment = s;

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_miscstring1 = s;

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_miscstring2 = s;

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_miscstring3 = s;

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_filename = s;

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_locationID = atoi(s);

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_productionID = atoi(s);
			}
			Rs->Close();
			bSuccess = TRUE;
		}
		catch (CDBException* e) {
			sErrorMessage.Format("Query failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();

			try {
				if (Rs->IsOpen())
					Rs->Close();
				delete Rs;
				Rs = NULL;
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime);
		}
	}

	if (Rs != NULL)
		delete Rs;

	return bSuccess;
}
*/
int CDatabaseManager::GetChannelCRC(int nMasterCopySeparationSet, int nInputMode, CString &sErrorMessage)
{
	sErrorMessage =  _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return -1;

	CRecordset Rs(m_pDB);
	CString sSQL, s;

	int nCRC = 0;

	sSQL.Format("SELECT TOP 1 CheckSumPdfPrint FROM PageTable WITH (NOLOCK) WHERE MasterCopySeparationSet=%d", nMasterCopySeparationSet);
	
	if (nInputMode == POLLINPUTTYPE_HIRESPDF)
		sSQL.Format("SELECT TOP 1 CheckSumPdfHighRes FROM PageTable WITH (NOLOCK) WHERE MasterCopySeparationSet=%d", nMasterCopySeparationSet);
	else if (nInputMode == POLLINPUTTYPE_LOWRESPDF)
		sSQL.Format("SELECT TOP 1 CheckSumPdfLowRes FROM PageTable WITH (NOLOCK) WHERE MasterCopySeparationSet=%d", nMasterCopySeparationSet);

	

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			return -1;
		}

		if (!Rs.IsEOF()) {
			Rs.GetFieldValue((short)0, s);
			nCRC = atoi(s);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return -1;
	}

	return nCRC;
}

int CDatabaseManager::GetChannelStatus(int nMasterCopySeparationSet, int nChannelID, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return -1;

	CRecordset Rs(m_pDB);
	CString sSQL, s;

	int nStatus = -1;


	sSQL.Format("SELECT TOP 1 Status FROM ChannelStatus WITH (NOLOCK) WHERE MasterCopySeparationSet=%d AND ChannelID=%d", nMasterCopySeparationSet, nChannelID);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			return -1;
		}

		if (!Rs.IsEOF()) {
			Rs.GetFieldValue((short)0, s);
			nStatus = atoi(s);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return -1;
	}

	return nStatus;
}


BOOL CDatabaseManager::HasDeleteJobQueued(CString &sErrorMessage)
{
	int nProductionID = 0;
	int nPublicationID = 0;
	int nEditionID = 0;
	int nSectionID = 0;
	CTime tPubDate;
	BOOL bSaveOldFiles = FALSE;
	BOOL bGenerateReport = FALSE;
	CTime tEventTime;
	BOOL bHasDBError = FALSE;
	CString sArchiveFolder;
	CString sMiscString;
	if (GetNextDeleteJob(nProductionID, nPublicationID, tPubDate, nEditionID, nSectionID, bSaveOldFiles, bGenerateReport, tEventTime, sArchiveFolder, sMiscString, sErrorMessage) == FALSE) {
		bHasDBError = TRUE;
		nProductionID = 0;
	}

	return bHasDBError == FALSE && nProductionID > 0 && nPublicationID > 0 && tPubDate.GetYear() > 1975;

}

BOOL CDatabaseManager::GetNextDeleteJob(int &nProductionID, int &nPublicationID, CTime &tPubDate, int &nEditionID, int &nSectionID,
	BOOL &bSaveOldFiles, BOOL &bGenerateReport, CTime &tEventTime, CString &sArchiveFolder, CString &sMiscString, CString &sErrorMessage)
{
	nProductionID = 0;
	nPublicationID = 0;
	tPubDate = CTime(1975, 1, 1, 0, 0, 0);
	nEditionID = 0;
	nSectionID = 0;
	bSaveOldFiles = FALSE;
	bGenerateReport = FALSE;
	tEventTime = CTime(1975, 1, 1, 0, 0, 0);

	sErrorMessage =  _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);
	CString sSQL, s;
	CDBVariant DBvar;

	sSQL = _T("SELECT TOP 1 ProductionID,PublicationID,PubDate,EditionID,SectionID,SaveOldFiles,GenerateReport,EventTime,SaveFolder,MiscString FROM ProductDeleteQueue WITH (NOLOCK) ORDER BY EventTime");

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {
			int f = 0;
			Rs.GetFieldValue((short)f++, s);
			nProductionID = atoi(s);

			Rs.GetFieldValue((short)f++, s);
			nPublicationID = atoi(s);

			Rs.GetFieldValue((short)f++, DBvar, SQL_C_TIMESTAMP);
			try {
				CTime t(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
				tPubDate = t;
			}
			catch (CException *ex) {}

			Rs.GetFieldValue((short)f++, s);
			nEditionID = atoi(s);

			Rs.GetFieldValue((short)f++, s);
			nSectionID = atoi(s);

			Rs.GetFieldValue((short)f++, s);
			bSaveOldFiles = atoi(s);

			Rs.GetFieldValue((short)f++, s);
			bGenerateReport = atoi(s);

			Rs.GetFieldValue((short)f++, DBvar, SQL_C_TIMESTAMP);
			try {
				CTime t(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
				tEventTime = t;
			}
			catch (CException *ex) {}

			Rs.GetFieldValue((short)f++, s);
			sArchiveFolder = s;

			Rs.GetFieldValue((short)f++, s);
			sMiscString = s;
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::CheckDeleteJob(int nProductionID, int &nPublicationID, CTime &tPubDate, int nEditionID, int nSectionID, CString &sPublicationName, BOOL &bIsDirty, CString &sErrorMessage)
{

	nPublicationID = 0;
	sPublicationName = _T("");
	bIsDirty = FALSE;

	tPubDate = CTime(1975, 1, 1, 0, 0, 0);

	sErrorMessage =  _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);
	CString sSQL, s;
	CDBVariant DBvar;

	if (nEditionID == 0 && nSectionID == 0)
		sSQL.Format("SELECT TOP 1 P.PublicationID,P.PubDate,PUB.Name,P.Dirty FROM PageTable AS P WITH (NOLOCK) INNER JOIN PublicationNames AS PUB with (NOLOCK) ON P.PublicationID=PUB.PublicationID WHERE P.Dirty=1 AND P.ProductionID=%d", nProductionID);
	else if (nEditionID > 0 && nSectionID == 0)
		sSQL.Format("SELECT TOP 1 P.PublicationID,P.PubDate,PUB.Name,P.Dirty FROM PageTable AS P WITH (NOLOCK) INNER JOIN PublicationNames AS PUB with (NOLOCK) ON P.PublicationID=PUB.PublicationID WHERE P.ProductionID=%d AND P.EditionID=%d", nProductionID, nEditionID);
	else if (nEditionID == 0 && nSectionID > 0)
		sSQL.Format("SELECT TOP 1 P.PublicationID,P.PubDate,PUB.Name,P.Dirty FROM PageTable AS P WITH (NOLOCK) INNER JOIN PublicationNames AS PUB with (NOLOCK) ON P.PublicationID=PUB.PublicationID WHERE P.ProductionID=%d AND P.SectionID=%d", nProductionID, nSectionID);
	else
		sSQL.Format("SELECT TOP 1 P.PublicationID,P.PubDate,PUB.Name,P.Dirty FROM PageTable AS P WITH (NOLOCK) INNER JOIN PublicationNames AS PUB with (NOLOCK) ON P.PublicationID=PUB.PublicationID WHERE P.ProductionID=%d AND EditionID=%d AND P.SectionID=%d", nProductionID, nEditionID, nSectionID);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {
			int f = 0;

			Rs.GetFieldValue((short)f++, s);
			nPublicationID = atoi(s);

			Rs.GetFieldValue((short)f++, DBvar, SQL_C_TIMESTAMP);
			try {
				CTime t(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
				tPubDate = t;
			}
			catch (CException *ex) {}

			Rs.GetFieldValue((short)f++, s);
			sPublicationName = s;

			Rs.GetFieldValue((short)f++, s);
			bIsDirty = atoi(s);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::DeleteLogData(int nKeepDaysLog, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;
	CString sSQL;
	sSQL.Format("DELETE FROM ServiceLog WHERE DATEDIFF(day,EventTime,GETDATE()) > %d", nKeepDaysLog);

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Delete failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		g_util.Logprintf("ERROR: DeleteLogData returned error - %s", e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;

}

BOOL CDatabaseManager::GetOldProductions(CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);
	CString sSQL, s;
	CDBVariant DBvar;

	if (g_prefs.m_overrulekeepdays == -1)
		sSQL.Format("SELECT DISTINCT P.ProductionID,PUB.Name,P.PubDate,%d,DATEDIFF(day,P.PubDate,GETDATE()),P.PublicationID FROM ProductionNames AS PROD WITH (NOLOCK) INNER JOIN PageTable AS P WITH (NOLOCK) ON PROD.ProductionID=P.ProductionID INNER JOIN PublicationNames AS PUB with (NOLOCK) ON P.PublicationID=PUB.PublicationID WHERE DATEDIFF(day,P.PubDate,GETDATE()) >= 0", g_prefs.m_overrulekeepdays);
	else if (g_prefs.m_overrulekeepdays > 0)
		sSQL.Format("SELECT DISTINCT P.ProductionID,PUB.Name,P.PubDate,%d,DATEDIFF(day,P.PubDate,GETDATE()),P.PublicationID FROM ProductionNames AS PROD WITH (NOLOCK) INNER JOIN PageTable AS P WITH (NOLOCK) ON PROD.ProductionID=P.ProductionID INNER JOIN PublicationNames AS PUB with (NOLOCK) ON P.PublicationID=PUB.PublicationID WHERE DATEDIFF(day,P.PubDate,GETDATE()) >= %d", g_prefs.m_overrulekeepdays, g_prefs.m_overrulekeepdays);
	else
		sSQL = _T("SELECT DISTINCT P.ProductionID,PUB.Name,P.PubDate,PUB.AutoPurgeKeepDays,DATEDIFF(day,P.PubDate,GETDATE()),P.PublicationID FROM ProductionNames AS PROD WITH (NOLOCK) INNER JOIN PageTable AS P WITH (NOLOCK) ON PROD.ProductionID=P.ProductionID INNER JOIN PublicationNames AS PUB with (NOLOCK) ON P.PublicationID=PUB.PublicationID WHERE PUB.AutoPurgeKeepDays>0 AND (PUB.DefaultCopies & 0x08) = 0 AND DATEDIFF(day,P.PubDate,GETDATE()) >= PUB.AutoPurgeKeepDays");

	g_prefs.m_ProductList.RemoveAll();
	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)sSQL);
			return FALSE;
		}

		while (!Rs.IsEOF()) {
			PRODUCTSTRUCT *pItem = new PRODUCTSTRUCT();
			Rs.GetFieldValue((short)0, s);
			pItem->m_productionID = atoi(s);
			Rs.GetFieldValue((short)1, pItem->m_publicationName);

			Rs.GetFieldValue((short)2, DBvar, SQL_C_TIMESTAMP);
			CTime t(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
			pItem->m_pubDate = t;

			Rs.GetFieldValue((short)3, s);
			pItem->m_autoPurgeKeepDays = atoi(s);

			Rs.GetFieldValue((short)4, s);
			pItem->m_ageDays = atoi(s);

			Rs.GetFieldValue((short)5, s);
			pItem->m_PublicationID = atoi(s);

			g_prefs.m_ProductList.Add(*pItem);

			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::GetMasterSeparationSetNumbers(int nProductionID, int nPublicationID, CTime tPubDate, CUIntArray &aMasterSeparationSetList, CString &sErrorMessage)
{
	return GetMasterSeparationSetNumbers(nProductionID, nPublicationID, tPubDate, 0, 0, aMasterSeparationSetList, sErrorMessage);
}

BOOL CDatabaseManager::GetMasterSeparationSetNumbers(int nProductionID, int nPublicationID, CTime tPubDate, int nEditionID, int nSectionID, CUIntArray &aMasterSeparationSetList, CString &sErrorMessage)
{

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);
	CString sSQL, s;

	aMasterSeparationSetList.RemoveAll();

	if (nEditionID > 0 && nSectionID > 0)
		sSQL.Format("SELECT DISTINCT MasterCopySeparationSet FROM PageTable WITH (NOLOCK) WHERE ProductionID=%d AND PublicationID=%d AND PubDate='%s' AND EditionID=%d AND SectionID=%d", nProductionID, nPublicationID, TranslateDate(tPubDate), nEditionID, nSectionID);
	else if (nEditionID > 0 && nSectionID == 0)
		sSQL.Format("SELECT DISTINCT MasterCopySeparationSet FROM PageTable WITH (NOLOCK) WHERE ProductionID=%d AND PublicationID=%d AND PubDate='%s' AND EditionID=%d", nProductionID, nPublicationID, TranslateDate(tPubDate), nEditionID);
	else if (nEditionID == 0 && nSectionID > 0)
		sSQL.Format("SELECT DISTINCT MasterCopySeparationSet FROM PageTable WITH (NOLOCK) WHERE ProductionID=%d AND PublicationID=%d AND PubDate='%s' AND SectionID=%d", nProductionID, nPublicationID, TranslateDate(tPubDate), nSectionID);
	else
		sSQL.Format("SELECT DISTINCT MasterCopySeparationSet FROM PageTable WITH (NOLOCK) WHERE ProductionID=%d AND PublicationID=%d AND PubDate='%s'", nProductionID, nPublicationID, TranslateDate(tPubDate));

	if (nPublicationID == 0)
		sSQL.Format("SELECT DISTINCT MasterCopySeparationSet FROM PageTable WITH (NOLOCK) WHERE ProductionID=%d", nProductionID, nPublicationID, TranslateDate(tPubDate));

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)sSQL);
			return FALSE;
		}

		while (!Rs.IsEOF()) {
			Rs.GetFieldValue((short)0, s);
			aMasterSeparationSetList.Add(atoi(s));

			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}
BOOL CDatabaseManager::GetMasterSeparationSetNumbersEx(int nProductionID, int nPublicationID, CTime tPubDate, CUIntArray &aMasterSeparationSetList, CUIntArray &aInputIDList, CStringArray &aFileNameList, CString &sErrorMessage)
{
	return GetMasterSeparationSetNumbersEx(nProductionID, nPublicationID, tPubDate, 0, 0, aMasterSeparationSetList, aInputIDList, aFileNameList, sErrorMessage);
}

BOOL CDatabaseManager::GetMasterSeparationSetNumbersEx(int nProductionID, int nPublicationID, CTime tPubDate,
	int nEditionID, int nSectionID, CUIntArray &aMasterSeparationSetList, CUIntArray &aInputIDList, CStringArray &aFileNameList, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);
	CString sSQL, s;

	aMasterSeparationSetList.RemoveAll();
	aInputIDList.RemoveAll();
	aFileNameList.RemoveAll();

	if (nEditionID > 0 && nSectionID > 0)
		sSQL.Format("SELECT DISTINCT MasterCopySeparationSet,InputID,FileName FROM PageTable WITH (NOLOCK) WHERE UniquePage=1 AND ProductionID=%d AND PublicationID=%d AND PubDate='%s' AND EditionID=%d AND SectionID=%d", nProductionID, nPublicationID, TranslateDate(tPubDate), nEditionID, nSectionID);
	else if (nEditionID > 0 && nSectionID == 0)
		sSQL.Format("SELECT DISTINCT MasterCopySeparationSet,InputID,FileName FROM PageTable WITH (NOLOCK) WHERE UniquePage=1 AND ProductionID=%d AND PublicationID=%d AND PubDate='%s' AND EditionID=%d", nProductionID, nPublicationID, TranslateDate(tPubDate), nEditionID);
	else if (nEditionID == 0 && nSectionID > 0)
		sSQL.Format("SELECT DISTINCT MasterCopySeparationSet,InputID,FileName FROM PageTable WITH (NOLOCK) WHERE UniquePage=1 AND ProductionID=%d AND PublicationID=%d AND PubDate='%s' AND SectionID=%d", nProductionID, nPublicationID, TranslateDate(tPubDate), nSectionID);
	else
		sSQL.Format("SELECT DISTINCT MasterCopySeparationSet,InputID,FileName FROM PageTable WITH (NOLOCK) WHERE UniquePage=1 AND ProductionID=%d AND PublicationID=%d AND PubDate='%s'", nProductionID, nPublicationID, TranslateDate(tPubDate));

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)sSQL);
			return FALSE;
		}

		while (!Rs.IsEOF()) {
			Rs.GetFieldValue((short)0, s);
			aMasterSeparationSetList.Add(atoi(s));

			Rs.GetFieldValue((short)1, s);
			aInputIDList.Add(atoi(s));

			Rs.GetFieldValue((short)2, s);
			if (s == _T("filename"))
				s = _T("");
			aFileNameList.Add(s);

			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}


int CDatabaseManager::TestForOtherUniqueMasterPage(int nProductionID, int nMasterCopySeparationSet, CString &sErrorMessage)
{
	int ret = 0;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return -1;


	CRecordset Rs(m_pDB);
	CString sSQL, s;

	sSQL.Format("SELECT TOP 1 MasterCopySeparationSet FROM PageTable WITH (NOLOCK) WHERE MasterCopySeparationSet=%d AND ProductionID<>%d AND UniquePage=1 AND Status>0 AND Dirty=0", nMasterCopySeparationSet, nProductionID);
	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)sSQL);
			return -1;
		}

		if (!Rs.IsEOF()) {
			ret = 1;
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return -1;
	}

	return ret;
}

int CDatabaseManager::TestMasterStillInPageTable(int nMasterCopySeparationSet, CString &sErrorMessage)
{
	int ret = 0;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return -1;

	CRecordset Rs(m_pDB);
	CString sSQL, s;


	sSQL.Format("SELECT TOP 1 MasterCopySeparationSet FROM PageTable WITH (NOLOCK) WHERE MasterCopySeparationSet=%d ", nMasterCopySeparationSet);
	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)sSQL);
			return -1;
		}

		if (!Rs.IsEOF()) {
			ret = 1;
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return -1;
	}

	return ret;
}


BOOL CDatabaseManager::DeleteProduction(int nProductionID, int nPublicationID, CTime tPubDate, int nEditionID, int nSectionID, CString &sErrorMessage)
{
	CTime tPubDateToCheck = CTime(1975, 1, 1, 0, 0, 0);
	int nPublicationIDToChek = 0;
	CString sPublicationNameToCheck = _T("");
	BOOL bIsDirty = FALSE;

	int nRetries = g_prefs.m_nQueryRetries;
	BOOL bSuccess = FALSE;

	while (nRetries-- > 0 && bSuccess == FALSE) {

		bSuccess = CheckDeleteJob(nProductionID, nPublicationIDToChek, tPubDateToCheck, nEditionID, nSectionID, sPublicationNameToCheck, bIsDirty, sErrorMessage);
		if (bSuccess == FALSE) {
			g_util.Logprintf("ERROR: CheckDeleteJob() returned FALSE for ProductionID %d - %s", nProductionID, sErrorMessage);
			::Sleep(g_prefs.m_QueryBackoffTime);
		}
	}

	if (nPublicationIDToChek == 0 || tPubDateToCheck.GetYear() < 2000) {
		g_util.Logprintf("ERROR: CheckDeleteJob() did not find any jobs for ProductionID %d .. delete request ignored..", nProductionID);
		return TRUE;
	}

	if (nPublicationID != nPublicationIDToChek || tPubDate != tPubDateToCheck) {
		g_util.Logprintf("ERROR: DeleteProductionEx() : PublicationID/Pubdate Mismatch for ProductionID %d - db delete skipped (%d/%d %s/%s)", nProductionID,
								nPublicationID, nPublicationIDToChek, TranslateDate(tPubDate), TranslateDate(tPubDateToCheck));
		return FALSE;
	}

	nRetries = g_prefs.m_nQueryRetries;
	bSuccess = FALSE;
	while (nRetries-- > 0 && bSuccess == FALSE) {

		bSuccess = DeleteProductionEx(nProductionID, nPublicationID, tPubDate, nEditionID, nSectionID, sErrorMessage);
		if (bSuccess == FALSE) {
			g_util.Logprintf("ERROR: DeleteProductionEx() returned FALSE for ProductionID %d - %s", nProductionID, sErrorMessage);
			::Sleep(g_prefs.m_QueryBackoffTime);
		}
	}

	return bSuccess;
}



BOOL CDatabaseManager::DeleteProductionEx(int nProductionID, int nPublicationID, CTime tPubDate, int nEditionID, int nSectionID, CString &sErrorMessage)
{
	CString sSQL;

	sSQL.Format("{CALL spImportCenterCleanProduction (%d,1,%d,%d) }", nProductionID, nEditionID, nSectionID);

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		g_util.Logprintf("ERROR: spImportCenterCleanProduction returned error - %s", e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::RemoveDeleteJob(int nProductionID, int nPublicationID, CTime tPubDate, int nEditionID, int nSectionID, CString &sErrorMessage)
{
	CString sSQL;

	sErrorMessage =  _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	sSQL.Format("DELETE FROM ProductDeleteQueue WHERE ProductionID=%d AND PublicationID=%d AND PubDate='%s' AND EditionID=%d AND SectionID=%d",
								nProductionID, nPublicationID, TranslateDate(tPubDate), nEditionID, nSectionID);

	if (g_prefs.m_logging > 1)
		g_util.Logprintf("INFO:  RemoveDeleteJob() - %s", sSQL);

	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::MarkDirtyProduction(int nProductionID, int nEditionID, int nSectionID, CString &sErrorMessage)
{
	int nRetries = g_prefs.m_nQueryRetries;
	BOOL bSuccess = FALSE;
	while (nRetries-- > 0 && bSuccess == FALSE) {
		bSuccess = MarkDirtyProductionEx(nProductionID, nEditionID, nSectionID, sErrorMessage);
		if (bSuccess == FALSE)
			::Sleep(g_prefs.m_QueryBackoffTime);
	}

	return bSuccess;
}

BOOL CDatabaseManager::MarkDirtyProductionEx(int nProductionID, int nEditionID, int nSectionID, CString &sErrorMessage)
{
	CString sSQL;

	sErrorMessage =  _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	if (nEditionID == 0 && nSectionID == 0)
		sSQL.Format("UPDATE PageTable SET Dirty=1 WHERE ProductionID=%d", nProductionID);
	else if (nEditionID > 0 && nSectionID == 0)
		sSQL.Format("UPDATE PageTable SET Dirty=1 WHERE ProductionID=%d AND EditionID=%d", nProductionID, nEditionID);
	else if (nEditionID == 0 && nSectionID > 0)
		sSQL.Format("UPDATE PageTable SET Dirty=1 WHERE ProductionID=%d AND SectionID=%d", nProductionID, nSectionID);
	else
		sSQL.Format("UPDATE PageTable SET Dirty=1 WHERE ProductionID=%d AND EditionID=%d AND SectionID=%d", nProductionID, nEditionID, nSectionID);

	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::MarkDirtyProduction(int nProductionID, int nPublicationID, CTime tPubDate, int nEditionID, int nSectionID, CString &sErrorMessage)
{
	int nRetries = g_prefs.m_nQueryRetries;
	BOOL bSuccess = FALSE;
	while (nRetries-- > 0 && bSuccess == FALSE) {
		bSuccess = MarkDirtyProductionEx(nProductionID, nPublicationID, tPubDate, nEditionID, nSectionID, sErrorMessage);
		if (bSuccess == FALSE)
			::Sleep(g_prefs.m_QueryBackoffTime);
	}

	return bSuccess;
}

BOOL CDatabaseManager::MarkDirtyProductionEx(int nProductionID, int nPublicationID, CTime tPubDate, int nEditionID, int nSectionID, CString &sErrorMessage)
{
	CString sSQL;

	sErrorMessage =  _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	if (nEditionID == 0 && nSectionID == 0)
		sSQL.Format("UPDATE PageTable SET Dirty=1 WHERE ProductionID=%d AND PublicationID=%d AND PubDate='%s'", nProductionID, nPublicationID, TranslateDate(tPubDate));
	else if (nEditionID > 0 && nSectionID == 0)
		sSQL.Format("UPDATE PageTable SET Dirty=1 WHERE ProductionID=%d AND PublicationID=%d AND PubDate='%s' AND EditionID=%d", nProductionID, nPublicationID, TranslateDate(tPubDate), nEditionID);
	else if (nEditionID == 0 && nSectionID > 0)
		sSQL.Format("UPDATE PageTable SET Dirty=1 WHERE ProductionID=%d AND PublicationID=%d AND PubDate='%s' AND SectionID=%d", nProductionID, nPublicationID, TranslateDate(tPubDate), nSectionID);
	else
		sSQL.Format("UPDATE PageTable SET Dirty=1 WHERE ProductionID=%d AND PublicationID=%d AND PubDate='%s' AND EditionID=%d AND SectionID=%d", nProductionID, nPublicationID, TranslateDate(tPubDate), nEditionID, nSectionID);

	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::KillDirtyPages(CString &sErrorMessage)
{
	int nRetries = g_prefs.m_nQueryRetries;
	BOOL bSuccess = FALSE;
	while (nRetries-- > 0 && bSuccess == FALSE) {
		bSuccess = KillDirtyPagesEx(sErrorMessage);
		if (bSuccess == FALSE)
			::Sleep(g_prefs.m_QueryBackoffTime);
	}

	return bSuccess;
}

BOOL CDatabaseManager::KillDirtyPagesEx(CString &sErrorMessage)
{
	return KillDirtyPagesEx(0, sErrorMessage);
}

BOOL CDatabaseManager::KillDirtyPagesEx(int nProductionID, CString &sErrorMessage)
{
	CString sSQL;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	if (nProductionID == 0)
		sSQL.Format("{CALL spImportCenterCleanDirtyPages}");
	else
		sSQL.Format("{CALL spImportCenterCleanDirtyPages (%d) }", nProductionID);

	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}


BOOL CDatabaseManager::GetFileName(int nCopyFlatSeparation, CString &sFileName, CString &sErrorMessage)
{
	sFileName = "";

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);
	CString sSQL, s;

	sSQL.Format("SELECT TOP 1 FileName FROM PageTable WITH (NOLOCK) WHERE CopyFlatSeparationSet=%d", nCopyFlatSeparation);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {
			Rs.GetFieldValue((short)0, s);
			sFileName = s;
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::GetAutoRetryJob(int &nProductionID, CString &sErrorMessage)
{
	nProductionID = 0;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);
	CString sSQL, s;

	sSQL.Format("SELECT TOP 1 ProductionID FROM AutoRetryQueue WITH (NOLOCK) ORDER BY EventTime");

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {
			Rs.GetFieldValue((short)0, s);
			nProductionID = atoi(s);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::RemoveAutoRetryJob(int nProductionID, CString &sErrorMessage)
{
	CString sSQL;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	sSQL.Format("DELETE FROM AutoRetryQueue WHERE ProductionID=%d", nProductionID);
	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::GetErrorRootFolder(CString &sErrorRootFolder, CString &sErrorMessage)
{
	CString sSQL, s;

	sErrorRootFolder = "";

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);


	sSQL.Format("SELECT TOP 1 ServerErrorPath FROM GeneralPreferences");

	try {
		if (!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", (LPCSTR)sSQL);

			return FALSE;
		}
		if (!Rs.IsEOF()) {
			Rs.GetFieldValue((short)0, s);
			sErrorRootFolder = s;
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::GetFileNameChannel(CString sFileName, CString sChannelName, int &nChannelID, int &nMasterCopySeparationSet, CString &sErrorMessage)
{
	CString sSQL, s;
	nChannelID = 0;
	nMasterCopySeparationSet = 0;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);

	sSQL.Format("SELECT DISTINCT CN.ChannelID,CS.MasterCopySeparationSet FROM ChannelStatus CS WITH (NOLOCK) INNER JOIN PageTable P WITH(NOLOCK) ON P.MasterCopySeparationSet = CS.MasterCopySeparationSet INNER JOIN ChannelNames CN WITH(NOLOCK) ON CN.ChannelID = CS.ChannelID WHERE P.FileName = '%s' AND CN.Name = '%s'", sFileName,sChannelName);

	try {
		if (!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", (LPCSTR)sSQL);

			return FALSE;
		}
		if (!Rs.IsEOF()) {

			Rs.GetFieldValue((short)0, s);
			nChannelID = atoi(s);

			Rs.GetFieldValue((short)1, s);
			nMasterCopySeparationSet = atoi(s);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}


BOOL CDatabaseManager::GetInputFolders(CStringArray &sInputFolders, CStringArray &sInputIDs, CStringArray &sQueueName, CString &sErrorMessage)
{
	CString sSQL, s;
	sInputFolders.RemoveAll();
	sInputIDs.RemoveAll();
	sQueueName.RemoveAll();

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);

	sSQL.Format("SELECT DISTINCT InputPath,InputID,InputName FROM InputConfigurations WHERE Enabled<>0 ORDER BY InputID");

	try {
		if (!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", (LPCSTR)sSQL);

			return FALSE;
		}
		while (!Rs.IsEOF()) {

			Rs.GetFieldValue((short)0, s);
			sInputFolders.Add(s);

			Rs.GetFieldValue((short)1, s);
			sInputIDs.Add(s);
			Rs.GetFieldValue((short)2, s);
			sQueueName.Add(s);
			

			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}



BOOL CDatabaseManager::PlanLock(int bRequestedPlanLock, int &bCurrentPlanLock, CString &sClientName, CString &sClientTime, CString &sErrorMessage)
{
	CString sSQL;
	sClientTime = _T("");
	sClientName =  _T("");
	bCurrentPlanLock = -1;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);

	TCHAR buf[260];
	DWORD len = 260;
	::GetComputerName(buf, &len);

	sSQL.Format("{CALL spPlanningLock (%d,'ProductDeleteService %s')}", bRequestedPlanLock, buf);
	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {
			CString s;
			Rs.GetFieldValue((short)0, s);
			bCurrentPlanLock = atoi(s);
			Rs.GetFieldValue((short)1, sClientName);

			CDBVariant DBvar;
			Rs.GetFieldValue((short)2, DBvar, SQL_C_TIMESTAMP);
			sClientTime.Format("%.4d-%.2d-%.2d %.2d:%.2d:%.2d", DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		//	e->GetErrorMessage(buf,260);
		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	return TRUE;
}

int   CDatabaseManager::QueryProdLock(CString &sErrorMessage)
{
	CString sSQL, s;

	int nCurrentPlanLock = -1;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return -1;

	CRecordset Rs(m_pDB);

	TCHAR buf[260];
	DWORD len = 260;
	::GetComputerName(buf, &len);
	CString sMyClientName;
	sMyClientName.Format("ProductDeleteService %s", buf);

	sSQL.Format("SELECT PlanLock, PlanLockClient, DATEDIFF(minute,PlanLockTime,GETDATE()),PlanLockTimeoutSec/60 FROM GeneralPreferences");
	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)sSQL);
			return -1;
		}

		if (!Rs.IsEOF()) {
			Rs.GetFieldValue((short)0, s);
			nCurrentPlanLock = atoi(s);
			Rs.GetFieldValue((short)1, s);
			if (s.CompareNoCase(sMyClientName) == 0) // My own lock...
				nCurrentPlanLock = 0;

			if (nCurrentPlanLock == 1) {
				Rs.GetFieldValue((short)2, s);
				int nMinSinceLock = atoi(s);
				Rs.GetFieldValue((short)3, s);
				if (nMinSinceLock > atoi(s))	// Timeout..
					nCurrentPlanLock = 0;
			}
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		//e->GetErrorMessage(buf,260);
		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return -1;
	}

	return nCurrentPlanLock;
}

BOOL	CDatabaseManager::GetEmailInfo(int nPublicationID, CString &sEmailReceiver, CString &sEmailCC,
	CString &EmailSubject, CString &EmailBody, CString &sErrorMessage)
{
	sEmailReceiver = _T("");
	sEmailCC = _T("");
	EmailSubject = _T("");
	EmailBody = _T("");


	sErrorMessage =  _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);
	CString sSQL, s;

	int nRet = 0;

	sSQL.Format("SELECT ISNULL(EmailRecipient,''),ISNULL(EmailCC,''),ISNULL(EmailSubject,''),ISNULL(EmailBody,'') FROM PublicationNames  WHERE PublicationID=%d", nPublicationID);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {
			Rs.GetFieldValue((short)0, sEmailReceiver);
			Rs.GetFieldValue((short)1, sEmailCC);
			Rs.GetFieldValue((short)2, EmailSubject);
			Rs.GetFieldValue((short)3, EmailBody);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::SetUnknownFilesDirty(int nDirty, CString &sErrorMessage)
{
	CString sSQL, s;
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;


	sSQL.Format("UPDATE UnknownFiles SET Dirty=%d", nDirty);
	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::InsertUnknownFile(CString sFileName, CString sErrorFolder, CString sInputFolder, CTime tFileTime, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);
	CString sSQL, s;

	BOOL bExitingFile = FALSE;

	sSQL.Format("SELECT FileName FROM UnknownFiles WHERE FileName='%s' AND ErrorFolder='%s' AND InputFolder='%s'", sFileName, sErrorFolder, sInputFolder);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {

			Rs.GetFieldValue((short)0, s);
			bExitingFile = TRUE;
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	if (bExitingFile)

		sSQL.Format("UPDATE UnknownFiles SET Dirty=0,EventTime=GETDATE() WHERE FileName='%s' AND ErrorFolder='%s' AND InputFolder='%s'",
			sFileName, sErrorFolder, sInputFolder);
	else
		sSQL.Format("INSERT INTO UnknownFiles (FileName,ErrorFolder,InputFolder,FileTime,Dirty,EventTime,Retry) VALUES ('%s','%s','%s','%s',0,GETDATE(),0)",
			sFileName, sErrorFolder, sInputFolder, TranslateTime(tFileTime));
	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::DeleteDirtyUnknownFiles(CString &sErrorMessage)
{

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CString sSQL, s;

	sSQL.Format("DELETE FROM UnknownFiles WHERE Dirty=1");
	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}




int CDatabaseManager::FieldExists(CString sTableName, CString sFieldName , CString &sErrorMessage)
{
	CString sSQL, s;
	BOOL bFound = FALSE;
	sErrorMessage = _T("");

	if (InitDB(sErrorMessage) == FALSE)
		return -1;
	
	CRecordset Rs(m_pDB);
	sSQL.Format("{ CALL sp_columns ('%s') }", (LPCSTR)sTableName);

	try {		
		if(!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", (LPCSTR)sSQL);
			return FALSE;
		}
		while (!Rs.IsEOF()) {
			
			Rs.GetFieldValue((short)3, s);

			if (s.CompareNoCase(sFieldName) == 0) {
				bFound = TRUE;
				break;
			}
			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch( CDBException* e ) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch( CDBException* e ) {
			// So what..! Go on!;
		}
		return -1;
	}

	return bFound ? 1 : 0;
}

int CDatabaseManager::TableExists(CString sTableName, CString &sErrorMessage)
{
	CString sSQL, s;
	BOOL bFound = FALSE;
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return -1;
	
	CRecordset Rs(m_pDB);
	sSQL.Format("{ CALL sp_tables ('%s') }", (LPCSTR)sTableName);

	try {		
		if(!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", (LPCSTR)sSQL);
			return FALSE;
		}
		if (!Rs.IsEOF()) {
			
			bFound = TRUE;
		}
		Rs.Close();
	}
	catch( CDBException* e ) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch( CDBException* e ) {
			// So what..! Go on!;
		}
		return -1;
	}

	return bFound ? 1 : 0;
}

int CDatabaseManager::StoredProcParameterExists(CString sSPname, CString sParameterName, CString &sErrorMessage)
{
	CString sSQL, s;
	BOOL bFound = FALSE;
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return -1;
	
	CRecordset Rs(m_pDB);
	sSQL.Format("{ CALL sp_sproc_columns ('%s') }", (LPCSTR)sSPname);

	try {		
		if(!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", (LPCSTR)sSQL);
			return FALSE;
		}
		while (!Rs.IsEOF()) {
			
			Rs.GetFieldValue((short)3, s);

			if (s.CompareNoCase(sParameterName) == 0) {
				bFound = TRUE;
				break;
			}
			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch( CDBException* e ) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch( CDBException* e ) {
			// So what..! Go on!;
		}
		return -1;
	}

	return bFound ? 1 : 0;
}

BOOL CDatabaseManager::RunCustomScript(CString &sErrorMessage)
{
	CString sSQL;
	sErrorMessage = _T("");
	sSQL.Format("{CALL spCustomMaintenance }");
	BOOL bSuccess = FALSE;
	int nRetries = g_prefs.m_nQueryRetries;

	while (!bSuccess && nRetries-- > 0) {
		if (InitDB(sErrorMessage) == FALSE) {
			Sleep(g_prefs.m_QueryBackoffTime);
			continue;
		}
		try {
			m_pDB->ExecuteSQL(sSQL);
			bSuccess = TRUE;
			break;
		}
		catch (CDBException* e) {
			CString s = e->m_strError;
			if (s.GetLength() > MAX_ERRMSG)
				s = s.Left(MAX_ERRMSG - 1);
			sErrorMessage.Format("Script failed - %s", (LPCSTR)s);

			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			try {
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			::Sleep(g_prefs.m_QueryBackoffTime);;
		}
	}

	return bSuccess;
}


BOOL CDatabaseManager::RunContinousCustomScript(CString sCustomSP, CString &sErrorMessage)
{
	CString sSQL;
	sErrorMessage = _T("");
	sSQL.Format("{CALL %s }", sCustomSP);
	BOOL bSuccess = FALSE;
	int nRetries = g_prefs.m_nQueryRetries;

	while (!bSuccess && nRetries-- > 0) {
		if (InitDB(sErrorMessage) == FALSE) {
			Sleep(g_prefs.m_QueryBackoffTime);
			continue;
		}
		try {
			m_pDB->ExecuteSQL(sSQL);
			bSuccess = TRUE;
			break;
		}
		catch (CDBException* e) {
			CString s = e->m_strError;
			if (s.GetLength() > MAX_ERRMSG)
				s = s.Left(MAX_ERRMSG - 1);
			sErrorMessage.Format("Script failed - %s", (LPCSTR)s);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			try {
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime);;
		}
	}

	return bSuccess;
}

BOOL CDatabaseManager::InsertPlanLogEntry(int nProcessID, int nEventCode, CString sProductionName, CString sPageDescription, int nProductionID, CString sUserName, CString &sErrorMessage)
{

	CString sSQL;

	sErrorMessage = _T("");

	sSQL.Format("INSERT INTO Log (EventTime, ProcessID,Event,Separation,FlatSeparation, ErrorMsg,[FileName],Version,MiscInt,MiscString) VALUES (GETDATE(), %d,%d,0,0,'%s','%s',1,%d,'%s')", nProcessID, nEventCode, sPageDescription, sProductionName, nProductionID, sUserName);


	if (InitDB(sErrorMessage) == FALSE) {
		return FALSE;
	}
	try {
		m_pDB->ExecuteSQL(sSQL);

	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}


	return TRUE;
}


BOOL CDatabaseManager::GetProductParameters(int nProductionID, int &nPublicationID, CString &sPublication, CTime &tPubDate, CString &sErrorMessage)
{
	CString sSQL, s;

	sPublication = "";
	nPublicationID = 0;

	CDBVariant DBvar;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);


	sSQL.Format("SELECT TOP 1 PublicationNames.Name,PageTable.PublicationID,PubDate FROM PageTable INNER JOIN PublicationNames ON PageTable.PublicationID=PublicationNames.PublicationID WHERE ProductionID=%d", nProductionID);

	try {
		if (!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", (LPCSTR)sSQL);

			return FALSE;
		}
		if (!Rs.IsEOF()) {
			Rs.GetFieldValue((short)0, s);
			sPublication = s;

			Rs.GetFieldValue((short)1, s);
			nPublicationID = atoi(s);

			Rs.GetFieldValue((short)2, DBvar, SQL_C_TIMESTAMP);
			try {
				CTime t(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
				tPubDate = t;
			}
			catch (CException *ex2)
			{
			}
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}


BOOL CDatabaseManager::GetNewProductions(CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);
	CString sSQL, s;
	CDBVariant DBvar;

	sSQL = _T("SELECT DISTINCT P.ProductionID,ISNULL(IA.ShortName,PUB.Name),P.PubDate,P.PublicationID FROM PageTable AS P WITH (NOLOCK) INNER JOIN PublicationNames AS PUB with (NOLOCK) ON P.PublicationID=PUB.PublicationID LEFT OUTER JOIN InputAliases IA ON IA.LongName=PUB.Name AND Type='Publication' WHERE DATEDIFF(day,GETDATE(),P.PubDate) >= 0 AND Dirty=0 AND DATEPART(year,P.PubDate) < 2100");
	g_prefs.m_ProductList.RemoveAll();
	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)sSQL);
			return FALSE;
		}

		while (!Rs.IsEOF()) {
			PRODUCTSTRUCT *pItem = new PRODUCTSTRUCT();
			Rs.GetFieldValue((short)0, s);
			pItem->m_productionID = atoi(s);
			Rs.GetFieldValue((short)1, pItem->m_publicationName);

			Rs.GetFieldValue((short)2, DBvar, SQL_C_TIMESTAMP);
			CTime t(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
			pItem->m_pubDate = t;

			Rs.GetFieldValue((short)3, s);
			pItem->m_PublicationID = atoi(s);

			g_prefs.m_ProductList.Add(*pItem);

			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	if (g_prefs.m_ProductList.GetCount() == 0 && g_prefs.m_overrulekeepdays == 0)
		return GetOldProductionsLastPlate(sErrorMessage);

	return TRUE;
}


BOOL CDatabaseManager::GetOldProductionsLastPlate(CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);
	CString sSQL, s;
	CDBVariant DBvar;

	sSQL = _T("SELECT DISTINCT P.ProductionID, PUB.Name, P.PubDate, PUB.AutoPurgeKeepDays, DATEDIFF(day, P.PubDate, GETDATE()), P.PublicationID, MAX(OutputTime) FROM ProductionNames AS PROD WITH(NOLOCK) INNER JOIN PageTable AS P WITH(NOLOCK) ON PROD.ProductionID = P.ProductionID INNER JOIN PublicationNames AS PUB with(NOLOCK) ON P.PublicationID = PUB.PublicationID WHERE P.Status = 50 AND DATEPART(year, P.OutputTime) > 2000 AND PUB.AutoPurgeKeepDays>0 AND(PUB.DefaultCopies & 0x08) > 0  GROUP BY P.ProductionID, PUB.Name, P.PubDate, PUB.AutoPurgeKeepDays, P.PublicationID");

	g_prefs.m_ProductList.RemoveAll();
	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)sSQL);
			return FALSE;
		}

		while (!Rs.IsEOF()) {

			Rs.GetFieldValue((short)0, s);
			int productionID = atoi(s);
			Rs.GetFieldValue((short)1, s);
			CString publicationName = s;

			Rs.GetFieldValue((short)2, DBvar, SQL_C_TIMESTAMP);
			CTime pubdate(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
			Rs.GetFieldValue((short)3, s);
			int autoPurgeKeepDays = atoi(s);
			Rs.GetFieldValue((short)4, s);
			int ageDays = atoi(s);
			Rs.GetFieldValue((short)5, s);
			int publicationID = atoi(s);

			Rs.GetFieldValue((short)6, DBvar, SQL_C_TIMESTAMP);
			CTime maxoutputtime(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);

			CTime tNow = CTime::GetCurrentTime();
			CTimeSpan ts = tNow - maxoutputtime;

			if (autoPurgeKeepDays > 0 && ts.GetTotalHours() / 24 > autoPurgeKeepDays) {

				PRODUCTSTRUCT *pItem = new PRODUCTSTRUCT();
				Rs.GetFieldValue((short)0, s);
				pItem->m_productionID = productionID;
				pItem->m_publicationName = publicationName;
				pItem->m_pubDate = pubdate;
				pItem->m_ageDays = ageDays;
				pItem->m_PublicationID = publicationID;

				g_prefs.m_ProductList.Add(*pItem);
			}

			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}


BOOL CDatabaseManager::MarkIllegalPageInProduction(BOOL bSetMark, int nProductionID, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CString sSQL, s;

	if (bSetMark)
		sSQL.Format("UPDATE PressRunID SET PlanVersion=(PlanVersion | 0x008) WHERE PressRunID IN (SELECT DISTINCT PressRunID FROM PageTable WITH (NOLOCK) WHERE ProductionID=%d)", nProductionID);
	else
		sSQL.Format("UPDATE PressRunID SET PlanVersion=(PlanVersion & 0xfff7) WHERE PressRunID IN (SELECT DISTINCT PressRunID FROM PageTable WITH (NOLOCK) WHERE ProductionID=%d)", nProductionID);
	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}


BOOL CDatabaseManager::SetIllegalFilesDirty(int nDirty, CString &sErrorMessage)
{
	CString sSQL, s;
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;


	sSQL.Format("UPDATE IllegalFiles SET Dirty=%d", nDirty);
	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::InsertIllegalFile(CString sFileName, CString sErrorFolder, CString sInputFolder, CTime tFileTime, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);
	CString sSQL, s;

	BOOL bExitingFile = FALSE;

	sSQL.Format("SELECT FileName FROM IllegalFiles WHERE FileName='%s' AND ErrorFolder='%s' AND InputFolder='%s'", sFileName, sErrorFolder, sInputFolder);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {

			Rs.GetFieldValue((short)0, s);
			bExitingFile = TRUE;
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	if (bExitingFile)

		sSQL.Format("UPDATE IllegalFiles SET Dirty=0,EventTime=GETDATE() WHERE FileName='%s' AND ErrorFolder='%s' AND InputFolder='%s'",
			sFileName, sErrorFolder, sInputFolder);
	else
		sSQL.Format("INSERT INTO IllegalFiles (FileName,ErrorFolder,InputFolder,FileTime,Dirty,EventTime,Retry) VALUES ('%s','%s','%s','%s',0,GETDATE(),0)",
			sFileName, sErrorFolder, sInputFolder, TranslateTime(tFileTime));
	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}


BOOL CDatabaseManager::DeleteDirtyIllegalFiles(CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CString sSQL, s;

	sSQL.Format("DELETE FROM IllegalFiles WHERE Dirty=1");
	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}




BOOL CDatabaseManager::GetProductFileNames(int nProductionID, CStringArray &aFileNames, CStringArray &aFileNames2, CStringArray &aFileNames3, CString sPrefix, CString &sErrorMessage)
{
	CString sSQL, s;
	CString sFileName;
	CString sFileName2;
	CString sFileName3;

	aFileNames.RemoveAll();
	aFileNames2.RemoveAll();
	aFileNames3.RemoveAll();

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);

	sSQL.Format("SELECT DISTINCT ED.Name,SEC.Name,PageIndex FROM PageTable P WITH (NOLOCK) INNER JOIN EditionNames ED ON Ed.EditionID=P.EditionID INNER JOIN SectionNames SEC ON SEC.SectionID=P.SectionID WHERE ProductionID=%d AND Dirty=0", nProductionID);
	try {
		if (!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", (LPCSTR)sSQL);

			return FALSE;
		}
		while (!Rs.IsEOF()) {
			Rs.GetFieldValue((short)0, s);
			CString sEdition = s;
			Rs.GetFieldValue((short)1, s);
			CString sSection = s;
			Rs.GetFieldValue((short)2, s);
			int nPage = atoi(s);
			sFileName.Format("%s-%s-%s-%d.pdf", sPrefix, sEdition, sSection, nPage);
			sFileName2.Format("%s-%s-%s-%.2d.pdf", sPrefix, sEdition, sSection, nPage);
			sFileName3.Format("%s-%s-%s-%.3d.pdf", sPrefix, sEdition, sSection, nPage);

			aFileNames.Add(sFileName);
			aFileNames2.Add(sFileName2);
			aFileNames3.Add(sFileName3);

			Rs.MoveNext();

		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

int CDatabaseManager::CountMasterCopySeparationSet(int nMasterCopySeparationSet, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return -1;

	CRecordset Rs(m_pDB);
	CString sSQL, s;

	int nCountMasterSet = -1;

	sSQL.Format("SELECT COUNT(Separation) FROM PageTable WITH (NOLOCK) WHERE MasterCopySeparationSet=%d", nMasterCopySeparationSet);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)sSQL);
			return -1;
		}

		if (!Rs.IsEOF()) {

			Rs.GetFieldValue((short)0, s);
			nCountMasterSet = atoi(s);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return -1;
	}

	return nCountMasterSet;
}

BOOL CDatabaseManager::GetMaxVersionCount(int nProductionID, int nPublicationID, CTime tPubDate, int &nMaxVersion, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);
	CString sSQL, s;

	nMaxVersion = 20;

	sSQL.Format("SELECT MAX(Version) FROM PageTable WITH (NOLOCK) WHERE ProductionID=%d AND PublicationID=%d AND PubDate='%s'", nProductionID, nPublicationID, TranslateDate(tPubDate));
	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {
			Rs.GetFieldValue((short)0, s);
			nMaxVersion = atoi(s);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}


int CDatabaseManager::HasMasterCopySeparationSet(int nMasterCopySeparationSet, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return -1;

	CRecordset Rs(m_pDB);
	CString sSQL, s;

	BOOL bHasMasterSet = FALSE;

	sSQL.Format("SELECT TOP 1 Separation FROM PageTable WITH (NOLOCK) WHERE MasterCopySeparationSet=%d", nMasterCopySeparationSet);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)sSQL);
			return -1;
		}

		if (!Rs.IsEOF()) {

			Rs.GetFieldValue((short)0, s);
			bHasMasterSet = TRUE;
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return -1;
	}

	return bHasMasterSet ? 1 : 0;
}



BOOL CDatabaseManager::GetPDFArchiveRootFolder(CString &sPDFArchiveFolder, CString &sPDFErrorFolder, CString &sPDFUnknownFolder, CString &sPDFInputFolder, CString &sTiffArchiveFolder, CString &sTiffPlateArchiveFolder, CString &sErrorMessage)
{
	CString sSQL, s;

	sPDFArchiveFolder = _T("");
	sPDFErrorFolder = _T("");
	sPDFUnknownFolder = _T("");
	sPDFInputFolder = _T("");
	sTiffArchiveFolder = _T("");
	sTiffPlateArchiveFolder = _T("");

	sErrorMessage = _T("");

	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);
	sSQL.Format("SELECT TOP 1 ISNULL(PDFArchiveFolder,''),ISNULL(PDFErrorFolder,''),ISNULL(PDFUnknownFolder,''),ISNULL(PDFInputFolder,''),ISNULL(TiffArchiveFolder,''),ISNULL(TiffPlateArchiveFolder,'')  FROM GeneralPreferencesExtra");

	try {
		if (!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", (LPCSTR)sSQL);

			return FALSE;
		}
		if (!Rs.IsEOF()) {
			Rs.GetFieldValue((short)0, s);
			sPDFArchiveFolder = s;

			Rs.GetFieldValue((short)1, s);
			sPDFErrorFolder = s;

			Rs.GetFieldValue((short)2, s);
			sPDFUnknownFolder = s;

			Rs.GetFieldValue((short)3, s);
			sPDFInputFolder = s;

			Rs.GetFieldValue((short)4, s);
			sTiffArchiveFolder = s;

			Rs.GetFieldValue((short)5, s);
			sTiffPlateArchiveFolder = s;
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::GetPlanExportJobs(CUIntArray &aProductionIDList, int nMiscInt, CString &sErrorMessage)
{
	aProductionIDList.RemoveAll();

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);
	CString sSQL, s;

	sSQL.Format("SELECT TOP 1 ProductionID, EventTime FROM PlanExportJobs WHERE MiscInt = %d ORDER BY EventTime", nMiscInt);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)sSQL);
			return FALSE;
		}

		while (!Rs.IsEOF()) {
			int f = 0;
			Rs.GetFieldValue((short)f++, s);
			aProductionIDList.Add(atoi(s));

			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::RemovePlanExportJob(int nProductionID, int nMiscInt, CString &sErrorMessage)
{
	CString sSQL;

	sErrorMessage = _T("");

	sSQL.Format("DELETE FROM PlanExportJobs WHERE ProductionID=%d AND MiscInt=%d", nProductionID, nMiscInt);

	if (InitDB(sErrorMessage) == FALSE) {
		return FALSE;
	}
	try {
		m_pDB->ExecuteSQL(sSQL);

	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Delete failed - %s", (LPCSTR)e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::GetInputMaskList(int nInputID, int nPublicationID, CStringArray &sMaskList, CStringArray &sMaskDateFormatList, CString &sErrorMessage)
{
	CString sSQL, s, s2;

	sMaskList.RemoveAll();
	sMaskDateFormatList.RemoveAll();

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);

	sSQL.Format("SELECT DISTINCT Mask,MaskDateFormat FROM InputAutoRetryMasks WHERE  PublicationID=%d", nInputID, nPublicationID);

	try {
		if (!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", (LPCSTR)sSQL);

			return FALSE;
		}
		while (!Rs.IsEOF()) {

			Rs.GetFieldValue((short)0, s);
			Rs.GetFieldValue((short)1, s2);

			CString s1 = "";

			// Second expression?
			int m = s.Find(";");
			if (m > 0 && s.GetLength() > m + 1) {		// xxx;yyy
				s1 = s.Mid(m + 1);
				s = s.Left(m);
			}

			if (s != "" && s2 != "") {
				sMaskList.Add(s);
				sMaskDateFormatList.Add(s2);
			}

			if (s1 != "" && s2 != "") {
				sMaskList.Add(s1);
				sMaskDateFormatList.Add(s2);
			}

			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}


int CDatabaseManager::GetMasterSetFromFileName(CString sFileName, CString &sErrorMessage)
{
	CString sSQL, s;

	int nMasterCopySeparationSet = 0;
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return -1;

	CRecordset Rs(m_pDB);

	sSQL.Format("SELECT TOP 1 MiscInt FROM ServiceLog WHERE FileName='%s' AND Event=10", sFileName.Trim());

	try {
		if (!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", (LPCSTR)sSQL);
			return -1;
		}
		if (!Rs.IsEOF()) {
			Rs.GetFieldValue((short)0, s);
			nMasterCopySeparationSet = atoi(s);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return -1;
	}

	return nMasterCopySeparationSet;
}

CString CDatabaseManager::GetInputAlias(CString sType, CString sLongName, CString &sErrorMessage)
{
	CString sSQL, s;

	CString sShortName = sLongName;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return sShortName;

	CRecordset Rs(m_pDB);

	if (sType == _T("Publication"))
		sSQL.Format("SELECT TOP 1 InputAlias FROM PublicationNames WHERE Name='%s'", sLongName);
	else
	sSQL.Format("SELECT TOP 1 ShortName FROM InputAliases WHERE Type='%s' AND LongName='%s'", sType, sLongName);

	try {
		if (!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", (LPCSTR)sSQL);
			return sShortName;
		}
		if (!Rs.IsEOF()) {
			Rs.GetFieldValue((short)0, s);
			sShortName = s;
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return sShortName;
	}

	return sShortName;
}

BOOL CDatabaseManager::LoadProductionData(int nProductionID, PlanData *pPlan,  CString &sErrorMessage)
{

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);
	CString sSQL, s;
	CDBVariant DBvar;

	sSQL.Format("SELECT PubDate,Publication,Edition,Press,PressRunID,Copies,[Section],PageName,Pagination,PageIndex,Version,UniquePage,PageType,Priority,PageID,MasterPageID,Color,PressRunComment,PressRunOrderNumber,PlanType,PressTime, Template,SheetNumber,SheetSide,PagePosition,PressSectionNumber, PagesAcross, PagesDown,PlanPageName,CopyFlatSeparationSet,TimedEditionFrom,TimedEditionTo,EditionOrderNumber,ZoneMasterEdition,PressTower,PressZone FROM vwPageTableSeparations WHERE ProductionID=%d AND PageType <>'Dummy' ORDER BY PressRunID,Edition,[Section],Pagination,ColorOrder", nProductionID);


	CString prevEdition = "";
	CString prevSection = "";
	CString prevPage = "";
	CString prevColor = "";

	PlanDataEdition *pEdition = NULL;
	PlanDataSection  *pSection = NULL;
	PlanDataPage *pPage = NULL;
	PlanDataPageSeparation *pSeparation = NULL;

	pPlan->customeralias = "";
	pPlan->customername = "";
	pPlan->pagenameprefix = "";

	CString thisEdition;
	CString thisPress;
	CString thisSection;
	CString thisPage;
	CString thisUnique;
	CString thisPageType;
	CString thisColor;
	CString thisPressRunComment;
	CString thisTemplate;
	CString thisSheetSide;
	CString thisSheetName;
	CString thisPlanPageName;
	CString thisTimedEditionFrom;
	CString thisTimedEditionTo;
	CString thisZoneMasterEdition;
	CString thisPressTower;
	CString thisPressZone;

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)sSQL);
			return FALSE;
		}

		while (!Rs.IsEOF()) {
			int f = 0;
			CTime pubdate = CTime(1975, 1, 1);
			try {
				Rs.GetFieldValue((short)f++, DBvar, SQL_C_TIMESTAMP);
				pubdate = CTime(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
			}
			catch (CException *ex)
			{
				;
			}
			pPlan->publicationDate = pubdate;

			Rs.GetFieldValue((short)f++, pPlan->publicationName);
			Rs.GetFieldValue((short)f++, thisEdition);
			Rs.GetFieldValue((short)f++, thisPress);

			Rs.GetFieldValue((short)f++, s);
			int thisPressRunID = atoi(s);

			Rs.GetFieldValue((short)f++, s);
			int copies = atoi(s);

			Rs.GetFieldValue((short)f++, thisSection);
			Rs.GetFieldValue((short)f++, thisPage);

			Rs.GetFieldValue((short)f++, s);
			int thisPagination = atoi(s);
			Rs.GetFieldValue((short)f++, s);
			int thisPageindex = atoi(s);
			Rs.GetFieldValue((short)f++, s);
			int thisVersion = atoi(s);

			Rs.GetFieldValue((short)f++, thisUnique);
			Rs.GetFieldValue((short)f++, thisPageType);

			Rs.GetFieldValue((short)f++, s);
			int thisPriority = atoi(s);
			Rs.GetFieldValue((short)f++, s);
			int thisPageID = atoi(s);
			Rs.GetFieldValue((short)f++, s);
			int thisMasterPageID = atoi(s);

			Rs.GetFieldValue((short)f++, thisColor);
			Rs.GetFieldValue((short)f++, thisPressRunComment);
			Rs.GetFieldValue((short)f++, pPlan->planID);

			Rs.GetFieldValue((short)f++, s);
			int  thisPlanType = atoi(s);
			CTime thisPressTime = CTime(1975, 1, 1);
			try {
				Rs.GetFieldValue((short)f++, DBvar, SQL_C_TIMESTAMP);
				thisPressTime = CTime(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
			}
			catch (CException *ex)
			{
				;
			}

			Rs.GetFieldValue((short)f++, thisTemplate);
			Rs.GetFieldValue((short)f++, s);
			int  thisSheetNumber = atoi(s);

			Rs.GetFieldValue((short)f++, thisSheetSide);


			Rs.GetFieldValue((short)f++, s);
			int  thisPagePosition = atoi(s);

			Rs.GetFieldValue((short)f++, s);
			int  thisPressSectionNumber = atoi(s);

			Rs.GetFieldValue((short)f++, s);
			int  thisTemplatePagesX = atoi(s);
			Rs.GetFieldValue((short)f++, s);
			int  thisTemplatePagesY = atoi(s);

			Rs.GetFieldValue((short)f++, thisPlanPageName);

			Rs.GetFieldValue((short)f++, s);
			int  thisCopyFlatSeparationSet = atoi(s);

			Rs.GetFieldValue((short)f++, thisTimedEditionFrom);
			Rs.GetFieldValue((short)f++, thisTimedEditionTo);

			Rs.GetFieldValue((short)f++, s);
			int     thisEditionOrderNumber = atoi(s);

			Rs.GetFieldValue((short)f++, thisZoneMasterEdition);
			Rs.GetFieldValue((short)f++, thisPressTower);
			Rs.GetFieldValue((short)f++, thisPressZone);


			if (thisEdition != prevEdition) {
				pEdition = new PlanDataEdition(thisEdition);
				if (thisEditionOrderNumber <= 0)
					thisEditionOrderNumber = 1;
				pEdition->IsTimed = thisTimedEditionFrom != "" || thisTimedEditionTo != "";
				pEdition->timedFrom = thisTimedEditionFrom;
				pEdition->timedTo = thisTimedEditionTo;
				pEdition->editionSequenceNumber = thisEditionOrderNumber;
				pEdition->zoneMaster = thisZoneMasterEdition;
				PlanDataPress *pPress = new PlanDataPress(thisPress);
				pPress->copies = copies;
				pPress->paperCopies = atoi(thisPressRunComment);

				pPress->postalUrl = thisPressRunComment;
				pPress->pressRunTime = thisPressTime;
				pEdition->pressList->Add(*pPress);
				pPlan->editionList->Add(*pEdition);
				prevSection = "";
				prevPage = "";
				prevColor = "";

				prevEdition = thisEdition;
			}

			if (thisSection != prevSection) {
				pSection = new PlanDataSection(thisSection);

				pSection->pagesInSection = 0;
				pEdition->sectionList->Add(*pSection);

				prevSection = thisSection;
				prevPage = "";
				prevColor = "";
			}

			

			if (thisPage != prevPage) {

				pPage = new PlanDataPage(thisPage);
				pPage->pagination = thisPagination;
				pPage->pageIndex = thisPageindex;
				pPage->version = thisVersion;
				if (thisUnique == "Common")
					pPage->uniquePage = PAGEUNIQUE_COMMON;
				else if (thisUnique == "Forced")
					pPage->uniquePage = PAGEUNIQUE_FORCED;
				else
					pPage->uniquePage = PAGEUNIQUE_UNIQUE;

				if (thisPageType == "Dummy")
					pPage->pageType = PAGETYPE_DUMMY;
				else if (thisPageType == "Panorama")
					pPage->pageType = PAGETYPE_PANORAMA;
				else if (thisPageType == "Antipanorama")
					pPage->pageType = PAGETYPE_ANTIPANORAMA;
				else
					pPage->pageType = PAGETYPE_NORMAL;

				pPage->priority = thisPriority;
				pPage->pageID.Format("%d", thisPageID);
				pPage->masterPageID.Format("%d", thisMasterPageID);
				pPage->planPageName = thisPlanPageName;

				pSection->pageList->Add(*pPage);
				prevPage = thisPage;
				prevColor = "";



			}

			if (thisColor != prevColor) {
				pSeparation = new PlanDataPageSeparation(thisColor);
				pPage->colorList->Add(*pSeparation);

				prevColor = thisColor;
			}


			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

void CDatabaseManager::CalculatePagePositions(int pagePosition, int pagesAcross, int pagesDown, int &xpos, int &ypos)
{
	xpos = 1;
	ypos = 1;

	if (pagePosition == 1)
		return;

	// 1-up
	if (pagesAcross == 1 && pagesDown == 1)
		return;


	if (pagesAcross == 2 && pagesDown == 1)
	{
		xpos = 2;
		ypos = 1;
	}
	else if (pagesAcross == 1 && pagesDown == 2)
	{
		xpos = 1;
		ypos = 2;
	}
	else if (pagesAcross == 2 && pagesDown == 2)
	{
		if (pagePosition == 2)
		{
			xpos = 2;
			ypos = 1;
		}
		else if (pagePosition == 3)
		{
			xpos = 1;
			ypos = 2;
		}
		else
		{
			xpos = 2;
			ypos = 2;
		}
	}

	else if (pagesAcross == 4 && pagesDown == 2)
	{
		if (pagePosition <= 4)
		{
			xpos = pagePosition;
			ypos = 1;
		}
		else
		{
			xpos = pagePosition - 4;
			ypos = 2;
		}
	}
	else if (pagesAcross == 2 && pagesDown == 4)
	{
		if (pagePosition <= 2)
		{
			xpos = pagePosition;
			ypos = 1;
		}
		else if (pagePosition >= 3 && pagePosition <= 4)
		{
			xpos = pagePosition - 2;
			ypos = 2;
		}
		else if (pagePosition >= 5 && pagePosition <= 6)
		{
			xpos = pagePosition - 4;
			ypos = 3;
		}
		else
		{
			xpos = pagePosition - 6;
			ypos = 4;
		}
	}

	else if (pagesAcross == 4 && pagesDown == 4)
	{
		if (pagePosition <= 4)
		{
			xpos = pagePosition;
			ypos = 1;
		}
		else if (pagePosition >= 5 && pagePosition <= 8)
		{
			xpos = pagePosition - 4;
			ypos = 2;
		}
		else if (pagePosition >= 9 && pagePosition <= 12)
		{
			xpos = pagePosition - 8;
			ypos = 3;
		}
		else
		{
			xpos = pagePosition - 12;
			ypos = 4;
		}
	}
}


/////////////////////////////////////////////////
// Translate CTime to SQL Server DATETIME type
/////////////////////////////////////////////////

CString CDatabaseManager::TranslateDate(CTime tDate)
{
	CString dd,mm,yy,yysmall;

	dd.Format("%.2d",tDate.GetDay());
	mm.Format("%.2d",tDate.GetMonth());
	yy.Format("%.4d",tDate.GetYear());
	yysmall.Format("%.2d",tDate.GetYear());

	return mm + "/" + dd + "/" + yy;

}

CString CDatabaseManager::TranslateTime(CTime tDate)
{
	CString dd,mm,yy,yysmall, hour, min, sec;

	dd.Format("%.2d",tDate.GetDay());
	mm.Format("%.2d",tDate.GetMonth());
	yy.Format("%.4d",tDate.GetYear());
	yysmall.Format("%.2d",tDate.GetYear());

	hour.Format("%.2d",tDate.GetHour());
	min.Format("%.2d",tDate.GetMinute());
	sec.Format("%.2d",tDate.GetSecond());

	return mm + "/" + dd + "/" + yy + " " + hour + ":" + min + ":" + sec;
}

void CDatabaseManager::LogprintfDB(const TCHAR *msg, ...)
{
	TCHAR	szLogLine[16000], szFinalLine[16000];
	va_list	ap;
	DWORD	n, nBytesWritten;
	SYSTEMTIME	ltime;


	HANDLE hFile = ::CreateFile(g_prefs.m_logFilePath + _T("\\MaintServiceDBerror.log"), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return;

	// Seek to end of file
	::SetFilePointer(hFile, 0, NULL, FILE_END);

	va_start(ap, msg);
	n = vsprintf(szLogLine, msg, ap);
	va_end(ap);
	szLogLine[n] = '\0';

	::GetLocalTime(&ltime);
	_stprintf(szFinalLine, "[%.2d-%.2d %.2d:%.2d:%.2d.%.3d] %s\r\n", (int)ltime.wDay, (int)ltime.wMonth, (int)ltime.wHour, (int)ltime.wMinute, (int)ltime.wSecond, (int)ltime.wMilliseconds, szLogLine);

	::WriteFile(hFile, szFinalLine, (DWORD)_tcsclen(szFinalLine), &nBytesWritten, NULL);

	::CloseHandle(hFile);

#ifdef _DEBUG
	TRACE(szFinalLine);
#endif
}



BOOL CDatabaseManager::GetFileRetryJobs(FILERETRYLIST &aFileRetryList, CString &sErrorMessage)
{
	aFileRetryList.RemoveAll();

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);
	CString sSQL, s, sFileName, sQueueName;
	CDBVariant DBvar;

	sSQL.Format("SELECT Type, FileName, QueueName, EventTime FROM RetryQueue WITH (NOLOCK) ORDER BY EventTime");

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)sSQL);
			return FALSE;
		}

		while (!Rs.IsEOF()) {
			Rs.GetFieldValue((short)0, s);
			int nType = atoi(s);
			Rs.GetFieldValue((short)1, sFileName);
			Rs.GetFieldValue((short)2, sQueueName);

			CTime tEventTime = CTime(1975, 1, 1);
			try {
				Rs.GetFieldValue((short)3, DBvar, SQL_C_TIMESTAMP);
				tEventTime = CTime(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
			}
			catch (CException *ex)
			{
				;
			}

			FILERETRYSTRUCT *pItem = new FILERETRYSTRUCT();
			pItem->nType = nType;
			pItem->sFileName = sFileName;
			pItem->sQueueName = sQueueName;
			pItem->tEventTime = tEventTime;

			aFileRetryList.Add(*pItem);
			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::RemoveFileRetryJobs(int nType, CString sFileName, CString sQueueName,  CString &sErrorMessage)
{
	CString sSQL;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	sSQL.Format("DELETE FROM RetryQueue WHERE Type=%d AND FileName='%s' AND QueueName='%s'", nType, sFileName, sQueueName);
	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::UpdateChannelStatus(int nMasterCopySeparationSet, int nChannelID, int nStatus, CString &sErrorMessage)
{
	CString sSQL;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	sSQL.Format("UPDATE ChannelStatus SET ExportStatus=%d,EventTime=GetDate() WHERE MasterCopySeparationSet=%d AND ChannelID=%d", nStatus, nMasterCopySeparationSet, nChannelID);
	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::ResetProofStatus(int nMasterCopySeparationSet, CString &sErrorMessage)
{
	CString sSQL;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	sSQL.Format("UPDATE PageTable  SET ProofStatus=0 WHERE MasterCopySeparationSet=%d", nMasterCopySeparationSet);
	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}


BOOL CDatabaseManager::ResetProcessingStatus(int nMasterCopySeparationSet,  int nPdfType, CString &sErrorMessage)
{
	CString sSQL;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	sSQL.Format("UPDATE ChannelStatus SET ProcessingStatus=0,EventTime=GetDate() WHERE MasterCopySeparationSet=%d", nMasterCopySeparationSet);
	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}


	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	if (nPdfType == POLLINPUTTYPE_LOWRESPDF)
		sSQL.Format("UPDATE PageTable SET StatusPdfLowres=0,CheckSumPdfLowres=0 WHERE MasterCopySeparationSet=%d", nMasterCopySeparationSet);
	else
		sSQL.Format("UPDATE PageTable SET StatusPdfPrint=0,CheckSumPdfPrint=0 WHERE MasterCopySeparationSet=%d", nMasterCopySeparationSet);
	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::UpdateProductionFlag(int nProductionID, int nFlag, CString &sErrorMessage)
{
	CString sSQL;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	sSQL.Format("UPDATE ProductionNames SET Combined=%d WHERE ProductionID=%d", nFlag, nProductionID);
	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	return TRUE;
}

BOOL CDatabaseManager::RemoveCustomMessageJob(int nPublicationID, CTime tPubdate, int nEditionID, int nSectionID, int nLocationID, int nMessageType, CString &sErrorMessage)
{
	int nRetries = g_prefs.m_nQueryRetries;

	while (nRetries-- > 0) {
		BOOL ret = RemoveCustomMessageJob2(nPublicationID, tPubdate, nEditionID, nSectionID, nLocationID, nMessageType, sErrorMessage);
		if (ret == TRUE)
			return ret;
		Sleep(g_prefs.m_QueryBackoffTime);
	}
	return FALSE;
}

BOOL CDatabaseManager::RemoveCustomMessageJob2(int nPublicationID, CTime tPubdate, int nEditionID, int nSectionID, int nLocationID, int nMessageType, CString &sErrorMessage)
{
	CString sSQL;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	sSQL.Format("DELETE FROM CustomMessageQueue WHERE PublicationID=%d AND PubDate='%s' AND EditionID=%d AND SectionID=%d AND LocationID=%d AND MessageType=%d", nPublicationID, TranslateDate(tPubdate), nEditionID, nSectionID, nLocationID, nMessageType);
	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	return TRUE;
}


BOOL CDatabaseManager::GetNextCustomMessage(int &nPublicationID, CTime &tPubDate, int &nEditionID, int &nSectionID, int &nLocationID, int &nMessageType, int &nMiscInt, CString &sMiscString, CString &sErrorMessage)
{
	nPublicationID = 0;
	tPubDate = CTime(1975, 1, 1, 0, 0, 0);
	nEditionID = 0;
	nSectionID = 0;
	nLocationID = 0;
	nMessageType = -1;
	nMiscInt = 0;
	sMiscString = _T("");

	sErrorMessage = _T("");

	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset *Rs = NULL;
	CString sSQL, s;

	sSQL.Format("SELECT TOP 1 PublicationID,PubDate,EditionID,SectionID,LocationID, MessageType,EventTime,MiscInt,MiscString FROM CustomMessageQueue ORDER BY EventTime DESC");

	BOOL bSuccess = FALSE;

	int nRetries = g_prefs.m_nQueryRetries;

	while (!bSuccess && nRetries-- > 0) {

		try {
			if (InitDB(sErrorMessage) == FALSE) {
				Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
				continue;
			}

			Rs = new CRecordset(m_pDB);

			if (!Rs->Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
				sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
				Sleep(g_prefs.m_QueryBackoffTime);
				delete Rs;
				Rs = NULL;
				continue;
			}
			if (!Rs->IsEOF()) {
				Rs->GetFieldValue((short)0, s);
				nPublicationID = atoi(s);

				CDBVariant DBvar;
				Rs->GetFieldValue((short)1, DBvar, SQL_C_TIMESTAMP);
				try {
					CTime t(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
					tPubDate = t;
				}
				catch (CException *ex2)
				{
				}
				Rs->GetFieldValue((short)2, s);
				nEditionID = atoi(s);
				Rs->GetFieldValue((short)3, s);
				nSectionID = atoi(s);
				Rs->GetFieldValue((short)4, s);
				nLocationID = atoi(s);
				Rs->GetFieldValue((short)5, s);
				nMessageType = atoi(s);
				Rs->GetFieldValue((short)6, s); // EventTime

				Rs->GetFieldValue((short)7, s);
				nMiscInt = atoi(s);
				Rs->GetFieldValue((short)8, sMiscString);
			}
			Rs->Close();
			bSuccess = TRUE;
		}

		catch (CDBException* e) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();

			try {
				if (Rs->IsOpen())
					Rs->Close();
				delete Rs;
				Rs = NULL;
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime);
			continue;
		}

	}

	if (Rs != NULL)
		delete Rs;

	return bSuccess;
}

BOOL CDatabaseManager::RemoveFileSendRequest(__int64 n64GUID, CString &sErrorMessage)
{
	CString sSQL;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	sSQL.Format("DELETE FROM SendRequestQueue WHERE GUID=%I64d", n64GUID);
	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	return TRUE;
}

BOOL CDatabaseManager::GetNextFileSendRequest(SENDREQUEST &aSentRequest, CString &sErrorMessage)
{
	aSentRequest.n64GUID = 0;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset *Rs = NULL;
	CString s;

	CString sSQL = "SELECT TOP  1 [GUID], [UseFTP], [FTPServer], [FTPUsername], [FTPPassword], [FTPFolder], [FtpUsePASV], [SMBFolder], [EventTime],SourceFile,DeleteFileAfter FROM SendRequestQueue ORDER BY EventTime";
	BOOL bSuccess = FALSE;

	int nRetries = g_prefs.m_nQueryRetries;

	while (!bSuccess && nRetries-- > 0) {

		try {
			if (InitDB(sErrorMessage) == FALSE) {
				::Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
				continue;
			}

			Rs = new CRecordset(m_pDB);

			if (!Rs->Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
				sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
				::Sleep(g_prefs.m_QueryBackoffTime);
				delete Rs;
				Rs = NULL;
				continue;
			}
			if (!Rs->IsEOF()) {
				Rs->GetFieldValue((short)0, s);
				aSentRequest.n64GUID = _atoi64(s);
				Rs->GetFieldValue((short)1, s);
				aSentRequest.bUseFtp = atoi(s);
				Rs->GetFieldValue((short)2, aSentRequest.sFtpServer);
				Rs->GetFieldValue((short)3, aSentRequest.sFtpUserName);
				Rs->GetFieldValue((short)4, aSentRequest.sFtpPassword);
				Rs->GetFieldValue((short)5, aSentRequest.sFtpFolder);
				Rs->GetFieldValue((short)6, s);
				aSentRequest.bPasv = atoi(s);
				Rs->GetFieldValue((short)7, aSentRequest.sSMBFolder);

				CDBVariant DBvar;
				Rs->GetFieldValue((short)8, DBvar, SQL_C_TIMESTAMP);
				try {
					CTime t(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
					aSentRequest.tEventTime = t;
				}
				catch (CException *ex2)
				{
				}
				Rs->GetFieldValue((short)9, aSentRequest.sSourceFile);
				Rs->GetFieldValue((short)10, s);
				aSentRequest.bDeleteFileAfter = atoi(s);

			}
			Rs->Close();
			bSuccess = TRUE;
		}

		catch (CDBException* e) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();

			try {
				if (Rs->IsOpen())
					Rs->Close();
				delete Rs;
				Rs = NULL;
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime);
			continue;
		}

	}

	if (Rs != NULL)
		delete Rs;

	return bSuccess;
}



BOOL CDatabaseManager::GetProductionID(int nPublicationID, CTime tPubDate, int &nProductionID, CString &sErrorMessage)
{
	nProductionID = 0;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset *Rs = NULL;
	CString sSQL, s;

	sSQL.Format("SELECT TOP 1 ProductionID FROM PageTable WITH (NOLOCK) WHERE PublicationID=%d AND PubDate='%s'", nPublicationID, TranslateDate(tPubDate));

	BOOL bSuccess = FALSE;

	int nRetries = g_prefs.m_nQueryRetries;

	while (!bSuccess && nRetries-- > 0) {

		try {
			if (InitDB(sErrorMessage) == FALSE) {
				::Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
				continue;
			}

			Rs = new CRecordset(m_pDB);

			if (!Rs->Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
				sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
				::Sleep(g_prefs.m_QueryBackoffTime);
				delete Rs;
				Rs = NULL;
				continue;
			}
			if (!Rs->IsEOF()) {
				Rs->GetFieldValue((short)0, s);
				nProductionID = atoi(s);
			}
			Rs->Close();
			bSuccess = TRUE;
		}

		catch (CDBException* e) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();

			try {
				if (Rs->IsOpen())
					Rs->Close();
				delete Rs;
				Rs = NULL;
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime);
			continue;
		}

	}

	if (Rs != NULL)
		delete Rs;

	return bSuccess;
}

BOOL CDatabaseManager::GetProductionParameters(int nProductionID, CString &sComment, int &nVersion, CString &sErrorMessage)
{
	sComment = _T("");
	nVersion = 1;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset *Rs = NULL;
	CString sSQL, s;

	sSQL.Format("SELECT TOP 1 Sections,Bindingstyle FROM ProductionNames WITH (NOLOCK) WHERE ProductionID=%d", nProductionID);

	BOOL bSuccess = FALSE;

	int nRetries = g_prefs.m_nQueryRetries;

	while (!bSuccess && nRetries-- > 0) {

		try {
			if (InitDB(sErrorMessage) == FALSE) {
				Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
				continue;
			}

			Rs = new CRecordset(m_pDB);

			if (!Rs->Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
				sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
				Sleep(g_prefs.m_QueryBackoffTime);
				delete Rs;
				Rs = NULL;
				continue;
			}
			if (!Rs->IsEOF()) {
				Rs->GetFieldValue((short)0, s);
				sComment = s;
				Rs->GetFieldValue((short)1, s);
				nVersion = atoi(s);
			}
			Rs->Close();
			bSuccess = TRUE;
		}

		catch (CDBException* e) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();

			try {
				if (Rs->IsOpen())
					Rs->Close();
				delete Rs;
				Rs = NULL;
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime);
			continue;
		}
	}

	if (Rs != NULL)
		delete Rs;

	return bSuccess;
}


BOOL CDatabaseManager::GetIDNames(int nPublicationID, int nEditionID, CString &sPublication, CString &sPublicationAlias, CString &sPublicationExtendedAlias, CString &sPublicationExtendedAlias2, CString &sEdition, CString &sEditionAlias, CString &sUploadFolder, CString &sErrorMessage)
{
	sPublication = _T("");
	sPublicationAlias = _T("");
	sEdition = _T("");
	sEditionAlias = _T("");
	sUploadFolder = _T("");
	sPublicationExtendedAlias = _T("");
	sPublicationExtendedAlias2 = _T("");
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset *Rs = NULL;
	CString sSQL, s;

	sSQL.Format("SELECT TOP 1 [Name],UploadFolder,InputAlias,ExtendedAlias,ExtendedAlias2 FROM PublicationNames WHERE PublicationID=%d", nPublicationID);

	BOOL bSuccess = FALSE;

	int nRetries = g_prefs.m_nQueryRetries;

	while (!bSuccess && nRetries-- > 0) {

		try {
			if (InitDB(sErrorMessage) == FALSE) {
				Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
				continue;
			}

			Rs = new CRecordset(m_pDB);

			if (!Rs->Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
				sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
				Sleep(g_prefs.m_QueryBackoffTime);
				delete Rs;
				Rs = NULL;
				continue;
			}
			if (!Rs->IsEOF()) {
				Rs->GetFieldValue((short)0, sPublication);
				Rs->GetFieldValue((short)1, sUploadFolder);
				Rs->GetFieldValue((short)2, sPublicationAlias);
				Rs->GetFieldValue((short)3, sPublicationExtendedAlias);
				Rs->GetFieldValue((short)4, sPublicationExtendedAlias2);
			}
			Rs->Close();
			bSuccess = TRUE;
		}

		catch (CDBException* e) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();

			try {
				if (Rs->IsOpen())
					Rs->Close();
				delete Rs;
				Rs = NULL;
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime);
			continue;
		}

	}

	if (Rs != NULL)
		delete Rs;

	if (sPublicationAlias == _T(""))
		sPublicationAlias = sPublication;


	if (nEditionID > 0 && bSuccess) {
		sSQL.Format("SELECT TOP 1 [Name] FROM EditionNames WHERE EditionID=%d", nEditionID);

		bSuccess = FALSE;

		nRetries = g_prefs.m_nQueryRetries;

		while (!bSuccess && nRetries-- > 0) {

			try {
				if (InitDB(sErrorMessage) == FALSE) {
					Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
					continue;
				}

				Rs = new CRecordset(m_pDB);

				if (!Rs->Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
					sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
					Sleep(g_prefs.m_QueryBackoffTime);
					delete Rs;
					Rs = NULL;
					continue;
				}
				if (!Rs->IsEOF()) {
					Rs->GetFieldValue((short)0, sEdition);
				}
				Rs->Close();
				bSuccess = TRUE;
			}

			catch (CDBException* e) {
				sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
				LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
				e->Delete();

				try {
					if (Rs->IsOpen())
						Rs->Close();
					delete Rs;
					Rs = NULL;
					m_DBopen = FALSE;
					m_pDB->Close();
				}
				catch (CDBException* e) {
					// So what..! Go on!;
				}
				Sleep(g_prefs.m_QueryBackoffTime);
				continue;
			}
		}

		if (Rs != NULL)
			delete Rs;

		sEditionAlias = sEdition;
		if (bSuccess)
			sEditionAlias = LoadSpecificAlias("Edition", sEdition, sErrorMessage);

		if (sEditionAlias == _T(""))
			sEditionAlias = sEdition;
	}

	return bSuccess;
}

BOOL CDatabaseManager::GetChannelInfo(int nProductionID, int nChannelID, CTime &tReleaseTime, int &nTriggerMode, CString &sErrorMessage)
{
	tReleaseTime = CTime(1975, 1, 1);
	nTriggerMode = TRIGGERMODE_NONE;

	sErrorMessage = _T("");

	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset *Rs = NULL;
	CString sSQL, s;

	sSQL.Format("SELECT DISTINCT CS.ReleaseTime, CN.TriggerMode FROM ChannelStatus CS WITH (NOLOCK) INNER JOIN PageTable P WITH (NOLOCK) ON P.MasterCopySeparationSet = CS.MasterCopySeparationSet INNER JOIN ChannelNames CN WITH (NOLOCK) ON CN.ChannelID = CS.ChannelID WHERE P.ProductionID = %d AND CS.ChannelID = %d", nProductionID, nChannelID);

	BOOL bSuccess = FALSE;

	int nRetries = g_prefs.m_nQueryRetries;

	while (!bSuccess && nRetries-- > 0) {

		try {
			if (InitDB(sErrorMessage) == FALSE) {
				Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
				continue;
			}

			Rs = new CRecordset(m_pDB);

			if (!Rs->Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
				sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
				::Sleep(g_prefs.m_QueryBackoffTime);
				delete Rs;
				Rs = NULL;
				continue;
			}
			if (!Rs->IsEOF()) {
				CDBVariant DBvar;
				Rs->GetFieldValue((short)0, DBvar, SQL_C_TIMESTAMP);
				try {
					CTime t(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
					tReleaseTime = t;
				}
				catch (CException *ex2)
				{
				}

				Rs->GetFieldValue((short)1, s);
				nTriggerMode = atoi(s);
			}
			Rs->Close();
			bSuccess = TRUE;
		}

		catch (CDBException* e) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();

			try {
				if (Rs->IsOpen())
					Rs->Close();
				delete Rs;
				Rs = NULL;
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime);
			continue;
		}
	}

	if (Rs != NULL)
		delete Rs;

	return bSuccess;
}


BOOL CDatabaseManager::GetPublicationCleanDays(CString &sErrorMessage)
{
	CString sSQL, s;
	g_prefs.m_ChannelPurgeList.RemoveAll();

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);

	sSQL.Format("SELECT PUB.AutoPurgeKeepDaysArchive, PUB.InputAlias,PUB.OutputAlias,PUB.ExtendedAlias,PUB.ExtendedAlias2,PC.ChannelID FROM PublicationNames PUB WITH (NOLOCK) INNER JOIN PublicationChannels PC WITH (NOLOCK) ON PUB.PublicationID = PC.PublicationID INNER JOIN ChannelNames CN WITH (NOLOCK) ON CN.ChannelID = PC.ChannelID WHERE PUB.AutoPurgeKeepDaysArchive > 0 AND  CN.DeleteOldOutputFilesDays>0");

	try {
		if (!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", (LPCSTR)sSQL);

			return FALSE;
		}
		while (!Rs.IsEOF()) {
			CHANNELPURGESTRUCT *pItem = new CHANNELPURGESTRUCT();


			Rs.GetFieldValue((short)0, s);
			pItem->m_nAutoPurgeKeepDaysArchive = atoi(s);
			Rs.GetFieldValue((short)1, pItem->m_sInputAlias.MakeUpper());
			Rs.GetFieldValue((short)2, pItem->m_sOutputAlias.MakeUpper());
			Rs.GetFieldValue((short)3, pItem->m_sExtendedAlias.MakeUpper());
			Rs.GetFieldValue((short)4, pItem->m_sExtendedAlias2.MakeUpper());
			Rs.GetFieldValue((short)5, s);
			pItem->m_nChannelID = atoi(s);
			g_prefs.m_ChannelPurgeList.Add(*pItem);

			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): SELECT failed - %s", (LPCSTR)e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::DeletePresentQueuedFiles(int nType, CString sInputSetupName, CString &sErrorMessage)
{
	CString sSQL;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	sSQL.Format("DELETE FROM QueuedFiles WHERE Type=%d AND QueueName='%s'", nType, sInputSetupName);
	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Delete failed - %s", e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	return TRUE;

}

BOOL CDatabaseManager::InsertPresentQueuedFiles(int nType, CString sInputSetupName, CString sFileName, CString &sErrorMessage)
{
	CString sSQL;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	sSQL.Format("INSERT INTO QueuedFiles (Type,QueueName,[FileName]) VALUES (%d,'%s','%s')", nType, sInputSetupName, sFileName);
	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Delete failed - %s", e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	return TRUE;

}