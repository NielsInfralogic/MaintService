#pragma once
#include <afxdb.h>
#include "Defs.h"
#include "PlanExport.h"

class CDatabaseManager
{
public:
	CDatabaseManager(void);
	virtual ~CDatabaseManager(void);

	BOOL	InitDB(CString sDBserver, CString sDatabase, CString sDBuser, CString sDBpassword, BOOL bIntegratedSecurity, CString &sErrorMessage);
	BOOL	InitDB(CString &sErrorMessage);
	void	ExitDB();

	BOOL	IsOpen();
	BOOL	LoadConfigIniFile(int nInstanceNumber, CString &sFileName, CString &sFileName2, CString &sFileName3, CString &sErrorMessage);
	BOOL	RegisterService(CString &sErrorMessage);
	BOOL	UpdateService(int nCurrentState, CString sCurrentJob, CString sLastError, CString &sErrorMessage);
	BOOL	UpdateServiceDB(int nCurrentState, CString sCurrentJob, CString sLastError, CString sAddedLogData, CString &sErrorMessage);
	BOOL	UpdateServiceFileServer(CString sServer, int nCurrentState, CString sCurrentJob, CString sLastError, CString sAddedLogData, CString &sErrorMessage);

	BOOL	UpdateService(int nCurrentState, CString sCurrentJob, CString sLastError, CString sAddedLogData, CString &sErrorMessage);
	BOOL	InsertLogEntry(int nEvent, CString sSource, CString sFileName, CString sMessage, int nMasterCopySeparationSet, int nVersion, int nMiscInt, CString sMiscString, CString &sErrorMessage);
	BOOL	LoadSTMPSetup(CString &sErrorMessage);

	int		GetMasterSetFromFileName(CString sFileName, CString &sErrorMessage);
	BOOL	ResetProcessingStatus(int nMasterCopySeparationSet, int nPdfType, CString &sErrorMessage);
	BOOL	ResetProofStatus(int nMasterCopySeparationSet, CString &sErrorMessage);
	BOOL	GetNextCustomMessage(int &nPublicationID, CTime &tPubDate, int &nEditionID, int &nSectionID, int &nLocationID, int &nMessageType, int &nMiscInt, CString &sMiscString, CString &sErrorMessage);
	BOOL	RemoveCustomMessageJob(int nPublicationID, CTime tPubdate, int nEditionID, int nSectionID, int nLocationID, int nMessageType, CString &sErrorMessage);
	BOOL	RemoveCustomMessageJob2(int nPublicationID, CTime tPubdate, int nEditionID, int nSectionID, int nLocationID, int nMessageType, CString &sErrorMessage);
	BOOL	GetProductionID(int nPublicationID, CTime tPubDate, int &nProductionID, CString &sErrorMessage);
	BOOL	GetProductionParameters(int nProductionID, CString &sComment, int &nVersion, CString &sErrorMessage);
	BOOL	GetIDNames(int nPublicationID, int nEditionID, CString &sPublication, CString &sPublicationAlias,  CString &sPublicationExtendedAlias, CString &sPublicationExtendedAlias2, CString &sEdition, CString &sEditionAlias, CString &sUploadFolder, CString &sErrorMessage);

	BOOL	GetChannelInfo(int nProductionID, int nChannelID, CTime &tReleaseTime, int &nTriggerMode, CString &sErrorMessage);
	BOOL	GetPublicationCleanDays(CString &sErrorMessage);

	BOOL    LoadPublicationList(CString &sErrorMessage);
	BOOL	LoadEditionNameList(CString &sErrorMessage);
	BOOL	LoadSectionNameList(CString &sErrorMessage);
	BOOL	LoadAliasList(CString &sErrorMessage);
	BOOL    LoadStatusList(CString sIDtable, STATUSLIST &v, CString &sErrorMessage);
	BOOL	LoadPublicationChannels(int nID, CString &sErrorMessage);
	BOOL	LoadFileServerList(CString &sErrorMessage);
	int     LoadPublisherList(CString &sErrorMessage);
	int		LoadChannelList(CString &sErrorMessage);

	BOOL	LoadAllPrefs(CString &sErrorMessage);
	int		LoadGeneralPreferences(CString &sErrorMessage);

	CString LoadNewPublicationName(int nID, CString &sErrorMessage);
	CString LoadNewEditionName(int nID, CString &sErrorMessage);
	int		LoadNewEditionID(CString sName, CString &sErrorMessage);
	CString LoadNewSectionName(int nID, CString &sErrorMessage);
	int		LoadNewSectionID(CString sName, CString &sErrorMessage);
	BOOL    LoadNewChannel(int nChannelID, CString &sErrorMessage);
	CString LoadNewPublisherName(int nID, CString &sErrorMessage);



	CTime	GetDatabaseTime(CString &sErrorMessage);

	CString  LoadSpecificAlias(CString sType, CString sLongName, CString &sErrorMessage);

//	BOOL	GetJobDetails(CJobAttributes *pJob, int nCopySeparationSet, CString &sErrorMessage);

