#include "stdafx.h"
#include "Canvas.h"

#include "Formulas.h"
#include "SketchFormulas.h"

//#include <boost/gil/image.hpp>

Canvas::~Canvas(void)
{
}

void Canvas::init(CDC *pDC, int cx, int cy)
{
	stop();
	ASSERT(!this->m_pThread);

	TRACE("Creating DC. Is worker thread? %s\n", ::AfxGetThread()==m_pThread?"true":"false");

	// create a memory DC like the one provided
	m_dc.DeleteDC();
	VERIFY(m_dc.CreateCompatibleDC(pDC));

	// this bitmap is a temporary; it is immediately replaced
	// in resize().
	CBitmap bmp;
	VERIFY(bmp.CreateBitmap(cx, cy, 1, 32, NULL));
	m_dc.SelectObject(&bmp);

	resize(cx, cy);

	setGamma(2.2f);
}

void Canvas::enableAntialiasing(bool aa) 
{
	// this doesn't do much of anything yet.
	// oversampling via canvas size is done by the view class
	m_antialiasing = aa;
}

void Canvas::resize(int cx, int cy)
{
	stop();
	ASSERT(!m_pThread);

	m_bmp.DeleteObject();

	// either of these works fine.
//	VERIFY(m_bmp.CreateCompatibleBitmap(&m_dc, cx, cy));
//	VERIFY(m_bmp.CreateBitmap(cx, cy, 1, 32, NULL));

	// testing... trying to get alpha channels in there somehow
		BITMAPINFO bmi;
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = cx;
		bmi.bmiHeader.biHeight = cy;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;         // four 8-bit components
		bmi.bmiHeader.biCompression = BI_RGB;
		bmi.bmiHeader.biSizeImage = cx * cy * 4;

		// create our DIB section and select the bitmap into the dc
		void *pvBits = NULL;
		HBITMAP hbitmap = CreateDIBSection(m_dc,
			&bmi, DIB_RGB_COLORS, &pvBits, NULL, 0x0);
		//VERIFY(bmp.CreateBitmapIndirect(&bitmap));
		VERIFY(m_bmp.Attach(hbitmap));
		ASSERT(pvBits);
		m_pBits = (RGBQUAD*)(pvBits);

		// experiment: this will obtain BITMAP, BITMAPINFOHEADER, and the bitmap data
		DIBSECTION sDibSection;
		int bytes = ::GetObjectA(hbitmap, sizeof(DIBSECTION), &sDibSection);

	m_dc.SelectObject(&m_bmp);
	m_nWidth = cx;
	m_nHeight = cy;

#ifdef _DEBUG
	// let's see if we can read the alpha channel...
	// weird... the fillsolidrect doesn't seem to work.
	m_dc.FillSolidRect(0, 0, cx, cy, 0x11223344);	// seems to do nothing
	m_dc.SetPixelV(0, cy-1, 0xaaBB6677);	// set top-left pixel; note that RGB but not A are stored
	TRACE("[0]: %08x [1]: %08x\n", m_pBits[0], m_pBits[1]);	// read RGBQUAD values
#endif
}

bool Canvas::isInitialized()
{
	return (m_dc.m_hDC != NULL);
}

CDC* Canvas::GetDC(void)
{
	// this needs to do some kind of blocking!
	if(!m_dc.m_hDC)
		return NULL;

	return &m_dc;
}

void Canvas::drawTo(CDC *pDC, const CRect &rc)
{
	if(m_dc.m_hDC)
	{
		m_dcMutex.Lock();
		//BOOL b = pDC->BitBlt(0, 0, rc.Width(), rc.Height(), &m_dc, 0, 0, SRCCOPY);
		pDC->SetStretchBltMode(STRETCH_DELETESCANS);
		pDC->StretchBlt(0, 0, rc.Width(), rc.Height(), &m_dc, 0, 0, m_nWidth, m_nHeight, SRCCOPY);
		m_dcMutex.Unlock();
	}
}

