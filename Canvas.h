#ifndef _RENDERCANVAS_H
#define _RENDERCANVAS_H 1

class Plottable;
class ComplexColorMap;
class SketchFormula;

#include "Mapping.h"
#include "GLcolor.h"
#include <afxmt.h>
#include "JsonSerializable.h"	// for ptrtype

// debug, usedib: 345K
// debug, non-dib: 110K
// release, non-dib: 90K wtf?!
// release, dib: 1130K

#define _USEDIB 1

#define MSG_CANVAS_NOTIFICATION (WM_USER+42)
#define MSG_RENDER_BEGIN (0)
#define MSG_RENDER_PROGRESS (1)
#define MSG_RENDER_COMPLETE (2)
#define MSG_RENDER_CANCELLED (3)

struct BigRGB
{
	unsigned int alpha, blue, green, red;
};

// canvas is the object that needs to be shared.
// the worker thread uses this object for painting
// the app uses it for updating
//
class Canvas
{
protected:
		int m_nWidth, m_nHeight;

		// Canvas owns and manages these DC objects
		CDC m_dc;
		CBitmap m_bmp;
		RGBQUAD *m_pBits;

		CMutex m_dcMutex;	// protects drawing for multithread access

		// time the current plot began
		clock_t m_plotStartTime;
		double m_lastPlotTime;	// time for last plot

		// window to notify on start/stop/interrupt/progress (if not NULL)
		CWnd *m_pNotifyWnd;

		// worker thread stuff
		ComplexColorMap *m_pfn;
		SketchFormula *m_pfnSketch;
		Mapping m_screenMapping;
		bool m_canvasSizeLocked;
		CWinThread *m_pThread;
		bool m_stopFlag, m_isRunning, m_isPaused;

		// stats apply only to sketches
		int m_nPointsCalced;
		int m_nPointsPlotted;

public:
		Canvas(): m_nWidth(-1), m_pfn(NULL), m_pfnSketch(NULL),
			m_pThread(NULL), m_pNotifyWnd(NULL),
			m_isRunning(false), m_stopFlag(false), m_isPaused(false),
			m_antialiasing(false), m_bGamma(false), m_fGamma(2.2f),
			m_canvasSizeLocked(false) {}
		~Canvas();

		bool isInitialized();
		void init(CDC *pDC, int cx, int cy);

		bool isRunning(void);
		void run(ptrtype pfn, const Mapping &m);

protected:
		void resize(int cx, int cy);
		void run(ComplexColorMap *pfn, const Mapping &m);
		void run(SketchFormula *pfn, const Mapping &m);

public:
		void stop(void);
		int getWidth(void) const { return m_nWidth; }
		int getHeight(void) const { return m_nHeight; }

		bool isRunning(void) const { return m_isRunning; }
		bool isStopping(void) const { return m_stopFlag; }
		bool isCancelled(void) const { return !m_isRunning && m_stopFlag; }
		bool isPaused(void) const { return m_isPaused; }
		void pause(bool b) { m_isPaused = b; }

		bool isCanvasSizeLocked() const { return m_canvasSizeLocked; }
		void lockCanvasSize(bool b) { m_canvasSizeLocked = b; }

		double getRunTime(void) const;
		double getElapsedTime(void) const;
		CString getSpeedString(void) const;

		CWnd* GetNotifyWnd(void) { return m_pNotifyWnd; }
		void SetNotifyWnd(CWnd *pWnd) { m_pNotifyWnd = pWnd; }

		void drawTo(CDC *pDC, const CRect &rc);

		const CBitmap& getBitmap() { return m_bmp; }

protected:
		CDC *GetDC(void);

		// thread fn
		friend UINT fnCanvasPlot(LPVOID pParam);
		
		UINT threadRenderFunction(void);

		std::string m_szHistory;

		// worker thread parts

		// type a
		void render(int x, int y, int dx, int dy, int coarseStep);
		void renderCoarse(int x, int y, int dx, int dy, int s);
		void renderRect(int x, int y, int dx, int dy);

		// type b (sand painting)
		void sketch(void);

		// graphics primitives
protected:
		unsigned char m_vnGamma[256];
		float m_vfGamma[256];
		unsigned char m_vnInvGamma[256];
		float m_fGamma;
		bool m_bGamma;
		GLcolor m_color;
		bool m_antialiasing;
public:
		void setGamma(float f=2.2);
		void enableGamma(bool b=true) { m_bGamma = b; }
		void disableGamma(void) { m_bGamma = false; }
		bool isGammaEnabled(void) const { return m_bGamma; }

		void enableAntialiasing(bool aa);
		bool isAntialiasingEnabled(void) const { return m_antialiasing; }

		// blended/antialiased setPixel ops
		void drawPixelAdd(POINT pt);
		void drawPixelAA(float x, float y);
		void drawPixelA(int x, int y, float alpha);

		// color blending
#ifdef _USEDIB
		// DIB versions
		// todo: do timings, see if in-place refs would be faster
		RGBQUAD blendAddGamma(RGBQUAD c0, const GLcolor & c1, float t) const;
		RGBQUAD blendAdd(RGBQUAD c0, const GLcolor & c1, float t) const;
#else
		// COLORREF versions
		COLORREF blendAddGamma(COLORREF c0, const GLcolor & c1, float t) const;
		COLORREF blendAdd(COLORREF c0, const GLcolor & c1, float t) const;
#endif
};


#endif