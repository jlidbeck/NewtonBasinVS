// NewtonBasinView.h : interface of the CNewtonBasinView class
//
#include "complex.h"
#include "glColor.h"
#include "Mapping.h"
#include "Canvas.h"
#include "JsonSerializable.h"	// for ptrtype
#include "PlottablesManager.h"

class Plottable;

#pragma once

class CNewtonBasinView : public CView
{
	// mouse state
	CPoint m_ptClick, m_ptLast;
	bool m_bLButton, m_bMButton;
	bool m_bMouseMoved;

	// mapping
	Mapping m_viewMapping;
	Mapping m_canvasMapping;

	int m_nOversampling;	// ratio of pixels rendered to window pixels

	// memory canvas
	Canvas m_canvas;

	// plottable formulas
	//int m_nSourceFn;			// index to the global source function bank
	ptrtype m_pFn;		// owned

	// UI--status
	CString m_szStatus;
		
protected: // create from serialization only
	CNewtonBasinView();
	DECLARE_DYNCREATE(CNewtonBasinView)

// Attributes
public:
	CNewtonBasinDoc* GetDocument() const;
	friend CNewtonBasinDoc;

// Operations
public:
	void RedrawGraph(void);
	void UpdateStatusBar(void);
	void SetStatusText(int n, LPCTSTR sz);

protected:
	void InitCanvas(int cx, int cy);
	void SizeCanvasToWindow();
	void DetachFn(void);		// delete and dispose of m_pFn

	void SetModified(bool b);
public:
	ptrtype GetCurrentFn(void) const;
	void SetCurrentFn(ptrtype pfn);

// Overrides
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// Implementation
public:
	virtual ~CNewtonBasinView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	void OnSettingsDialogOpen();
	//void OnSettingsDialogApply();

	void zoom(double factor);

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnFunctionNext();
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
protected:
	virtual BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
public:
	afx_msg void OnTimer(UINT nIDEvent);
protected:
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
public:
	afx_msg void OnRenderStop();
	afx_msg void OnFunctionProperties();
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRenderRestart();
	afx_msg void OnFunctionSketch();
	afx_msg void OnFunctionPrevious();
	afx_msg void OnRenderAntialias();
	afx_msg void OnUpdateRenderAntialias(CCmdUI *pCmdUI);
	afx_msg void OnUpdateRenderGamma(CCmdUI *pCmdUI);
	afx_msg void OnRenderGamma();
	afx_msg void OnDestroy();
	afx_msg void OnUpdateRenderPause(CCmdUI *pCmdUI);
	afx_msg void OnRenderPause();
	afx_msg void OnLockCanvasSize();
	afx_msg void OnUpdateLockCanvasSize(CCmdUI *pCmdUI);
	afx_msg void OnEditCopy();
};

#ifndef _DEBUG  // debug version in NewtonBasinView.cpp
inline CNewtonBasinDoc* CNewtonBasinView::GetDocument() const
   { return reinterpret_cast<CNewtonBasinDoc*>(m_pDocument); }
#endif

