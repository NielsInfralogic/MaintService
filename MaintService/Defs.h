#pragma once

#include "stdafx.h"



#define NETWORKTYPE_SHARE	0
#define NETWORKTYPE_FTP		1
#define NETWORKTYPE_EMAIL	2

#define MAXREGEXP 100


#define TEMPFILE_NONE	0
#define TEMPFILE_NAME	1
#define TEMPFILE_FOLDER	2

#define FTPTYPE_NORMAL	0
#define FTPTYPE_FTPES	1
#define FTPTYPE_FTPS	2
#define FTPTYPE_SFTP	3

#define REMOTEFILECHECK_NONE		0
#define REMOTEFILECHECK_EXIST		1
#define REMOTEFILECHECK_SIZE		2
#define REMOTEFILECHECK_READBACK	3

#define TXNAMEOPTION_REMOVESPACES		1
#define TXNAMEOPTION_ALLSMALLCPAS		2
#define TXNAMEOPTION_ZIPPAGES			4
//#define TXNAMEOPTION_MERGEPAGES			8


#define TXNAMEOPTION_GENERATEXML		4
#define TXNAMEOPTION_SENDCOMMONPAGES	8


#define MAX_ERRMSG 2048
#define MAXCHANNELSPERPUBLICATION 100
#define MAXFILESINSCAN 5000

#define LOCKCHECKMODE_NONE		0
#define LOCKCHECKMODE_READ		1
#define LOCKCHECKMODE_READWRITE	2
#define LOCKCHECKMODE_RANGELOCK	3

#define SERVICETYPE_PLANIMPORT	1
#define SERVICETYPE_FILEIMPORT	2
#define SERVICETYPE_PROCESSING	3
#define SERVICETYPE_EXPORT		4
#define SERVICETYPE_DATABASE	5
#define SERVICETYPE_FILESERVER	6
#define SERVICETYPE_MAINTENANCE 7

#define SERVICESTATUS_STOPPED	0
#define SERVICESTATUS_RUNNING	1
#define SERVICESTATUS_HASERROR	2

#define POLLINPUTTYPE_LOWRESPDF 0
#define POLLINPUTTYPE_HIRESPDF	1
#define POLLINPUTTYPE_PRINTPDF	2

#define CHANNELSTATUSTYPE_INPUT			0
#define CHANNELSTATUSTYPE_PROCESSING	1
#define CHANNELSTATUSTYPE_EXPORT		2 
#define CHANNELSTATUSTYPE_MERGE			3

#define PLANLOCK_LOCK		1
#define PLANLOCK_NOLOCK		0
#define PLANLOCK_UNLOCK		0
#define PLANLOCK_UNKNOWN	-1

#define PLANLOCKMODE_OFF					0
#define PLANLOCKMODE_FULLLOCK				1
#define PLANLOCKMODE_LOCKDURINGDELETEONLY	2
#define PROOFSTATUSNUMBER_PROOFING			5
#define PROOFSTATUSNUMBER_PROOFERROR		6
#define PROOFSTATUSNUMBER_PROOFED			10

#define CONVERTSTATUSNUMBER_CONVERTING   5
#define CONVERTSTATUSNUMBER_CONVERTERROR 6
#define CONVERTSTATUSNUMBER_CONVERTED   10

#define EVENTCODE_PROOFERROR		16
#define EVENTCODE_PROOFED			20

#define EVENTCODE_CONVERTED			110
#define EVENTCODE_CONVERTERROR		116

#define FILESERVERTYPE_MAIN			1
#define FILESERVERTYPE_LOCAL		2
#define FILESERVERTYPE_OUTPUTCENTER	3
#define FILESERVERTYPE_INKCENTER	4
#define FILESERVERTYPE_WEBCENTER	5
#define FILESERVERTYPE_BACKUPSERVER	6
#define FILESERVERTYPE_PDFSERVER	7
#define FILESERVERTYPE_PROOFCENTER	8
#define FILESERVERTYPE_ADDITIONALPREVIEW	9
#define FILESERVERTYPE_ADDITIONALTHUMBNAIL	10
#define FILESERVERTYPE_ADDITIONALREADVIEWPREVIEW	11

#define MAILTYPE_DBERROR		0
#define MAILTYPE_FILEERROR		1
#define MAILTYPE_FOLDERERROR	2

#define MAILTYPE_TXERROR		4
#define MAILTYPE_POLLINGERROR	5
#define MAILTYPE_PROOFINGERROR	6

