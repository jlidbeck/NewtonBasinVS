// NewtonBasinView.cpp : implementation of the CNewtonBasinView class
//

#include "stdafx.h"
#include "NewtonBasin.h"
#include <winuser.h>

#include "NewtonBasinDoc.h"
#include "NewtonBasinView.h"

#include "Formulas.h"
#include "SketchFormulas.h"
#include "PlottablesManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "MainFrm.h"
#include "ChildFrm.h"


CMainFrame* GetMainWnd(void)
{
	CWnd *p = ::AfxGetMainWnd();
//	ASSERT_KINDOF(RUNTIME_CLASS(CMainFrame), p);
	((CMainFrame*)p)->AssertValid();
	return (CMainFrame*)p;
}

CNewtonBasinApp* GetApp(void)
{
	CWinApp* p = ::AfxGetApp();
//	ASSERT_KINDOF(RUNTIME_CLASS(CNewtonBasinApp), p);
	((CNewtonBasinApp*)p)->AssertValid();
	return (CNewtonBasinApp*)(p);
}

// CNewtonBasinView

IMPLEMENT_DYNCREATE(CNewtonBasinView, CView)

BEGIN_MESSAGE_MAP(CNewtonBasinView, CView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_SIZE()
	ON_COMMAND(ID_FUNCTION_NEXT, OnFunctionNext)
	ON_COMMAND(ID_FUNCTION_PREVIOUS, OnFunctionPrevious)
	ON_WM_SIZING()
	ON_WM_KEYDOWN()
	ON_WM_CHAR()
	ON_WM_RBUTTONUP()
	ON_WM_ERASEBKGND()
	ON_WM_TIMER()
//	ON_WM_ACTIVATE()
//	ON_WM_CHILDACTIVATE()
ON_COMMAND(ID_RENDER_STOP, OnRenderStop)
ON_COMMAND(ID_FUNCTION_PROPERTIES, OnFunctionProperties)
ON_WM_MBUTTONDOWN()
ON_WM_MBUTTONUP()
ON_COMMAND(ID_RENDER_RESTART, OnRenderRestart)
ON_COMMAND(ID_FUNCTION_SKETCH, OnFunctionSketch)
ON_COMMAND(ID_RENDER_ANTIALIAS, OnRenderAntialias)
ON_UPDATE_COMMAND_UI(ID_RENDER_ANTIALIAS, OnUpdateRenderAntialias)
ON_UPDATE_COMMAND_UI(ID_RENDER_GAMMA, OnUpdateRenderGamma)
ON_COMMAND(ID_RENDER_GAMMA, OnRenderGamma)
ON_WM_DESTROY()
ON_UPDATE_COMMAND_UI(ID_RENDER_PAUSE, OnUpdateRenderPause)
ON_COMMAND(ID_RENDER_PAUSE, OnRenderPause)
ON_COMMAND(ID_LOCK_CANVAS_SIZE, &CNewtonBasinView::OnLockCanvasSize)
ON_UPDATE_COMMAND_UI(ID_LOCK_CANVAS_SIZE, &CNewtonBasinView::OnUpdateLockCanvasSize)
ON_COMMAND(ID_EDIT_COPY, &CNewtonBasinView::OnEditCopy)
END_MESSAGE_MAP()


// CNewtonBasinView construction/destruction

CNewtonBasinView::CNewtonBasinView()
{
	m_nOversampling = 1;

	m_bLButton = false;
	m_bMButton = false;

	m_canvasMapping.setModel(Complex(-2.0, 1.5), Complex(2.0, -1.5));
	m_viewMapping.setModel(Complex(-2.0, 1.5), Complex(2.0, -1.5));

	// init plots

	SetCurrentFn(theApp.getFunctionFactory()->getNextPlottable(NULL));

	if(dynamic_cast<Plottable*>(m_pFn.get())) {
		(dynamic_cast<Plottable*>(m_pFn.get()))->SetModified(false);
	}
}

CNewtonBasinView::~CNewtonBasinView()
{
}

BOOL CNewtonBasinView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	//cs.style |= WS_MAXIMIZE;	// this does nothing here

	return CView::PreCreateWindow(cs);
}