bool Canvas::isRunning()
{
	return (m_pThread != NULL && m_pThread->m_hThread && m_isRunning);
}

// this launches a new worker thread
void Canvas::run(ptrtype pfn, const Mapping &m)
{
#ifdef _DEBUG
	m_szHistory += (std::string("run(") + pfn->getClassName() + ")...");

	TRACE("New Run: %d x %d\n", m.getWindowWidth(), m.getWindowHeight());
#endif

	stop();
	ASSERT(!m_pThread);
	ASSERT(!m_isRunning);

	m_isRunning = true;
	SketchFormula *sf = dynamic_cast<SketchFormula*>(pfn.get());
	ComplexColorMap *bf = dynamic_cast<ComplexColorMap*>(pfn.get());

	if(sf) {
		run(sf, m);
	}
	else if(bf) {
		run(bf, m);
	}
	else {
		throw("Unrecognized function type. Only SketchFormula- and ComplexColorMap-derived functions are permitted.");
	}
}

void Canvas::run(ComplexColorMap *pfn, const Mapping &m)
{
	m_pfn = pfn;
	m_pfnSketch = NULL;
	m_screenMapping = m;

	m_pThread = ::AfxBeginThread(&fnCanvasPlot, this
		/* , THREAD_PRIORITY_ABOVE_NORMAL*/);
}

void Canvas::run(SketchFormula *pfn, const Mapping &m)
{
	m_pfn = NULL;
	m_pfnSketch = pfn;
	m_screenMapping = m;

	m_pThread = ::AfxBeginThread(&fnCanvasPlot, this);
}

// the thread function. note that it goes right back into the Canvas object
UINT fnCanvasPlot( LPVOID pParam )
{
	//ASSERT(dynamic_cast<Canvas*>(pParam));
	Canvas *pCanvas = (Canvas*)(pParam);
	ASSERT(pCanvas && pCanvas->GetDC());
	return pCanvas->threadRenderFunction();
}

// the thread function
UINT Canvas::threadRenderFunction(void)
{
	ASSERT(m_dc.m_hDC != NULL);
	ASSERT(m_bmp.m_hObject != NULL);
	ASSERT(m_pfn || m_pfnSketch);
	ASSERT(m_isRunning);

	m_stopFlag = false;
	m_plotStartTime = clock();

#ifdef _DEBUG
	{
		m_szHistory += "fnRun:start...";
	}
#endif

	m_dc.FillSolidRect(0, 0, m_nWidth, m_nHeight, 0x000000);

	// notify: starting
	if(m_pNotifyWnd->GetSafeHwnd()) {
		m_pNotifyWnd->PostMessage(MSG_CANVAS_NOTIFICATION, MSG_RENDER_BEGIN, 0);
	}

	// coarse preview
	int coarseStep = (m_nWidth + m_nHeight) / 200;

	if(m_pfn)
		render(0, 0, m_nWidth, m_nHeight, coarseStep);
	else
		sketch();

#ifdef _DEBUG
	{
		m_szHistory += "fnRun:stop...";
	}
#endif

	ASSERT(m_isRunning);
	m_isRunning = false;

	if(m_stopFlag)
	{
		// the operation was aborted--before we terminate naturally,
		// send a quick notification to the user-interface thread.
		if(m_pNotifyWnd) {
			m_pNotifyWnd->PostMessage(MSG_CANVAS_NOTIFICATION, MSG_RENDER_CANCELLED, 3);
		}
	}
	else
	{
		clock_t tEnd = clock();
		m_lastPlotTime = (double)(tEnd - m_plotStartTime) / CLOCKS_PER_SEC;

		// we've completed the task to our own satisfaction--
		// now notify the other thread/window
		if(m_pNotifyWnd) {
			m_pNotifyWnd->PostMessage(MSG_CANVAS_NOTIFICATION, MSG_RENDER_COMPLETE, 2);
		}
	}

	return 0;
}

