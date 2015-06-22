#include "stdafx.h"
#include "Mapping.h"
#include <float.h>

CArchive& operator<<(CArchive &ar, const Complex &c)
{
	ar << c.real() << c.imag();
	return ar;
}

CArchive& operator>>(CArchive &ar, Complex &c)
{
	double re, im;
	ar >> re >> im;
	c = Complex(re, im);
	return ar;
}

void Mapping::Serialize(CArchive &ar)
{
	if(ar.IsStoring())
	{
		ar << CString("Mapping");
		ar << m_cUL << m_cLR;
	}
	else
	{
		CString sz;
		ar >> sz;
		ASSERT(!sz.Compare(L"Mapping"));
		ar >> m_cUL >> m_cLR;
		recalc();
	}
}

void Mapping::setModel(const Complex &cUL, const Complex &cLR)
{
	m_cUL = cUL;
	m_cLR = cLR;
	recalc();
}

void Mapping::matchModel(const Mapping &mapping)
{
	m_cUL = mapping.m_cUL;
	m_cLR = mapping.m_cLR;
	recalc();
}

void Mapping::offset(CPoint p)
{
	Complex delta = m_cUL - map(p);
	m_cUL += delta;
	m_cLR += delta;
	recalc();
}

Complex Mapping::map(int i, int j) const
{
	double y = myy*j + ty;
	double x = mxx*i + tx;
	return Complex(x, y);
}

POINT Mapping::unmap(const Complex &z) const
{
	POINT pt;
	pt.x = (z.real() - tx) / mxx;
	pt.y = (z.imag() - ty) / myy;
	return pt;
}

void Mapping::unmap(const Complex &z, float &fx, float &fy) const
{
	fx = (z.real() - tx) / mxx;
	fy = (z.imag() - ty) / myy;
}

void Mapping::zoomOut(double f)
{
	Complex cMid = 0.5*(m_cUL+m_cLR);
	Complex cHalfDiagonal = 0.5*(m_cLR-m_cUL);
	m_cUL = cMid - f * cHalfDiagonal;
	m_cLR = cMid + f * cHalfDiagonal;

	recalc();
}

void Mapping::zoomToRect(CRect rc)
{
		Complex ul = map(rc.TopLeft());
		Complex lr = map(rc.BottomRight());

		m_cUL = ul;
		m_cLR = lr;

		recalc();
}

// resize: do our best to stretch the image to fit the same
// area in the canvas, while preserving the center and the aspect ratio
//
void Mapping::resize(int cx, int cy)
{
	ASSERT(cx>0 && cy>0);

	m_cx = cx;
	m_cy = cy;

	recalc();
}

void Mapping::recalc(void)
{
	if(m_cx>0 && m_cy>0)
	{
		mxx = (m_cLR.real() - m_cUL.real()) / m_cx;
		myy = (m_cLR.imag() - m_cUL.imag()) / m_cy;

		// expand view as necessary to preserve 1:1 aspect ratio
		if(fabs(mxx)>fabs(myy))
		{
			// widen extent vertically
			double oldmyy = myy;
			// myy = mxx, preserving sign
			myy = _copysign(mxx, myy);

			double ymid = (m_cUL.imag()+m_cLR.imag())*0.5;
			m_cUL.imag(ymid + myy/oldmyy*(m_cUL.imag()-ymid));
			m_cLR.imag(ymid + myy/oldmyy*(m_cLR.imag()-ymid));
		}
		else
		{
			// will appear stretched horizontally--
			// widen extent to show more detail at left and right

			double oldmxx = mxx;
			// mxx = myy, preserving sign
			mxx = _copysign(myy, mxx);

			double xmid = (m_cUL.real()+m_cLR.real())*0.5;
			m_cUL.real(xmid + mxx/oldmxx*(m_cUL.real()-xmid));
			m_cLR.real(xmid + mxx/oldmxx*(m_cLR.real()-xmid));
		}

		tx = m_cUL.real();
		ty = m_cUL.imag();
	}

}