void CNewtonBasinView::OnDestroy()
{
	CView::OnDestroy();

	DetachFn();
}

// CNewtonBasinView drawing

BOOL CNewtonBasinView::OnEraseBkgnd(CDC* pDC)
{
//	return CView::OnEraseBkgnd(pDC);
	return TRUE;
}

void CNewtonBasinView::OnDraw(CDC* pDC)
{
	CNewtonBasinDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	CRect rc;
	GetClientRect(&rc);
	if(rc.Width() > 0 && rc.Height() > 0)
	{
		// and copy bits when it's able
		m_canvas.drawTo(pDC, rc);

		// now the mouse zoom rect, if any
		if(m_bLButton && m_bMouseMoved)
		{
			// draw
			rc.SetRect(m_ptClick, m_ptLast);
			rc.NormalizeRect();
			pDC->FrameRect(&rc, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));
		}

		// provide running update
		if(m_canvas.isRunning() || m_canvas.isCancelled())
		{
			CString sz = (m_canvas.isCancelled() ? L"Cancelled." :
				(m_canvas.isStopping() ? L"Stopping..." : 
				(m_canvas.isPaused() ? L"Paused." : L"Running... "+m_canvas.getSpeedString())));
			pDC->SetBkMode(TRANSPARENT);
			pDC->SetTextColor(0x000000);
			pDC->TextOut(11, 11, sz);
			pDC->SetTextColor(0xFFFFFF);
			pDC->TextOut(10, 10, sz);
		}
	}

}


// CNewtonBasinView printing

BOOL CNewtonBasinView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CNewtonBasinView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CNewtonBasinView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}


// CNewtonBasinView diagnostics

#ifdef _DEBUG
void CNewtonBasinView::AssertValid() const
{
	CView::AssertValid();
}

void CNewtonBasinView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CNewtonBasinDoc* CNewtonBasinView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNewtonBasinDoc)));
	return (CNewtonBasinDoc*)m_pDocument;
}
#endif //_DEBUG


// CNewtonBasinView message handlers

void CNewtonBasinView::OnMouseMove(UINT nFlags, CPoint point)
{
	if(m_bMButton)
	{
		// apply mouse drag to mappings
		CPoint d = (point - m_ptLast);
		m_viewMapping.offset(d);
		m_canvasMapping.matchModel(m_viewMapping);
		
		SetModified(true);

		m_ptLast = point;
		m_bMouseMoved = true;

		RedrawGraph();
	}
	else if(m_bLButton)
	{
		CDC *pDC = GetDC();
		CRect rc;

		// un-draw
		if(m_bMouseMoved)
		{
			rc.SetRect(m_ptClick, m_ptLast);
			rc.NormalizeRect();
//			pDC->DrawFocusRect(&rc);
			InvalidateRect(&rc, FALSE);
		}
		// draw
		rc.SetRect(m_ptClick, point);
		rc.NormalizeRect();
//		pDC->DrawFocusRect(&rc);
		pDC->FrameRect(&rc, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));
		m_ptLast = point;

		m_bMouseMoved = true;
	}

	CView::OnMouseMove(nFlags, point);
}

void CNewtonBasinView::OnLButtonDown(UINT nFlags, CPoint point)
{
	m_bLButton = true;
	m_bMouseMoved = false;
	m_ptClick = point;
	m_ptLast = point;

	SetCapture();

	CView::OnLButtonDown(nFlags, point);
}

void CNewtonBasinView::OnLButtonUp(UINT nFlags, CPoint point)
{
	if(m_bLButton && m_bMouseMoved)
	{
		// un-draw (?) zoomrect
		CDC *pDC = GetDC();
		CRect rc(m_ptClick, m_ptLast);
		rc.NormalizeRect();
		//pDC->DrawFocusRect(&rc);
		pDC->FrameRect(&rc, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));

		m_viewMapping.zoomToRect(rc);
		m_canvasMapping.matchModel(m_viewMapping);
		SetModified(true);

		RedrawGraph();
	}
	m_bLButton = false;

	ReleaseCapture();

	CView::OnLButtonUp(nFlags, point);
}