#define SAVEFILEOPTION_NONE		0
#define SAVEFILEOPTION_UNKNOWN	1
#define SAVEFILEOPTION_ARCHIVE	2

#define EVENTCODE_PRODUCTDELETE		610
#define EVENTCODE_REPROCESS_REQUEST 611 
#define EVENTCODE_REEXPORT_REQUEST	612
#define EVENTCODE_REINPUT_REQUEST	613
#define EVENTCODE_DATABASE_CHECK	614
#define EVENTCODE_FILESERVER_CHECK	615
#define EVENTCODE_FILERETRY_REQUEST	616
#define EVENTCODE_TRIGGER_SENT		617
#define EVENTCODE_AUTOITRIGGER_SENT	618


#define TRIGGERMODE_NONE		0
#define TRIGGERMODE_MANUELFILE	1
#define TRIGGERMODE_MANUELEMAIL 2
#define TRIGGERMODE_AUTOFILE	3
#define TRIGGERMODE_AUTOEMAIL	4

typedef struct {
	CString	sFileName;
	CString	sFolder;
	CTime	tJobTime;
	CTime	tWriteTime;
	DWORD	nFileSize;
} FILEINFOSTRUCT;

typedef CList <FILEINFOSTRUCT, FILEINFOSTRUCT&> FILELISTTYPE;

typedef struct {
	CString sFromPath;
	CString sToPath;
	CTime	tWriteTime;
} FILESTOMOVE;


typedef struct {
	int			m_productionID;
	CString		m_publicationName;
	CTime		m_pubDate;
	int			m_autoPurgeKeepDays;
	int			m_ageDays;
	int			m_PublicationID;
} PRODUCTSTRUCT;

typedef CArray <PRODUCTSTRUCT, PRODUCTSTRUCT&> PRODUCTLIST;

typedef struct {
	int		m_publicationID;
	CTime	m_pubdate;
	int		m_productionID;
	int		m_pressID;
	int		m_editionID;
	int		m_pagecount;
	int		m_pagesreceived;
	int		m_pagesapproved;
	int		m_totalplates;
	int		m_platesdone;
	int		m_platesused;
	int		m_colorplatesused;
	int		m_monoplatesused;
	CTime	m_lastpagereceived;
	CTime	m_lastpageapproved;
	CTime	m_lastplateoutput;

	CString m_pagestring;
	CString m_publication;
	BOOL	m_allpagesreceived;
	BOOL	m_allplatesoutput;
	CString m_press;
} PRODUCTIONINFO;

typedef CArray <PRODUCTIONINFO, PRODUCTIONINFO&> PRODUCTIONLIST;

class REGEXPSTRUCT {
public:
	REGEXPSTRUCT() {
		m_useExpression = FALSE;
		m_matchExpression = _T("");
		m_formatExpression = _T("");
		m_partialmatch = FALSE;
		m_comment = _T("");
	};

	BOOL		m_useExpression;
	CString		m_matchExpression;
	CString		m_formatExpression;
	BOOL		m_partialmatch;
	CString		m_comment;
};

typedef CArray <REGEXPSTRUCT, REGEXPSTRUCT&> REGEXPLIST;

/////////////////////////////////////////
// Static strings for setup
/////////////////////////////////////////

static CString sHardproofFormats[] = { _T("TIFF"),_T("PDF"),_T("PostScript/EPS"),_T("DCS"),_T("HP-RTL"),_T("HP-PCL"),_T("Epson ESC/P2"),_T("CIP3") };
static CString sICCRenderingIntent[] = { _T("Perceptual"),_T("Relative Colorimetric"),_T("Saturation"),_T("Absolute Colorimetric") };
static CString sProofTypes[] = { _T("Softproof"),_T("Hardproof"),_T("File") };

/////////////////////////////////////////
//
// Structure and list collection definitions
//
/////////////////////////////////////////

typedef struct {
	CString m_folder;
	CString m_searchmask;
	int m_days;
	int m_hours;
	int m_minutes;
	BOOL m_deleteEmptyFolders;
	BOOL m_searchsubfolders;
} OLDFOLDERSTRUCT;

typedef CArray <OLDFOLDERSTRUCT, OLDFOLDERSTRUCT&> OLDFOLDERLIST;


