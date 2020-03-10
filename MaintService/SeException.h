#pragma once
#include "stdafx.h"

class CSeException : public CException
{
	DECLARE_DYNAMIC(CSeException)
public:
	CSeException(UINT nSeCode, _EXCEPTION_POINTERS* pExcPointers);
	CSeException(CSeException & CseExc);

	UINT GetSeCode(void);
	_EXCEPTION_POINTERS* GetSePointers(void);
	PVOID GetExceptionAddress(void);

	void Delete(void);
	void ReportError(TCHAR *szSourceOfException);

	BOOL GetErrorMessage(CString & CsErrDescr, PUINT pnHelpContext = NULL);
	BOOL GetErrorMessage(LPTSTR lpszError, UINT nMaxError, PUINT pnHelpContext = NULL);
private:
	UINT m_nSeCode;
	_EXCEPTION_POINTERS* m_pExcPointers;
};

void SeTranslator(UINT nSeCode, _EXCEPTION_POINTERS* pExcPointers);