void CNewtonBasinView::OnRButtonUp(UINT nFlags, CPoint point)
{
	zoom(2.0);

	CView::OnRButtonUp(nFlags, point);
}

void CNewtonBasinView::zoom(double factor)
{
	// zoom out
	m_viewMapping.zoomOut(factor);
	m_canvasMapping.matchModel(m_viewMapping);

	SetModified(true);

	RedrawGraph();
}


void CNewtonBasinView::OnMButtonDown(UINT nFlags, CPoint point)
{
	m_bMButton = true;
	m_bMouseMoved = false;
	m_ptClick = point;
	m_ptLast = point;
	SetCapture();
	m_canvas.pause(true);

	CView::OnMButtonDown(nFlags, point);
}

void CNewtonBasinView::OnMButtonUp(UINT nFlags, CPoint point)
{
	m_bMButton = false;
	ReleaseCapture();
	m_canvas.pause(false);

	CView::OnMButtonUp(nFlags, point);
}


void CNewtonBasinView::OnSizing(UINT fwSide, LPRECT pRect)
{
	CView::OnSizing(fwSide, pRect);

	TRACE("OnSizing(%u...)\n", fwSide);
}

void CNewtonBasinView::OnSize(UINT nType, int cx, int cy)
{
	// OnSize is sometimes called even when the size hasn't actually changed,
	// for example, when the view is activated full-screen
	CView::OnSize(nType, cx, cy);

	if(cx>0 && cy>0)
	{
		m_viewMapping.resize(cx, cy);
		SizeCanvasToWindow();
	}
}

void CNewtonBasinView::SizeCanvasToWindow()
{
	CRect rc;
	CView::GetClientRect(&rc);
	int canvasWidth, canvasHeight;
	if(m_canvas.isCanvasSizeLocked()) {
		// this is verbose... trim this up
		canvasWidth = m_canvas.getWidth();
		canvasHeight = m_canvas.getHeight();
	}
	else {
		canvasWidth = m_nOversampling * rc.Width();
		canvasHeight = m_nOversampling * rc.Height();
	}
	InitCanvas(canvasWidth, canvasHeight);
}

void CNewtonBasinView::InitCanvas(int cx, int cy)
{
	m_canvasMapping.resize(cx, cy);

	// this stuff needs to be done before first call to RedrawGraph
	if(!m_canvas.isInitialized() || m_canvas.getWidth()!=cx || m_canvas.getHeight()!=cy)
	{
		// first time drawn at this size?
		m_canvas.init(GetDC(), cx, cy);
		//m_canvas.enableAntialiasing(true);
		m_canvas.enableGamma(true);
		m_canvas.setGamma(2.2f);

		RedrawGraph();
	}

}

void CNewtonBasinView::OnFunctionPrevious()
{
	SetCurrentFn(theApp.getFunctionFactory()->getPreviousPlottable(m_pFn));
	//SetModified(false);
}

void CNewtonBasinView::OnFunctionNext()
{
	SetCurrentFn(theApp.getFunctionFactory()->getNextPlottable(m_pFn));
	//SetModified(false);
}

void CNewtonBasinView::OnFunctionSketch()
{
	//this->SetCurrentFn(GetSourceFn()->clone());
	//SetModified(false);
}

void CNewtonBasinView::SetModified(bool b)
{
	GetDocument()->SetModifiedFlag(b);
	(dynamic_cast<Plottable*>(m_pFn.get()))->SetModified(b);
}

