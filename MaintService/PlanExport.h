#pragma once
#include "stdafx.h"

static TCHAR szPageTypes[4][14] = { "Normal","Panorama","Antipanorama","Dummy" };
static TCHAR szPageUniques[3][8] = { "false","true","force" };

#define FORMAT_AUTO				0
#define FORMAT_PREPAIRED		1
#define FORMAT_FORCEBROADSHEET	2
#define FORMAT_FORCETABLOID		3

#define PAGETYPE_NORMAL			0
#define PAGETYPE_PANORAMA		1
#define PAGETYPE_ANTIPANORAMA	2
#define PAGETYPE_DUMMY			3

#define PAGEUNIQUE_UNIQUE		1
#define PAGEUNIQUE_COMMON		0
#define PAGEUNIQUE_FORCED		2

#define PLANSTATE_UNKNOWN		0
#define PLANSTATE_NOTIMPORTED	1
#define PLANSTATE_IMPORTED		2
#define PLANSTATE_ERROR			3
#define PLANSTATE_INPROGRESS	4

#define MAXPLATESETS 4096


////////////////////////////////

/// <summary>
///  Level 5 - page separation
/// </summary>
/// 
class PlanDataPageSeparation
{
public:
	CString colorName;
public:
	PlanDataPageSeparation()
	{
	}

	PlanDataPageSeparation(CString color)
	{
		colorName = color;
	}
};

typedef CArray <PlanDataPageSeparation, PlanDataPageSeparation&> ListPlanDataPageSeparation;


/// <summary>
///  Level 4 - pages
/// </summary>
/// 
class PlanDataPage
{
public:

	CString pageName;
	CString fileName;
	CString pageID;
	int		pageType;
	int		pagination;
	int		pageIndex;
	CString comment;
	ListPlanDataPageSeparation *colorList;
	int		uniquePage;
	CString masterPageID;

	BOOL	approve;
	BOOL	hold;
	int		priority;
	int		version;
	CString masterEdition;
	CString miscstring1;
	CString miscstring2;
	int	miscint;
	CString pageformat;
	CString planPageName;

public:
	PlanDataPage()
	{
		pageName = "";
		fileName = "";
		pageID = "";
		pageType = PAGETYPE_NORMAL;
		pagination = 0;
		pageIndex = 0;
		comment = "";

		masterPageID = "";
		masterEdition = "";
		approve = FALSE;
		hold = FALSE;
		priority = 50;
		version = 0;
		uniquePage = PAGEUNIQUE_UNIQUE;
		miscstring1 = "";
		miscstring2 = "";
		miscint = 0;
		pageformat = "";
		colorList = new ListPlanDataPageSeparation();
		planPageName = "";

	}

	PlanDataPage(CString sPageName)
	{
		pageName = sPageName;
		fileName = "";
		pageID = "";
		pageType = PAGETYPE_NORMAL;
		pagination = 0;
		pageIndex = 0;
		comment = "";

		masterPageID = "";
		approve = FALSE;
		hold = FALSE;
		priority = 50;
		version = 0;
		uniquePage = PAGEUNIQUE_UNIQUE;
		masterEdition = "";
		miscstring1 = "";
		miscstring2 = "";
		miscint = 0;

		colorList = new ListPlanDataPageSeparation();
	}


	BOOL ClonePage(PlanDataPage *pCopy)
	{
		if (pCopy == NULL)
			return FALSE;

		pCopy->approve = approve;
		pCopy->comment = comment;
		pCopy->fileName = fileName;
		pCopy->hold = hold;
		pCopy->masterEdition = masterEdition;
		pCopy->masterPageID = masterPageID;
		pCopy->miscint = miscint;
		pCopy->miscstring1 = miscstring1;
		pCopy->miscstring2 = miscstring2;
		pCopy->pageID = pageID;
		pCopy->pageIndex = pageIndex;
		pCopy->pageName = pageName;
		pCopy->pageType = pageType;
		pCopy->pagination = pagination;
		pCopy->priority = priority;
		pCopy->uniquePage = uniquePage;
		pCopy->version = version;
		pCopy->colorList->RemoveAll();

		for (int color = 0; color<colorList->GetCount(); color++) {
			PlanDataPageSeparation *org = &colorList->GetAt(color);
			PlanDataPageSeparation *sep = new PlanDataPageSeparation(org->colorName);
			pCopy->colorList->Add(*sep);
		}

		return TRUE;
	}


};

