#include "stdafx.h"
#include "Defs.h"
#include "Utils.h"
#include "Prefs.h"
#include "PlanExport.h"
#include "Markup.h"
#include "DatabaseManager.h"
#include "WorkThread.h"
#include "FtpClient.h"
#include "SFtpClient.h"
#include "SeException.h"

extern CUtils	g_util;
extern CPrefs	g_prefs;
extern BOOL g_BreakSignal;
CDatabaseManager g_db;

UINT WorkThread(LPVOID nothing)
{
	int nTicks = 0;
	BOOL bHasDBError = FALSE;
	CString sErrorMessage = _T("");

	CTime tLastUnrelatedCleanupTime = CTime::GetCurrentTime();
	BOOL bPurgeDone = FALSE;
	BOOL bUnrelatedPurgeDone = FALSE;
	BOOL bRunScript = FALSE;
	CString sClientName;
	CString sClientTime;
	int nCurrentPlanLock = PLANLOCK_UNKNOWN;
	int nWaitLoop = 0;
	BOOL bCleanDirtyRequest = FALSE;
	BOOL bUnrelatedPurgeRequest = FALSE;
	BOOL bMaintRequest = FALSE;
	BOOL bPlanExportRequest = FALSE;
	FILEINFOSTRUCT aFilesReady[10000];
	FILESTOMOVE aFilesToMove[10000];

	FILERETRYLIST aRetryFileList;

	if (g_prefs.m_enableautodelete)
		g_util.Logprintf("INFO:  Auto-delete enabled");
	if (g_prefs.m_OldFolderList.GetCount() > 0)
		g_util.Logprintf("INFO:  Old file delete enabled");

	if (g_prefs.m_FolderNotificationList.GetCount() > 0)
		g_util.Logprintf("INFO:  Old/many file notification enabled");
	if (g_prefs.m_autoretry)
		g_util.Logprintf("INFO:  Auto errorfile retry (on demand) enabled");
	if (g_prefs.m_enablecontinouscustomscript)
		g_util.Logprintf("INFO:  Continous custom housekeeping script enabled");
	if (g_prefs.m_enablecustomscript)
		g_util.Logprintf("INFO:  Daily custom housekeeping script enabled");
	if (g_prefs.m_doexportplans)
		g_util.Logprintf("INFO:  Plan export enabled");

	if (g_db.InitDB(g_prefs.m_DBserver, g_prefs.m_Database, g_prefs.m_DBuser, g_prefs.m_DBpassword, g_prefs.m_IntegratedSecurity, sErrorMessage) == FALSE) {
		g_util.Logprintf("Error connecting to database - " + sErrorMessage);
		bHasDBError = TRUE;
	}

	g_db.LoadFileServerList(sErrorMessage);

	int nDaysToKeepDB = 0;
	int nDaysToKeepLogDB = 0;
	CString sVisioLinkTemplateFile = _T("");
	g_db.LoadMaintParamters(g_prefs.m_instancenumber, nDaysToKeepDB, nDaysToKeepLogDB, sVisioLinkTemplateFile, sErrorMessage);
	if (nDaysToKeepDB > 0)
		g_prefs.m_overrulekeepdays = nDaysToKeepDB;
	if (nDaysToKeepLogDB > 0)
		g_prefs.m_keepdayslog = nDaysToKeepLogDB;
	if (sVisioLinkTemplateFile != _T(""))
		g_prefs.m_CustomMessageTemplate = sVisioLinkTemplateFile;

	while (g_BreakSignal == FALSE) {
		for (int i = 0; i < 20; i++) {
			::Sleep(100);
			if (g_BreakSignal)
				break;
		}

		nTicks++;
		if (g_BreakSignal)
			break;


		if (bHasDBError) {
			Sleep(5000);
			g_db.ExitDB();
			Sleep(5000);
			g_util.Logprintf("INFO:  Attempting re-connection to database..");
			bHasDBError = FALSE;
			if (g_db.InitDB(g_prefs.m_DBserver, g_prefs.m_Database, g_prefs.m_DBuser, g_prefs.m_DBpassword, g_prefs.m_IntegratedSecurity, sErrorMessage) == FALSE)
				bHasDBError = TRUE;
			if (g_BreakSignal)
				break;
		}

		if (bHasDBError)
			continue;

		// Fileretry
		aRetryFileList.RemoveAll();
		if (g_db.GetFileRetryJobs(aRetryFileList, sErrorMessage) == FALSE) {
			g_util.Logprintf("ERROR: db.GetFileRetryJobs() - %s", sErrorMessage);
		}

		for (int i=0; i< aRetryFileList.GetCount(); i++) {
			if (aRetryFileList[i].nType == FILERETRYTYPE_INPUT) {
				g_util.Logprintf("INFO: Input file retry detected - %s to queue %s..", aRetryFileList[i].sFileName, aRetryFileList[i].sQueueName);
				InputFileRetry(aRetryFileList[i].sFileName, aRetryFileList[i].sQueueName);
				g_db.InsertLogEntry(EVENTCODE_REINPUT_REQUEST, aRetryFileList[i].sQueueName, aRetryFileList[i].sFileName, "", 0, 0, 0, "", sErrorMessage);
			}
			else if (aRetryFileList[i].nType == FILERETRYTYPE_EXPORT) {
				g_util.Logprintf("INFO: Export file retry detected - %s to channel %s..", aRetryFileList[i].sFileName, aRetryFileList[i].sQueueName);
				ExportFileRetry(aRetryFileList[i].sFileName, aRetryFileList[i].sQueueName);
				g_db.InsertLogEntry(EVENTCODE_REEXPORT_REQUEST, aRetryFileList[i].sQueueName, aRetryFileList[i].sFileName, "", 0, 0, 0, "", sErrorMessage);
			}
			else if (aRetryFileList[i].nType == FILERETRYTYPE_PROCESSING) {
				g_util.Logprintf("INFO: Process file retry detected - %s to profile %s..", aRetryFileList[i].sFileName, aRetryFileList[i].sQueueName);
				ProcessFileRetry(aRetryFileList[i].sFileName, aRetryFileList[i].sQueueName);
				g_db.InsertLogEntry(EVENTCODE_REPROCESS_REQUEST, aRetryFileList[i].sQueueName, aRetryFileList[i].sFileName, "", 0, 0, 0, "", sErrorMessage);
			}
			g_db.RemoveFileRetryJobs(aRetryFileList[i].nType, aRetryFileList[i].sFileName, aRetryFileList[i].sQueueName, sErrorMessage);
		}

		if ((nTicks % 10) == 0)
			ProcessTriggers();

		if ((nTicks % 5) == 0)
			ProcessSendRequests();


		if ((nTicks % 10) == 0)
			EnumerateQueuedFile();
			
		// Clean folders every 2 minutes..
		if (g_prefs.m_OldFolderList.GetCount() > 0 && (nTicks % (2 * 30)) == 0)
			CleanFolders();


		// Report folder access trouble every minute.
		if ((nTicks % 1 * 30) == 0) {
			g_db.UpdateService(1,"","","",sErrorMessage);
			ReportFolderAccess();
		}
		// Enumerate illegal files every 5. minute
		if ((nTicks % (5 * 30)) == 0) {
			if (g_db.TableExists("IllegalFiles", sErrorMessage) == 1) {
				int fFilesFound = EnumerateIllegalPages();
				g_util.Logprintf("EnumerateIllegalPages() - %d files found", fFilesFound);

			}
		}

		// Run custom script every minute..
		if (g_prefs.m_enablecontinouscustomscript && (nTicks % 30) == 0 && g_prefs.m_continouscustomscript != _T("")) {

			if (g_db.RunContinousCustomScript(g_prefs.m_continouscustomscript, sErrorMessage) == FALSE)
				g_util.Logprintf("ERROR: RunContinousCustomScript() - %s", sErrorMessage);
			else
				if (g_prefs.m_logging > 1)
					g_util.Logprintf("INFO: Continous housekeeping script called (%s)", g_prefs.m_continouscustomscript);
		}

		// Run every 5 minutes...
		if (g_prefs.m_doexportplans && (nTicks % 150) == 0) {
			ExportPlanJobs();
		}

		if (g_prefs.m_enabledeleteunrelatedfiles || g_prefs.m_EnableDeleteUnrelatedFilesWeekly || bUnrelatedPurgeRequest) {

			// Autoclean part
			CTime tNow = CTime::GetCurrentTime();
			BOOL bDoDaily = (tNow.GetHour() >= g_prefs.m_autopurgeunrelatedhoursbegin) && (tNow.GetHour() <= g_prefs.m_autopurgeunrelatedhoursend) && (bUnrelatedPurgeDone == FALSE);
			BOOL bDoWeekly = bDoDaily && (tNow.GetDayOfWeek() == g_prefs.m_weekday + 1);

			if (bUnrelatedPurgeRequest || bDoDaily || bDoWeekly) {

				g_util.Logprintf("INFO:  Starting delete of unrelated files..");

				bHasDBError = g_prefs.ConnectToDB(false, sErrorMessage) == FALSE;

				int nFilesDeleted = 0;

				if (g_prefs.m_planlock)
					nCurrentPlanLock = GetPlanLock();

				PurgeUnrelatedFiles(nFilesDeleted);

				if (g_prefs.m_planlock)
					g_db.PlanLock(PLANLOCK_UNLOCK, nCurrentPlanLock, sClientName, sClientTime, sErrorMessage);

			
				if (bUnrelatedPurgeRequest == FALSE)
					bUnrelatedPurgeDone = TRUE;

				g_util.Logprintf("INFO:  Delete of unrelated files done");
			}

			if (tNow.GetHour() > g_prefs.m_autopurgeunrelatedhoursend)
				bUnrelatedPurgeDone = FALSE;

			bCleanDirtyRequest = FALSE;
			bUnrelatedPurgeRequest = FALSE;
		}



		if (g_prefs.m_enableautodelete) {

			// Autoclean part
			CTime tNow = CTime::GetCurrentTime();

			if (g_prefs.m_planlock == PLANLOCKMODE_FULLLOCK)
				nCurrentPlanLock = g_db.QueryProdLock(sErrorMessage);
			else
				nCurrentPlanLock = PLANLOCK_UNLOCK;


			if ((tNow.GetHour() >= g_prefs.m_autopurgehoursbegin && tNow.GetHour() <= g_prefs.m_autopurgehoursend && bPurgeDone == FALSE && nCurrentPlanLock != PLANLOCK_LOCK)) {

				// Re-init db connection
				bHasDBError = g_prefs.ConnectToDB(false, sErrorMessage) == FALSE;

				if (g_prefs.m_keepdayslog > 0)
					g_db.DeleteLogData(g_prefs.m_keepdayslog, sErrorMessage);

				g_prefs.m_ProductList.RemoveAll();
				if (g_db.GetOldProductions(sErrorMessage) == FALSE) {
					bHasDBError = TRUE;
					g_util.Logprintf("ERROR: GetOldProductions() - %s", sErrorMessage);
					continue;
				}

				if (g_prefs.m_ProductList.GetCount() > 0)
					g_util.Logprintf("INFO:  %d product(s) to delete", g_prefs.m_ProductList.GetCount());

				if (g_prefs.m_ProductList.GetCount() == 0) {
					//					g_util.Logprintf("INFO:  Autodelete  done");
					continue;
				}

				g_db.LoadAllPrefs(sErrorMessage);

				for (int i = 0; i < g_prefs.m_ProductList.GetCount(); i++) {

					PRODUCTSTRUCT *pProduct = &g_prefs.m_ProductList[i];
					g_util.Logprintf("INFO:  Starting purge process for %s-%.4d-%.2d-%.2d (DayToKeep=%d)...", pProduct->m_publicationName, pProduct->m_pubDate.GetYear(), pProduct->m_pubDate.GetMonth(), pProduct->m_pubDate.GetDay(), pProduct->m_autoPurgeKeepDays);
					int nFiles = 0;

					// Apply for production lock
					nCurrentPlanLock = PLANLOCK_UNKNOWN;

					if (g_prefs.m_planlock == PLANLOCKMODE_FULLLOCK)
						nCurrentPlanLock = g_db.QueryProdLock(sErrorMessage);

					if (g_db.MarkDirtyProduction(pProduct->m_productionID, pProduct->m_PublicationID, pProduct->m_pubDate, 0, 0, sErrorMessage) == FALSE)
						g_util.Logprintf("ERROR: MarkDirtyProduction() - %s", sErrorMessage);

					g_util.Logprintf("Deleting files...");
					DeleteProductFiles(pProduct, 0, 0, nFiles, FALSE, "");
					CString sProdName;
					sProdName.Format("%s-%.4d%.2d%.2d", pProduct->m_publicationName, pProduct->m_pubDate.GetYear(), pProduct->m_pubDate.GetMonth(), pProduct->m_pubDate.GetDay());

					g_util.Logprintf("INFO:  %s - %d files deleted from server shares", sProdName, nFiles);

					if (g_prefs.m_planlock == PLANLOCKMODE_LOCKDURINGDELETEONLY)
						nCurrentPlanLock = GetPlanLock();

					if (g_db.DeleteProduction(pProduct->m_productionID, pProduct->m_PublicationID, pProduct->m_pubDate, 0, 0, sErrorMessage) == FALSE) {
						g_util.Logprintf("ERROR: DeleteProduction() - %s", sErrorMessage);
						g_db.InsertLogEntry(EVENTCODE_PRODUCTDELETE, "ERROR autodeleting", sProdName, sErrorMessage, 0, 0, pProduct->m_productionID, "", sErrorMessage);
					}
					else {
						g_util.Logprintf("INFO:  %s - records deleted from database", sProdName);

						g_db.InsertPlanLogEntry(998, 999, sProdName, "Deleted", pProduct->m_productionID, "Autodeleted", sErrorMessage);

						g_db.InsertLogEntry(EVENTCODE_PRODUCTDELETE, "Autodeleted", sProdName, "", 0, 0, pProduct->m_productionID, "", sErrorMessage);

					}

					if (g_BreakSignal)
						break;
				}

				// Release lock..
				if (g_prefs.m_planlock != PLANLOCKMODE_OFF)
					g_db.PlanLock(PLANLOCK_UNLOCK, nCurrentPlanLock, sClientName, sClientTime, sErrorMessage);

				bPurgeDone = TRUE;

				// Clean old export folders 
				CleanExportFolders();


			}

			if (tNow.GetHour() > g_prefs.m_autopurgehoursend)
				bPurgeDone = FALSE;

			//g_util.Logprintf("INFO:  Autodelete done");
			if (g_prefs.m_planlock)
				g_db.PlanLock(PLANLOCK_UNLOCK, nCurrentPlanLock, sClientName, sClientTime, sErrorMessage);

		}


		if (g_prefs.m_autoretry) {

			CString sErrorRootFolder = "";
			CStringArray sInputFolders;
			CStringArray sInputIDs;
			CString sPublication = "";
			int		nPublicationID;
			CTime tPubDate;
			CStringArray sMaskList;
			CStringArray sQueueList;
			CStringArray sMaskDateFormatList;

			BOOL bRetryDone = FALSE;
		//	if (g_prefs.m_logging > 1)
			//	g_util.Logprintf("INFO:  Looking for auto retry requests..");

			// Re-init db connection
			bHasDBError = g_prefs.ConnectToDB(FALSE, sErrorMessage) == FALSE;
			int nProductionID;

			int nFilesToMove = 0;

			if (g_db.GetAutoRetryJob(nProductionID, sErrorMessage) == FALSE) {
				g_util.Logprintf("ERROR: GetAutoRetryJob() - %s", sErrorMessage);
				bHasDBError = TRUE;
				bRetryDone = TRUE;
			}


			if (nProductionID == 0) {
				bRetryDone = TRUE;
				g_db.RemoveAutoRetryJob(0, sErrorMessage);
			}

			if (bRetryDone == FALSE) {

				if (g_db.GetErrorRootFolder(sErrorRootFolder, sErrorMessage) == FALSE) {
					g_util.Logprintf("ERROR: GetErrorRootFolder() - %s", sErrorMessage);
					bHasDBError = TRUE;
					bRetryDone = TRUE;
				}

				if (g_db.GetInputFolders(sInputFolders, sInputIDs, sQueueList, sErrorMessage) == FALSE) {
					g_util.Logprintf("ERROR: GetInputFolders() - %s", sErrorMessage);
					bHasDBError = TRUE;
					bRetryDone = TRUE;
				}
				if (g_db.GetProductParameters(nProductionID, nPublicationID, sPublication, tPubDate, sErrorMessage) == FALSE) {
					g_util.Logprintf("ERROR: GetProductParameters() - %s", sErrorMessage);
					bHasDBError = TRUE;
					bRetryDone = TRUE;
					g_db.RemoveAutoRetryJob(nProductionID, sErrorMessage);
				}
				if (sPublication == "") {
					g_util.Logprintf("ERROR: No product found for ProductionID %d", nProductionID);
					bRetryDone = TRUE;
					g_db.RemoveAutoRetryJob(nProductionID, sErrorMessage);
				}

			}
			CString sProdName = "";

			if (bRetryDone == FALSE) {

				CString sPublicationAlias = g_db.GetInputAlias("Publication", sPublication, sErrorMessage);
				if (sErrorMessage != _T(""))
					g_util.Logprintf("ERROR: GetInputAlias() - %s", sErrorMessage);
				
				sProdName.Format("%s %.02d%.02d%.04d", sPublicationAlias, tPubDate.GetDay(), tPubDate.GetMonth(), tPubDate.GetYear());
				g_util.Logprintf("INFO:  AutoRetry ProductionID=%d, Publication=%s (%s), PubDate=%.02d%.02d%.04d", nProductionID, sPublication, sPublicationAlias, tPubDate.GetDay(), tPubDate.GetMonth(), tPubDate.GetYear());
				sPublication = sPublicationAlias;


				CString sDefaultMask = "";
				CString sDefaultDateFormat = "";


				if (g_db.GetInputMaskList(0, 0, sMaskList, sMaskDateFormatList, sErrorMessage) == FALSE) {
					g_util.Logprintf("ERROR: GetInputMaskList() - %s", sErrorMessage);
					bHasDBError = TRUE;
					Sleep(1000);
				}
				// APHAZ1_20190904*

				// Default mask!
				if (sMaskList.GetCount() == 0 && sMaskDateFormatList.GetCount() == 0) {
					sMaskList.Add("++++*########*.pdf");
					sMaskDateFormatList.Add("YYYYMMDD");
				}
				if (sMaskList.GetCount() >= 1 && sMaskDateFormatList.GetCount() >= 1) {
					sDefaultMask = sMaskList[0];
					sDefaultDateFormat = sMaskDateFormatList[0];
				}

				sMaskList.RemoveAll();
				sMaskDateFormatList.RemoveAll();


				BOOL nMaskHits = 0;

				for (int ii = -1; ii < 2; ii++) { // ii=0 Defined masks for publication, ii=1: default mask if no pub-specific mask defined
					for (int i = 0; i < sInputFolders.GetCount(); i++) {
						if (ii == 1 && (nMaskHits > 0 || sDefaultMask == "" || sDefaultDateFormat == ""))
							break;

						g_util.Logprintf("INFO:  Inputfolder %s..", sInputFolders[i]);

						if (ii == 0) {
							sMaskList.RemoveAll();
							sMaskDateFormatList.RemoveAll();
							if (g_db.GetInputMaskList(atoi(sInputIDs[i]), nPublicationID, sMaskList, sMaskDateFormatList, sErrorMessage) == FALSE) {
								g_util.Logprintf("ERROR: GetInputMaskList() - %s", sErrorMessage);
								bHasDBError = TRUE;
								Sleep(1000);
								continue;
							}

							nMaskHits += (int)sMaskList.GetCount();
						}
						else {
							sMaskList.RemoveAll();
							sMaskDateFormatList.RemoveAll();
							sMaskList.Add(sDefaultMask);
							sMaskDateFormatList.Add(sDefaultDateFormat);
						}

						for (int j = 0; j < sMaskList.GetCount(); j++) {

							CString sPubDate = g_util.Date2String(tPubDate, sMaskDateFormatList[j]);
							CString sMask = sMaskList[j];

							sMask.Replace("###########", sPubDate);
							sMask.Replace("##########", sPubDate);
							sMask.Replace("#########", sPubDate);
							sMask.Replace("########", sPubDate);
							sMask.Replace("#######", sPubDate);
							sMask.Replace("######", sPubDate);
							sMask.Replace("#####", sPubDate);
							sMask.Replace("####", sPubDate);
							sMask.Replace("###", sPubDate);
							sMask.Replace("##", sPubDate);

							sPublication += "????????????????????????????";
							//             123456789012
							sMask.Replace("++++++++++++", sPublication.Left(12));
							sMask.Replace("+++++++++++", sPublication.Left(11));
							sMask.Replace("++++++++++", sPublication.Left(10));
							sMask.Replace("+++++++++", sPublication.Left(9));
							sMask.Replace("++++++++", sPublication.Left(8));
							sMask.Replace("+++++++", sPublication.Left(7));
							sMask.Replace("++++++", sPublication.Left(6));
							sMask.Replace("+++++", sPublication.Left(5));
							sMask.Replace("++++", sPublication.Left(4));
							sMask.Replace("+++", sPublication.Left(3));
							sMask.Replace("++", sPublication.Left(2));
							sMask.Replace("+", sPublication.Left(1));

/*							int nPchars = 0;
							for (int k = 0; k < sMask.GetLength(); k++)
								if (sMask[k] == _T('+'))
									nPchars++;

							if (sPublication.GetLength() > nPchars)
								sPublication = sPublication.Left(nPchars);
							if (sPublication.GetLength() < nPchars) {
								CString s = sPublication + _T("                      ");
								sPublication = s.Left(nPchars);
							}

							CString sM = "++++++++++++";
							for (int k = 10; k > 0; k--) {
								if (sPublication.GetLength() == k) {
									sMask.Replace(sM.Left(k), sPublication);
									break;
								}
							}
							*/
							CString sFolderToSearch = sErrorRootFolder + _T("\\") + sInputIDs[i];

							g_util.Logprintf("INFO:  Trying mask %s in Errorfolder %s ..", sMask, sFolderToSearch);


							int nFiles = g_util.ScanDir(sFolderToSearch, sMask, aFilesReady, 10000, "",TRUE,FALSE);

							if (nFiles < 0) {
								g_util.Logprintf("ERROR: Unable to scan folder %s with mask %s", sFolderToSearch, sMask);
								continue;
							}

							g_util.Logprintf("INFO:  %d files found in folder %s", nFiles, sFolderToSearch);

							for (int it = 0; it < nFiles; it++) {
								FILEINFOSTRUCT fi = aFilesReady[it];

								CString sFullInputPath = sFolderToSearch + "\\" + fi.sFileName;
								CString sFullOutputPath = sInputFolders[i] + "\\" + fi.sFileName;

								if (g_prefs.m_maxageunknownfileshours > 0) {
									int nAge = g_util.GetFileAge(sFullInputPath);
									if (nAge > 0 && nAge > g_prefs.m_maxageunknownfileshours) {
										::DeleteFile(sFullInputPath);
										if (g_prefs.m_logging > 1)
											g_util.Logprintf("INFO:  Deleted old file %s (age: %d, limit: %d)", sFullInputPath, nAge, g_prefs.m_maxageunknownfileshours);
										continue;
									}
								}
								aFilesToMove[nFilesToMove].sFromPath = sFullInputPath;
								aFilesToMove[nFilesToMove].sToPath = sFullOutputPath;
								aFilesToMove[nFilesToMove++].tWriteTime = fi.tWriteTime;

								//								::MoveFileEx(sFullInputPath, sFullOutputPath,  MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH); 

								//								if (g_prefs.m_logging > 1)
								//									g_util.Logprintf("INFO:  Moved file %s to %s", sFullInputPath, sFullOutputPath);

							}
						}

						sMaskList.RemoveAll();
						sMaskDateFormatList.RemoveAll();
					}

				}

			}

			if (bRetryDone == FALSE) {
				// Ensure we only take the latest file if multiple filenames are the same (in different folders)

				for (int i = 0; i < nFilesToMove; i++) {
					if (aFilesToMove[i].sFromPath != "") {
						CStringArray aS;
						CTime tLatest = aFilesToMove[i].tWriteTime;
						int nIndexToUse = i;

						// Scan to see if there are any newer..
						for (int j = 0; j < nFilesToMove; j++) {
							if (g_util.GetFileName(aFilesToMove[i].sFromPath) == g_util.GetFileName(aFilesToMove[j].sFromPath)) {
								if (aFilesToMove[j].tWriteTime > tLatest) {
									tLatest = aFilesToMove[j].tWriteTime;
									nIndexToUse = j;
								}
							}
						}
						::MoveFileEx(aFilesToMove[nIndexToUse].sFromPath, aFilesToMove[nIndexToUse].sToPath, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);

						if (g_prefs.m_logging > 1)
							g_util.Logprintf("INFO:  Moved file %s to %s", aFilesToMove[nIndexToUse].sFromPath, aFilesToMove[nIndexToUse].sToPath);

						// Indicate file is processed..
						for (int j = 0; j < nFilesToMove; j++) {
							if (g_util.GetFileName(aFilesToMove[nIndexToUse].sFromPath) == g_util.GetFileName(aFilesToMove[j].sFromPath)) {
								::DeleteFile(aFilesToMove[j].sFromPath);
								aFilesToMove[j].sFromPath = _T("");
							}
						}

					}
				}

				g_util.Logprintf("INFO:  AutoRetry Done");

				g_db.InsertLogEntry(EVENTCODE_FILERETRY_REQUEST, "Post-plan file retry", sProdName, "", 0, 0, nProductionID, "", sErrorMessage);


				g_db.RemoveAutoRetryJob(nProductionID, sErrorMessage);

			}
			sInputIDs.RemoveAll();
			sInputFolders.RemoveAll();

		}

		if (g_prefs.m_enablecustomscript || bMaintRequest) {

			CTime tNow = CTime::GetCurrentTime();
			if (bMaintRequest || (tNow.GetHour() >= g_prefs.m_customscripthourbegin && tNow.GetHour() <= g_prefs.m_customscripthourend && bRunScript == FALSE)) {

				//	g_util.Logprintf("INFO:  Checking archiving 2..");

				if (g_db.RunCustomScript(sErrorMessage) == FALSE)
					g_util.Logprintf("ERROR: RunCustomScript() - %s", sErrorMessage);
				else
					g_util.Logprintf("INFO: Daily housekeeping script called (spCustomMaintenance)");

				if (bMaintRequest == FALSE)
					bRunScript = TRUE;

				bMaintRequest = FALSE;

				CString sResult = _T("");
				if (g_db.RunCheckDB(sResult, sErrorMessage) == FALSE)
					g_util.Logprintf("ERROR: RunCheckDB() - %s", sErrorMessage);
				if (sResult != "") {
					g_db.InsertLogEntry(EVENTCODE_DATABASE_CHECK, "DB check", "", sResult, 0, 0, 0, "", sErrorMessage);
					g_db.UpdateServiceDB(1, sResult, "", "", sErrorMessage);
				}
			}

			if (tNow.GetHour() > g_prefs.m_customscripthourend)
				bRunScript = FALSE;
		}
	}


	g_db.UpdateService(0, "Stopped", "", "", sErrorMessage);
	return 0;
}