void CNewtonBasinView::DetachFn(void)
{
	m_canvas.stop();
	while(m_canvas.isRunning())
	{
		ASSERT(m_canvas.isStopping());
		::Sleep(100);
	}
	ASSERT(!m_canvas.isRunning());

	if(m_pFn)
	{
		// what do we do with the current fn?
		// it might be a stock fn, or loaded/unmodified, or
		// saved/unmodified, in which case we don't
		// need to save it.
		// Otherwise we'll save it in the history.
		// We'll use the modified flag to determine.

		if((dynamic_cast<Plottable*>(m_pFn.get()))->IsModified())
		{
			// function was modified.
			//ptrtype copy = m_pPlottablesManager->clone(m_pFn);
			//g_pmHistory.Add(copy);
		}

		m_pFn = NULL;
	}
}

void CNewtonBasinView::RedrawGraph(void)
{
	// update status
	UpdateStatusBar();

	ASSERT(m_pFn);
	ASSERT(dynamic_cast<Plottable*>(m_pFn.get()));

	// fire up the ol' worker thread
	m_canvas.SetNotifyWnd(this);
	m_canvas.run(m_pFn, m_canvasMapping);

	Invalidate();

}

void CNewtonBasinView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default
	switch(nChar) {
	case VK_OEM_MINUS:
		// zoom out
		zoom(2.0);
		break;

	case VK_OEM_PLUS:
		// zoom in
		zoom(0.5);
		break;

	case VK_SHIFT:
	case VK_CONTROL:
		break;

	default:
		TRACE("  key down: char: 0x%0x flags: %d\n", nChar, nFlags);
	}

	if(nChar==VK_BROWSER_BACK)
		TRACE("back!");
	if(nChar==VK_MBUTTON)
		TRACE("MBUTTON1!");
	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CNewtonBasinView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default
	if(nChar==VK_BROWSER_BACK)
		TRACE("back!");
	if(nChar==VK_MBUTTON)
		TRACE("MBUTTON1!");

	CView::OnChar(nChar, nRepCnt, nFlags);
}

BOOL CNewtonBasinView::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	if(message == MSG_CANVAS_NOTIFICATION)
	{
		CString sz;

		// message is from the worker thread
		switch(wParam)
		{
		case MSG_RENDER_BEGIN:
			// thread is just starting work: begin a timer to periodically update
			// as the worker progresses
			SetTimer(42, 100, NULL);
			sz.Format(L"Rendering %d x %d...", m_canvas.getWidth(), m_canvas.getHeight());
			SetStatusText(0, sz);
			break;

		case MSG_RENDER_PROGRESS:
			sz.Format(L"%d%% complete...", lParam);
			SetStatusText(0, sz);
			break;

		case MSG_RENDER_COMPLETE:
			sz.Format(L"Rendered %d x %d Time: %lf", m_canvas.getWidth(), m_canvas.getHeight(), m_canvas.getRunTime());
			SetStatusText(0, sz);
			KillTimer(42);
			Invalidate();
			break;

		case MSG_RENDER_CANCELLED:
			SetStatusText(0, L"Cancelled.");
			KillTimer(42);
			Invalidate();
		}

		return TRUE;
	}

	static WPARAM xb = 0;
	if(message == WM_XBUTTONDOWN)
	{
		xb = (wParam & (MK_XBUTTON1 | MK_XBUTTON2));
	}
	else if(message == WM_XBUTTONUP)
	{
		if(xb == MK_XBUTTON1)
			MessageBox(L"Back.");
		else if(xb == MK_XBUTTON2)
			MessageBox(L"Forward.");
		xb = (wParam & (MK_XBUTTON1 | MK_XBUTTON2));
		return TRUE;
	}

#ifdef _DEBUG
	// trying to isolate mouse messages from side buttons.
	static WPARAM n = 0;
	if(message == WM_XBUTTONDOWN || message == WM_XBUTTONUP || message == WM_XBUTTONDBLCLK)
		TRACE("...X-button %04x\n", message);
	else if(n && message!=WM_ERASEBKGND && message!=WM_PAINT
		&& message!=WM_TIMER && message!=0x0363)
		TRACE("...message %04x\n", message);
	if(message == WM_RBUTTONDOWN)
		n = 1;
	if(message == WM_RBUTTONUP)
		n = 0;
#endif

	return CView::OnWndMsg(message, wParam, lParam, pResult);
}