typedef CArray <PlanDataPage, PlanDataPage&> ListPlanDataPage;

/// <summary>
///  Group level 3 - sections
/// </summary>
class PlanDataSection
{
public:
	CString sectionName;

	ListPlanDataPage *pageList;
	int pagesInSection;
	CString format;
public:
	PlanDataSection()
	{
		sectionName = "";
		pagesInSection = 0;
		format = "Tabloid";

		//pageList.RemoveAll();
		pageList = new ListPlanDataPage();
	}

	PlanDataSection(CString sSectionName)
	{
		sectionName = sSectionName;
		pagesInSection = 0;
		//pageList.RemoveAll();
		pageList = new ListPlanDataPage();
	}

	PlanDataPage *GetPageObject(CString sPageName)
	{
		for (int i = 0; i<pageList->GetCount(); i++) {
			PlanDataPage *p = &pageList->GetAt(i);
			if (sPageName.CompareNoCase(p->pageName) == 0)
				return p;
		}

		return NULL;
	}

	PlanDataPage *GetPageObject(int nPageIndex)
	{
		for (int i = 0; i<pageList->GetCount(); i++) {
			PlanDataPage *p = &pageList->GetAt(i);
			if (nPageIndex == p->pageIndex)
				return p;
		}

		return NULL;
	}
};

typedef CArray <PlanDataSection, PlanDataSection&> ListPlanDataSection;

/// <summary>
/// Sheet level 4 - sheet press cylinder
/// </summary>
class PlanDataSheetPressCylinder
{
public:

	CString pressCylinder;
	CString colorName;
	CString formID;
	CString plateID;
	CString name;
	CString sortingPosition;
public:
	PlanDataSheetPressCylinder()
	{
		name = "";
		pressCylinder = "";
		colorName = "K";
		formID = "";
		plateID = "";
		sortingPosition = "";
	}

	PlanDataSheetPressCylinder(CString colorNameIn)
	{
		name = "";
		pressCylinder = "";
		colorName = colorNameIn;
		formID = "";
		plateID = "";
		sortingPosition = "";
	}
};

typedef CArray <PlanDataSheetPressCylinder, PlanDataSheetPressCylinder&> ListPlanDataSheetPressCylinder;

/// <summary>
/// Sheet level 3 - sheet item (page)
/// </summary>
class PlanDataSheetItem
{
public:
	int pagePositionX;
	int pagePositionY;
	// CString pageName;
	CString pageID;
	CString masterPageID;
	int rotation;

public:
	PlanDataSheetItem()
	{
		pagePositionX = 1;
		pagePositionY = 1;
		pageID = "";
		masterPageID = "";
		rotation = 0;
	}
};

typedef CArray <PlanDataSheetItem, PlanDataSheetItem&> ListPlanDataSheetItem;


/// <summary>
/// Sheet level 3 - sheet side 
/// </summary>
class PlanDataSheetSide
{
public:
	CString SortingPosition;
	CString PressTower;
	CString PressZone;
	CString PressHighLow;
	int		ActiveCopies;
	CString CollectPosition;

	ListPlanDataSheetItem *sheetItems;
	ListPlanDataSheetPressCylinder  *pressCylinders;

public:
	PlanDataSheetSide()
	{
		SortingPosition = "";
		PressTower = "";
		PressZone = "";
		PressHighLow = "";
		ActiveCopies = 1;
		CollectPosition = "";

		sheetItems = new ListPlanDataSheetItem();
		pressCylinders = new ListPlanDataSheetPressCylinder();
	}
};

typedef CArray <PlanDataSheetSide, PlanDataSheetSide&> ListPlanDataSheetSide;

/// <summary>
/// Sheet level 2 - sheet (both sides) 
/// </summary>
class PlanDataSheet
{
public:
	CString sheetName;
	CString templateName;
	int		pagesOnPlate;
	CString markGroups;

	BOOL	hasback;
	PlanDataSheetSide *frontSheet;
	PlanDataSheetSide *backSheet;
	int frontCopyFlatSeparationSet;
	int backCopyFlatSeparationSet;