void Canvas::sketch(void)
{
	int maxn = m_pfnSketch->GetGroupSize();
	int j, n;
	m_nPointsPlotted = 0;
	m_nPointsCalced = 0;
	double th(0);
	float fx, fy;
	Complex *pts = new Complex[maxn];
	GLcolor *cols = new GLcolor[maxn];

	for(int i=0; !m_stopFlag; ++i)
	{
		th += 0.0015;

		n = m_pfnSketch->GetGroup(th, pts, cols, maxn);

		for(j=0; j<n; ++j)
		{
			m_color = cols[j];
			if(m_antialiasing)
			{
				m_screenMapping.unmap(pts[j], fx, fy);
				drawPixelAA(fx, fy);
			}
			else
				drawPixelAdd(m_screenMapping.unmap(pts[j]));
		}

		m_nPointsCalced += maxn;
		m_nPointsPlotted += n;

		while(m_isPaused)
		{
			::Sleep(100);
		}
	}

	delete [] pts;
	delete [] cols;
}

void Canvas::drawPixelAdd(POINT pt)
{
	if(pt.x<0 || pt.y<0 || pt.x>=m_nWidth || pt.y>=m_nHeight)
		return;

#ifdef _USEDIB
	int offset = (pt.x + m_nWidth * (m_nHeight - 1 - pt.y));
	GLcolor c1(m_pBits[offset]);
	c1.BlendAddA(m_color);
	m_pBits[offset] = c1;
#else
	GLcolor c1 = m_dc.GetPixel(pt);
	c1.BlendAddA(m_color);	// linear blend
	m_dc.SetPixelV(pt, c1);
#endif
}

void Canvas::drawPixelAA(float x, float y)
{
		int ix = (int)(x), iy = (int)(y);
		float fx = x-ix, fy = y-iy;
		
		drawPixelA(ix, iy, (1-fx)*(1-fy));
		drawPixelA(ix+1, iy, fx*(1-fy));
		drawPixelA(ix, iy+1, (1-fx)*fy);
		drawPixelA(ix+1, iy+1, fx*fy);
}

// draw semitransparent pixel
// interpolate color value using some alpha function
void Canvas::drawPixelA(int x, int y, float alpha)
{
	if(x<0 || y<0 || x>=m_nWidth || y>=m_nHeight)
		return;

#ifdef _USEDIB
	int offset = x + m_nWidth*(m_nHeight-1-y);
	RGBQUAD rgb = m_pBits[offset];

	RGBQUAD rgb2 = m_bGamma ? 
		blendAddGamma(rgb, m_color, alpha) :
		blendAdd(rgb, m_color, alpha);
	
	m_pBits[offset] = rgb2;
#else
	COLORREF c0 = m_dc.GetPixel(x, y);
	COLORREF c = m_bGamma ? 
		blendAddGamma(c0, m_color, alpha) :
		blendAdd(c0, m_color, alpha);
	m_dc.SetPixelV(x, y, c);
#endif
}

	// adds components of 2 RRGGBB colors: c0 + t*c1
	// uses t*c1.alpha to scale c1 before it's added to c0
	// c0.alpha is ignored.
	// RGB channels are clipped to <=255.
