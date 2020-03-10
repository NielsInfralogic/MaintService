#pragma once
#include "Stdafx.h"
#include "DatabaseManager.h"


class CPrefs
{
public:
	CPrefs(void);
	~CPrefs(void);

	void	LoadIniFile(CString sIniFile);
	void	LoadIniFileConvert(CString sIniFile);
	BOOL	ConnectToDB(BOOL bLoadDBNames, CString &sErrorMessage);

	BOOL LoadPreferencesFromRegistry();

	CString GetPublicationName(CDatabaseManager &DB, int nID);
	CString GetPublicationName(int nID);
	CString GetPublicationNameNoReload(int nID);
	PUBLICATIONSTRUCT *GetPublicationStruct(int nID);
	PUBLICATIONSTRUCT *GetPublicationStructNoDbLookup(int nID);
	PUBLICATIONSTRUCT *GetPublicationStruct(CDatabaseManager &DB, int nID);
	CString  GetExtendedAlias(int nPublicationID);
	void FlushPublicationName(CString sName);


	CString GetEditionName(int nID);
	CString GetEditionName(CDatabaseManager &DB, int nID);
	int		GetEditionID(CString s);
	int		GetEditionID(CDatabaseManager &DB, CString s);
	CString GetSectionName(int nID);
	CString GetSectionName(CDatabaseManager &DB, int nID);
	int		GetSectionID(CString s);
	int		GetSectionID(CDatabaseManager &DB, CString s);

	CString GetPublisherName(int nID);
	CString GetPublisherName(CDatabaseManager &DB, int nID);

	CString  LookupAbbreviationFromName(CString sType, CString sLongName);
	CString  LookupNameFromAbbreviation(CString sType, CString sShortName);
	CString		GetChannelName(int nID);
	CHANNELSTRUCT	*GetChannelStruct(int nID);
	CHANNELSTRUCT	*GetChannelStruct(CDatabaseManager &DB, int nID);
	CHANNELPURGESLIST m_ChannelPurgeList;
	
	int		GetPageFormatIDFromPublication(int nPublicationID);
	int		GetPageFormatIDFromPublication(CDatabaseManager &DB, int nPublicationID, BOOL bForceReload);

	CString FormCCFilesName(int nPDFtype, int nMasterCopySeparationSet, CString sFileName);
	CString FormCcdataFileNameLowres(CString sCCFileName, int nMasterCopySeparationSet);
	CString FormCcdataFileNamePrint(CString sCCFileName, int nMasterCopySeparationSet);
	CString FormCcdataFileNameHires(CString sCCFileName, int nMasterCopySeparationSet);


	
	CString m_setupXMLpath;
	CString m_setupEnableXMLpath;

	BOOL m_FieldExists_Log_FileName;
	BOOL m_FieldExists_Log_MiscString;

	CString	m_apppath;
	CString m_title;



	BOOL m_bypasspingtest;
	BOOL m_bypassreconnect;
	int m_lockcheckmode;
	DWORD m_maxlogfilesize;


	BOOL		m_logtodatabase;
	CString		m_DBserver;
	CString		m_Database;
	CString		m_DBuser;
	CString		m_DBpassword;
	BOOL		m_IntegratedSecurity;
	int			m_databaselogintimeout;
	int			m_databasequerytimeout;
	int			m_nQueryRetries;
	int			m_QueryBackoffTime;


	BOOL		m_persistentconnection;

	CString m_logFilePath;

	int			m_scripttimeout;




	int	m_instancenumber;

	BOOL	m_emailonfoldererror;
	BOOL	m_emailonfileerror;
	CString m_emailsmtpserver;
	int		m_mailsmtpport;
	CString m_mailsmtpserverusername;
	CString m_mailsmtpserverpassword;
	BOOL	m_mailusessl;
	CString m_emailfrom;
	CString m_emailto;
	CString m_emailcc;
	CString m_emailsubject;
	BOOL	m_emailpreventflooding;
	int		m_emailpreventfloodingdelay;
	int    m_emailtimeout;

	BOOL m_bypassdiskfreetest;


	BOOL m_firecommandonfolderreconnect;
	CString m_sFolderReconnectCommand;

	CString m_lowresPDFPath;
	CString m_hiresPDFPath;		// == CCFiles ServerShare
	CString m_printPDFPath;

	CString m_previewPath;
	CString m_thumbnailPath;
	CString m_serverName;
	CString m_serverShare;
	CString m_readviewpreviewPath;
	CString m_configPath;
	CString m_errorPath;
	CString m_webPath;
	BOOL   m_copyToWeb;
	int		m_mainserverusecussrentuser;
	CString m_mainserverusername;
	CString m_mainserverpassword;

	BOOL m_usecurrentuserweb;
	CString m_webFTPuser;
	CString m_webFTPpassword;

	PUBLICATIONLIST		m_PublicationList;
	ITEMLIST			m_EditionList;
	ITEMLIST			m_SectionList;
	FILESERVERLIST		m_FileServerList;
	ALIASLIST			m_AliasList;
	STATUSLIST			m_StatusList;
	STATUSLIST			m_ExternalStatusList;

	CHANNELLIST			m_ChannelList;
	ITEMLIST			m_PublisherList;
	PRODUCTLIST		m_ProductList;

	BOOL m_ccdatafilenamemode;

	CDatabaseManager m_DBmaint;
	int m_minMBfreeCCDATA;

	CString m_alivelogFilePath;

	BOOL m_bSortOnCreateTime;


	BOOL m_enableautodelete; 

	BOOL m_overrulekeepdays;

	OLDFOLDERLIST   m_OldFolderList;
	FOLDERNOTIFICATIONLIST  m_FolderNotificationList;

	BOOL		m_autoretry;
	BOOL		m_enabledeleteunrelatedfiles;
	int			m_autopurgeunrelatedhoursbegin;
	int			m_autopurgeunrelatedhoursend;


	BOOL		m_cleandirty;
	int			m_cleandirtyhoursbegin;
	int			m_cleandirtyhoursend;

	int			m_maxageunknownfileshours;
	BOOL		m_planlock;

	BOOL		m_enablecustomscript;
	BOOL		m_enablecontinouscustomscript;
	CString		m_continouscustomscript;
	BOOL		m_EnableDeleteUnrelatedFilesWeekly;
	int			m_weekday;
	CString		m_PlanExportFolder;
	BOOL		m_doexportplans;



	int			m_logging;

	BOOL		m_alwaysusecurrentuser;

	int			m_autopurgehoursend;
	int			m_autopurgehoursbegin;
	int			m_customscripthourbegin;
	int			m_customscripthourend;

	BOOL       m_verifyCRC;

	int			m_keepdayslog;


	CString m_CustomMessageFileName;
	CString m_CustomMessageTemplate;
	CString m_CustomMessageDateFormat;
	CString m_CustomMessageReleasePubDateFormat;
	CString m_CustomMessageReleaseDateFormat;
};