class ALIASSTRUCT {
public:
	ALIASSTRUCT() { sType = _T("Color"); sLongName = _T(""); sShortName = _T(""); };
	CString	sType;
	CString sLongName;
	CString sShortName;
};
typedef CArray <ALIASSTRUCT, ALIASSTRUCT&> ALIASLIST;



typedef struct {
	int m_ID;
	CString m_name;
} ITEMSTRUCT;

typedef CArray <ITEMSTRUCT, ITEMSTRUCT&> ITEMLIST;


class STATUSSTRUCT {
public:
	STATUSSTRUCT() { m_statusnumber = 0; m_statusname = _T(""); m_statuscolor = _T("FFFFFF"); };
	int	m_statusnumber;
	CString m_statusname;
	CString m_statuscolor;
};

typedef CArray <STATUSSTRUCT, STATUSSTRUCT&> STATUSLIST;

typedef struct {
	int m_channelID;
	int m_pushtrigger;
	int m_pubdatemovedays;
	int m_releasedelay;

} PUBLICATIONCHANNELSTRUCT;



class PUBLICATIONSTRUCT {
public:
	PUBLICATIONSTRUCT() {
		m_name = _T("");
		m_PageFormatID = 0;
		m_TrimToFormat = FALSE;
		m_LatestHour = 0.0;
		m_ProofID = 1;
		m_Approve = FALSE;
		m_autoPurgeKeepDays = 0;
		m_emailrecipient = _T("");
		m_emailCC = _T("");
		m_emailSubject = _T("");
		m_emailBody = _T("");
		m_customerID = 0;
		m_uploadfolder = _T("");
		m_deadline = CTime(1975, 1, 1);

		m_allowunplanned = TRUE;
		m_priority = 50;
		m_annumtext = _T("");
		m_releasedays = 0;
		m_releasetimehour = 0;
		m_releasetimeminute = 0;
		m_pubdatemove = FALSE;
		m_pubdatemovedays = 0;
		m_alias = _T("");
		m_outputalias = _T("");
		m_extendedalias = _T("");
		m_publisherID = 0;
		m_numberOfChannels = 0;
		for (int i = 0; i < MAXCHANNELSPERPUBLICATION; i++)
			m_channels[i].m_channelID = 0;

	};

	int			m_ID;
	CString		m_name;
	int			m_PageFormatID;
	int			m_TrimToFormat;
	double		m_LatestHour;
	int			m_ProofID;

	int			m_Approve;

	int			m_autoPurgeKeepDays;
	CString		m_emailrecipient;
	CString		m_emailCC;
	CString		m_emailSubject;
	CString		m_emailBody;
	int			m_customerID;
	CString		m_uploadfolder;
	CTime		m_deadline;

	//	CString		m_channelIDList;

	BOOL		m_allowunplanned;
	int			m_priority;

	CString		m_annumtext;

	int			m_releasedays;
	int			m_releasetimehour;
	int			m_releasetimeminute;


	BOOL		m_pubdatemove;
	int  		m_pubdatemovedays;
	CString		m_alias;
	CString		m_outputalias;
	CString		m_extendedalias;

	int			m_publisherID;

	int			m_numberOfChannels;
	PUBLICATIONCHANNELSTRUCT m_channels[MAXCHANNELSPERPUBLICATION];

};

typedef CArray <PUBLICATIONSTRUCT, PUBLICATIONSTRUCT&> PUBLICATIONLIST;