	int  pressSectionNumber;
	PlanDataSheet()
	{
		sheetName = "";
		templateName = "";
		pagesOnPlate = 1;
		markGroups = "";
		hasback = TRUE;
		frontCopyFlatSeparationSet = 0;
		backCopyFlatSeparationSet = 0;

		frontSheet = new PlanDataSheetSide();
		backSheet = new PlanDataSheetSide();
		pressSectionNumber = 1;

	}
};

typedef CArray <PlanDataSheet, PlanDataSheet&> ListPlanDataSheet;


class PlanDataPress
{
public:
	CString	pressName;
	int		copies;
	CTime	pressRunTime;
	int		paperCopies;
	CString postalUrl;
	CString jdfPressName;
	BOOL	isLocalPress;
public:
	PlanDataPress() {
		pressName = "";
		copies = 1;
		pressRunTime = CTime(1975, 1, 1, 0, 0, 0);
		paperCopies = 0;
		postalUrl = "";
		jdfPressName = "";
		isLocalPress = TRUE;
	}

	PlanDataPress(CString sPressName) {
		pressName = sPressName;
		copies = 1;
		pressRunTime = CTime(1975, 1, 1, 0, 0, 0);
		paperCopies = 0;
		postalUrl = "";
		jdfPressName = "";
		isLocalPress = TRUE;

	}
};

typedef CArray <PlanDataPress, PlanDataPress&> ListPlanDataPress;


/// <summary>
///  Group level 2 - edtion
/// </summary>
class PlanDataEdition
{
public:
	CString				editionName;

	ListPlanDataPress	*pressList;
	ListPlanDataSection *sectionList;
	ListPlanDataSheet	*sheetList;

	BOOL				masterEdition;
	int					editionCopy;
	int					editionSequenceNumber;
	CString				editionComment;


	CString timedFrom;
	CString timedTo;
	BOOL	IsTimed;
	CString zoneMaster;
	CString	locationMaster;

public:
	PlanDataEdition()
	{
		editionName = "";
		editionCopy = 0;
		masterEdition = FALSE;
		pressList = new ListPlanDataPress();
		sectionList = new ListPlanDataSection();
		sheetList = new ListPlanDataSheet();
		editionSequenceNumber = 1;
		editionComment = _T("");
		IsTimed = TRUE;
		timedTo = _T("");
		timedFrom = _T("");
		zoneMaster = _T("");
		locationMaster = _T("");

	}


	PlanDataEdition(CString sEditionName)
	{
		editionName = sEditionName;
		masterEdition = FALSE;
		editionCopy = 0;

		pressList = new ListPlanDataPress();
		sectionList = new ListPlanDataSection();
		sheetList = new ListPlanDataSheet();

	}


	PlanDataPress *GetPressObject(CString sPressName)
	{
		for (int i = 0; i<pressList->GetCount(); i++) {
			PlanDataPress *p = &pressList->GetAt(i);
			if (sPressName.CompareNoCase(p->pressName) == 0)
				return p;
		}

		return NULL;
	}

	PlanDataSection *GetSectionObject(CString sSectionName)
	{
		for (int i = 0; i<sectionList->GetCount(); i++) {
			PlanDataSection *p = &sectionList->GetAt(i);
			if (sSectionName.CompareNoCase(p->sectionName) == 0)
				return p;
		}

		return NULL;
	}

	PlanDataSheet *GetSheetObject(CString sSheetName)
	{
		for (int i = 0; i<sheetList->GetCount(); i++) {
			PlanDataSheet *p = &sheetList->GetAt(i);
			if (sSheetName.CompareNoCase(p->sheetName) == 0)
				return p;
		}

		return NULL;
	}

