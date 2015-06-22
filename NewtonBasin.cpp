// NewtonBasin.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "NewtonBasin.h"
#include "MainFrm.h"

#include "ChildFrm.h"
#include "NewtonBasinDoc.h"
#include "NewtonBasinView.h"
#include "newtonbasin.h"
#include "Plottable.h"
#include "PlottablesManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CNewtonBasinApp

BEGIN_MESSAGE_MAP(CNewtonBasinApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
//	ON_UPDATE_COMMAND_UI(ID_RENDER_GAMMA, OnUpdateRenderGamma)
//	ON_COMMAND(ID_RENDER_GAMMA, OnRenderGamma)
END_MESSAGE_MAP()


// CNewtonBasinApp construction

CNewtonBasinApp::CNewtonBasinApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CNewtonBasinApp object

CNewtonBasinApp theApp;

// CNewtonBasinApp initialization

BOOL CNewtonBasinApp::InitInstance()
{
	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	InitCommonControls();

	CWinApp::InitInstance();

	m_nCmdShow = SW_MAXIMIZE;	// added by JSL

	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}
	AfxEnableControlContainer();
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));
	LoadStdProfileSettings(4);  // Load standard INI file options (including MRU)
	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(IDR_NewtonBasinTYPE,
		RUNTIME_CLASS(CNewtonBasinDoc),
		RUNTIME_CLASS(CChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CNewtonBasinView));
	if (!pDocTemplate)
		return FALSE;
	// jon...
	{
		CString sz;
		pDocTemplate->GetDocString(sz, CDocTemplate::filterName);
		pDocTemplate->GetDocString(sz, CDocTemplate::filterExt);
		// this one doesn't seem to work from OnFileNew
		pDocTemplate->GetDocString(sz, CDocTemplate::fileNewName);
	}
	// ...end

	AddDocTemplate(pDocTemplate);
	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame || !pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	m_pMainWnd = pMainFrame;

	// init UI elements before views are created
	m_settingsDlg.Create(CNewtonSettingsDlg::IDD, pMainFrame);

	// call DragAcceptFiles only if there's a suffix
	//  In an MDI app, this should occur immediately after setting m_pMainWnd
	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;
	// The main window has been initialized, so show and update it
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();


	return TRUE;
}



// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

// App command to run the dialog
void CNewtonBasinApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}


// CNewtonBasinApp message handlers


PlottablesManager* CNewtonBasinApp::getFunctionFactory()
{
	return &PlottablesManager::getInstance();
}


void CNewtonBasinApp::updateFunctionPropertiesDialog()
{
	POSITION pos = GetFirstDocTemplatePosition();
	while(pos) {
		CDocTemplate *pTemplate = (CDocTemplate*)GetNextDocTemplate(pos);
		POSITION pos2 = pTemplate->GetFirstDocPosition();
		while(pos2) {
			CNewtonBasinDoc *pDocument = (CNewtonBasinDoc*)pTemplate->GetNextDoc(pos2);
			if(pDocument && pDocument->GetView()) {
				ptrtype p = pDocument->GetView()->GetCurrentFn();
				if(!p)
					return;

				m_settingsDlg.setFunction(p, getFunctionFactory()->isReadOnly(p));
				m_settingsDlg.ShowWindow(SW_SHOW);
				pDocument->GetView()->OnSettingsDialogOpen();
				return;
			}
		}
	}
}

void CNewtonBasinApp::applyNewFunctionSettings(bool copyToNew)
{
	ptrtype pfn = m_settingsDlg.saveAsCopy();
	ptrtype sourceFn = m_settingsDlg.getSourceFunction();

	TRACE("Applying settings named '%s'\n", pfn->getInstanceName().c_str());
	
	PlottablesManager *pFactory = getFunctionFactory();

	int n = pFactory->getIndexByName(pfn->getInstanceName().c_str());
	if(copyToNew || pFactory->isReadOnly(sourceFn)) {
		pFactory->add(pfn, pFactory->getNextUnusedName(pfn->getInstanceName()));
		pFactory->setReadOnly(pfn, false);
	}
	else {
		pFactory->replace(sourceFn, pfn);
	}

	POSITION pos = GetFirstDocTemplatePosition();
	while(pos) {
		CDocTemplate *pTemplate = (CDocTemplate*)GetNextDocTemplate(pos);
		POSITION pos2 = pTemplate->GetFirstDocPosition();
		while(pos2) {
			CNewtonBasinDoc *pDocument = (CNewtonBasinDoc*)pTemplate->GetNextDoc(pos2);
			if(pDocument) {
				pDocument->GetView()->
					SetCurrentFn(pfn);
				m_settingsDlg.setFunction(pfn, false);
				return;
			}
		}
	}
}