	int     GetChannelCRC(int nMasterCopySeparationSet, int nInputMode, CString &sErrorMessage);
	int		GetChannelStatus(int nMasterCopySeparationSet, int nChannelID, CString &sErrorMessage);
	BOOL	GetChannelsForMasterSet(int nMasterCopySeparationSet, int nPDFType, CUIntArray &aChannelIDList, CString &sErrorMessage);

	BOOL	HasDeleteJobQueued(CString &sErrorMessage);
	BOOL	GetNextDeleteJob(int &nProductionID, int &nPublicationID, CTime &tPubDate, int &nEditionID, int &nSectionID,
						BOOL &bSaveOldFiles, BOOL &bGenerateReport, CTime &tEventTime, CString &sArchiveFolder, CString &sMiscString, CString &sErrorMessage);
	BOOL	CheckDeleteJob(int nProductionID, int &nPublicationID, CTime &tPubDate, int nEditionID, int nSectionID, CString &sPublicationName, BOOL &bIsDirty, CString &sErrorMessage);
	BOOL	GetOldProductions(CString &sErrorMessage);
	BOOL	GetMasterSeparationSetNumbers(int nProductionID, int nPublicationID, CTime tPubDate, CUIntArray &aMasterSeparationSetList, CString &sErrorMessage);
	BOOL	GetMasterSeparationSetNumbers(int nProductionID, int nPublicationID, CTime tPubDate, int nEditionID, int nSectionID, CUIntArray &aMasterSeparationSetList, CString &sErrorMessage);
	BOOL	GetMasterSeparationSetNumbersEx(int nProductionID, int nPublicationID, CTime tPubDate, CUIntArray &aMasterSeparationSetList, CUIntArray &aInputIDList, CStringArray &aFileNameList, CString &sErrorMessage);
	BOOL	GetMasterSeparationSetNumbersEx(int nProductionID, int nPublicationID, CTime tPubDate, int nEditionID, int nSectionID, CUIntArray &aMasterSeparationSetList, CUIntArray &aInputIDList, CStringArray &aFileNameList, CString &sErrorMessage);
	int		TestForOtherUniqueMasterPage(int nProductionID, int nMasterCopySeparationSet, CString &sErrorMessage);
	int		TestMasterStillInPageTable(int nMasterCopySeparationSet, CString &sErrorMessage);
	BOOL	DeleteProduction(int nProductionID, int nPublicationID, CTime tPubDate, int nEditionID, int nSectionID, CString &sErrorMessage);
	BOOL	DeleteProductionEx(int nProductionID, int nPublicationID, CTime tPubDate, int nEditionID, int nSectionID, CString &sErrorMessage);
	BOOL	RemoveDeleteJob(int nProductionID, int nPublicationID, CTime tPubDate, int nEditionID, int nSectionID, CString &sErrorMessage);
	BOOL	MarkDirtyProduction(int nProductionID, int nEditionID, int nSectionID, CString &sErrorMessage);
	BOOL	MarkDirtyProductionEx(int nProductionID, int nEditionID, int nSectionID, CString &sErrorMessage);
	BOOL	MarkDirtyProduction(int nProductionID, int nPublicationID, CTime tPubDate, int nEditionID, int nSectionID, CString &ErrorMessage);
	BOOL	MarkDirtyProductionEx(int nProductionID, int nPublicationID, CTime tPubDate, int nEditionID, int nSectionID, CString &ErrorMessage);
	BOOL	KillDirtyPages(CString &sErrorMessage);
	BOOL	KillDirtyPagesEx(CString &sErrorMessage);
	BOOL	KillDirtyPagesEx(int nProductionID, CString &sErrorMessage);
	BOOL	GetFileName(int nCopyFlatSeparation, CString &sFileName, CString &sErrorMessage);
	BOOL	GetAutoRetryJob(int &nProductionID, CString &sErrorMessage);
	BOOL	RemoveAutoRetryJob(int nProductionID, CString &sErrorMessage);
	BOOL	GetErrorRootFolder(CString &sErrorRootFolder, CString &sErrorMessage);
	BOOL	GetInputFolders(CStringArray &sInputFolders, CStringArray &sInputIDs, CStringArray &sQueueName, CString &sErrorMessage);
	BOOL	PlanLock(int bRequestedPlanLock, int &bCurrentPlanLock, CString &sClientName, CString &sClientTime, CString &sErrorMessage);
	int		QueryProdLock(CString &sErrorMessage);
	BOOL	GetEmailInfo(int nPublicationID, CString &sEmailReceiver, CString &sEmailCC, CString &EmailSubject, CString &EmailBody, CString &sErrorMessage);
	BOOL	SetUnknownFilesDirty(int nDirty, CString &sErrorMessage);
	BOOL	InsertUnknownFile(CString sFileName, CString sErrorFolder, CString sInputFolder, CTime tFileTime, CString &sErrorMessage);
	BOOL	DeleteDirtyUnknownFiles(CString &sErrorMessage);