#define STATUSFEEDBACKTYPE_INPUT	1
#define STATUSFEEDBACKTYPE_ALLSENT	2


/*
BOOL ProcessStatusFeedbackFiles()
{
	CUtils util;
	CString sErrorMessage = _T("");
	for (int tp = 1; tp<=2; tp++) {
		CStringArray sPageNames;
		CStringArray sEventTimes;

		if (g_db.GetNextFeedbackMessage(tp, sPageNames, sEventTimes, sErrorMessage) == FALSE) {
			g_util.Logprintf("ERROR: GetNextFeedbackMessage() - %s", sErrorMessage);
			return FALSE;
		}

		if (tp == STATUSFEEDBACKTYPE_INPUT)
}
	*/

BOOL ProcessSendRequests()
{
	CUtils util;
	CString sErrorMessage;

	BOOL bMoreRequests = TRUE;

	while (bMoreRequests) {
		SENDREQUEST aSendRequest;
		aSendRequest.n64GUID = 0;
		if (g_db.GetNextFileSendRequest(aSendRequest, sErrorMessage) == FALSE) {
			g_util.Logprintf("ERROR: GetNextFileSendRequest() - %s", sErrorMessage);
			return FALSE;
		}
		if (aSendRequest.n64GUID == 0)
			break;

		if (util.FileExist(aSendRequest.sSourceFile) == FALSE) {
			g_util.Logprintf("ERROR: ProcessSendRequests() - unable to locate source file - %s", aSendRequest.sSourceFile);
			g_db.RemoveFileSendRequest(aSendRequest.n64GUID, sErrorMessage);
			continue;
		}

		// Send the file.!!
		if (aSendRequest.bUseFtp == FALSE && aSendRequest.sSMBFolder != _T("")) {
			if (util.MoveFileWithRetry(aSendRequest.sSourceFile,
				aSendRequest.sSMBFolder + _T("\\") + util.GetFileName(aSendRequest.sSourceFile), sErrorMessage) == FALSE) {
				g_util.Logprintf("ERROR: ProcessSendRequests() - unable to send source file - %s using SMB to folder %s", aSendRequest.sSourceFile, aSendRequest.sSMBFolder);
				g_db.RemoveFileSendRequest(aSendRequest.n64GUID, sErrorMessage);
				continue;
			} else 
					g_util.Logprintf("INFO: File %s sent to SMB folder %s", aSendRequest.sSourceFile, aSendRequest.sSMBFolder);

		} else { // FTP
			CString sConnectFailReason;
			CFTPClient ftp_out;

			int nRetries = 3;
			BOOL bFileCopyOK = FALSE;
			BOOL bIsConnected = FALSE;
			while (bFileCopyOK == FALSE && nRetries-- >= 0) {

				ftp_out.InitSession();
				ftp_out.SetAutoXCRC(TRUE);

				bIsConnected = ftp_out.OpenConnection(aSendRequest.sFtpServer, aSendRequest.sFtpUserName,
						aSendRequest.sFtpPassword, aSendRequest.bPasv, aSendRequest.sFtpFolder, 21,
						120, sErrorMessage, sConnectFailReason);
				if (bIsConnected)
					g_util.Logprintf("INFO: Connected til FTP %s..", aSendRequest.sFtpServer);
				else {
					g_util.Logprintf("ERROR: Unable to connect til FTP %s.. %s ", aSendRequest.sFtpServer, sConnectFailReason);
					g_util.Logprintf("ERROR: FTP Connection reason: %s", sConnectFailReason);
					g_util.Logprintf("ERROR: FTP Connection reply: %s", g_util.LimitErrorMessage(ftp_out.GetLastReply()));
					g_util.Logprintf("ERROR: FTP Connection error: %s", sErrorMessage);

					ftp_out.Disconnect();
					bFileCopyOK = FALSE;
					::Sleep(1000);
					continue; // retry
				}

				CString sOutputFile = g_util.GetFileName(aSendRequest.sSourceFile);
				bFileCopyOK = ftp_out.PutFile(aSendRequest.sSourceFile, sOutputFile, FALSE, sErrorMessage);

				if (bFileCopyOK == FALSE) {
					g_util.Logprintf("ERROR: PutFile(%s,%s) %s", aSendRequest.sSourceFile, sOutputFile, sErrorMessage);

					// Force close 
					ftp_out.Disconnect();
	
					Sleep(2000);
					continue; // retry
				}

			} //end while retries (per file)
			if (bFileCopyOK)
				g_util.Logprintf("INFO: File %s sent to FTP server %s/%s", aSendRequest.sSourceFile, aSendRequest.sFtpServer, aSendRequest.sFtpFolder);
		}

		if (aSendRequest.bDeleteFileAfter)
			::DeleteFile(aSendRequest.sSourceFile);

		g_db.RemoveFileSendRequest(aSendRequest.n64GUID, sErrorMessage);
	}

	return TRUE;
}

BOOL ProcessTriggers()
{
	CUtils util;
	int nPublicationID = 0;
	CTime tPubDate;
	int nEditionID = 0;
	int nSectionID = 0;
	int nLocationID = 0;

	int nMessageType = -1;
	CString s;
	int nMiscInt = 0;					// This is version 
	CString sMiscString = _T("");		// This is ChannelIDList
	int nProductionID = 0;

	CString sErrorMessage = _T("");

	if (g_db.GetNextCustomMessage(nPublicationID, tPubDate, nEditionID, nSectionID, nLocationID, nMessageType, nMiscInt, sMiscString, sErrorMessage) == FALSE) {
		g_util.Logprintf("ERROR: ProcessCustomMessageQueue() - %s", sErrorMessage);
		return FALSE;
	}
	if (nPublicationID <= 0)
		return TRUE;

	g_util.Logprintf("INFO: Get Trigger message..(PubID %d, %.2d-%.2d) Type %d %d - %s", nPublicationID,tPubDate.GetDay(), tPubDate.GetMonth(), nMessageType, nMiscInt, sMiscString);

	CStringArray aChannelIDList;

	util.StringSplitter(sMiscString, ",", aChannelIDList);
	if (aChannelIDList.GetCount() == 0) {
		g_db.RemoveCustomMessageJob(nPublicationID, tPubDate, nEditionID, nSectionID, nLocationID, nMessageType, sErrorMessage);
		return TRUE;
	}

	if (g_db.GetProductionID(nPublicationID, tPubDate, nProductionID, sErrorMessage) == FALSE)
		g_util.Logprintf("ERROR: ProcessCustomMessageQueue() - %s", sErrorMessage);
	if (nProductionID <= 0) {
		g_db.RemoveCustomMessageJob(nPublicationID, tPubDate, nEditionID, nSectionID, nLocationID, nMessageType, sErrorMessage);
		return TRUE;
	}

	CString sXMLTemplate = g_util.LoadFile(g_prefs.m_CustomMessageTemplate);

	if (sXMLTemplate == "")
		g_util.Logprintf("ERROR: ProcessCustomMessageQueue() - template file %s not found", g_prefs.m_CustomMessageTemplate);


	CString sPublication = _T("");
	CString sPublicationAlias = _T("");
	CString sEdition = _T("");
	CString sEditionAlias = _T("");
	CString sUploadFolder = _T("");
	CString sComment = _T("");
	int nVersion = 0;

	CString sPublicationExtendedAlias = _T("");
	CString sPublicationExtendedAlias2 = _T("");
	// Construct filename
	CString sFileName = g_prefs.m_CustomMessageFileName;
	CString sDate = g_util.Date2String(tPubDate, g_prefs.m_CustomMessageDateFormat, TRUE, 0);
	g_db.GetIDNames(nPublicationID, nEditionID, sPublication, sPublicationAlias, sPublicationExtendedAlias, sPublicationExtendedAlias2, sEdition, sEditionAlias, sUploadFolder, sErrorMessage);

	g_db.GetProductionParameters(nProductionID, sComment, nVersion, sErrorMessage);


	sPublication = sPublicationExtendedAlias2 != _T("") ? sPublicationExtendedAlias2 : (sPublicationExtendedAlias != _T("") ? sPublicationExtendedAlias : sPublicationAlias);

	sFileName.Replace("%p", sPublicationAlias);
	sPublicationAlias += "     ";
	sFileName.Replace("%2p", sPublicationAlias.Left(2));
	sFileName.Replace("%3p", sPublicationAlias.Left(3));
	sFileName.Replace("%4p", sPublicationAlias.Left(4));
	sFileName.Replace("%5p", sPublicationAlias.Left(5));

	sFileName.Replace("%P", sPublication);
	sPublication += "      ";
	sFileName.Replace("%2P", sPublication.Left(2));
	sFileName.Replace("%3P", sPublication.Left(3));
	sFileName.Replace("%4P", sPublication.Left(4));
	sFileName.Replace("%5P", sPublication.Left(5));

	sFileName.Replace("%e", sEditionAlias);
	sFileName.Replace("%E", sEdition);
	sFileName.Replace("%d", sDate);
	sFileName.Replace("%D", sDate);
	sFileName.Replace("%k", sComment);
	sFileName.Replace("%K", sComment);


	// Get Release info
	CStringArray aReleaseList;
	g_util.StringSplitter(sUploadFolder, ";", aReleaseList);

	int nReleaseDaysBeforePubDate = 0;
	int nReleaseHour = 0;
	int nReleaseMinute = 0;
	if (aReleaseList.GetCount() >= 5) {
		nReleaseDaysBeforePubDate = atoi(aReleaseList[2]);
		nReleaseHour = atoi(aReleaseList[3]);
		nReleaseMinute = atoi(aReleaseList[4]);
	}



	CString sThisXMLTemplate = sXMLTemplate;
	sThisXMLTemplate.Replace("%d", g_util.Date2String(tPubDate, g_prefs.m_CustomMessageReleasePubDateFormat, TRUE, 0));

	CString sVersion;
	sVersion.Format("%d", nMiscInt > 0 ? nMiscInt : 1);
	sThisXMLTemplate.Replace("%v", sVersion);

	sFileName.Replace("%v", sVersion);
	sFileName.Replace("%V", sVersion);

	sThisXMLTemplate.Replace("%k", sComment);
	sThisXMLTemplate.Replace("%K", sComment);

	s.Format("%d", nVersion);
	sThisXMLTemplate.Replace("%w", s);
	sThisXMLTemplate.Replace("%W", s);
	sFileName.Replace("%w", s);
	sFileName.Replace("%W", s);

	for (int i = 0; i < aChannelIDList.GetCount(); i++) {
		int nChannelID = atoi(aChannelIDList[i]);
		CString sChannelFolder = _T("");
		BOOL bGenerateXmlCommandFile = FALSE;
		CTime tReleaseTime;
		int nTriggerType = TRIGGERMODE_NONE;
		if (g_db.GetChannelInfo(nProductionID, nChannelID, tReleaseTime, nTriggerType, sErrorMessage) == FALSE) {
			g_util.Logprintf("ERROR: GetChannelInfo() - %s", sErrorMessage);
			continue;
		}

		if (nTriggerType == TRIGGERMODE_NONE)
			continue;

		sThisXMLTemplate.Replace("%x", g_util.Date2String(tReleaseTime, g_prefs.m_CustomMessageReleaseDateFormat, TRUE, 0));

		CString sReleaseTime;
		sReleaseTime.Format("%.2d:%.2d", tReleaseTime.GetHour(), tReleaseTime.GetMinute());
		sThisXMLTemplate.Replace("%y", sReleaseTime);

		::DeleteFile(util.GetTempFolder() + _T("\\") + sFileName);

		g_util.SaveFile(util.GetTempFolder() + _T("\\") + sFileName, sThisXMLTemplate);
	
		g_util.Logprintf("DEBUG: %s", sThisXMLTemplate);
		
		if (SendFileToChannel(util.GetTempFolder() + _T("\\") + sFileName, nChannelID, _T("")) == TRUE)
			g_db.UpdateProductionFlag(nProductionID, 5, sErrorMessage);
	
		if (sChannelFolder != "")
			g_util.Logprintf("INFO: Generated custom message %s", sChannelFolder + _T("\\") + sFileName);
		else
			g_util.Logprintf("INFO: Generated custom message %s",  sFileName);
	}

	if (g_db.RemoveCustomMessageJob(nPublicationID, tPubDate, nEditionID, nSectionID, nLocationID, nMessageType, sErrorMessage) == FALSE) {
		g_util.Logprintf("ERROR: ProcessCustomMessageQueue() - %s", sErrorMessage);
		return FALSE;
	}

	return TRUE;
}