	BOOL CloneEdition(PlanDataEdition *pCopy)
	{
		if (pCopy == NULL)
			return FALSE;
		pCopy->editionName = editionName;
		pCopy->editionCopy = editionCopy;
		pCopy->masterEdition = masterEdition;

		pCopy->timedFrom = timedFrom;
		pCopy->timedTo = timedTo;
		pCopy->IsTimed = IsTimed;
		pCopy->zoneMaster = zoneMaster;
		pCopy->editionComment = editionComment;

		for (int i = 0; i<pressList->GetCount(); i++) {
			PlanDataPress *pPress = &pressList->GetAt(i);
			PlanDataPress *pPressCopy = new PlanDataPress();
			pPressCopy->copies = pPress->copies;
			pPressCopy->isLocalPress = pPress->isLocalPress;
			pPressCopy->jdfPressName = pPress->jdfPressName;
			pPressCopy->paperCopies = pPress->paperCopies;
			pPressCopy->postalUrl = pPress->postalUrl;
			pPressCopy->pressName = pPress->pressName;
			pPressCopy->pressRunTime = pPress->pressRunTime;
			pCopy->pressList->Add(*pPressCopy);
		}
		/*
		for(int i=0; i<sectionList->GetCount(); i++) {
		PlanDataSection *pSection = &sectionList->GetAt(i);
		PlanDataSection *pSectionCopy = new PlanDataSection();
		pSectionCopy->pagesInSection = pSection->pagesInSection;
		pSectionCopy->sectionName = pSection->sectionName;
		pCopy->sectionList->Add(*pSectionCopy);
		}
		*/
		return TRUE;
	}



};

typedef CArray <PlanDataEdition, PlanDataEdition&> ListPlanDataEdition;

#define PLANSTATE_UNLOCKED 0
#define PLANSTATE_LOCKED   1


class PlanData
{
public:
	CString planName;
	CString planID;
	CString publicationName;
	CString publicationAlias;
	CString publicationPart;
	CTime publicationDate;
	int version;
	ListPlanDataEdition *editionList;
	int state;

	CString pagenameprefix;

	CTime   updatetime;
	CTime printTime;
	CString sender;

	CStringArray arrSectionNames;
	CUIntArray arrSectionFormats;
	CUIntArray arrSectionPagecount;

	CString defaultColors;

	CStringArray arrSectionColors;

	CString customername;
	CString customeralias;
	int weekNumber;
	CString command;
	CString planFileName;
public:

	PlanData()
	{
		weekNumber = 0;
		pagenameprefix = "";
		planName = "";
		planID = "";
		publicationName = "";
		publicationAlias = "";
		publicationPart = "";
		publicationDate = CTime(1975, 1, 1, 0, 0, 0);
		printTime = CTime(1975, 1, 1, 0, 0, 0);
		updatetime = CTime(1975, 1, 1, 0, 0, 0);
		sender = "";
		version = 1;
		state = PLANSTATE_LOCKED;

		editionList = new ListPlanDataEdition();

		arrSectionNames.RemoveAll();
		arrSectionFormats.RemoveAll();
		arrSectionColors.RemoveAll();
		arrSectionPagecount.RemoveAll();


		defaultColors = "CMYK";
		customername = "";
		customeralias = "";
		command = "";
		planFileName = "";
	}





	PlanDataEdition *GetEditionObject(CString sEditionName)
	{
		for (int i = 0; i<editionList->GetCount(); i++) {
			PlanDataEdition *p = &editionList->GetAt(i);
			if (sEditionName.CompareNoCase(p->editionName) == 0)
				return p;
		}

		return NULL;
	}

	PlanDataPage *FindMasterPage(CString sectionName, int pageIndex)
	{
		for (int edition = 0; edition<editionList->GetCount(); edition++) {
			PlanDataEdition *pEdition = &editionList->GetAt(edition);
			for (int section = 0; section<pEdition->sectionList->GetCount(); section++) {
				PlanDataSection *pSection = &pEdition->sectionList->GetAt(section);
				if (sectionName.CompareNoCase(pSection->sectionName) == 0) {
					for (int page = 0; page<pSection->pageList->GetCount(); page++) {
						PlanDataPage *pPage = &pSection->pageList->GetAt(page);
						if (pPage->pageIndex == pageIndex && pPage->pageName != "") {
							return pPage;
						}
					}
				}
			}
		}

		return NULL;
	}


	PlanDataPage *FindMasterPage(CString masterEditionName, CString sectionName, int pageIndex)
	{
		PlanDataEdition *pEdition = GetEditionObject(masterEditionName);
		if (pEdition == NULL)
			return NULL;

		for (int section = 0; section<pEdition->sectionList->GetCount(); section++) {
			PlanDataSection *pSection = &pEdition->sectionList->GetAt(section);
			if (sectionName.CompareNoCase(pSection->sectionName) == 0) {

				for (int page = 0; page<pSection->pageList->GetCount(); page++) {
					PlanDataPage *pPage = &pSection->pageList->GetAt(page);
					if (pPage->pageIndex == pageIndex && pPage->pageName != "") {
						return pPage;
					}
				}
			}
		}

		return NULL;
	}