void CNewtonBasinView::SetStatusText(int n, LPCTSTR sz)
{
	m_szStatus = sz;

	CChildFrame *p = (CChildFrame*)GetMainWnd()->GetActiveFrame();
	if(p)
	{
		ASSERT_VALID(p);
		if(p && p->GetActiveView() == this)
			GetMainWnd()->SetStatusText(n, sz);
	}
}

void CNewtonBasinView::OnTimer(UINT nIDEvent)
{
	Invalidate();

	CView::OnTimer(nIDEvent);
}


void CNewtonBasinView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
	UpdateStatusBar();
	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

void CNewtonBasinView::UpdateStatusBar(void)
{
	ASSERT(m_pFn);
	if(!m_pFn)
		return;
	GetMainWnd()->SetStatusText(0, m_szStatus);
	CString sz = S2WS(m_pFn->getClassName());
	GetMainWnd()->SetStatusText(1, sz);
}

ptrtype CNewtonBasinView::GetCurrentFn(void) const
{
	return m_pFn;
}

void CNewtonBasinView::SetCurrentFn(ptrtype pfn)
{
	ASSERT(dynamic_cast<Plottable*>(pfn.get()));
	_ASSERT_VALID(pfn);

	if(this->GetSafeHwnd()) {
		m_canvas.stop();
		DetachFn();
		ASSERT(!m_pFn);
		m_pFn = pfn;
		RedrawGraph();
		theApp.updateFunctionPropertiesDialog();
	}
	else {
		// view is initializing; just set the member
		m_pFn = pfn;
	}
}

void CNewtonBasinView::OnRenderStop()
{
	m_canvas.stop();
}

void CNewtonBasinView::OnRenderRestart()
{
	RedrawGraph();
}

void CNewtonBasinView::OnFunctionProperties()
{
	ASSERT_VALID(this);
	theApp.updateFunctionPropertiesDialog();
	//OnSettingsDialogOpen();
}


void CNewtonBasinView::OnSettingsDialogOpen()
{
}

/*void CNewtonBasinView::OnSettingsDialogApply()
{
	SetCurrentFn(theApp.m_settingsDlg.applyChanges());
	SetModified(true);
}*/

void CNewtonBasinView::OnRenderAntialias()
{
	m_nOversampling = 5 - m_nOversampling;
	m_canvas.enableAntialiasing(m_nOversampling > 1);	// not doing much here

	InitCanvas(m_nOversampling * m_viewMapping.getWindowWidth(), m_nOversampling * m_viewMapping.getWindowHeight());
}

void CNewtonBasinView::OnUpdateRenderAntialias(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_canvas.isAntialiasingEnabled() ? 1 : 0);
}

void CNewtonBasinView::OnUpdateRenderGamma(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_canvas.isGammaEnabled() ? 1 : 0);
}

void CNewtonBasinView::OnRenderGamma()
{
	m_canvas.enableGamma(!m_canvas.isGammaEnabled());
}

void CNewtonBasinView::OnUpdateRenderPause(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_canvas.isPaused());
}

void CNewtonBasinView::OnRenderPause()
{
	m_canvas.pause(!m_canvas.isPaused());
}


void CNewtonBasinView::OnLockCanvasSize()
{
	m_canvas.lockCanvasSize(!m_canvas.isCanvasSizeLocked());
	if(!m_canvas.isCanvasSizeLocked()) {
		SizeCanvasToWindow();
	}
}


void CNewtonBasinView::OnUpdateLockCanvasSize(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_canvas.isCanvasSizeLocked() ? 1 : 0);
}


void CNewtonBasinView::OnEditCopy()
{
	::OpenClipboard(GetSafeHwnd());
	::EmptyClipboard();
	CBitmap junk;

	HANDLE hbmp = ::CopyImage(m_canvas.getBitmap().GetSafeHandle(), IMAGE_BITMAP, m_canvas.getWidth(), m_canvas.getHeight(), 0);
	junk.Attach(hbmp);

	::SetClipboardData(CF_BITMAP, junk.m_hObject);
	::CloseClipboard();
}
