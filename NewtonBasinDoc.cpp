// NewtonBasinDoc.cpp : implementation of the CNewtonBasinDoc class
//

#include "stdafx.h"
#include "NewtonBasin.h"

#include "NewtonBasinDoc.h"
#include "NewtonBasinView.h"
#include "Formulas.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CNewtonBasinDoc

IMPLEMENT_DYNCREATE(CNewtonBasinDoc, CDocument)

BEGIN_MESSAGE_MAP(CNewtonBasinDoc, CDocument)
END_MESSAGE_MAP()


// CNewtonBasinDoc construction/destruction

CNewtonBasinDoc::CNewtonBasinDoc()
{
	// TODO: add one-time construction code here

}

CNewtonBasinDoc::~CNewtonBasinDoc()
{
}

CNewtonBasinView* CNewtonBasinDoc::GetView(void)
{
	POSITION pos = GetFirstViewPosition();
	ASSERT(pos);
	CView *pView = GetNextView(pos);
	ASSERT_KINDOF(CNewtonBasinView, pView);
	return (CNewtonBasinView*)(pView);
}

BOOL CNewtonBasinDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here

	// (SDI documents will reuse this document)

	return TRUE;
}




// CNewtonBasinDoc serialization

void CNewtonBasinDoc::Serialize(CArchive& ar)
{
	CNewtonBasinView *pView = GetView();
	ASSERT(pView);
	if(!pView)
		return;

	if (ar.IsStoring())
	{
/*		ar << CString("Mapping");
		pView->m_canvasMapping.Serialize(ar);

		ptrtype pfn = pView->m_pFn;
		ASSERT(pfn);
		ar << CString(pfn->getClassName());
		pfn->Serialize(ar);
	}
	else
	{
		try
		{
			Plottable *pfn;
			CString sz;
			bool bGotFn = false;

			while(true)
			{
				ar >> sz;
				if(!sz.Compare("Mapping"))
				{
					pView->m_canvasMapping.Serialize(ar);
				}
				else
				{
					//pfn = Plottable::Create(sz);
					pfn = PlottablesManager::GetInstance().createPlottable();

					if(!pfn)
						AfxThrowArchiveException(CArchiveException::none);

					pfn->Serialize(ar);
					bGotFn = true;
					ASSERT(!pfn->IsModified());
					pfn->SetModified(false);
					pView->SetCurrentFn(pfn);
				}
			}	// while

			if(!bGotFn)
				AfxThrowArchiveException(CArchiveException::none);
		}
		catch(CArchiveException *e)
		{
			if(e->m_cause == CArchiveException::endOfFile)
			{
				// success!
				TRACE("Successfully read archive.\n");
				if(e->m_bAutoDelete)
					e->Delete();
			}
			else
			{
				char sz[501];
				e->GetErrorMessage(sz, 500);
				TRACE(sz);
				throw(e);
			}
		}*/
	}
}


// CNewtonBasinDoc diagnostics

#ifdef _DEBUG
void CNewtonBasinDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CNewtonBasinDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CNewtonBasinDoc commands
