// NewtonBasin.h : main header file for the NewtonBasin application
//
#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include "NewtonSettingsDlg2.h"

// CNewtonBasinApp:
// See NewtonBasin.cpp for the implementation of this class
//

class CNewtonBasinApp : public CWinApp
{
	PlottablesManager m_factory;

public:
	CNewtonSettingsDlg m_settingsDlg;

public:
	CNewtonBasinApp();


// Overrides
public:
	virtual BOOL InitInstance();

// Implementation
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
//	virtual CDocument* OpenDocumentFile(LPCTSTR lpszFileName);
	afx_msg void OnFileOpen();

public:
	PlottablesManager* getFunctionFactory();
	void applyNewFunctionSettings(bool copyToNew);
	void updateFunctionPropertiesDialog();
};

extern CNewtonBasinApp theApp;