	CString FindNextFreePageID()
	{
		int nMaxPageId = 0;
		for (int edition = 0; edition<editionList->GetCount(); edition++) {
			PlanDataEdition *pEdition = &editionList->GetAt(edition);
			for (int section = 0; section<pEdition->sectionList->GetCount(); section++) {
				PlanDataSection *pSection = &pEdition->sectionList->GetAt(section);
				for (int page = 0; page<pSection->pageList->GetCount(); page++) {
					PlanDataPage *pPage = &pSection->pageList->GetAt(page);
					if (atoi(pPage->pageID) > nMaxPageId) {

						nMaxPageId = atoi(pPage->pageID);
					}
				}
			}
		}

		CString s;
		s.Format("%d", nMaxPageId + 1);
		return s;
	}

	PlanDataPage *FindPageByFileName(CString fileName, CString sEditionName)
	{
		PlanDataEdition *pEdition = GetEditionObject(sEditionName);
		if (pEdition == NULL)
			return NULL;

		CString fileNameNoExt = fileName;
		fileNameNoExt.MakeUpper();

		// 0123456.pdf
		int n = fileNameNoExt.Find(".PDF");
		if (n != -1)
			fileNameNoExt = fileName.Mid(0, n);

		for (int section = 0; section<pEdition->sectionList->GetCount(); section++) {
			PlanDataSection *pSection = &pEdition->sectionList->GetAt(section);
			for (int page = 0; page<pSection->pageList->GetCount(); page++) {
				PlanDataPage *pPage = &pSection->pageList->GetAt(page);
				if (fileName.CompareNoCase(pPage->fileName) == 0) {
					return pPage;
				}
				if (fileNameNoExt.CompareNoCase(pPage->fileName) == 0) {
					return pPage;
				}
			}
		}

		return NULL;
	}

	PlanDataPage *FindPageByPageID(CString sPageID)
	{
		for (int edition = 0; edition < editionList->GetCount(); edition++) {
			PlanDataEdition *pEdition = &editionList->GetAt(edition);
			for (int section = 0; section < pEdition->sectionList->GetCount(); section++) {
				PlanDataSection *pSection = &pEdition->sectionList->GetAt(section);
				for (int page = 0; page < pSection->pageList->GetCount(); page++) {
					PlanDataPage *pPage = &pSection->pageList->GetAt(page);
					if (pPage->pageID == sPageID)
						return pPage;
				}
			}
		}

		return NULL;

	}


	PlanDataPage *FindPageByPageIndex(CString sEditionName, CString sSectionName, int nPageIndex)
	{
		PlanDataEdition *pEdition = GetEditionObject(sEditionName);
		if (pEdition == NULL)
			return NULL;

		PlanDataSection *pPlanDataSection = pEdition->GetSectionObject(sSectionName);
		if (pPlanDataSection == NULL)
			return NULL;

		for (int page = 0; page<pPlanDataSection->pageList->GetCount(); page++) {
			PlanDataPage *pPage = &pPlanDataSection->pageList->GetAt(page);
			if (pPage->pageIndex == nPageIndex)
				return pPage;
		}

		return NULL;
	}

	PlanDataPage *FindPageByRunningPagination(CString sEditionName, int nPagination)
	{
		PlanDataEdition *pEdition = GetEditionObject(sEditionName);
		if (pEdition == NULL)
			return NULL;

		for (int section = 0; section<pEdition->sectionList->GetCount(); section++) {
			PlanDataSection *pSection = &pEdition->sectionList->GetAt(section);
			for (int page = 0; page<pSection->pageList->GetCount(); page++) {
				PlanDataPage *pPage = &pSection->pageList->GetAt(page);
				if (pPage->pagination == nPagination) {
					return pPage;
				}
			}
		}

		return NULL;
	}

	PlanDataPage *FindPageByMiscString1(CString sEditionName, CString sMiscString1)
	{
		PlanDataEdition *pEdition = GetEditionObject(sEditionName);
		if (pEdition == NULL)
			return NULL;

		for (int section = 0; section<pEdition->sectionList->GetCount(); section++) {
			PlanDataSection *pSection = &pEdition->sectionList->GetAt(section);
			for (int page = 0; page<pSection->pageList->GetCount(); page++) {
				PlanDataPage *pPage = &pSection->pageList->GetAt(page);
				if (pPage->miscstring1 == sMiscString1) {
					return pPage;
				}
			}
		}

		return NULL;
	}