BOOL SendFileToChannel(CString sFileName, int nChannelID, CString sSubFolder)
{

	CString sErrorMessage;
	CString sConnectFailReason;
	CFTPClient ftp_out;
	CSFTPClient sftp_out;

	g_util.Logprintf("INFO: SendFileToChannel(ChannelID %d, file %s)", nChannelID,sFileName);

	if (g_util.FileExist(sFileName) == FALSE)
	{
		g_util.Logprintf("ERROR: Trigger file %s not found", sFileName);
		return FALSE;
	}

	g_db.LoadNewChannel(nChannelID, sErrorMessage);

	CHANNELSTRUCT *pChannel = g_prefs.GetChannelStruct(nChannelID);

	if (pChannel == NULL) {
		g_util.Logprintf("ERROR: Unable to obtain channel structure..");
		return FALSE;
	}

	BOOL bIsConnected = FALSE;
	BOOL bFileCopyOK = FALSE;
	if (pChannel->m_outputuseftp == NETWORKTYPE_FTP) {
		int nRetries = pChannel->m_retries;
		if (nRetries < 1)
			nRetries = 1;

		while ((bIsConnected == FALSE || bFileCopyOK== FALSE) && nRetries-- >= 0) {

			if (pChannel->m_outputfpttls != FTPTYPE_SFTP) {
				ftp_out.InitSession();
				if (pChannel->m_outputftpXCRC)
					ftp_out.SetAutoXCRC(TRUE);

				if (pChannel->m_outputfpttls == FTPTYPE_FTPES) {
					ftp_out.SetSSL(FALSE);
					ftp_out.SetAuthTLS(TRUE);
				}
				else if (pChannel->m_outputfpttls == FTPTYPE_FTPS) {
					ftp_out.SetSSL(TRUE);
					ftp_out.SetAuthTLS(FALSE);
				}

				if (pChannel->m_outputftpbuffersize > 0) {			
					ftp_out.SetSendBufferSize(pChannel->m_outputftpbuffersize);
				}
				bIsConnected = ftp_out.OpenConnection(pChannel->m_outputftpserver, pChannel->m_outputftpuser,
								pChannel->m_outputftppw, pChannel->m_outputftppasv, pChannel->m_outputftpfolder, pChannel->m_outputftpport,
								pChannel->m_ftptimeout, sErrorMessage, sConnectFailReason);
				if (bIsConnected)
					g_util.Logprintf("INFO: Connected til FTP %s..", pChannel->m_outputftpserver);
				else
					g_util.Logprintf("ERROR: Unable to connect til FTP %s.. %s ", pChannel->m_outputftpserver, sConnectFailReason);
			}
			else {
				sftp_out.InitSession();
				bIsConnected = sftp_out.OpenConnection(pChannel->m_outputftpserver, pChannel->m_outputftpuser,
							pChannel->m_outputftppw, pChannel->m_outputftppasv, pChannel->m_outputftpfolder, pChannel->m_outputftpport,
							pChannel->m_ftptimeout, sErrorMessage, sConnectFailReason);
			}

			if (bIsConnected == FALSE) {
				CString sFolder;
				g_util.Logprintf("ERROR: FTP Connection error");
				g_util.Logprintf("ERROR: FTP Connection reason: %s", sConnectFailReason);
				if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
					g_util.Logprintf("ERROR: FTP Connection reply: %s", g_util.LimitErrorMessage(ftp_out.GetLastReply()));
				g_util.Logprintf("ERROR: FTP Connection error: %s", sErrorMessage);

				if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
					ftp_out.Disconnect();
				else
					sftp_out.Disconnect();
				bFileCopyOK = FALSE;
				::Sleep(1000);
				continue; // retry
			}
			/*
			if (pChannel->m_outputftpfolder != "") {
				CString sCurrentDir = _T("");
				if (pChannel->m_outputfpttls != FTPTYPE_SFTP) {
					if (ftp_out.GetCurrentDirectory(sCurrentDir, sErrorMessage) == FALSE)
						g_util.Logprintf("ERROR: GetCurrentDirectory() - %s", sErrorMessage);
					if (sCurrentDir.CompareNoCase(pChannel->m_outputftpfolder) != 0)
						if (ftp_out.ChangeDirectory(pChannel->m_outputftpfolder, sErrorMessage) == FALSE)
							g_util.Logprintf("ERROR: ChangeDirectory(%s) - %s", pChannel->m_outputftpfolder, sErrorMessage);
				}
				else {
					if (sftp_out.GetCurrentDirectory(sCurrentDir, sErrorMessage) == FALSE)
						g_util.Logprintf("ERROR: GetCurrentDirectory() %s ", sErrorMessage);
					if (sCurrentDir.CompareNoCase(pChannel->m_outputftpfolder) != 0)
						if (sftp_out.ChangeDirectory(pChannel->m_outputftpfolder, sErrorMessage) == FALSE)
							g_util.Logprintf("ERROR: ChangeDirectory(%s) - %s", pChannel->m_outputftpfolder, sErrorMessage);
				}
			}
			*/

			if (sSubFolder != "") {
				BOOL bExists = FALSE;
				BOOL bChDirOK = FALSE;
				if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
					bExists = ftp_out.DirectoryExists(sSubFolder, sErrorMessage);
				else
					bExists = sftp_out.DirectoryExists(sSubFolder, sErrorMessage);

				if (bExists == FALSE) {

					if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
						bChDirOK = ftp_out.CreateDirectory(sSubFolder, sErrorMessage);
					else
						bChDirOK = sftp_out.CreateDirectory(sSubFolder, sErrorMessage);

					if (bChDirOK == FALSE) {

						g_util.Logprintf( "ERROR: FTP CreateDirectory(%s) failed - %s", sSubFolder, sErrorMessage);
						
						if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
							ftp_out.Disconnect();
						else
							sftp_out.Disconnect();
						bFileCopyOK = FALSE;
						Sleep(2000);
						continue; // retry
					}
				}

			}

			CString sOutputFile = g_util.GetFileName(sFileName);

			if (sSubFolder != "") 
				sOutputFile = "/" + ftp_out.TrimSlashes(pChannel->m_outputftpfolder) + "/" + ftp_out.TrimSlashes(sSubFolder + "/") + sOutputFile;
			
			
			if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
				bFileCopyOK = ftp_out.PutFile(sFileName, sOutputFile, FALSE, sErrorMessage);
			else
				bFileCopyOK = sftp_out.PutFile(sFileName, sOutputFile, FALSE, sErrorMessage);
			g_util.Logprintf("RESPONSE: PutFile(%s,%s) %s", sFileName, sOutputFile, ftp_out.GetLastReply());

			if (bFileCopyOK == FALSE) {
				g_util.Logprintf("ERROR: PutFile(%s,%s) %s", sFileName, sOutputFile, sErrorMessage);

				// Force close 
				if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
					ftp_out.Disconnect();
				else
					sftp_out.Disconnect();

				Sleep(2000);
				continue; // retry
			}

		} //end while retries (per file)


		if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
			ftp_out.Disconnect();
		else
			sftp_out.Disconnect();

		return bFileCopyOK;
 
	}
	else if (pChannel->m_outputuseftp == NETWORKTYPE_SHARE) {
		if (sSubFolder != "")
			return ::CopyFile(sFileName, pChannel->m_outputfolder + _T("\\") + sSubFolder + _T("\\") + g_util.GetFileName(sFileName), FALSE);
		else
			return ::CopyFile(sFileName, pChannel->m_outputfolder + _T("\\") + g_util.GetFileName(sFileName), FALSE);
	}

	return TRUE;
}

void ReportFolderAccess()
{
	CString sResult = _T("");
	CString sServerName = _T("");
	CString sErrorMessage = _T("");
	BOOL bHasError = FALSE;
	for (int i = 0; i < g_prefs.m_FileServerList.GetCount(); i++) {
		FILESERVERSTRUCT *pFileServer = &g_prefs.m_FileServerList[i];
		if (pFileServer->m_servertype == 1) {

			CString sBaseFolder = g_util.IsValidIP(pFileServer->m_IP) ?
				_T("\\\\") + pFileServer->m_IP + _T("\\") + pFileServer->m_sharename :
				_T("\\\\") + pFileServer->m_servername + _T("\\") + pFileServer->m_sharename;

			if (g_util.CheckFolderWithPing(sBaseFolder) == FALSE) {
				g_util.Logprintf("ERROR: CheckFolderWithPing(%s) failed", sBaseFolder);
				bHasError = TRUE;
			}

			break;
		}
	}

	sResult = bHasError ? _T("Error accessing share!") : _T("Access OK");

	g_db.UpdateServiceFileServer(sServerName, 1, sResult, "", "", sErrorMessage);

	g_db.InsertLogEntry(EVENTCODE_FILESERVER_CHECK, "Fileserver check", "", sResult, 0, 0, 0, "", sErrorMessage);

}

BOOL InputFileRetry(CString sFileName, CString sQueueName)
{
	CString sErrorMessage = _T("");
	CString sErrorRootFolder = _T("");
	CStringArray sInputIDs;
	CStringArray sInputFolders;
	CStringArray sInputQueues;
	FILEINFOSTRUCT aFiles[MAXFILESINSCAN];

	if (g_db.GetErrorRootFolder(sErrorRootFolder, sErrorMessage) == FALSE) 
		return FALSE;
	sInputIDs.RemoveAll();
	sInputFolders.RemoveAll();
	if (g_db.GetInputFolders(sInputFolders, sInputIDs, sInputQueues, sErrorMessage) == FALSE)
		return FALSE;

	// retry all trigger ??
	if (sFileName == _T("") && sQueueName == _T("")) {
		for (int i = 0; i < sInputFolders.GetCount(); i++) {
			int nFiles = g_util.ScanDir(sErrorRootFolder + _T("\\") + sInputIDs[i], _T("*.pdf"), aFiles, MAXFILESINSCAN, _T(""), TRUE, FALSE);
			if (nFiles > 0)
			{
				for (int j = 0; j < nFiles; j++) {
					::CopyFile(sErrorRootFolder + _T("\\") + sInputIDs[i] + _T("\\") + aFiles[j].sFileName,
							 sInputFolders[i] + _T("\\") + aFiles[j].sFileName, FALSE);
				}
			}
		}

		return TRUE;
	}

	if (sInputIDs.GetCount() == 0 || sInputIDs.GetCount() != sInputFolders.GetCount())
		return FALSE;
	for (int i = 0; i < sInputQueues.GetCount(); i++) {
		if (sQueueName.CompareNoCase(sInputQueues[i]) == 0) {
			if (g_util.FileExist(sErrorRootFolder + _T("\\") + sInputIDs[i] + _T("\\") + sFileName)) {
				::CopyFile(sErrorRootFolder + _T("\\") + sInputIDs[i] + _T("\\") + sFileName, sInputFolders[i] + _T("\\") + sFileName, FALSE);
				break;
			}
		}
	}

	return TRUE;
}

BOOL ExportFileRetry(CString sFileName, CString sChannelName)
{
	CString sErrorMessage = _T("");
	int nChannelID = 0;
	int nMasterCopySeparationSet = 0;
	if (g_db.GetFileNameChannel(sFileName, sChannelName, nChannelID, nMasterCopySeparationSet, sErrorMessage) == FALSE) {
		g_util.Logprintf("ERROR: db.GetFileNameChannel() - %s", sErrorMessage);
		return FALSE;
	}

	if (nChannelID > 0 && nMasterCopySeparationSet > 0) {
		if (g_db.UpdateChannelStatus(nMasterCopySeparationSet, nChannelID, 0, sErrorMessage) == FALSE) {
			g_util.Logprintf("ERROR: db.UpdateChannelStatus() - %s", sErrorMessage);
			return FALSE;
		}
	}

	return TRUE;
	
}

BOOL ProcessFileRetry(CString sFileName, CString sProcessingName)
{
	CString sErrorMessage = _T("");
	BOOL bRet = TRUE;
	
	sProcessingName.MakeLower();
	int nPdfType = POLLINPUTTYPE_LOWRESPDF;
	if (sProcessingName.Find("low") == -1)
		nPdfType = POLLINPUTTYPE_PRINTPDF;

	int nMasterCopySeparationSet = g_db.GetMasterSetFromFileName(sFileName,  sErrorMessage);

	if (nMasterCopySeparationSet > 0)
		bRet = g_db.ResetProcessingStatus(nMasterCopySeparationSet, nPdfType, sErrorMessage);

	// Also reproof....
	g_db.ResetProofStatus(nMasterCopySeparationSet, sErrorMessage);

	return bRet;
}


int DeleteOldFilesMaskedShare(CHANNELPURGESTRUCT *pChannelPurge, CHANNELSTRUCT *pChannel, CString sFilePath)
{
	if (pChannelPurge == NULL || pChannel == NULL)
		return -1;

	int nFiles = g_util.DeleteOldFiles(sFilePath, pChannelPurge->m_sInputAlias + "*.*", pChannelPurge->m_nAutoPurgeKeepDaysArchive, 0, 0);
	if (pChannelPurge->m_sOutputAlias != "")
		nFiles += g_util.DeleteOldFiles(sFilePath, pChannelPurge->m_sOutputAlias + "*.*", pChannelPurge->m_nAutoPurgeKeepDaysArchive, 0, 0);
	if (pChannelPurge->m_sExtendedAlias != "")
		nFiles += g_util.DeleteOldFiles(sFilePath, pChannelPurge->m_sExtendedAlias + "*.*", pChannelPurge->m_nAutoPurgeKeepDaysArchive, 0, 0);
	if (pChannelPurge->m_sExtendedAlias2 != "")
		nFiles += g_util.DeleteOldFiles(sFilePath, pChannelPurge->m_sExtendedAlias2 + "*.*", pChannelPurge->m_nAutoPurgeKeepDaysArchive, 0, 0);

	return nFiles;
}

int DeleteOldFilesMaskedFTP(CFTPClient *pFtp, CHANNELPURGESTRUCT *pChannelPurge, CHANNELSTRUCT *pChannel, CString sFilePath, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (pChannelPurge == NULL || pChannel == NULL || pFtp == NULL)
		return -1;

	// Major hack for deleting APHA_xxx
	CString sSpecialAlias = "";
	if (pChannelPurge->m_sInputAlias == "APHA1" || pChannelPurge->m_sInputAlias == "APHA2")
		sSpecialAlias = _T("APHA");

	int nFiles = pFtp->DeleteFiles(sFilePath, pChannelPurge->m_sInputAlias + "*.*", pChannelPurge->m_nAutoPurgeKeepDaysArchive, sErrorMessage);
	if (nFiles < 0)
		return -1;
	if (pChannelPurge->m_sOutputAlias != "") {
		nFiles = pFtp->DeleteFiles(sFilePath, pChannelPurge->m_sOutputAlias + "*.*", pChannelPurge->m_nAutoPurgeKeepDaysArchive, sErrorMessage);
		if (nFiles < 0)
			return -1;
	}
	if (pChannelPurge->m_sExtendedAlias != "") {
		nFiles = pFtp->DeleteFiles(sFilePath, pChannelPurge->m_sExtendedAlias + "*.*", pChannelPurge->m_nAutoPurgeKeepDaysArchive, sErrorMessage);
		if (nFiles < 0)
			return -1;
	}
	if (pChannelPurge->m_sExtendedAlias2 != "") {
		nFiles = pFtp->DeleteFiles(sFilePath, pChannelPurge->m_sExtendedAlias2 + "*.*", pChannelPurge->m_nAutoPurgeKeepDaysArchive, sErrorMessage);
		if (nFiles < 0)
			return -1;
	}

	if (sSpecialAlias != "")
		nFiles = pFtp->DeleteFiles(sFilePath, sSpecialAlias + "*.*", pChannelPurge->m_nAutoPurgeKeepDaysArchive, sErrorMessage);
	if (nFiles < 0)
		return -1;

	return nFiles;
}





BOOL PublicationNameMatch(CString sFileName, int nChannelID)
{
	sFileName.MakeUpper();
	sFileName.Replace("_", "-");
	sFileName.Replace(".", "-");
	int n;
	CHANNELPURGESTRUCT *pChannelPurge = NULL;
	for (int j = 0; j < g_prefs.m_ChannelPurgeList.GetCount(); j++) {
		if (g_prefs.m_ChannelPurgeList[j].m_nChannelID == nChannelID && g_prefs.m_ChannelPurgeList[j].m_nAutoPurgeKeepDaysArchive > 0) {
			pChannelPurge = &g_prefs.m_ChannelPurgeList[j];

			n = pChannelPurge->m_sInputAlias.GetLength();
			if (n > 0) {
				if (sFileName.Left(n) == pChannelPurge->m_sInputAlias)
					return TRUE;
			}

			n = pChannelPurge->m_sOutputAlias.GetLength();
			if (n > 0) {
				if (sFileName.Left(n) == pChannelPurge->m_sOutputAlias)
					return TRUE;
			}

			n = pChannelPurge->m_sExtendedAlias.GetLength();
			if (n > 0) {
				if (sFileName.Left(n) == pChannelPurge->m_sExtendedAlias)
					return TRUE;
			}

			n = pChannelPurge->m_sExtendedAlias2.GetLength();
			if (n > 0) {
				if (sFileName.Left(n) == pChannelPurge->m_sExtendedAlias2)
					return TRUE;
			}
		}
	}
	return FALSE;

}


