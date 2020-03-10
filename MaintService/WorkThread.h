#pragma once
#include "stdafx.h"
#include "Defs.h"

UINT WorkThread(LPVOID nothing);

int EnmurateUnknownFiles();
int EnumerateIllegalPages();
BOOL CleanFolders();
BOOL CleanExportFolders();
BOOL ExportPlanJobs();
BOOL DeleteProductFiles(PRODUCTSTRUCT *pProduct, int nEditionID, int nSectionID, int &nFilesDeleted, BOOL bSaveOldFiles, CString sArchiveFolder);
int GetPlanLock();
BOOL PurgeUnrelatedFiles(int &nFilesDeleted);

BOOL ExportFileRetry(CString sFileName, CString sChannelName);
BOOL InputFileRetry(CString sFileName, CString sQueueName);
BOOL ProcessFileRetry(CString sFileName, CString sChannelName);

void ReportFolderAccess();
BOOL ProcessTriggers();
BOOL SendFileToChannel(CString sFileName, int nChannelID, CString sSubFolder);

void EnumerateQueuedFile();
BOOL ProcessSendRequests();