#ifdef _USEDIB
RGBQUAD Canvas::blendAddGamma(
					RGBQUAD c0, const GLcolor & c1, float t) const
{
		if(t <= 0) return c0;
//		t*=((c1 >> 24)/255.0f);
		t *= c1.a;
		
		int r1 = (int)(255 * c1.r);
		int g1 = (int)(255 * c1.g);
		int b1 = (int)(255 * c1.b);
		ASSERT(r1<256 && g1<256 && b1<256);
		int r0 = c0.rgbRed;
		int g0 = c0.rgbGreen;
		int b0 = c0.rgbBlue;

		int r = (int)(t*m_vfGamma[r1] + m_vfGamma[r0]);
		if(r>255) r=255; else r=m_vnInvGamma[r];
		int g = (int)(t*m_vfGamma[g1] + m_vfGamma[g0]);
		if(g>255) g=255; else g=m_vnInvGamma[g];
		int b = (int)(t*m_vfGamma[b1] + m_vfGamma[b0]);
		if(b>255) b=255; else b=m_vnInvGamma[b];
		
		// alpha = 0
		RGBQUAD q = { b, g, r, 0 };
		return q;
}
#else
COLORREF Canvas::blendAddGamma(
					COLORREF c0, const GLcolor & c1, float t) const
{
		if(t <= 0) return c0;
//		t*=((c1 >> 24)/255.0f);
		t *= c1.a;
		
		int r1 = (int)(255 * c1.r);
//		if(r1 > 255) 			r1=255;
		int g1 = (int)(255 * c1.g);
//		if(g1 > 255) 			g1=255;
		int b1 = (int)(255 * c1.b);
//		if(b1 > 255) 			b1=255;
		ASSERT(r1<256 && g1<256 && b1<256);
		int r0 = (int)(c0 & 0xff);
		int g0 = (int)((c0>>8) & 0xff);
		int b0 = (int)((c0>>16) & 0xff);

		int r = (int)(t*m_vfGamma[r1] + m_vfGamma[r0]);
		if(r>255) r=255; else r=m_vnInvGamma[r];
		int g = (int)(t*m_vfGamma[g1] + m_vfGamma[g0]);
		if(g>255) g=255; else g=m_vnInvGamma[g];
		int b = (int)(t*m_vfGamma[b1] + m_vfGamma[b0]);
		if(b>255) b=255; else b=m_vnInvGamma[b];
		
//		return (0xff000000 | (r<<16) | (g<<8) | b);
		return ((b<<16) | (g<<8) | r);	// alpha=0
}
#endif

	// adds components of 2 RRGGBB colors: c0 + t*c1
	// uses t*c1.alpha to scale c1 before it's added to c0
	// c0.alpha is ignored.
	// RGB channels are clipped to <=255.
#ifdef _USEDIB
RGBQUAD Canvas::blendAdd(
					RGBQUAD c0, const GLcolor & c1, float t) const
{
		if(t <= 0) return c0;
		t *= c1.a;
		
		int r1 = (int)(255 * c1.r);
		int g1 = (int)(255 * c1.g);
		int b1 = (int)(255 * c1.b);
		int r0 = c0.rgbRed;
		int g0 = c0.rgbGreen;
		int b0 = c0.rgbBlue;

		int r = (int)(t*r1 + r0);
		if(r>255) r=255;
		int g = (int)(t*g1 + g0);
		if(g>255) g=255;
		int b = (int)(t*b1 + b0);
		if(b>255) b=255;
		
		RGBQUAD q = { b, g, r, 0 };
		return q;
}
#else
COLORREF Canvas::blendAdd(
					COLORREF c0, const GLcolor & c1, float t) const
{
		if(t <= 0) return c0;
		t *= c1.a;
		
		int r1 = (int)(255 * c1.r);
		int g1 = (int)(255 * c1.g);
		int b1 = (int)(255 * c1.b);
		int r0 = (int)(c0 & 0xff);
		int g0 = (int)((c0>>8) & 0xff);
		int b0 = (int)((c0>>16) & 0xff);

		int r = (int)(t*r1 + r0);
		if(r>255) r=255;
		int g = (int)(t*g1 + g0);
		if(g>255) g=255;
		int b = (int)(t*b1 + b0);
		if(b>255) b=255;
		
		return ((b<<16) | (g<<8) | r);	// alpha=0
}
#endif

void Canvas::setGamma(float f)
{
	m_fGamma = f;
	for(int i=0; i<256; ++i)
	{
		m_vfGamma[i] = (255.0f * ::pow((float)(i/255.0f), m_fGamma));
		m_vnGamma[i] = (int)(0.5+m_vfGamma[i]);
		m_vnInvGamma[i] = (int)(255*::pow(i/255.0, 1.0/m_fGamma));
	}
}

