#ifndef _MAPPING_H
#define _MAPPING_H 1

#include "complex.h"

class Mapping
{
protected:
	Complex m_cUL, m_cLR;		// model
	int m_cx, m_cy;				// view, window or canvas
	// trans matrix to map win->model
	double mxx, myy, tx, ty;

public:
	Mapping() : m_cx(0), m_cy(0), m_cUL(-2, 1.5), m_cLR(2, -1.5) { }

	// these affect both view and model
	void zoomIn(double f);
	void zoomOut(double f);
	void zoomToRect(CRect rc);
	void offset(CPoint p);

	// model
	void setModel(const Complex &cUL, const Complex &cLR);
	void matchModel(const Mapping &mapping);
	Complex getModelUL() { return m_cUL; }
	Complex getModelLR() { return m_cLR; }

	// view
	void resize(int cx, int cy);
	int getWindowWidth() const { return m_cx; }
	int getWindowHeight() const { return m_cy; }

	// view -> model
	Complex map(int i, int j) const;
	inline Complex map(const CPoint &pt) const { return map(pt.x, pt.y); }
	
	// model -> view
	POINT unmap(const Complex &z) const;
	void unmap(const Complex &z, float &x, float &y) const;

protected:
	void recalc(void);

public:
	void Serialize(CArchive &ar);
};

#endif