class CHANNELSTRUCT {
public:
	CHANNELSTRUCT() {
		m_ownerinstance = -1;
		m_channelID = 0;
		m_channelname = _T("");
		m_channelnamealias = _T("");
		m_enabled = TRUE;

		m_usereleasetime = FALSE;
		m_releasetimehour = 0;
		m_releasetimeminute = 0;

		m_transmitnameconv = _T("");
		m_transmitdateformat = _T("");
		m_transmitnameoptions = 0;
		m_transmitnameuseabbr = TRUE;
		m_subfoldernameconv = _T("");

		m_configdata = _T("");

		m_pdftype = POLLINPUTTYPE_LOWRESPDF;
		m_mergePDF = FALSE;

		m_precommand = _T("");
		m_useprecommand = FALSE;

		m_archivefolder = _T("");
		m_archivefiles = FALSE;
		m_archivefolder2 = _T("");
		m_archivefiles2 = FALSE;

		m_moveonerror = FALSE;
		m_errorfolder = _T("");

		m_outputfolder = _T("");
		m_outputusecurrentuser = TRUE;
		m_outputusername = _T("");
		m_outputpassword = _T("");

		m_outputuseftp = NETWORKTYPE_SHARE;
		m_outputftpserver = _T("");
		m_outputftpuser = _T("");
		m_outputftppw = _T("");
		m_outputftpfolder = _T("");
		m_outputftpport = 21;
		m_outputftpbuffersize = 4096;
		m_outputftpXCRC = FALSE;
		m_useack = FALSE;
		m_statusfolder = _T("");
		m_outputftppasv = TRUE;

		m_useregexp = FALSE;

		m_NumberOfRegExps = 0;
		for (int j = 0; j < MAXREGEXP; j++) {
			m_RegExpList[j].m_matchExpression = _T("");
			m_RegExpList[j].m_formatExpression = _T("");
			m_RegExpList[j].m_partialmatch = FALSE;
		}

		m_usescript = FALSE;
		m_script = _T("");
		m_nooverwrite = FALSE;

		m_retries = 3;
		m_keepoutputconnection = TRUE;
		m_timeout = 60;

		m_outputftpusetempfolder = FALSE;
		m_outputftpaccount = _T("");
		m_outputftptempfolder = _T("");
		m_outputftppasv = FALSE;
		m_FTPpostcheck = REMOTEFILECHECK_SIZE;
		m_outputfpttls = FTPTYPE_NORMAL;
		m_ftptimeout = 120;

		m_ensureuniquefilename = FALSE;

		m_archivedays = 0;

		m_breakifnooutputaccess = FALSE;

		m_filebuildupmethod = TEMPFILE_NAME;

		m_copymatchexpression = _T("");
		m_archiveconditionally = FALSE;

		m_inputemailnotification = FALSE;
		m_inputemailnotificationTo = _T("");
		m_inputemailnotificationSubject = _T("");

		m_emailsmtpserver = _T("");
		m_emailsmtpport = 25;
		m_emailsmtpserverusername = _T("");
		m_emailsmtpserverpassword = _T("");
		m_emailfrom = _T("");
		m_emailto = _T("");
		m_emailcc = _T("");
		m_emailbody = _T("");
		m_emailsubject = _T("");
		m_emailusehtml = FALSE;
		m_emailusessl = FALSE;
		m_emailtimeout = 60;

		m_editionstogenerate = 0;
		m_subfoldernameconv = _T("");

		m_configchangetime = CTime(1975, 1, 1);

		m_pdfprocessID = 0;

		m_triggermode = TRIGGERMODE_NONE;
		m_triggeremail = _T("");

		m_usepackagenames = FALSE;

		m_deleteoldoutputfilesdays = 0;
	};



	int		m_channelID;
	CString m_channelname;
	int     m_ownerinstance;
	BOOL	m_enabled;

	//	int		m_channelgroupID;
	//	int     m_publisherID;
	BOOL	m_usereleasetime;
	int		m_releasetimehour;
	int		m_releasetimeminute;

	CString m_transmitnameconv;
	CString m_transmitdateformat;
	int		m_transmitnameoptions;
	int		m_transmitnameuseabbr;
	CString m_subfoldernameconv;

	int		m_miscint;
	CString m_miscstring;

	int     m_pdftype;
	BOOL	m_mergePDF;


	CString m_configdata;

	CString m_archivefolder;
	BOOL	m_archivefiles;
	CString m_archivefolder2;
	BOOL	m_archivefiles2;
	int		m_archivedays;

	CString m_precommand;
	BOOL m_useprecommand;


	BOOL	m_moveonerror;
	CString m_errorfolder;
	CString m_outputfolder;

	BOOL	m_outputusecurrentuser;
	CString m_outputusername;
	CString m_outputpassword;

	int		m_outputuseftp;

	CString m_outputftpserver;
	CString	m_outputftpuser;
	CString	m_outputftppw;
	CString m_outputftpfolder;

	BOOL	m_outputftpusetempfolder;
	CString	m_outputftpaccount;
	CString	m_outputftptempfolder;
	BOOL	m_outputftppasv;
	int		m_FTPpostcheck;
	int		m_outputftpbuffersize;
	int		m_outputftpport;
	BOOL	m_outputftpXCRC;
	int		m_outputfpttls;
	int		m_ftptimeout;

	BOOL	m_useack;
	CString m_statusfolder;
	BOOL	m_ensureuniquefilename;
	BOOL	m_useregexp;
	int		m_NumberOfRegExps;
	REGEXPSTRUCT	m_RegExpList[MAXREGEXP];