void Canvas::render(int x, int y, int dx, int dy, int coarseStep)
{
	renderCoarse(x, y, dx, dy, coarseStep);
	if(!m_stopFlag)
		renderRect(x, y, dx, dy);
}

void Canvas::renderCoarse(int x, int y, int dx, int dy, int s)
{
		for(int j=y; j<y+dy; j+=s)
		{
			m_dcMutex.Lock();
			for(int i=x; i<x+dx; i+=s)
			{
				Complex z = m_screenMapping.map(i, j);
				//Complex z(tx + mxx * i, ty + myy * j);
				COLORREF clr = m_pfn->getColor(z);
//				pfn->m_pDC->FillSolidRect(i, j, s, s, clr);
				m_dc.FillSolidRect(i, j, s, s, clr);
			}
			m_dcMutex.Unlock();
			if(m_stopFlag)
				break;
		}
}

void Canvas::renderRect(int x, int y, int dx, int dy)
{
	if(m_stopFlag)
		return;

	if(dx<10 || dy<10)
	{
		m_dcMutex.Lock();
		for(int j=y; j<y+dy; ++j)
		{
			for(int i=x; i<x+dx; ++i)
			{
				Complex z = m_screenMapping.map(i, j);
				COLORREF clr = m_pfn->getColor(z);
				m_dc.SetPixelV(i, j, clr);
			}
		}
		m_dcMutex.Unlock();
	}
	else
	{
//		if(dx<64 && dy<64)
//			m_pDC->FillSolidRect(x, y, dx, dy, getColor(m_screenMapping.map(x, y)));

		// recursively subdivide this render rect
		// cut the longer dimension in half
		if(dx>dy)
		{
			renderRect(x, y, dx/2, dy);
			renderRect(x+dx/2, y, (dx+1)/2, dy);
		}
		else
		{
			renderRect(x, y, dx, dy/2);
			renderRect(x, y+dy/2, dx, (dy+1)/2);
		}

//		if(m_stopFlag)
//			return;
//
//		if(m_pNotifyWnd)
//			m_pNotifyWnd->PostMessage(MSG_CANVAS_NOTIFICATION, MSG_RENDER_PROGRESS, rand()%100);
	}
}

void Canvas::stop(void)
{
	if(m_pThread)
	{
		if(m_pThread->m_hThread && m_isRunning)
		{
			ASSERT(m_isRunning);
			ASSERT(::AfxGetThread() != m_pThread);
			m_stopFlag = true;

			m_pThread->BeginWaitCursor();

			int wait = 0;
			while(m_isRunning)
			{
				::Sleep(100);
				wait += 100;
				if(wait > 5000) {
					TRACE("worker thread would not die\n");
					m_pThread->SuspendThread();
					break;
					ASSERT(wait < 10000);
				}
			}
		}
		m_pThread = NULL;
	}
}

// only call this while a run is in progress.
double Canvas::getRunTime(void) const
{
	if(m_isRunning)
		return getElapsedTime();
	return m_lastPlotTime;
}

double Canvas::getElapsedTime(void) const
{
	// it's not necessary that the thread is still running,
	// just more meaningful
	ASSERT(m_isRunning);

	clock_t tEnd = clock();
	return (double)(tEnd - m_plotStartTime) / CLOCKS_PER_SEC;
}

CString Canvas::getSpeedString(void) const
{
	CString sz;
	if(m_pfnSketch)
	{
		sz.Format(L"Calc: %d (%dK pps)\r\nPlot: %d (%dK pps) (%d%%)", 
			m_nPointsCalced,
			(int)(0.5 + m_nPointsCalced / getElapsedTime() * 0.001),
			m_nPointsPlotted,
			(int)(0.5 + m_nPointsPlotted / getElapsedTime() * 0.001),
			m_nPointsPlotted * 100 / m_nPointsCalced);
	}
	return sz;
}