	void FindPagesByMiscString2(CString sEditionName, CString sMiscString2, PlanDataPage **pPage, PlanDataPage **pPage2, CString &sSection)
	{
		*pPage = NULL;
		*pPage2 = NULL;
		sSection = _T("");
		PlanDataEdition *pEdition = GetEditionObject(sEditionName);
		if (pEdition == NULL)
			return;

		for (int section = 0; section<pEdition->sectionList->GetCount(); section++) {
			PlanDataSection *pSection = &pEdition->sectionList->GetAt(section);
			sSection = pSection->sectionName;
			for (int page = 0; page<pSection->pageList->GetCount(); page++) {
				PlanDataPage *pPageTest = &pSection->pageList->GetAt(page);
				if (pPageTest->miscstring2 == sMiscString2) {
					if (*pPage == NULL)
						*pPage = pPageTest;
					else
						*pPage2 = pPageTest;
				}
				if (*pPage != NULL && *pPage2 != NULL)
					break;
			}
			if (*pPage != NULL && *pPage2 != NULL)
				break;
		}
	}



	void SortPlanPages()
	{
		for (int eds = 0; eds<editionList->GetCount(); eds++) {
			PlanDataEdition *planDataEdition = &editionList->GetAt(eds);
			for (int sections = 0; sections<planDataEdition->sectionList->GetCount(); sections++) {
				PlanDataSection *planDataSection = &planDataEdition->sectionList->GetAt(sections);

				for (int page = 0; page<planDataSection->pageList->GetCount() - 1; page++) {
					PlanDataPage *planDataPage = &planDataSection->pageList->GetAt(page);

					for (int page2 = page + 1; page2<planDataSection->pageList->GetCount(); page2++) {
						PlanDataPage *planDataPage2 = &planDataSection->pageList->GetAt(page2);
						if (planDataPage->pagination > planDataPage2->pagination) {
							// Swap these pages
							PlanDataPage *pTemp = new PlanDataPage();
							planDataPage->ClonePage(pTemp);
							planDataPage2->ClonePage(planDataPage);
							pTemp->ClonePage(planDataPage2);
							delete pTemp;
						}
					}
				}
			}
		}
	}

	void DisposePlanData()
	{
		for (int edition = 0; edition < editionList->GetCount(); edition++) {
			PlanDataEdition *editem = &editionList->GetAt(edition);

			//	for (int i = 0; i<editem->pressList->GetCount(); i++)
			//		delete &editem->pressList[i];
			editem->pressList->RemoveAll();
			editem->pressList->FreeExtra();
			delete editem->pressList;

			for (int section = 0; section < editem->sectionList->GetCount(); section++) {
				PlanDataSection *secitem = &editem->sectionList->GetAt(section);
				for (int page = 0; page < secitem->pageList->GetCount(); page++) {
					PlanDataPage *pageitem = &secitem->pageList->GetAt(page);
					pageitem->colorList->RemoveAll();
					pageitem->colorList->FreeExtra();
					delete pageitem->colorList;
					//if (g_newdelete)
					//  delete pageitem;
				}
				secitem->pageList->RemoveAll();
				secitem->pageList->FreeExtra();
				delete secitem->pageList;
				//if (g_newdelete)
				// delete secitem;
			}
			editem->sectionList->RemoveAll();
			editem->sectionList->FreeExtra();
			delete editem->sectionList;

			for (int sheet = 0; sheet < editem->sheetList->GetCount(); sheet++) {
				PlanDataSheet *sheetItem = &editem->sheetList->GetAt(sheet);

				sheetItem->frontSheet->pressCylinders->RemoveAll();
				sheetItem->frontSheet->pressCylinders->FreeExtra();
				delete sheetItem->frontSheet->pressCylinders;

				sheetItem->frontSheet->sheetItems->RemoveAll();
				sheetItem->frontSheet->sheetItems->FreeExtra();
				delete sheetItem->frontSheet->sheetItems;

				sheetItem->backSheet->sheetItems->RemoveAll();
				sheetItem->backSheet->sheetItems->FreeExtra();
				delete sheetItem->backSheet->sheetItems;

				sheetItem->backSheet->pressCylinders->RemoveAll();
				sheetItem->backSheet->pressCylinders->FreeExtra();
				delete sheetItem->backSheet->pressCylinders;

				delete sheetItem->frontSheet;
				delete sheetItem->backSheet;

				//if (g_newdelete)
				//	delete sheetItem;

			}

			editem->sheetList->RemoveAll();
			editem->sheetList->FreeExtra();
			delete editem->sheetList;
			//if (g_newdelete)
			//  delete editem;
		}
		editionList->RemoveAll();
		editionList->FreeExtra();
		delete editionList;

		planName = "";
		planID = "";
		publicationName = "";
		publicationAlias = "";
		publicationDate = CTime(1975, 1, 1, 0, 0, 0);
		version = 1;
		state = PLANSTATE_LOCKED;

		arrSectionNames.RemoveAll();
		arrSectionNames.FreeExtra();
		arrSectionFormats.RemoveAll();
		arrSectionFormats.FreeExtra();
		arrSectionColors.RemoveAll();
		arrSectionColors.FreeExtra();

	}