	BOOL	m_usescript;
	CString	m_script;

	BOOL	m_nooverwrite;
	int		m_retries;
	BOOL	m_keepoutputconnection;
	int		m_timeout;

	BOOL	m_breakifnooutputaccess;

	int m_filebuildupmethod;

	CString m_copymatchexpression;
	BOOL m_archiveconditionally;

	BOOL m_inputemailnotification;
	CString m_inputemailnotificationTo;
	CString m_inputemailnotificationSubject;

	CString m_emailsmtpserver;
	int m_emailsmtpport;
	CString m_emailsmtpserverusername;
	CString m_emailsmtpserverpassword;
	CString m_emailfrom;
	CString m_emailto;
	CString m_emailcc;
	CString m_emailbody;
	CString m_emailsubject;
	BOOL m_emailusehtml;
	BOOL m_emailusessl;
	int m_emailtimeout;

	CString m_channelnamealias;
	int		m_editionstogenerate;
	BOOL	m_sendCommonPages;

	CTime m_configchangetime;


	int m_pdfprocessID;
	int m_triggermode;
	CString m_triggeremail;
	BOOL m_usepackagenames;

	int m_deleteoldoutputfilesdays;

};
typedef CArray <CHANNELSTRUCT, CHANNELSTRUCT&> CHANNELLIST;

typedef struct {
	CString m_servername;
	CString m_sharename;
	int	m_servertype;
	CString m_IP;
	CString m_username;
	CString m_password;
	int	m_locationID;
	BOOL m_uselocaluser;
} FILESERVERSTRUCT;

typedef CArray <FILESERVERSTRUCT, FILESERVERSTRUCT&> FILESERVERLIST;


typedef struct {
	CString m_folder;
	CString m_searchmask;
	int		m_hours;
	int		m_minutes;
	int     m_maxfiles;
	CString m_sMailReceiver;
	CString m_sMailSubject;
	BOOL	m_moveOnError;
	CString	m_errorfolder;
	CTime   m_lastNotificationEvent;
	int		m_usewritetime;
} FOLDERNOTIFICATIONSTRUCT;

typedef CArray <FOLDERNOTIFICATIONSTRUCT, FOLDERNOTIFICATIONSTRUCT&> FOLDERNOTIFICATIONLIST;

#define FILERETRYTYPE_IMPORT	 1
#define FILERETRYTYPE_INPUT		 2
#define FILERETRYTYPE_PROCESSING 3
#define FILERETRYTYPE_EXPORT	 4

typedef struct
{
	int nType;
	CString sFileName;
	CString sQueueName;
	CTime tEventTime;
} FILERETRYSTRUCT;

typedef CArray <FILERETRYSTRUCT, FILERETRYSTRUCT&> FILERETRYLIST;

class CHANNELPURGESTRUCT {
public:
	CHANNELPURGESTRUCT() {
		m_nChannelID = 0;		
		m_sInputAlias = _T("");
		m_sOutputAlias = _T("");
		m_sExtendedAlias = _T("");
		m_sExtendedAlias2 = _T("");
		m_nAutoPurgeKeepDaysArchive = 0;
	};

	int m_nChannelID;
	CString m_sInputAlias;
	CString m_sOutputAlias;
	CString m_sExtendedAlias;
	CString m_sExtendedAlias2;
	int m_nAutoPurgeKeepDaysArchive;
};

typedef CArray <CHANNELPURGESTRUCT, CHANNELPURGESTRUCT&> CHANNELPURGESLIST;

class SENDREQUEST
{
public:
	SENDREQUEST() {
		n64GUID =0;
		sSourceFile = _T("");
		sFileToSend = _T("");
		sFtpServer = _T("");
		sFtpUserName = _T("");
		sFtpPassword = _T("");
		sFtpFolder = _T("");
		bPasv = TRUE;
		bUseFtp = TRUE;
		sSMBFolder = _T("");
		tEventTime = CTime(1975, 1, 1);
		bDeleteFileAfter = TRUE;
	};

	__int64 n64GUID;
	CString sSourceFile;
	CString sFileToSend;
	BOOL	bUseFtp;
	CString sFtpServer;
	CString sFtpUserName;
	CString sFtpPassword;
	CString sFtpFolder;
	BOOL	bPasv;
	CString sSMBFolder;
	BOOL bDeleteFileAfter;
	CTime tEventTime;
};