#define MAXFILESPERSCAN 5000
BOOL CleanExportFolders()
{

	FILEINFOSTRUCT aFtpFiles[MAXFILESPERSCAN];
	CFTPClient ftp_out;
	CSFTPClient sftp_out;

	CStringArray aSubLevel1;
	CStringArray aSubLevel2;
	CStringArray aSubLevel3;
	CStringArray aSubLevel4;

	CString sErrorMessage = _T("");
	CString sConnectFailReason = _T("");
	// ChannelNames.DeleteOldOutputFilesDays

	g_prefs.m_ChannelPurgeList.RemoveAll();
	if (g_db.GetPublicationCleanDays(sErrorMessage) == FALSE) {
		g_util.Logprintf("ERROR: GetPublicationCleanDays() - %s", sErrorMessage);
		return FALSE;
	}

	if (g_prefs.m_logging > 1)
		g_util.Logprintf("INFO: Starting deleting of old files in %d export output folder(s)..", g_prefs.m_ChannelPurgeList.GetCount());

	for(int i=0; i<g_prefs.m_ChannelPurgeList.GetCount(); i++) {
		CHANNELPURGESTRUCT *pChannelPurge = &g_prefs.m_ChannelPurgeList[i];
		CHANNELSTRUCT *pChannel = g_prefs.GetChannelStruct(pChannelPurge->m_nChannelID);
		if (pChannel == NULL)
			continue;
	

		// Go down four levels non-recursively..
		if (pChannel->m_outputuseftp == NETWORKTYPE_SHARE) {
			int nFiles = DeleteOldFilesMaskedShare(pChannelPurge, pChannel, pChannel->m_outputfolder);
			g_util.GetDirectoryList(pChannel->m_outputfolder, _T("*.*"), aSubLevel1);
			for (int j = 0; j < aSubLevel1.GetCount(); j++) {
				CString sThisFolder1 = pChannel->m_outputfolder + _T("\\") + aSubLevel1[j];
				nFiles += DeleteOldFilesMaskedShare(pChannelPurge, pChannel, sThisFolder1);
				aSubLevel2.RemoveAll();
				g_util.GetDirectoryList(sThisFolder1, _T("*.*"), aSubLevel2);

				for (int k = 0; k < aSubLevel2.GetCount(); k++) {
					CString sThisFolder2 = sThisFolder1 + _T("\\") + aSubLevel2[k];
					

					aSubLevel3.RemoveAll();
					g_util.GetDirectoryList(sThisFolder2, _T("*.*"), aSubLevel3);

					for (int m = 0; m < aSubLevel3.GetCount(); m++) {

						CString sThisFolder3 = sThisFolder2 + _T("\\") + aSubLevel3[m];
						nFiles += DeleteOldFilesMaskedShare(pChannelPurge, pChannel, sThisFolder3);

						aSubLevel4.RemoveAll();
						g_util.GetDirectoryList(sThisFolder3, _T("*.*"), aSubLevel4);
						for (int m = 0; m < aSubLevel3.GetCount(); m++) {

							CString sThisFolder4 = sThisFolder3 + _T("\\") + aSubLevel4[m];
							nFiles += DeleteOldFilesMaskedShare(pChannelPurge, pChannel, sThisFolder4);

							::RemoveDirectory(sThisFolder4);
						}

						::RemoveDirectory(sThisFolder3);
					}
					::RemoveDirectory(sThisFolder2);
				}
				::RemoveDirectory(sThisFolder1);
			}

		}
		else if (pChannel->m_outputuseftp == NETWORKTYPE_FTP) {
			BOOL bIsConnected = FALSE;
			if (pChannel->m_outputfpttls != FTPTYPE_SFTP) {
				ftp_out.InitSession();
				if (pChannel->m_outputftpXCRC)
					ftp_out.SetAutoXCRC(TRUE);

				if (pChannel->m_outputfpttls == FTPTYPE_FTPES) {
					ftp_out.SetSSL(FALSE);
					ftp_out.SetAuthTLS(TRUE);
				}
				else if (pChannel->m_outputfpttls == FTPTYPE_FTPS) {
					ftp_out.SetSSL(TRUE);
					ftp_out.SetAuthTLS(FALSE);
				}

				bIsConnected = ftp_out.OpenConnection(pChannel->m_outputftpserver, pChannel->m_outputftpuser,
					pChannel->m_outputftppw, pChannel->m_outputftppasv, pChannel->m_outputftpfolder, pChannel->m_outputftpport,
					pChannel->m_ftptimeout, sErrorMessage, sConnectFailReason);
			}
			else {
				sftp_out.InitSession();
				bIsConnected = sftp_out.OpenConnection(pChannel->m_outputftpserver, pChannel->m_outputftpuser,
					pChannel->m_outputftppw, pChannel->m_outputftppasv, pChannel->m_outputftpfolder, pChannel->m_outputftpport,
					pChannel->m_ftptimeout, sErrorMessage, sConnectFailReason);

			}

			if (bIsConnected == FALSE) {
				CString sMsg;
				if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
					sMsg.Format(_T("ftp://%s:%d/%s"), pChannel->m_outputftpserver, pChannel->m_outputftpport, pChannel->m_outputftpfolder);
				else
					sMsg.Format(_T("sftp://%s:%d/%s"), pChannel->m_outputftpserver, pChannel->m_outputftpport, pChannel->m_outputftpfolder);

				g_util.Logprintf("ERROR: Unable to connect to FTP %s - %s", sMsg, sConnectFailReason);
				if (pChannel->m_outputfpttls != FTPTYPE_SFTP)
					ftp_out.Disconnect();
				else
					sftp_out.Disconnect();
				continue; // next channel
			}

			// Connected..

			// move to subfolder?
			if (DeleteOldFilesMaskedFTP(&ftp_out, pChannelPurge, pChannel, _T(""), sErrorMessage) < 0) {
				g_util.Logprintf("ERROR: ftp.DeleteFiles() - %s", sErrorMessage);
				ftp_out.Disconnect();
			}

			CStringArray aFtpSubFolders;
			CStringArray aFtpSubFolders2;
			CStringArray aFtpSubFolders3;
			CStringArray aFtpSubFolders4;

			if (ftp_out.GetSubfolders(aFtpSubFolders, sErrorMessage) != -1) {
				for (int subf = 0; subf < aFtpSubFolders.GetCount(); subf++) {
					if (ftp_out.ChangeDirectory(aFtpSubFolders[subf], sErrorMessage) == FALSE) {
						g_util.Logprintf("ERROR: ftp.ChangeDirectory() - %s", sErrorMessage);
						continue;
					}

					//g_util.Logprintf("INFO: ftp.DeleteFiles() in %s", aFtpSubFolders[subf]);

					if (DeleteOldFilesMaskedFTP(&ftp_out, pChannelPurge, pChannel, _T(""), sErrorMessage) < 0) {
						g_util.Logprintf("ERROR: ftp.DeleteFiles() - %s", sErrorMessage);
						continue;
					}

					aFtpSubFolders2.RemoveAll();
					if (ftp_out.GetSubfolders(aFtpSubFolders2, sErrorMessage) != -1) {

						for (int subf2 = 0; subf2 < aFtpSubFolders2.GetCount(); subf2++) {
							if (ftp_out.ChangeDirectory(aFtpSubFolders2[subf2], sErrorMessage) == FALSE)
								continue;

						//	g_util.Logprintf("INFO: ftp.DeleteFiles() in %s", aFtpSubFolders2[subf2]);

							if (DeleteOldFilesMaskedFTP(&ftp_out, pChannelPurge, pChannel, _T(""), sErrorMessage) < 0) {
								g_util.Logprintf("ERROR: ftp.DeleteFiles() - %s", sErrorMessage);
								continue;
							}

							aFtpSubFolders3.RemoveAll();
							if (ftp_out.GetSubfolders(aFtpSubFolders3, sErrorMessage) != -1) {

								for (int subf3 = 0; subf3 < aFtpSubFolders3.GetCount(); subf3++) {
									if (ftp_out.ChangeDirectory(aFtpSubFolders3[subf3], sErrorMessage) == FALSE)
										continue;

								//	g_util.Logprintf("INFO: ftp.DeleteFiles() in %s", aFtpSubFolders3[subf3]);

									if (DeleteOldFilesMaskedFTP(&ftp_out, pChannelPurge, pChannel, _T(""), sErrorMessage) < 0) {
										g_util.Logprintf("ERROR: ftp.DeleteFiles() - %s", sErrorMessage);
										continue;
									}

									aFtpSubFolders4.RemoveAll();
									if (ftp_out.GetSubfolders(aFtpSubFolders4, sErrorMessage) != -1) {

										for (int subf4 = 0; subf4 < aFtpSubFolders4.GetCount(); subf4++) {
											if (ftp_out.ChangeDirectory(aFtpSubFolders4[subf4], sErrorMessage) == FALSE)
												continue;

										//	g_util.Logprintf("INFO: ftp.DeleteFiles() in %s", aFtpSubFolders4[subf4]);

											if (DeleteOldFilesMaskedFTP(&ftp_out, pChannelPurge, pChannel, _T(""), sErrorMessage) < 0) {
												g_util.Logprintf("ERROR: ftp.DeleteFiles() - %s", sErrorMessage);
												continue;
											}

											ftp_out.ChangeDirectory("..", sErrorMessage);
											ftp_out.DeleteDirectory(aFtpSubFolders4[subf4], sErrorMessage);
										}
									}

									ftp_out.ChangeDirectory("..", sErrorMessage);
									ftp_out.DeleteDirectory(aFtpSubFolders3[subf3], sErrorMessage);
								}
							}

							ftp_out.ChangeDirectory("..", sErrorMessage);
							ftp_out.DeleteDirectory(aFtpSubFolders2[subf2], sErrorMessage);

						}
					}

					ftp_out.ChangeDirectory("..", sErrorMessage);
					ftp_out.DeleteDirectory(aFtpSubFolders[subf], sErrorMessage);

				}

			}
					
					
		}

	}

	return TRUE;
}

BOOL CleanFolders()
{
	CStringArray aSubLevel1;
	CStringArray aSubLevel2;
	CStringArray aSubLevel3;

	if (g_prefs.m_logging > 1)
		g_util.Logprintf("INFO: Starting deleting of old files in %d folder(s)", g_prefs.m_OldFolderList.GetCount());

	for (int i = 0; i < g_prefs.m_OldFolderList.GetCount(); i++) {
		if (g_BreakSignal)
			break;
		OLDFOLDERSTRUCT *pItem = &g_prefs.m_OldFolderList[i];

		int nFiles = g_util.DeleteOldFiles(pItem->m_folder, pItem->m_searchmask, pItem->m_days, pItem->m_hours, pItem->m_minutes);

		if (pItem->m_searchsubfolders) {
			aSubLevel1.RemoveAll();
			g_util.GetDirectoryList(pItem->m_folder, _T("*.*"), aSubLevel1);
			for (int j = 0; j < aSubLevel1.GetCount(); j++) {
				CString sThisFolder1 = pItem->m_folder + _T("\\") + aSubLevel1[j];
				nFiles += g_util.DeleteOldFiles(sThisFolder1, pItem->m_searchmask, pItem->m_days, pItem->m_hours, pItem->m_minutes);

				aSubLevel2.RemoveAll();
				g_util.GetDirectoryList(sThisFolder1, _T("*.*"), aSubLevel2);

				for (int k = 0; k < aSubLevel2.GetCount(); k++) {
					CString sThisFolder2 = sThisFolder1 + _T("\\") + aSubLevel2[k];
					nFiles += g_util.DeleteOldFiles(sThisFolder2, pItem->m_searchmask, pItem->m_days, pItem->m_hours, pItem->m_minutes);

					aSubLevel3.RemoveAll();
					g_util.GetDirectoryList(sThisFolder2, _T("*.*"), aSubLevel3);

					for (int m = 0; m < aSubLevel3.GetCount(); m++) {

						CString sThisFolder3 = sThisFolder2 + _T("\\") + aSubLevel3[m];
						nFiles += g_util.DeleteOldFiles(sThisFolder3, pItem->m_searchmask, pItem->m_days, pItem->m_hours, pItem->m_minutes);

						if (pItem->m_deleteEmptyFolders)
							::RemoveDirectory(sThisFolder3);
					}
					if (pItem->m_deleteEmptyFolders)
						::RemoveDirectory(sThisFolder2);
				}
				if (pItem->m_deleteEmptyFolders)
					::RemoveDirectory(sThisFolder1);
			}
		}
		if (g_prefs.m_logging > 1)
			g_util.Logprintf("INFO: Deleted %d file(s) from folder %s (mask %s)", nFiles, pItem->m_folder, pItem->m_searchmask);
	}

	return TRUE;
}

// For PDFHUB status view
void EnumerateQueuedFile()
{
	CString sErrorMessage = _T("");
	
	CStringArray aInputSetupNames;
	CStringArray aInputSetupID;
	CStringArray aInputFolders;

	CStringArray aFoundFiles;

	g_db.GetInputFolders(aInputFolders, aInputSetupID, aInputSetupNames, sErrorMessage);

	for (int i = 0; i < aInputFolders.GetCount(); i++) {

		aFoundFiles.RemoveAll();
		int nFiles = g_util.ScanDirSimple(aInputFolders[i], "*.*", aFoundFiles, "*.log", 1000);

		g_db.DeletePresentQueuedFiles(0, aInputSetupNames[i], sErrorMessage);
		for (int j = 0; j < aFoundFiles.GetCount(); j++) {
			if (aFoundFiles[j].GetLength() > 0)
				if (aFoundFiles[j][0] != '.')
					g_db.InsertPresentQueuedFiles(0, aInputSetupNames[i], aFoundFiles[j], sErrorMessage);
		}

	}
}

FILEINFOSTRUCT sUnknownFiles[4096];
int EnumerateIllegalPages()
{
	CString sErrorMessage = _T("");
	CString sArchiveFolder;
	CString sPDFArchiveFolder = _T("");
	CString sPDFErrorFolder = _T("");
	CString sPDFUnknownFolder = _T("");
	CString sPDFInputFolder = _T("");
	CString sTiffArchiveFolder = _T("");
	CString sTiffPlateArchiveFolder = _T("");
	CString sPrefix;

	CStringArray aUnknownPageNames;

	if (g_db.GetPDFArchiveRootFolder(sPDFArchiveFolder, sPDFErrorFolder, sPDFUnknownFolder, sPDFInputFolder, sTiffArchiveFolder, sTiffPlateArchiveFolder, sErrorMessage) == FALSE) {
		g_util.Logprintf("ERROR: GetPDFArchiveRootFolder() - %s", sErrorMessage);
		return FALSE;
	}

	CStringArray aPageNames;
	CStringArray aPageNames2;
	CStringArray aPageNames3;
	CStringArray sArchiveFiles;
	if (g_db.GetNewProductions(sErrorMessage) == FALSE) {
		g_util.Logprintf("ERROR: GetNewProductions() - %s", sErrorMessage);
		return FALSE;
	}


	for (int i = 0; i < g_prefs.m_ProductList.GetCount(); i++) {
		BOOL bFoundPageInProduction = FALSE;
		PRODUCTSTRUCT *pItem = &g_prefs.m_ProductList[i];
		sPrefix.Format("%s-%.4d%.2d%.2d", pItem->m_publicationName, pItem->m_pubDate.GetYear(), pItem->m_pubDate.GetMonth(), pItem->m_pubDate.GetDay());
		//g_util.Logprintf("DEBUG: testing with Prefix  %s", sPrefix);
		if (g_db.GetProductFileNames(pItem->m_productionID, aPageNames, aPageNames2, aPageNames3, sPrefix, sErrorMessage) == FALSE) {
			g_util.Logprintf("ERROR: GetProductFileNames() - %s", sErrorMessage);
			continue;
		}



		sArchiveFolder.Format("%s\\%.4d-%.2d-%.2d\\%s", sPDFArchiveFolder, pItem->m_pubDate.GetYear(), pItem->m_pubDate.GetMonth(), pItem->m_pubDate.GetDay(), pItem->m_publicationName);
		sArchiveFiles.RemoveAll();
		//g_util.Logprintf("DEBUG: Archive folder: %s", sArchiveFolder);
		int nFiles = g_util.ScanDirSimple(sArchiveFolder, "*.*", sArchiveFiles, "*.log", 200);


		for (int j = 0; j < sArchiveFiles.GetCount(); j++) {
			BOOL bFound = FALSE;

			for (int k = 0; k < aPageNames.GetCount(); k++)
			{

				if (aPageNames[k].CompareNoCase(sArchiveFiles[j]) == 0) {
					bFound = TRUE;
					break;
				}
				if (aPageNames2[k].CompareNoCase(sArchiveFiles[j]) == 0) {
					bFound = TRUE;
					break;
				}
				if (aPageNames3[k].CompareNoCase(sArchiveFiles[j]) == 0) {
					bFound = TRUE;
					break;
				}
			}
			//if (bFound)
			//	g_util.Logprintf("DEBUG: Filename found: %s ", sArchiveFiles[j]);
			if (bFound == FALSE) {
				aUnknownPageNames.Add(sArchiveFiles[j]);
				bFoundPageInProduction = TRUE;
			}
		}

		g_db.MarkIllegalPageInProduction(bFoundPageInProduction, pItem->m_productionID, sErrorMessage);
	}

	if (g_db.SetIllegalFilesDirty(1, sErrorMessage) == FALSE) {
		g_util.Logprintf("ERROR: SetIllegalFilesDirty() - %s", sErrorMessage);
		return FALSE;
	}

	for (int i = 0; i < aUnknownPageNames.GetCount(); i++) {
		if (g_db.InsertIllegalFile(aUnknownPageNames[i], "", "", CTime::GetCurrentTime(), sErrorMessage) == FALSE)
			g_util.Logprintf("ERROR: InsertIllegalFile() - %s", sErrorMessage);
	}

	if (g_db.DeleteDirtyIllegalFiles(sErrorMessage) == FALSE)
		g_util.Logprintf("ERROR: DeleteDirtyIllegalFiles() - %s", sErrorMessage);

	return aUnknownPageNames.GetCount();

}


int EnmurateUnknownFiles()
{
	CUtils util;
	CString sErrorMessage;
	CString sErrorRootFolder;
	CStringArray sInputIDs;
	CStringArray sInputQueues;
	CStringArray sInputFolders;
	CString sFolder;

	sErrorRootFolder = _T("");
	if (g_db.GetErrorRootFolder(sErrorRootFolder, sErrorMessage) == FALSE) {
		return FALSE;
	}

	if (sErrorRootFolder == "")
		return FALSE;

	sInputIDs.RemoveAll();
	sInputFolders.RemoveAll();
	if (g_db.GetInputFolders(sInputFolders, sInputIDs, sInputQueues, sErrorMessage) == FALSE)
		return FALSE;

	if (sInputIDs.GetCount() == 0 || sInputIDs.GetCount() != sInputFolders.GetCount())
		return FALSE;

	CString sPDFArchiveFolder = _T("");
	CString sPDFErrorFolder = _T("");
	CString sPDFUnknownFolder = _T("");
	CString sPDFInputFolder = _T("");
	CString sTiffArchiveFolder = _T("");
	CString sTiffPlateArchiveFolder = _T("");
	g_db.GetPDFArchiveRootFolder(sPDFArchiveFolder, sPDFErrorFolder, sPDFUnknownFolder, sPDFInputFolder, sTiffArchiveFolder, sTiffPlateArchiveFolder, sErrorMessage);

	if (g_db.SetUnknownFilesDirty(1, sErrorMessage) == FALSE) {
		return FALSE;
	}


	BOOL bHasError = FALSE;
	for (int i = 0; i < sInputIDs.GetCount(); i++) {
		sFolder.Format("%s%s", sErrorRootFolder, sInputIDs[i]);

		int nFiles = g_util.ScanDir(sFolder, "*.*", sUnknownFiles, 4096, "*.log", TRUE, FALSE);
		for (int j = 0; j < nFiles; j++) {
			if (g_db.InsertUnknownFile(sUnknownFiles[j].sFileName, sFolder, sInputFolders[i], sUnknownFiles[j].tWriteTime, sErrorMessage) == FALSE) {
				bHasError = TRUE;
			}
		}

		if (sPDFUnknownFolder != "" && sPDFInputFolder != "") {
			nFiles = g_util.ScanDir(sPDFUnknownFolder, "*.*", sUnknownFiles, 4096, "*.log",TRUE,FALSE);
			for (int j = 0; j < nFiles; j++) {
				if (g_db.InsertUnknownFile(sUnknownFiles[j].sFileName, sPDFUnknownFolder, sPDFInputFolder, sUnknownFiles[j].tWriteTime, sErrorMessage) == FALSE) {
					bHasError = TRUE;
				}
			}
		}
	}

	if (bHasError == FALSE)
		g_db.DeleteDirtyUnknownFiles(sErrorMessage);

	return bHasError == FALSE;
}