	void LogPlanData(CString sFileName)
	{

		/*		CUtils util;

		util.Logprintf("\r\nParsing result of file %s", sFileName);
		util.Logprintf("----------------------------------");

		util.Logprintf("PlanName         : %s",planName);
		util.Logprintf("Version          : %d",version);
		util.Logprintf("Publication      : %s (alias %d)",publicationName, publicationAlias);
		util.Logprintf("PubDate          : %.4d-%.2d-%.2d (YYYY-MM-DD)",publicationDate.GetYear(), publicationDate.GetMonth(), publicationDate.GetDay());
		util.Logprintf("Editions in plan : %d", editionList->GetCount());
		for (int edition=0; edition < editionList->GetCount(); edition++) {
		PlanDataEdition editem = editionList->GetAt(edition);
		util.Logprintf("  Edition        : %s",editem.editionName);

		CString s = "";
		for (int press=0; press<editem.pressList->GetCount(); press++) {
		PlanDataPress pressitem = editem.pressList->GetAt(press);
		if (s != "")
		s += ", ";
		s += pressitem.pressName;
		}
		util.Logprintf("  Presses to use : %s", s);

		util.Logprintf("  Sections in edition : %d",editem.sectionList->GetCount());
		for (int section=0; section < editem.sectionList->GetCount(); section++) {
		PlanDataSection secitem = editem.sectionList->GetAt(section);
		util.Logprintf("    Section      : %s",secitem.sectionName);
		util.Logprintf("    Pages in section: %d", secitem.pageList->GetCount());
		for (int page=0; page < secitem.pageList->GetCount(); page++) {
		PlanDataPage pageitem = secitem.pageList->GetAt(page);
		CString sColors = "";
		for (int color=0; color < pageitem.colorList->GetCount(); color++) {
		PlanDataPageSeparation coloritem = pageitem.colorList->GetAt(color);
		sColors += coloritem.colorName + "  ";
		}
		util.Logprintf("      Pagename : %s  Pagination %d PageIndex %d Colors (%s) PageID %s MasterPageID %s",	pageitem.pageName, pageitem.pagination, pageitem.pageIndex, sColors, pageitem.pageID, pageitem.masterPageID);				}
		}

		} */
	}
};

class LayoutTemplate
{
public:

	CString templateName;
	int pagesAcross;
	int pagesDown;
	int pageRotationList[64];
	BOOL isBroadsheet;

	CString format;


public:
	LayoutTemplate()
	{
		templateName = "";
		pagesAcross = 1;
		pagesDown = 1;
		isBroadsheet = TRUE;

		SetFormat();
	}

	LayoutTemplate(CString sName)
	{
		pagesAcross = 1;
		templateName = sName;
		pagesDown = 1;
		isBroadsheet = TRUE;
		SetFormat();
	}

	void SetFormat()
	{
		if (isBroadsheet)
		{
			if (pagesAcross * pagesDown == 1)
				format = "B";
			else if (pagesAcross * pagesDown == 2)
				format = "T";
			else
				format = "M";
		}
		else
		{
			if (pagesAcross * pagesDown <= 2)
				format = "B";
			else if (pagesAcross * pagesDown == 4)
				format = "T";
			else
				format = "M";
		}

	}
};