	BOOL	RunContinousCustomScript(CString sCustomSP, CString &sErrorMessage);
	BOOL	RunCustomScript(CString &sErrorMessage);

	BOOL	InsertPlanLogEntry(int nProcessID, int nEventCode, CString sProductionName, CString sPageDescription, int nProductionID, CString sUserName, CString &sErrorMessage);
	BOOL	GetProductParameters(int nProductionID, int &nPublicationID, CString &sPublication, CTime &tPubDate, CString &sErrorMessage);
	BOOL	GetPDFArchiveRootFolder(CString &sPDFArchiveFolder, CString &sPDFErrorFolder, CString &sPDFUnknownFolder, CString &sPDFInputFolder, CString &sTiffArchiveFolder, CString &sTiffPlateArchiveFolder, CString &sErrorMessage);
	BOOL	GetNewProductions(CString &sErrorMessage);
	BOOL	GetProductFileNames(int nProductionID, CStringArray &aFileNames, CStringArray &aFileNames2, CStringArray &aFileNames3, CString sPrefix, CString &sErrorMessage);
	BOOL	GetOldProductionsLastPlate(CString &sErrorMessage);
	BOOL	MarkIllegalPageInProduction(BOOL bSetMark, int nProductionID, CString &sErrorMessage);
	BOOL	DeleteDirtyIllegalFiles(CString &sErrorMessage);
	BOOL	InsertIllegalFile(CString sFileName, CString sErrorFolder, CString sInputFolder, CTime tFileTime, CString &sErrorMessage);
	BOOL	SetIllegalFilesDirty(int nDirty, CString &sErrorMessage);
	BOOL	GetPlanExportJobs(CUIntArray &aProductionIDList, int nMiscInt, CString &sErrorMessage);
	void	CalculatePagePositions(int pagePosition, int pagesAcross, int pagesDown, int &xpos, int &ypos);
	BOOL	LoadProductionData(int nProductionID, PlanData *pPlan, CString &sErrorMessage);
	BOOL	RemovePlanExportJob(int nProductionID, int nMiscInt, CString &sErrorMessage);
	BOOL	GetInputMaskList(int nInputID, int nPublicationID, CStringArray &sMaskList, CStringArray &sMaskDateFormatList, CString &sErrorMessage);
	CString GetInputAlias(CString sType, CString sLongName, CString &sErrorMessage);
	int		HasMasterCopySeparationSet(int nMasterCopySeparationSet, CString &sErrorMessage);
	int		CountMasterCopySeparationSet(int nMasterCopySeparationSet, CString &sErrorMessage);
	BOOL	GetMaxVersionCount(int nProductionID, int nPublicationID, CTime tPubDate, int &nMaxVersion, CString &sErrorMessage);

	int		StoredProcParameterExists(CString sSPname, CString sParameterName, CString &sErrorMessage);
	int		TableExists(CString sTableName, CString &sErrorMessage);
	int		FieldExists(CString sTableName, CString sFieldName , CString &sErrorMessage);
	CString TranslateDate(CTime tDate);
	CString TranslateTime(CTime tDate);
	void	LogprintfDB(const TCHAR *msg, ...);
	BOOL	GetFileRetryJobs(FILERETRYLIST &aFileRetryList, CString &sErrorMessage);
	BOOL	RemoveFileRetryJobs(int nType, CString sFileName, CString sQueueName, CString &sErrorMessage);

	BOOL	GetFileNameChannel(CString sFileName, CString sChannelName, int &nChannelID, int &nMasterCopySeparationSet, CString &sErrorMessage);
	BOOL	UpdateChannelStatus(int nMasterCopySeparationSet, int nChannelID, int nStatus, CString &sErrorMessage);
	BOOL	LoadMaintParamters(int nInstanceNumber, int &nDaysToKeep, int &nDaysToKeepLogData,
		CString &sVisioLinkTemplateFile, CString &sErrorMessage);
	BOOL	DeleteLogData(int nKeepDaysLog, CString &sErrorMessage);
	BOOL	RunCheckDB(CString &sResult, CString &sErrorMessage);

	BOOL	DeletePresentQueuedFiles(int nType, CString sInputSetupName, CString &sErrorMessage);

	BOOL	InsertPresentQueuedFiles(int nType, CString sInputSetupName, CString sFileName, CString &sErrorMessage);

	BOOL	GetNextFileSendRequest(SENDREQUEST &aSendRequest, CString &sErrorMessage);
	BOOL	RemoveFileSendRequest(__int64 n64GUID, CString &sErrorMessage);

	BOOL	UpdateProductionFlag(int nProductionID, int nFlag, CString &sErrorMessage);

private:
	BOOL	m_DBopen;
	CDatabase *m_pDB;

	BOOL	m_IntegratedSecurity;
	CString m_DBserver;
	CString m_Database;
	CString m_DBuser;
	CString m_DBpassword;
	BOOL	m_PersistentConnection;
};