BOOL ExportPlanJobs()
{
	CString sErrorMessage = _T("");
	CString sFileName;
	CUIntArray aProductionIDList;
	CMarkup xml;

	if (g_prefs.m_PlanExportFolder == _T(""))
		return FALSE;

	if (g_db.GetPlanExportJobs(aProductionIDList, 0, sErrorMessage) == FALSE)
	{
		g_util.Logprintf("ERROR: GetPlanExportJobs() - %s", sErrorMessage);
		return FALSE;
	}

	for (int i = 0; i < aProductionIDList.GetCount(); i++) {
		if (aProductionIDList[i] > 0) {
			PlanData *pPlan = new PlanData();
			pPlan->updatetime = CTime::GetCurrentTime();
			pPlan->version = 1;
			pPlan->weekNumber = 0;
			pPlan->state = PLANSTATE_LOCKED;
			pPlan->sender = "InfraLogic";
			pPlan->publicationName = "";
			pPlan->command = "AddPlan";

			if (g_db.LoadProductionData(aProductionIDList[i], pPlan, sErrorMessage) == FALSE) {
				g_util.Logprintf("ERROR:  LoadProductionData() %s", sErrorMessage);
				g_db.RemovePlanExportJob(aProductionIDList[i], 0, sErrorMessage);
				return FALSE;
			}

			if (pPlan->publicationName == "") {
				g_db.RemovePlanExportJob(aProductionIDList[i], 0, sErrorMessage);
				continue;
			}

			PlanDataEdition *pFirstEdition = &pPlan->editionList->GetAt(0);
			PlanDataPress *pFirstPress = &pFirstEdition->pressList->GetAt(0);



			pPlan->planName.Format("%s_%s_%.4d-%.2d-%.2d_%d",
				pFirstPress->pressName, pPlan->publicationName, pPlan->publicationDate.GetYear(), pPlan->publicationDate.GetMonth(), pPlan->publicationDate.GetDay(), pPlan->version);

			pPlan->planFileName = g_prefs.m_PlanExportFolder + _T("\\") + pPlan->planName + ".xml";

			g_util.Logprintf("INFO:  Exporting plan file %s...", pPlan->planFileName);

			xml.SetDoc("<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\r\n");
			xml.AddElem("Plan");
			xml.AddAttrib("Version", g_util.Int2String(pPlan->version));
			xml.AddAttrib("Plantype", "PressPlan");
			xml.AddAttrib("Command", "AddPlan");
			xml.AddAttrib("Planmode", "Add");
			xml.AddAttrib("ID", pPlan->planID);
			xml.AddAttrib("Name", pPlan->planName);
			CTime tNow = CTime::GetCurrentTime();
			CString sTime;
			sTime.Format("%.4d-%.2d-%.2dT%.2d:%.2d:%.2d", tNow.GetYear(), tNow.GetMonth(), tNow.GetDay(), tNow.GetHour(), tNow.GetMinute(), tNow.GetSecond());
			xml.AddAttrib("UpdateTime", sTime);
			xml.AddAttrib("Sender", "PlanExporter");
			xml.AddAttrib("xmlns", "http://tempuri.org/ImportCenter.xsd");
			xml.AddChildElem("Publication");
			xml.IntoElem();

			sTime.Format("%.4d-%.2d-%.2d", pPlan->publicationDate.GetYear(), pPlan->publicationDate.GetMonth(), pPlan->publicationDate.GetDay());
			xml.AddAttrib("PubDate", sTime);

			xml.AddAttrib("WeekReference", pPlan->weekNumber);

			xml.AddAttrib("Name", pPlan->publicationName);
			xml.AddAttrib("Alias", pPlan->publicationAlias);

			xml.AddAttrib("Customer", pPlan->customername);
			xml.AddAttrib("CustomerAlias", pPlan->customeralias);
			xml.AddChildElem("Issues");
			xml.IntoElem();
			xml.AddChildElem("Issue");
			xml.IntoElem();
			xml.AddAttrib("Name", "Main");
			xml.AddChildElem("Editions");
			xml.IntoElem();

			for (int eds = 0; eds < pPlan->editionList->GetCount(); eds++) {
				PlanDataEdition *planDataEdition = &pPlan->editionList->GetAt(eds);

				BOOL bMustProduceEdition = TRUE;
				/*	for (int press=0; press<planDataEdition->pressList->GetCount(); press++) {
				PlanDataPress *planDataPress = &planDataEdition->pressList->GetAt(press);
				if (sPressToUse.CompareNoCase(planDataPress->pressName) == 0) {
				bMustProduceEdition = TRUE;
				break;
				}
				} */
				if (bMustProduceEdition) {

					xml.AddChildElem("Edition");
					xml.IntoElem();
					xml.AddAttrib("Name", planDataEdition->editionName);
					xml.AddAttrib("EditionCopies", g_util.Int2String(planDataEdition->editionCopy));
					xml.AddAttrib("IsTimed", planDataEdition->IsTimed ? "true" : "false");
					xml.AddAttrib("TimedFrom", planDataEdition->timedFrom);
					xml.AddAttrib("TimedTo", planDataEdition->timedTo);
					xml.AddAttrib("EditionOrderNumber", planDataEdition->editionSequenceNumber);
					xml.AddAttrib("EditionComment", planDataEdition->editionComment);
					xml.AddAttrib("ZoneMasterEdition", planDataEdition->masterEdition ? "" : planDataEdition->zoneMaster);

					xml.AddChildElem("IntendedPresses");
					xml.IntoElem();

					for (int press = 0; press < planDataEdition->pressList->GetCount(); press++) {
						PlanDataPress *planDataPress = &planDataEdition->pressList->GetAt(press);

						//if (sPressToUse.CompareNoCase(planDataPress->pressName) == 0) {

						xml.AddChildElem("IntendedPress");
						xml.IntoElem();
						xml.AddAttrib("Name", planDataPress->pressName);
						xml.AddAttrib("PressTime", g_util.Time2String(planDataPress->pressRunTime));
						xml.AddAttrib("PlateCopies", g_util.Int2String(planDataPress->copies));
						xml.AddAttrib("Copies", g_util.Int2String(planDataPress->paperCopies));
						xml.AddAttrib("PostalUrl", planDataPress->postalUrl);

						xml.OutOfElem();
						//	}
					}
					xml.OutOfElem();
					xml.AddChildElem("Sections");
					xml.IntoElem();

					for (int sections = 0; sections < planDataEdition->sectionList->GetCount(); sections++) {
						PlanDataSection *planDataSection = &planDataEdition->sectionList->GetAt(sections);

						xml.AddChildElem("Section");
						xml.IntoElem();
						xml.AddAttrib("Name", planDataSection->sectionName);

						xml.AddAttrib("Format", planDataSection->format);
						xml.AddChildElem("Pages");
						xml.IntoElem();

						for (int page = 0; page < planDataSection->pageList->GetCount(); page++) {

							PlanDataPage *planDataPage = &planDataSection->pageList->GetAt(page);

							xml.AddChildElem("Page");
							xml.IntoElem();

							xml.AddAttrib("Name", planDataPage->pageName);

							xml.AddAttrib("Pagination", planDataPage->pagination);
							xml.AddAttrib("PageType", szPageTypes[planDataPage->pageType]);

							// Straighten out inconsistent unique flag.....
							int nUnique = planDataPage->uniquePage;
							if (planDataPage->pageID == planDataPage->masterPageID)
								nUnique = PAGEUNIQUE_UNIQUE;
							xml.AddAttrib("Unique", szPageUniques[nUnique]);

							xml.AddAttrib("PageIndex", planDataPage->pageIndex);
							xml.AddAttrib("FileName", "");
							xml.AddAttrib("Comment", planDataPage->comment);
							//	xml.AddAttrib("Approve", planDataPage->approve ? "1" : "0");
							//	xml.AddAttrib("Hold", planDataPage->hold ? "1" : "0");

							//	s.Format("%d", planDataPage->miscint);
							//	xml.AddAttrib("MiscInt", s);
							//	xml.AddAttrib("MiscString1",planDataPage->miscstring1);
							//	xml.AddAttrib("MiscString2",planDataPage->miscstring2);
							xml.AddAttrib("PageFormat", planDataPage->pageformat);

							xml.AddAttrib("PageID", planDataPage->pageID);
							xml.AddAttrib("MasterPageID", planDataPage->masterPageID != "" ? planDataPage->masterPageID : planDataPage->pageID);
							//		xml.AddAttrib("Priority", util.Int2String(planDataPage->priority));
							//		xml.AddAttrib("Version", util.Int2String(planDataPage->version));

							//xml.AddAttrib("MasterEdition", sMasterEdition);

							xml.AddChildElem("Separations");
							xml.IntoElem();

							for (int col = 0; col < planDataPage->colorList->GetCount(); col++) {
								PlanDataPageSeparation *planDataPageSeparation = &planDataPage->colorList->GetAt(col);
								xml.AddChildElem("Separation");
								xml.IntoElem();
								xml.AddAttrib("Name", planDataPageSeparation->colorName);
								xml.OutOfElem();// </Separation>
							} // for (int col..

							xml.OutOfElem();// </Separations>

							xml.OutOfElem();// </Page>
						} // for (int page..
						xml.OutOfElem();// </Pages>

						xml.OutOfElem();// </Section>
					} // for (int sections..

					xml.OutOfElem();// </Sections>

				}

				xml.OutOfElem();// </Edition>

			} // for (int eds..
			xml.OutOfElem();// </Editions>
			xml.OutOfElem();// </Issue>
			xml.OutOfElem();// </Issues>
			xml.OutOfElem();// </Publication>


			CString sCmdBuffer = xml.GetDoc();
			HANDLE hHndlWrite;
			DWORD nBytesWritten;



			if ((hHndlWrite = ::CreateFile(pPlan->planFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {

				CString sErr = g_util.GetLastWin32Error();
				//	sErrorMessage.Format("Unable to create plan XML file %s - %s", pPlan->planFileName, sErr);
				g_util.Logprintf("ERROR: GenerateXML - CreateFile(%s) failed - %s", pPlan->planFileName, sErr);
				return FALSE;
			}

			if (::WriteFile(hHndlWrite, sCmdBuffer, sCmdBuffer.GetLength(), &nBytesWritten, NULL) == FALSE) {
				//AddToLog("Cannot write XML plan file "+ pPlan->planFileName);
				CloseHandle(hHndlWrite);
				g_util.Logprintf("ERROR: GenerateXML - WriteFile() failed");
				//sErrorMessage.Format("Unable to write plan XML file %s - %s", sXmlFile);
				return FALSE;
			}
			CloseHandle(hHndlWrite);
		}


		g_db.RemovePlanExportJob(aProductionIDList[i], 0, sErrorMessage);
	}

	return TRUE;
}


int GetPlanLock()
{
	CString sErrorMessage;

	CString sClientName;
	CString sClientTime;

	int nCurrentPlanLock = PLANLOCK_UNKNOWN;
	BOOL bHasDBError = FALSE;
	int nWaitLoop = 0;

	while (nCurrentPlanLock != PLANLOCK_LOCK) {
		if (g_db.PlanLock(PLANLOCK_LOCK, nCurrentPlanLock, sClientName, sClientTime, sErrorMessage) == FALSE) {
			g_util.Logprintf("ERROR: PlanLock() - %s", sErrorMessage);
			bHasDBError = TRUE;
			break;
		}
		if (nCurrentPlanLock == PLANLOCK_NOLOCK) {

			nWaitLoop++;
			Sleep(3000);
		}
		else if (nCurrentPlanLock == PLANLOCK_LOCK) {
			g_util.Logprintf("INFO:  Got planlock..");
			break;
		}
	}

	return nCurrentPlanLock;
}


BOOL PurgeUnrelatedFiles(int &nFilesDeleted)
{
	CString sErrorMessage = _T("");
	nFilesDeleted = 0;
	BOOL bHasError = FALSE;
	CStringArray aFilesFound;
	CStringArray aColorList;
	aColorList.Add("C");
	aColorList.Add("M");
	aColorList.Add("Y");
	aColorList.Add("K");
	FILEINFOSTRUCT aReadviewFolders[MAXFILESINSCAN];

	int nFiles = -1;
	int nMaxVersions = 10;

	int f = 0;

	CString sTileBaseFolder;
	CString sFullPath;
	CString sBaseFolder;
	CString sSearchMask;

	for (int i = 0; i < g_prefs.m_FileServerList.GetCount(); i++) {
		FILESERVERSTRUCT *pFileServer = &g_prefs.m_FileServerList[i];

		sBaseFolder = g_util.IsValidIP(pFileServer->m_IP) ?
			_T("\\\\") + pFileServer->m_IP + _T("\\") + pFileServer->m_sharename :
			_T("\\\\") + pFileServer->m_servername + _T("\\") + pFileServer->m_sharename;

		g_util.Logprintf("INFO:  Servertype %d basefolder is %s", pFileServer->m_servertype, sBaseFolder);

		if (g_util.CheckFolderWithPing(sBaseFolder) == FALSE) {
			g_util.Logprintf("ERROR: CheckFolderWithPing(%s) failed", sBaseFolder);
			bHasError = TRUE;
			continue;
		}

		if (g_prefs.m_alwaysusecurrentuser == FALSE) {
			if (pFileServer->m_uselocaluser == FALSE && pFileServer->m_username != _T("")) {
				g_util.Reconnect(sBaseFolder, pFileServer->m_username, pFileServer->m_password);
			}
		}


		if (g_util.CheckFolder(sBaseFolder) == FALSE) {
			g_util.Logprintf("ERROR: CheckFolder(%s) failed", sBaseFolder);
			bHasError = TRUE;
			continue;
		}


		switch (pFileServer->m_servertype) {
		case FILESERVERTYPE_MAIN:
		case FILESERVERTYPE_BACKUPSERVER:
			f = nFilesDeleted;
			aFilesFound.RemoveAll();

			nFiles = g_util.ScanDirSimple(sBaseFolder + "\\CCFilesLowres", _T("*.*"), aFilesFound, _T(""), 10000);
			if (nFiles < 0) {
				g_util.Logprintf("ERROR: ScanDirSimple(%s) failed", sBaseFolder + "\\CCFilesLowres");
				bHasError = TRUE;
				aFilesFound.RemoveAll();
				continue;
			}
			// APABZ5_20190225_E1_1_1_001#3592.PDF
			for (int it = 0; it < nFiles; it++) {
				CString sFile = g_util.GetFileName(aFilesFound[it], TRUE);
				int mm = sFile.ReverseFind('#');
				if (mm != -1)
					sFile = sFile.Mid(mm + 1);

				int nMasterCopySeparationSet = atoi(sFile);
				if (nMasterCopySeparationSet == 0)
					continue;

				int test1 = g_db.HasMasterCopySeparationSet(nMasterCopySeparationSet, sErrorMessage);
				if (test1 > 0)
					continue; // Master number is use..
				if (test1 < 0 || sErrorMessage != _T("")) {
					g_util.Logprintf("ERROR: HasMasterCopySeparationSet(%d) failed - %s", nMasterCopySeparationSet, sErrorMessage);
					continue;
				}

				int test2 = g_db.CountMasterCopySeparationSet(nMasterCopySeparationSet, sErrorMessage);
				if (test2 > 0)
					continue; // Master number is use..
				if (test2 < 0 || sErrorMessage != _T("")) {
					g_util.Logprintf("ERROR: CountMasterCopySeparationSet(%d) failed - %s", nMasterCopySeparationSet, sErrorMessage);
					continue;
				}

				if (pFileServer->m_servertype == FILESERVERTYPE_MAIN)
					g_util.LogDeletedFiles(nMasterCopySeparationSet, "Unrelated file deleted");

				CString sFullPath = sBaseFolder + _T("\\CCFilesLowres\\") + aFilesFound[it];
				if (g_util.DeleteFileEx(sFullPath))
					nFilesDeleted++;
			}


			if (pFileServer->m_servertype == FILESERVERTYPE_MAIN) {

				nFiles = g_util.ScanDirSimple(sBaseFolder + "\\CCFilesHires", "*.pdf", aFilesFound, "", MAXFILESINSCAN);

				for (int it = 0; it < nFiles; it++) {
					CString sFile = g_util.GetFileName(aFilesFound[it], TRUE);
					int mm = sFile.Find("#");
					if (mm != -1)
						sFile = sFile.Mid(mm + 1);

					int nMasterCopySeparationSet = atoi(sFile);
					if (nMasterCopySeparationSet == 0)
						continue;

					int test1 = g_db.HasMasterCopySeparationSet(nMasterCopySeparationSet, sErrorMessage);
					if (test1 > 0)
						continue; // Master number is use..
					if (test1 < 0 || sErrorMessage != _T("")) {
						g_util.Logprintf("ERROR: HasMasterCopySeparationSet(%d) failed - %s", nMasterCopySeparationSet, sErrorMessage);
						continue;
					}

					int test2 = g_db.CountMasterCopySeparationSet(nMasterCopySeparationSet, sErrorMessage);
					if (test2 > 0)
						continue; // Master number is use..
					if (test2 < 0 || sErrorMessage != _T("")) {
						g_util.Logprintf("ERROR: CountMasterCopySeparationSet(%d) failed - %s", nMasterCopySeparationSet, sErrorMessage);
						continue;
					}

					CString sFullPath = sBaseFolder + _T("\\CCFilesHires\\") + aFilesFound[it];
					if (g_util.DeleteFileEx(sFullPath))
						nFilesDeleted++;
				}



				nFiles = g_util.ScanDirSimple(sBaseFolder + "\\CCFilesPrint", "*.pdf", aFilesFound, "", MAXFILESINSCAN);
				if (nFiles < 0) {
					g_util.Logprintf("ERROR: ScanDirSimple(%s) failed", sBaseFolder + "\\CCFilesPrint");
					bHasError = TRUE;
					aFilesFound.RemoveAll();
					continue;
				}
				for (int it = 0; it < nFiles; it++) {
					CString sFile = g_util.GetFileName(aFilesFound[it], TRUE);
					int mm = sFile.ReverseFind('#');
					if (mm != -1)
						sFile = sFile.Mid(1 + 4);
					int nMasterCopySeparationSet = atoi(sFile);
					if (nMasterCopySeparationSet == 0)
						continue;

					int test1 = g_db.HasMasterCopySeparationSet(nMasterCopySeparationSet, sErrorMessage);
					if (test1 > 0)
						continue; // Master number is use..
					if (test1 < 0 || sErrorMessage != _T("")) {
						g_util.Logprintf("ERROR: HasMasterCopySeparationSet(%d) failed - %s", nMasterCopySeparationSet, sErrorMessage);
						continue;
					}

					int test2 = g_db.CountMasterCopySeparationSet(nMasterCopySeparationSet, sErrorMessage);
					if (test2 > 0)
						continue; // Master number is use..
					if (test2 < 0 || sErrorMessage != _T("")) {
						g_util.Logprintf("ERROR: CountMasterCopySeparationSet(%d) failed - %s", nMasterCopySeparationSet, sErrorMessage);
						continue;
					}

					CString sFullPath = sBaseFolder + _T("\\CCFilesPrint\\") + aFilesFound[it];
					if (g_util.DeleteFileEx(sFullPath))
						nFilesDeleted++;
				}

			} // end of special channel stuff

			// Unrelared thumbnails

			aFilesFound.RemoveAll();
			nFiles = g_util.ScanDirSimple(sBaseFolder + "\\CCThumbnails", "*.*", aFilesFound, "", MAXFILESINSCAN);
			if (nFiles < 0) {
				g_util.Logprintf("ERROR: ScanDirSimple(%s) failed", sBaseFolder + "\\CCThumbnails");
				bHasError = TRUE;
				aFilesFound.RemoveAll();
				continue;
			}
			for (int it = 0; it < nFiles; it++) {
				CString sFile = g_util.GetFileName(aFilesFound[it], TRUE);
				int mm = sFile.ReverseFind('#');
				if (mm != -1)
					sFile = sFile.Mid(mm + 1);

				int nMasterCopySeparationSet = atoi(sFile);

				if (nMasterCopySeparationSet == 0)
					continue;

				int test1 = g_db.HasMasterCopySeparationSet(nMasterCopySeparationSet, sErrorMessage);
				if (test1 > 0)
					continue; // Master number is use..
				if (test1 < 0 || sErrorMessage != _T("")) {
					g_util.Logprintf("ERROR: HasMasterCopySeparationSet(%d) failed - %s", nMasterCopySeparationSet, sErrorMessage);
					continue;
				}

				int test2 = g_db.CountMasterCopySeparationSet(nMasterCopySeparationSet, sErrorMessage);
				if (test2 > 0)
					continue; // Master number is use..
				if (test2 < 0 || sErrorMessage != _T("")) {
					g_util.Logprintf("ERROR: CountMasterCopySeparationSet(%d) failed - %s", nMasterCopySeparationSet, sErrorMessage);
					continue;
				}

				sFullPath.Format("%s\\CCThumbnails\\%s", sBaseFolder, aFilesFound[it]);
				if (g_util.DeleteFileEx(sFullPath))
					nFilesDeleted++;
			}



			// Unrelared previews

			aFilesFound.RemoveAll();
			nFiles = g_util.ScanDirSimple(sBaseFolder + "\\CCPreviews", "*.*", aFilesFound, "", MAXFILESINSCAN);
			if (nFiles < 0) {
				g_util.Logprintf("ERROR: ScanDirSimple(%s) failed", sBaseFolder + "\\CCPreviews");
				bHasError = TRUE;
				aFilesFound.RemoveAll();
				continue;
			}
			for (int it = 0; it < nFiles; it++) {
				CString sFile = g_util.GetFileName(aFilesFound[it], TRUE);
				int mm = sFile.ReverseFind('#');
				if (mm != -1)
					sFile = sFile.Mid(mm + 1);

				int nMasterCopySeparationSet = atoi(sFile);
				if (nMasterCopySeparationSet == 0)
					continue;

				int test1 = g_db.HasMasterCopySeparationSet(nMasterCopySeparationSet, sErrorMessage);
				if (test1 > 0)
					continue; // Master number is use..
				if (test1 < 0 || sErrorMessage != _T("")) {
					g_util.Logprintf("ERROR: HasMasterCopySeparationSet(%d) failed - %s", nMasterCopySeparationSet, sErrorMessage);
					continue;
				}

				int test2 = g_db.CountMasterCopySeparationSet(nMasterCopySeparationSet, sErrorMessage);
				if (test2 > 0)
					continue; // Master number is use..
				if (test2 < 0 || sErrorMessage != _T("")) {
					g_util.Logprintf("ERROR: CountMasterCopySeparationSet(%d) failed - %s", nMasterCopySeparationSet, sErrorMessage);
					continue;
				}

				sFullPath.Format("%s\\CCPreviews\\%d.jpg", sBaseFolder, nMasterCopySeparationSet);
				if (g_util.DeleteFileEx(sFullPath))
					nFilesDeleted++;

				sSearchMask.Format("%d_*.jpg", nMasterCopySeparationSet);
				nFilesDeleted += g_util.DeleteFiles(sBaseFolder + "\\CCPreviews", sSearchMask);
				sSearchMask.Format("%d_mask.jpg", nMasterCopySeparationSet);
				nFilesDeleted += g_util.DeleteFiles(sBaseFolder + "\\CCPreviews", sSearchMask);
				sSearchMask.Format("%d_*.jpg", nMasterCopySeparationSet);
				nFilesDeleted += g_util.DeleteFiles(sBaseFolder + "\\CCReadviewPreviews", sSearchMask);
				sSearchMask.Format("*_%d.jpg", nMasterCopySeparationSet);
				nFilesDeleted += g_util.DeleteFiles(sBaseFolder + "\\CCReadviewPreviews", sSearchMask);
				sSearchMask.Format("%d_*_mask.jpg", nMasterCopySeparationSet);
				nFilesDeleted += g_util.DeleteFiles(sBaseFolder + "\\CCReadviewPreviews", sSearchMask);
				sSearchMask.Format("*_%d_mask.jpg", nMasterCopySeparationSet);
				nFilesDeleted += g_util.DeleteFiles(sBaseFolder + "\\CCReadviewPreviews", sSearchMask);

				for (int v = 0; v <= nMaxVersions; v++) {
					if (v == 0)
						sTileBaseFolder.Format("\\CCpreviews\\%d", nMasterCopySeparationSet);
					else
						sTileBaseFolder.Format("\\CCpreviews\\%d-%d", nMasterCopySeparationSet, v);

					g_util.DeleteFileEx(sBaseFolder + sTileBaseFolder + "\\imageProperties.xml");

					nFilesDeleted += g_util.DeleteFiles(sBaseFolder + sTileBaseFolder + "\\TileGroup0", "*.jpg", TRUE);

					// SeaDragin tiles.
					g_util.DeleteFileEx(sBaseFolder + sTileBaseFolder + _T("\\image.xml"));
					for (int z = 0; z < 25; z++) {
						g_util.DeleteFiles(sBaseFolder + sTileBaseFolder + _T("\\image_files\\") + g_util.Int2String(z), "*.*", TRUE);
					}
					g_util.DeleteFolder(sBaseFolder + sTileBaseFolder + _T("\\image_files"));

					g_util.DeleteFolder(sBaseFolder + sTileBaseFolder);

				}
			}

			// Unrelared readview-previews


			aFilesFound.RemoveAll();
			nFiles = g_util.ScanDirSimple(sBaseFolder + "\\CCReadviewPreviews", "*.*", aFilesFound, "", MAXFILESINSCAN);
			if (nFiles < 0) {
				g_util.Logprintf("ERROR: ScanDirSimple(%s) failed", sBaseFolder + "\\CCPreviews");
				bHasError = TRUE;
				aFilesFound.RemoveAll();
				continue;
			}

			for (int it = 0; it < nFiles; it++) {
				CString s = g_util.GetFileName(aFilesFound[it], TRUE);
				int ns = s.Find("_");
				if (ns == -1)
					continue;

				int nMasterCopySeparationSet1 = atoi(s.Left(ns));
				int nMasterCopySeparationSet2 = atoi(s.Mid(ns + 1));
				if (nMasterCopySeparationSet1 == 0 && nMasterCopySeparationSet2 == 0)
					continue;

				g_util.Logprintf("DEBUG: Testing %s (%d_%d) ...", aFilesFound[it], nMasterCopySeparationSet1, nMasterCopySeparationSet2);

				if (nMasterCopySeparationSet1 > 0) {
					int test1 = g_db.HasMasterCopySeparationSet(nMasterCopySeparationSet1, sErrorMessage);
					if (test1 > 0)
						continue; // Master number is use..
					if (test1 < 0 || sErrorMessage != _T("")) {
						g_util.Logprintf("ERROR: HasMasterCopySeparationSet(%d) failed - %s", nMasterCopySeparationSet1, sErrorMessage);
						continue;
					}

					int test2 = g_db.CountMasterCopySeparationSet(nMasterCopySeparationSet1, sErrorMessage);
					if (test2 > 0)
						continue; // Master number is use..
					if (test2 < 0 || sErrorMessage != _T("")) {
						g_util.Logprintf("ERROR: CountMasterCopySeparationSet(%d) failed - %s", nMasterCopySeparationSet1, sErrorMessage);
						continue;
					}
				}
				if (nMasterCopySeparationSet2 > 0) {
					int test1 = g_db.HasMasterCopySeparationSet(nMasterCopySeparationSet2, sErrorMessage);
					if (test1 > 0)
						continue; // Master number is use..
					if (test1 < 0 || sErrorMessage != _T("")) {
						g_util.Logprintf("ERROR: HasMasterCopySeparationSet(%d) failed - %s", nMasterCopySeparationSet2, sErrorMessage);
						continue;
					}

					int test2 = g_db.CountMasterCopySeparationSet(nMasterCopySeparationSet2, sErrorMessage);
					if (test2 > 0)
						continue; // Master number is use..
					if (test2 < 0 || sErrorMessage != _T("")) {
						g_util.Logprintf("ERROR: CountMasterCopySeparationSet(%d) failed - %s", nMasterCopySeparationSet2, sErrorMessage);
						continue;
					}
				}

				sSearchMask.Format("%d_%d_*.*", nMasterCopySeparationSet1, nMasterCopySeparationSet2);
				g_util.Logprintf("DEBUG: SearchMask1 %s");
				nFilesDeleted += g_util.DeleteFiles(sBaseFolder + "\\CCReadviewPreviews", sSearchMask);


				sSearchMask.Format("%d_%d.*", nMasterCopySeparationSet1, nMasterCopySeparationSet2);
				g_util.Logprintf("DEBUG: SearchMask2 %s");
				nFilesDeleted += g_util.DeleteFiles(sBaseFolder + "\\CCReadviewPreviews", sSearchMask);

				int nDirs = g_util.GetDirectoryList(sBaseFolder + _T("\\CCReadviewPreviews\\"), sSearchMask, aReadviewFolders, MAXFILESINSCAN);
				if (nDirs > 0) {
					for (int m = 0; m < nDirs; m++) {
						sTileBaseFolder.Format("\\%s", aReadviewFolders[m].sFileName);

						if (g_util.DeleteFileEx(sBaseFolder + _T("\\CCReadviewPreviews") + sTileBaseFolder + _T("\\imageProperties.xml")))
							nFiles++;
						nFiles += g_util.DeleteFiles(sBaseFolder + _T("\\CCReadviewPreviews") + sTileBaseFolder + _T("\\TileGroup0"), "*.*", TRUE);

						// SeaDragin tiles.
						g_util.DeleteFileEx(sBaseFolder + _T("\\CCReadviewPreviews") + sTileBaseFolder + _T("\\image.xml"));
						for (int z = 0; z < 25; z++) {
							g_util.DeleteFiles(sBaseFolder + _T("\\CCReadviewPreviews") + sTileBaseFolder + _T("\\image_files\\") + g_util.Int2String(z), "*.*", TRUE);
						}
						g_util.DeleteFolder(sBaseFolder + _T("\\CCReadviewPreviews") + sTileBaseFolder + _T("\\image_files"));

						g_util.DeleteFolder(sBaseFolder + _T("\\CCReadviewPreviews") + sTileBaseFolder);
					}
				}
			}

			aFilesFound.RemoveAll();
			nFiles = g_util.ScanDirSimple(sBaseFolder + "\\CCPDFLogs", "*.*", aFilesFound, "", MAXFILESINSCAN);
			if (nFiles < 0) {
				g_util.Logprintf("ERROR: ScanDirSimple(%s) failed", sBaseFolder + "\\CCPDFLogs");
				bHasError = TRUE;
				aFilesFound.RemoveAll();
				continue;
			}
			for (int it = 0; it < nFiles; it++) {
				CString s = aFilesFound[it];
				int m = s.Find("_log_");
				if (m == -1)
					continue;
				int nMasterCopySeparationSet = atoi(s.Mid(m + 5));
				if (nMasterCopySeparationSet == 0)
					continue;

				int test1 = g_db.HasMasterCopySeparationSet(nMasterCopySeparationSet, sErrorMessage);
				if (test1 > 0)
					continue; // Master number is use..
				if (test1 < 0 || sErrorMessage != _T("")) {
					continue;
				}

				int test2 = g_db.CountMasterCopySeparationSet(nMasterCopySeparationSet, sErrorMessage);
				if (test2 > 0)
					continue; // Master number is use..
				if (test2 < 0 || sErrorMessage != _T("")) {
					continue;
				}
				CString sFullPath;
				sFullPath.Format("%s\\CCPDFLogs\\%s", sBaseFolder, aFilesFound[it]);
				if (g_util.DeleteFileEx(sFullPath))
					nFilesDeleted++;
			}

			for (int it = 0; it < nFiles; it++) {
				CString s = aFilesFound[it];
				int m = s.Find("_report_");
				if (m == -1)
					continue;
				int nMasterCopySeparationSet = atoi(s.Mid(m + 5));
				if (nMasterCopySeparationSet == 0)
					continue;

				int test1 = g_db.HasMasterCopySeparationSet(nMasterCopySeparationSet, sErrorMessage);
				if (test1 > 0)
					continue; // Master number is use..
				if (test1 < 0 || sErrorMessage != _T("")) {
					continue;
				}

				int test2 = g_db.CountMasterCopySeparationSet(nMasterCopySeparationSet, sErrorMessage);
				if (test2 > 0)
					continue; // Master number is use..
				if (test2 < 0 || sErrorMessage != _T("")) {
					continue;
				}
				CString sFullPath;
				sFullPath.Format("%s\\CCPDFLogs\\%s", sBaseFolder, aFilesFound[it]);
				if (g_util.DeleteFileEx(sFullPath))
					nFilesDeleted++;
			}

			aFilesFound.RemoveAll();
			nFiles = g_util.ScanDirSimple(sBaseFolder + "\\CCPDFfiles", "*.*", aFilesFound, "", MAXFILESINSCAN);
			if (nFiles < 0) {
				g_util.Logprintf("ERROR: ScanDirSimple(%s) failed", sBaseFolder + "\\CCPDFfiles");
				bHasError = TRUE;
				aFilesFound.RemoveAll();
				continue;
			}
			for (int it = 0; it < nFiles; it++) {
				CString s = aFilesFound[it];
				int m = s.ReverseFind('_');
				if (m == -1)
					continue;
				int nMasterCopySeparationSet = atoi(s.Mid(m + 5));
				if (nMasterCopySeparationSet == 0)
					continue;

				int test1 = g_db.HasMasterCopySeparationSet(nMasterCopySeparationSet, sErrorMessage);
				if (test1 > 0)
					continue; // Master number is use..
				if (test1 < 0 || sErrorMessage != _T("")) {
					continue;
				}

				int test2 = g_db.CountMasterCopySeparationSet(nMasterCopySeparationSet, sErrorMessage);
				if (test2 > 0)
					continue; // Master number is use..
				if (test2 < 0 || sErrorMessage != _T("")) {
					continue;
				}
				CString sFullPath;
				sFullPath.Format("%s\\CCPDFfiles\\%s", sBaseFolder, aFilesFound[it]);
				if (g_util.DeleteFileEx(sFullPath))
					nFilesDeleted++;
			}

			g_util.Logprintf("INFO: %d unrelated files deleted from main server", nFilesDeleted - f);

			aFilesFound.RemoveAll();
			break;

		case FILESERVERTYPE_ADDITIONALPREVIEW:
			f = nFilesDeleted;
			aFilesFound.RemoveAll();

			nFiles = g_util.ScanDirSimple(sBaseFolder + "\\CCPreviews", "*.jpg", aFilesFound, "",  MAXFILESINSCAN);
			if (nFiles < 0) {
				g_util.Logprintf("ERROR: ScanDirSimple(%s) failed", sBaseFolder + "\\CCPreviews");
				bHasError = TRUE;
				aFilesFound.RemoveAll();
				continue;
			}
			for (int it = 0; it < nFiles; it++) {
				int nMasterCopySeparationSet = atoi(g_util.GetFileName(aFilesFound[it], TRUE));
				CString sFullPath = sBaseFolder + _T("\\CCPreviews\\") + aFilesFound[it];
				if (nMasterCopySeparationSet > 0) {
					int test1 = g_db.HasMasterCopySeparationSet(nMasterCopySeparationSet, sErrorMessage);
					if (test1 > 0)
						continue; // Master number is use..
					if (test1 < 0 || sErrorMessage != _T("")) {
						g_util.Logprintf("ERROR: HasMasterCopySeparationSet(%d) failed - %s", nMasterCopySeparationSet, sErrorMessage);
						continue;
					}

					int test2 = g_db.CountMasterCopySeparationSet(nMasterCopySeparationSet, sErrorMessage);
					if (test2 > 0)
						continue; // Master number is use..
					if (test2 < 0 || sErrorMessage != _T("")) {
						g_util.Logprintf("ERROR: CountMasterCopySeparationSet(%d) failed - %s", nMasterCopySeparationSet, sErrorMessage);
						continue;
					}


					CString sSearchMask;
					sFullPath.Format("%s\\CCThumbnails\\%d.jpg", sBaseFolder, nMasterCopySeparationSet);
					if (g_util.DeleteFileEx(sFullPath))
						nFilesDeleted++;
					sFullPath.Format("%s\\CCPreviews\\%d.jpg", sBaseFolder, nMasterCopySeparationSet);
					if (g_util.DeleteFileEx(sFullPath))
						nFilesDeleted++;
					sSearchMask.Format("%d_*.jpg", nMasterCopySeparationSet);
					nFilesDeleted += g_util.DeleteFiles(sBaseFolder + "\\CCPreviews", sSearchMask);
					sSearchMask.Format("%d_*.jpg", nMasterCopySeparationSet);
					nFilesDeleted += g_util.DeleteFiles(sBaseFolder + "\\CCReadviewPreviews", sSearchMask);
					sSearchMask.Format("*_%d.jpg", nMasterCopySeparationSet);
					nFilesDeleted += g_util.DeleteFiles(sBaseFolder + "\\CCReadviewPreviews", sSearchMask);
					sSearchMask.Format("%d_*_mask.jpg", nMasterCopySeparationSet);
					nFilesDeleted += g_util.DeleteFiles(sBaseFolder + "\\CCReadviewPreviews", sSearchMask);
					sSearchMask.Format("*_%d_mask.jpg", nMasterCopySeparationSet);
					nFilesDeleted += g_util.DeleteFiles(sBaseFolder + "\\CCReadviewPreviews", sSearchMask);

				}
			}

			g_util.Logprintf("INFO: %d unrelated files deleted from Additional Preview folder", nFilesDeleted - f);

			aFilesFound.RemoveAll();
			break;

		case FILESERVERTYPE_WEBCENTER:

			f = nFilesDeleted;
			aFilesFound.RemoveAll();
			aFilesFound.SetSize(MAXFILESINSCAN);
			nFiles = g_util.ScanDirSimple(sBaseFolder + "\\CCPreviews", "*.jpg", aFilesFound, "", MAXFILESINSCAN);
			if (nFiles < 0) {
				g_util.Logprintf("ERROR: ScanDirSimple(%s) failed", sBaseFolder + "\\CCPreviews");
				bHasError = TRUE;
				aFilesFound.RemoveAll();
				continue;
			}
			for (int it = 0; it < nFiles; it++) {
				// Only pure masterset.jpg files..
				if (aFilesFound[it].Find("_") != -1)
					continue;
				int nMasterCopySeparationSet = atoi(g_util.GetFileName(aFilesFound[it], TRUE));
				if (nMasterCopySeparationSet <= 0)
					continue;

				int test1 = g_db.HasMasterCopySeparationSet(nMasterCopySeparationSet, sErrorMessage);
				if (test1 > 0)
					continue; // Master number is use..
				if (test1 < 0 || sErrorMessage != _T("")) {
					g_util.Logprintf("ERROR: HasMasterCopySeparationSet(%d) failed - %s", nMasterCopySeparationSet, sErrorMessage);
					continue;
				}

				int test2 = g_db.CountMasterCopySeparationSet(nMasterCopySeparationSet, sErrorMessage);
				if (test2 > 0)
					continue; // Master number is use..
				if (test2 < 0 || sErrorMessage != _T("")) {
					g_util.Logprintf("ERROR: CountMasterCopySeparationSet(%d) failed - %s", nMasterCopySeparationSet, sErrorMessage);
					continue;
				}

				if (g_util.DeleteFileEx(sBaseFolder + _T("\\CCPreviews\\") + aFilesFound[it]))
					nFilesDeleted++;

				if (g_util.DeleteFileEx(sBaseFolder + _T("\\CCThumbnils\\") + aFilesFound[it]))
					nFilesDeleted++;

				CString sSearchMask;
				sSearchMask.Format("%d_*.jpg", nMasterCopySeparationSet);
				nFilesDeleted += g_util.DeleteFiles(sBaseFolder + "\\CCReadviewPreviews", sSearchMask);
				sSearchMask.Format("*_%d.jpg", nMasterCopySeparationSet);
				nFilesDeleted += g_util.DeleteFiles(sBaseFolder + "\\CCReadviewPreviews", sSearchMask);


				// NON Color specific files
				CString sTileBaseFolder;
				for (int v = 0; v <= nMaxVersions; v++) {
					if (v == 0)
						sTileBaseFolder.Format("\\CCpreviews\\%d", nMasterCopySeparationSet);
					else
						sTileBaseFolder.Format("\\CCpreviews\\%d-%d", nMasterCopySeparationSet, v);

					g_util.DeleteFileEx(sBaseFolder + sTileBaseFolder + "\\imageProperties.xml");

					nFilesDeleted += g_util.DeleteFiles(sBaseFolder + sTileBaseFolder + "\\TileGroup0", "*.jpg", TRUE);

					// SeaDragin tiles.
					g_util.DeleteFileEx(sBaseFolder + sTileBaseFolder + _T("\\image.xml"));
					for (int z = 0; z < 25; z++) {
						g_util.DeleteFiles(sBaseFolder + sTileBaseFolder + _T("\\image_files\\") + g_util.Int2String(z), "*.*", TRUE);
					}
					g_util.DeleteFolder(sBaseFolder + sTileBaseFolder + _T("\\image_files"));

					g_util.DeleteFolder(sBaseFolder + sTileBaseFolder);

				}

				// Color specific files
				for (int k = 0; k < aColorList.GetCount(); k++) {
					for (int v = 0; v < nMaxVersions; v++) {
						if (v == 0)
							sTileBaseFolder.Format("\\CCpreviews\\%d_%s", nMasterCopySeparationSet, aColorList[k]);
						else
							sTileBaseFolder.Format("\\CCpreviews\\%d_%s-%d", nMasterCopySeparationSet, aColorList[k], v);

						g_util.DeleteFileEx(sBaseFolder + sTileBaseFolder + "\\imageProperties.xml");

						nFilesDeleted += g_util.DeleteFiles(sBaseFolder + sTileBaseFolder + "\\TileGroup0", "*.jpg", TRUE);

						// SeaDragin tiles.
						g_util.DeleteFileEx(sBaseFolder + sTileBaseFolder + _T("\\image.xml"));
						for (int z = 0; z < 25; z++) {
							g_util.DeleteFiles(sBaseFolder + sTileBaseFolder + _T("\\image_files\\") + g_util.Int2String(z), "*.*", TRUE);
						}
						g_util.DeleteFolder(sBaseFolder + sTileBaseFolder + _T("\\image_files"));

						g_util.DeleteFolder(sBaseFolder + sTileBaseFolder);
					}
				}

				// Mask files
				for (int v = 0; v <= nMaxVersions; v++) {
					if (v == 0)
						sTileBaseFolder.Format("\\CCpreviews\\%d_mask", nMasterCopySeparationSet);
					else
						sTileBaseFolder.Format("\\CCpreviews\\%d-%d_mask", nMasterCopySeparationSet, v);

					g_util.DeleteFileEx(sBaseFolder + sTileBaseFolder + "\\imageProperties.xml");

					nFilesDeleted += g_util.DeleteFiles(sBaseFolder + sTileBaseFolder + "\\TileGroup0", "*.jpg", TRUE);

					// SeaDragin tiles.
					g_util.DeleteFileEx(sBaseFolder + sTileBaseFolder + _T("\\image.xml"));
					for (int z = 0; z < 25; z++) {
						g_util.DeleteFiles(sBaseFolder + sTileBaseFolder + _T("\\image_files\\") + g_util.Int2String(z), "*.*", TRUE);
					}
					g_util.DeleteFolder(sBaseFolder + sTileBaseFolder + _T("\\image_files"));

					g_util.DeleteFolder(sBaseFolder + sTileBaseFolder);
				}

			}

			aFilesFound.RemoveAll();
			nFiles = g_util.ScanDirSimple(sBaseFolder + "\\CCReadviewPreviews", "*.*", aFilesFound, "", MAXFILESINSCAN);
			if (nFiles < 0) {
				g_util.Logprintf("ERROR: ScanDirSimple(%s) failed", sBaseFolder + "\\CCPreviews");
				bHasError = TRUE;
				aFilesFound.RemoveAll();
				continue;
			}

			for (int it = 0; it < nFiles; it++) {
				CString s = g_util.GetFileName(aFilesFound[it], TRUE);
				int ns = s.Find("_");
				if (ns == -1)
					continue;

				int nMasterCopySeparationSet1 = atoi(s.Left(ns));
				int nMasterCopySeparationSet2 = atoi(s.Mid(ns + 1));
				if (nMasterCopySeparationSet1 == 0 && nMasterCopySeparationSet2 == 0)
					continue;

				if (nMasterCopySeparationSet1 > 0) {
					int test1 = g_db.HasMasterCopySeparationSet(nMasterCopySeparationSet1, sErrorMessage);
					if (test1 > 0)
						continue; // Master number is use..
					if (test1 < 0 || sErrorMessage != _T("")) {
						g_util.Logprintf("ERROR: HasMasterCopySeparationSet(%d) failed - %s", nMasterCopySeparationSet1, sErrorMessage);
						continue;
					}

					int test2 = g_db.CountMasterCopySeparationSet(nMasterCopySeparationSet1, sErrorMessage);
					if (test2 > 0)
						continue; // Master number is use..
					if (test2 < 0 || sErrorMessage != _T("")) {
						g_util.Logprintf("ERROR: CountMasterCopySeparationSet(%d) failed - %s", nMasterCopySeparationSet1, sErrorMessage);
						continue;
					}
				}
				if (nMasterCopySeparationSet2 > 0) {
					int test1 = g_db.HasMasterCopySeparationSet(nMasterCopySeparationSet2, sErrorMessage);
					if (test1 > 0)
						continue; // Master number is use..
					if (test1 < 0 || sErrorMessage != _T("")) {
						g_util.Logprintf("ERROR: HasMasterCopySeparationSet(%d) failed - %s", nMasterCopySeparationSet2, sErrorMessage);
						continue;
					}

					int test2 = g_db.CountMasterCopySeparationSet(nMasterCopySeparationSet2, sErrorMessage);
					if (test2 > 0)
						continue; // Master number is use..
					if (test2 < 0 || sErrorMessage != _T("")) {
						g_util.Logprintf("ERROR: CountMasterCopySeparationSet(%d) failed - %s", nMasterCopySeparationSet2, sErrorMessage);
						continue;
					}
				}

				sSearchMask.Format("%d_%d_*.*", nMasterCopySeparationSet1, nMasterCopySeparationSet2);
				nFilesDeleted += g_util.DeleteFiles(sBaseFolder + "\\CCReadviewPreviews", sSearchMask);

				sSearchMask.Format("%d_%d_*.*", nMasterCopySeparationSet2, nMasterCopySeparationSet1);
				nFilesDeleted += g_util.DeleteFiles(sBaseFolder + "\\CCReadviewPreviews", sSearchMask);


				sSearchMask.Format("%d_%d.*", nMasterCopySeparationSet1, nMasterCopySeparationSet2);
				nFilesDeleted += g_util.DeleteFiles(sBaseFolder + "\\CCReadviewPreviews", sSearchMask);

				sSearchMask.Format("%d_%d.*", nMasterCopySeparationSet2, nMasterCopySeparationSet1);
				nFilesDeleted += g_util.DeleteFiles(sBaseFolder + "\\CCReadviewPreviews", sSearchMask);

				int nDirs = g_util.GetDirectoryList(sBaseFolder + _T("\\CCReadviewPreviews\\"), sSearchMask, aReadviewFolders, MAXFILESINSCAN);
				if (nDirs > 0) {
					for (int m = 0; m < nDirs; m++) {
						sTileBaseFolder.Format("\\%s", aReadviewFolders[m].sFileName);

						if (g_util.DeleteFileEx(sBaseFolder + _T("\\CCReadviewPreviews") + sTileBaseFolder + _T("\\imageProperties.xml")))
							nFiles++;
						nFiles += g_util.DeleteFiles(sBaseFolder + _T("\\CCReadviewPreviews") + sTileBaseFolder + _T("\\TileGroup0"), "*.*", TRUE);

						// SeaDragin tiles.
						g_util.DeleteFileEx(sBaseFolder + _T("\\CCReadviewPreviews") + sTileBaseFolder + _T("\\image.xml"));
						for (int z = 0; z < 25; z++) {
							g_util.DeleteFiles(sBaseFolder + _T("\\CCReadviewPreviews") + sTileBaseFolder + _T("\\image_files\\") + g_util.Int2String(z), "*.*", TRUE);
						}
						g_util.DeleteFolder(sBaseFolder + _T("\\CCReadviewPreviews") + sTileBaseFolder + _T("\\image_files"));

						g_util.DeleteFolder(sBaseFolder + _T("\\CCReadviewPreviews") + sTileBaseFolder);
					}
				}
			}

			aFilesFound.RemoveAll();
			nFiles = g_util.ScanDirSimple(sBaseFolder + "\\CCPDFLogs", "*.*", aFilesFound, "", MAXFILESINSCAN);
			if (nFiles < 0) {
				g_util.Logprintf("ERROR: ScanDirSimple(%s) failed", sBaseFolder + "\\CCPDFLogs");
				bHasError = TRUE;
				aFilesFound.RemoveAll();
				continue;
			}
			for (int it = 0; it < nFiles; it++) {
				CString s = aFilesFound[it];
				int m = s.Find("_log_");
				if (m == -1)
					continue;
				int nMasterCopySeparationSet = atoi(s.Mid(m + 5));
				if (nMasterCopySeparationSet == 0)
					continue;

				int test1 = g_db.HasMasterCopySeparationSet(nMasterCopySeparationSet, sErrorMessage);
				if (test1 > 0)
					continue; // Master number is use..
				if (test1 < 0 || sErrorMessage != _T("")) {
					continue;
				}

				int test2 = g_db.CountMasterCopySeparationSet(nMasterCopySeparationSet, sErrorMessage);
				if (test2 > 0)
					continue; // Master number is use..
				if (test2 < 0 || sErrorMessage != _T("")) {
					continue;
				}
				CString sFullPath;
				sFullPath.Format("%s\\CCPDFLogs\\%s", sBaseFolder, aFilesFound[it]);
				if (g_util.DeleteFileEx(sFullPath))
					nFilesDeleted++;
			}

			for (int it = 0; it < nFiles; it++) {
				CString s = aFilesFound[it];
				int m = s.Find("_report_");
				if (m == -1)
					continue;
				int nMasterCopySeparationSet = atoi(s.Mid(m + 5));
				if (nMasterCopySeparationSet == 0)
					continue;

				int test1 = g_db.HasMasterCopySeparationSet(nMasterCopySeparationSet, sErrorMessage);
				if (test1 > 0)
					continue; // Master number is use..
				if (test1 < 0 || sErrorMessage != _T("")) {
					continue;
				}

				int test2 = g_db.CountMasterCopySeparationSet(nMasterCopySeparationSet, sErrorMessage);
				if (test2 > 0)
					continue; // Master number is use..
				if (test2 < 0 || sErrorMessage != _T("")) {
					continue;
				}
				CString sFullPath;
				sFullPath.Format("%s\\CCPDFLogs\\%s", sBaseFolder, aFilesFound[it]);
				if (g_util.DeleteFileEx(sFullPath))
					nFilesDeleted++;
			}

			aFilesFound.RemoveAll();
			nFiles = g_util.ScanDirSimple(sBaseFolder + "\\CCPDFfiles", "*.*", aFilesFound, "", MAXFILESINSCAN);
			if (nFiles < 0) {
				g_util.Logprintf("ERROR: ScanDirSimple(%s) failed", sBaseFolder + "\\CCPDFfiles");
				bHasError = TRUE;
				aFilesFound.RemoveAll();
				continue;
			}
			for (int it = 0; it < nFiles; it++) {
				CString s = aFilesFound[it];
				int m = s.ReverseFind('_');
				if (m == -1)
					continue;
				int nMasterCopySeparationSet = atoi(s.Mid(m + 5));
				if (nMasterCopySeparationSet == 0)
					continue;

				int test1 = g_db.HasMasterCopySeparationSet(nMasterCopySeparationSet, sErrorMessage);
				if (test1 > 0)
					continue; // Master number is use..
				if (test1 < 0 || sErrorMessage != _T("")) {
					continue;
				}

				int test2 = g_db.CountMasterCopySeparationSet(nMasterCopySeparationSet, sErrorMessage);
				if (test2 > 0)
					continue; // Master number is use..
				if (test2 < 0 || sErrorMessage != _T("")) {
					continue;
				}
				CString sFullPath;
				sFullPath.Format("%s\\CCPDFfiles\\%s", sBaseFolder, aFilesFound[it]);
				if (g_util.DeleteFileEx(sFullPath))
					nFilesDeleted++;
			}

			g_util.Logprintf("INFO: %d unrelated files deleted from web server", nFilesDeleted - f);
			aFilesFound.RemoveAll();

			break;
				
		}
	}

	//PurgeVersionFiles(g_prefs.m_overrulekeepdays > 0 ? g_prefs.m_overrulekeepdays : 10);


	return TRUE;
}

CString MakePreviewGUID(int nPublicationID, CTime tPubDate)
{
	CString sGuid;
	sGuid.Format("%.4d%.2d%.2d%.2d", nPublicationID, tPubDate.GetYear() - 2000, tPubDate.GetMonth(), tPubDate.GetDay());

	return sGuid;
}

BOOL DeleteProductFiles(PRODUCTSTRUCT *pProduct, int nEditionID, int nSectionID, int &nFilesDeleted, BOOL bSaveOldFiles, CString sArchiveFolder)
{
	int nProductionID = pProduct->m_productionID;
	int nPublicationID = pProduct->m_PublicationID;
	CTime tPubDate = pProduct->m_pubDate;

	CString sErrorMessage;
	nFilesDeleted = 0;

	CUIntArray aMasterSeparationSetList;
	CUIntArray aInputIDList;
	CString sFileToDelete;
	CString sSearchMask;
	CUIntArray aColorList;
	CStringArray aFileNameList;

	CString sErrorRootFolder = "";
	int nMaxVersions = 10;
	CUIntArray aCRCList;
	FILEINFOSTRUCT aReadviewFolders[MAXFILESINSCAN];

	CTime t = pProduct->m_pubDate;
	if (t.GetYear() > 2099) {
		int nCentury = t.GetYear() / 100;
		int nY = t.GetYear() - nCentury * 100;
		t = CTime(2000 + nY, t.GetMonth(), t.GetDay(), 0, 0, 0);
	}

	g_util.Logprintf("INFO: DeleteProductFiles(%d)", nProductionID);
	//CString sPreviewGUID = MakePreviewGUID(pProduct->m_PublicationID, t);

	if (g_db.GetErrorRootFolder(sErrorRootFolder, sErrorMessage) == FALSE) {
		g_util.Logprintf("ERROR: GetErrorRootFolder() - %s", sErrorMessage);
		return FALSE;
	}

	if (g_db.GetMasterSeparationSetNumbersEx(nProductionID, nPublicationID, tPubDate, nEditionID, nSectionID, aMasterSeparationSetList, aInputIDList, aFileNameList, sErrorMessage) == FALSE) {
		g_util.Logprintf("ERROR: GetMasterSeparationSetNumbers() - %s", sErrorMessage);
		return FALSE;
	}

	if (aMasterSeparationSetList.GetCount() == 0) {
		g_util.Logprintf("INFO:  No Unique Mastersets found for ProductionID %d - may be all common production", nProductionID);
		//return FALSE;	
	}

	if (g_db.GetMaxVersionCount(nProductionID, nPublicationID, tPubDate, nMaxVersions, sErrorMessage) == FALSE) {
		g_util.Logprintf("ERROR: GetMaxVersionCount() - %s", sErrorMessage);
		nMaxVersions = 20;
	}

	g_util.Logprintf("INFO:  Unique MasterSeparationSetNumbers %d", aMasterSeparationSetList.GetCount());

	int nFiles = 0;
	BOOL bHasError = FALSE;

	try
	{
		for (int i = 0; i < g_prefs.m_FileServerList.GetCount(); i++) {
			FILESERVERSTRUCT *pFileServer = &g_prefs.m_FileServerList[i];

			CString sBaseFolder = g_util.IsValidIP(pFileServer->m_IP) ?
				_T("\\\\") + pFileServer->m_IP + _T("\\") + pFileServer->m_sharename :
				_T("\\\\") + pFileServer->m_servername + _T("\\") + pFileServer->m_sharename;

			g_util.Logprintf("INFO:  Servertype %d basefolder is %s", pFileServer->m_servertype, sBaseFolder);

			if (g_util.CheckFolderWithPing(sBaseFolder) == FALSE) {
				g_util.Logprintf("ERROR: CheckFolderWithPing(%s) failed", sBaseFolder);
				bHasError = TRUE;
				continue;
			}

			if (g_prefs.m_alwaysusecurrentuser == FALSE) {
				if (pFileServer->m_uselocaluser == FALSE && pFileServer->m_username != _T("")) {
					g_util.Reconnect(sBaseFolder, pFileServer->m_username, pFileServer->m_password);
				}
			}

			if (g_util.CheckFolder(sBaseFolder) == FALSE) {
				g_util.Logprintf("ERROR: CheckFolder(%s) failed", sBaseFolder);
				bHasError = TRUE;
				continue;
			}

			// 1. Masterset stuff

			// CCFiles
			// CCPreviews
			// CCReadviewPreviews
			// CCThumbnails

			// FILESERVERTYPE_MAIN comes first!

			if (pFileServer->m_servertype == FILESERVERTYPE_MAIN ||
				pFileServer->m_servertype == FILESERVERTYPE_LOCAL ||
				pFileServer->m_servertype == FILESERVERTYPE_BACKUPSERVER ||
				pFileServer->m_servertype == FILESERVERTYPE_WEBCENTER ||
				pFileServer->m_servertype == FILESERVERTYPE_ADDITIONALPREVIEW) {
				g_util.Logprintf("INFO: Cleaning main server...");

				for (int j = 0; j < aMasterSeparationSetList.GetCount(); j++) {
					//g_util.Logprintf("INFO: MasterSeparationSetList %d ..", aMasterSeparationSetList[j]);

					// re-accuire lock..
		//			if (((j%100) == 0) && g_prefs.m_planlock)
			//			GetPlanLock();

					if (pFileServer->m_servertype == FILESERVERTYPE_MAIN) {
						int nRet = g_db.TestForOtherUniqueMasterPage(nProductionID, aMasterSeparationSetList[j], sErrorMessage);
						if (nRet > 0) {
							g_util.Logprintf("WARNING: MasterCopySeparationSet %d exists as unique page in another production - file is not deleted", aMasterSeparationSetList[j]);
							g_util.LogDeletedFiles(aMasterSeparationSetList[j], "Other unique exists - file not deleted");
							aMasterSeparationSetList[j] = 0; // Leave file(s) !!
						}

						nRet = g_db.TestMasterStillInPageTable(aMasterSeparationSetList[j], sErrorMessage);
						if (nRet == 0) {
							g_util.Logprintf("WARNING: MasterCopySeparationSet %d gone from pagetable (!) - file is not delrted", aMasterSeparationSetList[j]);
							g_util.LogDeletedFiles(aMasterSeparationSetList[j], "Table entry gone while processing - file not deleted");
							aMasterSeparationSetList[j] = 0; // Leave file(s) !!
						}
					}

					if (aMasterSeparationSetList[j] == 0)
						continue;


					// Save File first
					/*					if (aInputIDList[j] > 0) {
						if (bSaveOldFiles == SAVEFILEOPTION_UNKNOWN && pFileServer->m_servertype == FILESERVERTYPE_MAIN) {

							CString sErrorFolder;
							sErrorFolder.Format("%s\\%d", sErrorRootFolder, aInputIDList[j]);

							if (aFileNameList[j] != _T("") && aFileNameList[j] != _T("filename")) {

								sFileToDelete.Format("%s\\CCFilesHires\\%s#%d.pdf", sBaseFolder, aFileNameList[j], aMasterSeparationSetList[j]);
								if (g_util.FileExist(sFileToDelete) == FALSE)
									sFileToDelete.Format("%s\\CCFilesHires\\%d.pdf", sBaseFolder, aMasterSeparationSetList[j]);

								if (g_prefs.m_logging > 1)
									g_util.Logprintf("INFO:  Saving to unknown %s -> %s", sFileToDelete, sErrorFolder + _T("\\") + aFileNameList[j]);

								::CopyFile(sFileToDelete, sErrorFolder + _T("\\") + aFileNameList[j], FALSE);
							}
						}


						if (bSaveOldFiles == SAVEFILEOPTION_ARCHIVE && sArchiveFolder != _T("") && pFileServer->m_servertype == FILESERVERTYPE_MAIN) {

							if (aFileNameList[j] != _T("") && aFileNameList[j] != _T("filename")) {

								sFileToDelete.Format("%s\\CCFiles\\%s====%d.pdf", sBaseFolder, aFileNameList[j], aMasterSeparationSetList[j]);
								if (g_util.FileExist(sFileToDelete) == FALSE)
									sFileToDelete.Format("%s\\CCFiles\\%d.pdf", sBaseFolder, aMasterSeparationSetList[j]);

								if (g_prefs.m_logging > 1)
									g_util.Logprintf("INFO:  Saving to archive %s -> %s", sFileToDelete, sArchiveFolder + _T("\\") + aFileNameList[j]);
								::CopyFile(sFileToDelete, sArchiveFolder + _T("\\") + aFileNameList[j], FALSE);
							}

						}
					}

	

					if (aMasterSeparationSetList[j] == 0)
						continue;
*/



					if (pFileServer->m_servertype != FILESERVERTYPE_WEBCENTER) {

						sFileToDelete.Format("%s\\CCFilesHires\\%s#%d.pdf", sBaseFolder, g_util.GetFileName(aFileNameList[j], TRUE), aMasterSeparationSetList[j]);
						if (g_util.FileExist(sFileToDelete) == FALSE)
							sFileToDelete.Format("%s\\CCFilesHires\\%d.%pdf", sBaseFolder, aMasterSeparationSetList[j]);
						if (g_util.DeleteFileEx(sFileToDelete))
							nFiles++;
						if (g_util.FileExist(sFileToDelete)) {
							CString s;
							s.Format("Unable to delete file %s - %s", sFileToDelete, g_util.GetLastWin32Error());
							g_util.LogDeletedFiles(aMasterSeparationSetList[j], s);
							aMasterSeparationSetList[j] = 0;
						}

						sSearchMask.Format("*-%s#%d.pdf", g_util.GetFileName(aFileNameList[j], TRUE), aMasterSeparationSetList[j]);
						g_util.DeleteFiles(sBaseFolder + _T("\\CCVersions"), sSearchMask);

						sSearchMask.Format("*-%d.pdf", aMasterSeparationSetList[j]);
						g_util.DeleteFiles(sBaseFolder + _T("\\CCVersions"), sSearchMask);
					}



					if (pFileServer->m_servertype == FILESERVERTYPE_MAIN) {
						sFileToDelete.Format("%s\\CCFilesPrint\\%s#%d.pdf", sBaseFolder, g_util.GetFileName(aFileNameList[j], TRUE), aMasterSeparationSetList[j]);
						if (g_util.FileExist(sFileToDelete) == FALSE)
							sFileToDelete.Format("%s\\CCFilesPrint\\%d.pdf", sBaseFolder, aMasterSeparationSetList[j]);
						if (g_util.DeleteFileEx(sFileToDelete))
							nFiles++;
						if (g_util.FileExist(sFileToDelete)) {
							//CString s;
							//s.Format("Unable to delete file %s - %s", sFileToDelete, g_util.GetLastWin32Error());
							aMasterSeparationSetList[j] = 0;
						}
						sFileToDelete.Format("%s\\CCFilesLowres\\%s#%d.pdf", sBaseFolder, g_util.GetFileName(aFileNameList[j], TRUE), aMasterSeparationSetList[j]);
						if (g_util.FileExist(sFileToDelete) == FALSE)
							sFileToDelete.Format("%s\\CCFilesLowres\\%d.pdf", sBaseFolder, aMasterSeparationSetList[j]);
						if (g_util.DeleteFileEx(sFileToDelete))
							nFiles++;
						if (g_util.FileExist(sFileToDelete)) {
							g_util.Logprintf("ERROR: Unable to delete file %s - %s", sFileToDelete, g_util.GetLastWin32Error());
							aMasterSeparationSetList[j] = 0;
						}
					}

					if (pFileServer->m_servertype == FILESERVERTYPE_MAIN)
						g_util.LogDeletedFiles(aMasterSeparationSetList[j], "Deleted");

					sFileToDelete.Format("%s\\CCPreviews\\%d.jpg", sBaseFolder, aMasterSeparationSetList[j]);
					if (g_util.DeleteFileEx(sFileToDelete))
						nFiles++;

					sFileToDelete.Format("%s\\CCPreviews\\%s#%d.jpg", sBaseFolder, g_util.GetFileName(aFileNameList[j]), aMasterSeparationSetList[j]);
					if (g_util.DeleteFileEx(sFileToDelete))
						nFiles++;

					// Versions.-
					sSearchMask.Format("%d_*.jpg", aMasterSeparationSetList[j]);
					nFiles += g_util.DeleteFiles(sBaseFolder + _T("\\CCPreviews"), sSearchMask);

					sSearchMask.Format("%s#%d_*.jpg", g_util.GetFileName(aFileNameList[j], TRUE), aMasterSeparationSetList[j]);
					nFiles += g_util.DeleteFiles(sBaseFolder + _T("\\CCPreviews"), sSearchMask);

					sFileToDelete.Format("%s\\CCPreviews\\%d_dns.jpg", sBaseFolder, aMasterSeparationSetList[j]);
					if (g_util.DeleteFileEx(sFileToDelete))
						nFiles++;
					sFileToDelete.Format("%s\\CCPreviews\\%s#%d_dns.jpg", sBaseFolder, g_util.GetFileName(aFileNameList[j], TRUE), aMasterSeparationSetList[j]);
					if (g_util.DeleteFileEx(sFileToDelete))
						nFiles++;

					sFileToDelete.Format("%s\\CCPreviews\\%d_mask.jpg", sBaseFolder, aMasterSeparationSetList[j]);
					if (g_util.DeleteFileEx(sFileToDelete))
						nFiles++;
					sFileToDelete.Format("%s\\CCPreviews\\%s#%d_mask.jpg", sBaseFolder, g_util.GetFileName(aFileNameList[j], TRUE), aMasterSeparationSetList[j]);
					if (g_util.DeleteFileEx(sFileToDelete))
						nFiles++;

					// Thumbnails--

					sFileToDelete.Format("%s\\CCThumbnails\\%d.jpg", sBaseFolder, aMasterSeparationSetList[j]);
					if (g_util.DeleteFileEx(sFileToDelete))
						nFiles++;
					sFileToDelete.Format("%s\\CCThumbnails\\%s#%d.jpg", sBaseFolder, g_util.GetFileName(aFileNameList[j], TRUE), aMasterSeparationSetList[j]);
					if (g_util.DeleteFileEx(sFileToDelete))
						nFiles++;



					sFileToDelete.Format("%s\\CCThumbnails\\%d_mask.jpg", sBaseFolder, aMasterSeparationSetList[j]);
					if (g_util.DeleteFileEx(sFileToDelete))
						nFiles++;
					sFileToDelete.Format("%s\\CCThumbnails\\%s#%d_mask.jpg", sBaseFolder, g_util.GetFileName(aFileNameList[j], TRUE), aMasterSeparationSetList[j]);
					if (g_util.DeleteFileEx(sFileToDelete))
						nFiles++;

					sSearchMask.Format("%d-*.jpg", aMasterSeparationSetList[j]);
					g_util.DeleteFiles(sBaseFolder + _T("\\CCThumbnails"), sSearchMask);

					sSearchMask.Format("%s#%d-*.jpg", g_util.GetFileName(aFileNameList[j], TRUE), aMasterSeparationSetList[j]);
					g_util.DeleteFiles(sBaseFolder + _T("\\CCThumbnails"), sSearchMask);


					sSearchMask.Format("*_%d.jpg", aMasterSeparationSetList[j]);
					nFiles += g_util.DeleteFiles(sBaseFolder + _T("\\CCReadviewPreviews"), sSearchMask);

					sSearchMask.Format("%d_*.jpg", aMasterSeparationSetList[j]);
					nFiles += g_util.DeleteFiles(sBaseFolder + _T("\\CCReadviewPreviews"), sSearchMask);

					sSearchMask.Format("%s#%d_*.jpg", g_util.GetFileName(aFileNameList[j], TRUE), aMasterSeparationSetList[j]);
					nFiles += g_util.DeleteFiles(sBaseFolder + _T("\\CCReadviewPreviews"), sSearchMask);

					sSearchMask.Format("%d_*_dns.jpg", aMasterSeparationSetList[j]);
					nFiles += g_util.DeleteFiles(sBaseFolder + _T("\\CCReadviewPreviews"), sSearchMask);
					sSearchMask.Format("%s#%d_*_dns.jpg", g_util.GetFileName(aFileNameList[j], TRUE), aMasterSeparationSetList[j]);
					nFiles += g_util.DeleteFiles(sBaseFolder + _T("\\CCReadviewPreviews"), sSearchMask);

					sSearchMask.Format("%d_*_mask.jpg", aMasterSeparationSetList[j]);
					nFiles += g_util.DeleteFiles(sBaseFolder + _T("\\CCReadviewPreviews"), sSearchMask);
					sSearchMask.Format("%s#%d_*_mask.jpg", g_util.GetFileName(aFileNameList[j], TRUE), aMasterSeparationSetList[j]);
					nFiles += g_util.DeleteFiles(sBaseFolder + _T("\\CCReadviewPreviews"), sSearchMask);

					// Delete tile folders
					if (pFileServer->m_servertype == FILESERVERTYPE_WEBCENTER) {
						CString sFileBaseFolder;

						for (int v = 0; v <= nMaxVersions; v++) {
							if (v == 0)
								sFileBaseFolder.Format("\\CCpreviews\\%d", aMasterSeparationSetList[j]);
							else
								sFileBaseFolder.Format("\\CCpreviews\\%d-%d", aMasterSeparationSetList[j], v);

							if (g_util.DeleteFileEx(sBaseFolder + sFileBaseFolder + _T("\\imageProperties.xml")))
								nFiles++;

							nFiles += g_util.DeleteFiles(sBaseFolder + sFileBaseFolder + _T("\\TileGroup0"), "*.jpg", TRUE);

							g_util.DeleteFolder(sBaseFolder + sFileBaseFolder);

							if (v == 0)
								sFileBaseFolder.Format("\\CCpreviews\\%s#%d", g_util.GetFileName(aFileNameList[j], TRUE), aMasterSeparationSetList[j]);
							else
								sFileBaseFolder.Format("\\CCpreviews\\%s#%d-%d", g_util.GetFileName(aFileNameList[j], TRUE), aMasterSeparationSetList[j], v);

							if (g_util.DeleteFileEx(sBaseFolder + sFileBaseFolder + _T("\\imageProperties.xml")))
								nFiles++;

							nFiles += g_util.DeleteFiles(sBaseFolder + sFileBaseFolder + _T("\\TileGroup0"), "*.jpg", TRUE);

							g_util.DeleteFolder(sBaseFolder + sFileBaseFolder);
							
							if (v == 0)
								sFileBaseFolder.Format("\\CCpreviews\\%d_mask", aMasterSeparationSetList[j]);
							else
								sFileBaseFolder.Format("\\CCpreviews\\%d-%d_mask", aMasterSeparationSetList[j], v);

							if (g_util.DeleteFileEx(sBaseFolder + sFileBaseFolder + _T("\\imageProperties.xml")))
								nFiles++;

							nFiles += g_util.DeleteFiles(sBaseFolder + sFileBaseFolder + _T("\\TileGroup0"), "*.jpg", TRUE);

							g_util.DeleteFolder(sBaseFolder + sFileBaseFolder);
							
							if (v == 0)
								sFileBaseFolder.Format("\\CCpreviews\\%s#%d_mask", g_util.GetFileName(aFileNameList[j], TRUE), aMasterSeparationSetList[j]);
							else
								sFileBaseFolder.Format("\\CCpreviews\\%s#%d-%d_mask", g_util.GetFileName(aFileNameList[j], TRUE), aMasterSeparationSetList[j], v);

							if (g_util.DeleteFileEx(sBaseFolder + sFileBaseFolder + _T("\\imageProperties.xml")))
								nFiles++;

							nFiles += g_util.DeleteFiles(sBaseFolder + sFileBaseFolder + _T("\\TileGroup0"), "*.jpg", TRUE);

							g_util.DeleteFolder(sBaseFolder + sFileBaseFolder);
						}


						sSearchMask.Format("%d_*", aMasterSeparationSetList[j]);

						int nDirs = g_util.GetDirectoryList(sBaseFolder + _T("\\CCReadviewPreviews\\"), sSearchMask, aReadviewFolders, MAXFILESINSCAN);
						for (int m = 0; m < nDirs; m++) {
							sFileBaseFolder.Format("\\%s", aReadviewFolders[m].sFileName);

							if (g_util.DeleteFileEx(sBaseFolder + _T("\\CCReadviewPreviews") + sFileBaseFolder + _T("\\imageProperties.xml")))
								nFiles++;
							nFiles += g_util.DeleteFiles(sBaseFolder + _T("\\CCReadviewPreviews") + sFileBaseFolder + _T("\\TileGroup0"), "*.*", TRUE);

							// SeaDragin tiles.
							g_util.DeleteFileEx(sBaseFolder + _T("\\CCReadviewPreviews") + sFileBaseFolder + "\\image.xml");
							for (int z = 0; z < 25; z++) {
								g_util.DeleteFiles(sBaseFolder + _T("\\CCReadviewPreviews") + sFileBaseFolder + _T("\\image_files\\") + g_util.Int2String(z), "*.*", TRUE);
							}
							g_util.DeleteFolder(sBaseFolder + _T("\\CCReadviewPreviews") + sFileBaseFolder + _T("\\image_files"));

							g_util.DeleteFolder(sBaseFolder + _T("\\CCReadviewPreviews") + sFileBaseFolder);

						}

						/*
						sSearchMask.Format("%s#%d_*", g_util.GetFileName(aFileNameList[j], TRUE), aMasterSeparationSetList[j]);

						nDirs = g_util.GetDirectoryList(sBaseFolder + _T("\\CCReadviewPreviews\\"), sSearchMask, aReadviewFolders, MAXFILESINSCAN);
						for (int m = 0; m < nDirs; m++) {
							sFileBaseFolder.Format("\\%s", aReadviewFolders[m].sFileName);

							if (g_util.DeleteFileEx(sBaseFolder + _T("\\CCReadviewPreviews") + sFileBaseFolder + _T("\\imageProperties.xml")))
								nFiles++;
							nFiles += g_util.DeleteFiles(sBaseFolder + _T("\\CCReadviewPreviews") + sFileBaseFolder + _T("\\TileGroup0"), "*.*", TRUE);

							// SeaDragin tiles.
							g_util.DeleteFileEx(sBaseFolder + _T("\\CCReadviewPreviews") + sFileBaseFolder + "\\image.xml");
							for (int z = 0; z < 25; z++) {
								g_util.DeleteFiles(sBaseFolder + _T("\\CCReadviewPreviews") + sFileBaseFolder + _T("\\image_files\\") + g_util.Int2String(z), "*.*", TRUE);
							}
							g_util.DeleteFolder(sBaseFolder + _T("\\CCReadviewPreviews") + sFileBaseFolder + _T("\\image_files"));

							g_util.DeleteFolder(sBaseFolder + _T("\\CCReadviewPreviews") + sFileBaseFolder);

						}

						// Mask readview tile files
						sSearchMask.Format("*_%d_mask", aMasterSeparationSetList[j]);

						nDirs = g_util.GetDirectoryList(sBaseFolder + _T("\\CCReadviewPreviews\\"), sSearchMask, aReadviewFolders, MAXFILESINSCAN);
						for (int m = 0; m < nDirs; m++) {
							sFileBaseFolder.Format("\\%s", aReadviewFolders[m].sFileName);

							if (g_util.DeleteFileEx(sBaseFolder + _T("\\CCReadviewPreviews") + sFileBaseFolder + _T("\\imageProperties.xml")))
								nFiles++;
							nFiles += g_util.DeleteFiles(sBaseFolder + _T("\\CCReadviewPreviews") + sFileBaseFolder + _T("\\TileGroup0"), "*.*", TRUE);

							// SeaDragin tiles.
							g_util.DeleteFileEx(sBaseFolder + _T("\\CCReadviewPreviews") + sFileBaseFolder + "\\image.xml");
							for (int z = 0; z < 25; z++) {
								g_util.DeleteFiles(sBaseFolder + _T("\\CCReadviewPreviews") + sFileBaseFolder + _T("\\image_files\\") + g_util.Int2String(z), "*.*", TRUE);
							}
							g_util.DeleteFolder(sBaseFolder + _T("\\CCReadviewPreviews") + sFileBaseFolder + _T("\\image_files"));

							g_util.DeleteFolder(sBaseFolder + _T("\\CCReadviewPreviews") + sFileBaseFolder);
						}

						sSearchMask.Format("%d_*_mask", aMasterSeparationSetList[j]);
						nDirs = g_util.GetDirectoryList(sBaseFolder + _T("\\CCReadviewPreviews\\"), sSearchMask, aReadviewFolders, MAXFILESINSCAN);
						for (int m = 0; m < nDirs; m++) {
							sFileBaseFolder.Format("\\%s", aReadviewFolders[m].sFileName);

							if (g_util.DeleteFileEx(sBaseFolder + _T("\\CCReadviewPreviews") + sFileBaseFolder + _T("\\imageProperties.xml")))
								nFiles++;
							nFiles += g_util.DeleteFiles(sBaseFolder + _T("\\CCReadviewPreviews") + sFileBaseFolder + _T("\\TileGroup0"), "*.*", TRUE);
							// SeaDragin tiles.
							g_util.DeleteFileEx(sBaseFolder + _T("\\CCReadviewPreviews") + sFileBaseFolder + "\\image.xml");
							for (int z = 0; z < 25; z++) {
								g_util.DeleteFiles(sBaseFolder + _T("\\CCReadviewPreviews") + sFileBaseFolder + _T("\\image_files\\") + g_util.Int2String(z), "*.*", TRUE);
							}
							g_util.DeleteFolder(sBaseFolder + _T("\\CCReadviewPreviews") + sFileBaseFolder + _T("\\image_files"));

							g_util.DeleteFolder(sBaseFolder + _T("\\CCReadviewPreviews") + sFileBaseFolder);

						}

						sSearchMask.Format("%s#%d_*_mask", g_util.GetFileName(aFileNameList[j]), aMasterSeparationSetList[j]);
						nDirs = g_util.GetDirectoryList(sBaseFolder + _T("\\CCReadviewPreviews\\"), sSearchMask, aReadviewFolders, MAXFILESINSCAN);
						for (int m = 0; m < nDirs; m++) {
							sFileBaseFolder.Format("\\%s", aReadviewFolders[m].sFileName);

							if (g_util.DeleteFileEx(sBaseFolder + _T("\\CCReadviewPreviews") + sFileBaseFolder + _T("\\imageProperties.xml")))
								nFiles++;
							nFiles += g_util.DeleteFiles(sBaseFolder + _T("\\CCReadviewPreviews") + sFileBaseFolder + _T("\\TileGroup0"), "*.*", TRUE);
							// SeaDragin tiles.
							g_util.DeleteFileEx(sBaseFolder + _T("\\CCReadviewPreviews") + sFileBaseFolder + "\\image.xml");
							for (int z = 0; z < 25; z++) {
								g_util.DeleteFiles(sBaseFolder + _T("\\CCReadviewPreviews") + sFileBaseFolder + _T("\\image_files\\") + g_util.Int2String(z), "*.*", TRUE);
							}
							g_util.DeleteFolder(sBaseFolder + _T("\\CCReadviewPreviews") + sFileBaseFolder + _T("\\image_files"));

							g_util.DeleteFolder(sBaseFolder + _T("\\CCReadviewPreviews") + sFileBaseFolder);
							
						}*/
					}

					
			
				

				}

	

			}


			if (pFileServer->m_servertype == FILESERVERTYPE_WEBCENTER ||
				pFileServer->m_servertype == FILESERVERTYPE_PDFSERVER) {

				g_util.Logprintf("INFO: Cleaning webserver...");
				for (int j = 0; j < aMasterSeparationSetList.GetCount(); j++) {

					// re-accuire lock..
		//			if (((j%100) == 0) && g_prefs.m_planlock)
		//				GetPlanLock();

					if (aMasterSeparationSetList[j] == 0)
						continue;

					sSearchMask.Format("*_log_%d.pdf", aMasterSeparationSetList[j]);
					nFiles += g_util.DeleteFiles(sBaseFolder + _T("\\CCPDFLogs"), sSearchMask);

					sSearchMask.Format("*_report_%d.pdf", aMasterSeparationSetList[j]);
					nFiles += g_util.DeleteFiles(sBaseFolder + _T("\\CCPDFLogs"), sSearchMask);

					sSearchMask.Format("*_%d.pdf", aMasterSeparationSetList[j]);
					nFiles += g_util.DeleteFiles(sBaseFolder + _T("\\CCPDFfiles"), sSearchMask);

					sSearchMask.Format("*_%d.pdf", aMasterSeparationSetList[j]);
					nFiles += g_util.DeleteFiles(sBaseFolder + _T("\\CCAlwanlogs"), sSearchMask);

				}
			}




		} // end while FileServers
	}
	catch (CSeException *ex)
	{
		ex->ReportError(_T("WorkerThread 1"));
		ex->Delete();
		return 0;
	}
	// Old style remote folder delete


	g_util.Logprintf("INFO: Done cleaning remote location folder(s)");

	nFilesDeleted = nFiles;

	aMasterSeparationSetList.RemoveAll();
	aInputIDList.RemoveAll();
	aColorList.RemoveAll();
	aCRCList.RemoveAll();
	aFileNameList.RemoveAll();

	return bHasError ? FALSE : TRUE;
}
