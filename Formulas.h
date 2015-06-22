#ifndef _FORMULAS_H
#define _FORMULAS_H 1

#include "complex.h"
#include "Mapping.h"
#include "GLcolor.h"
#include "ColorSet.h"
#include <fstream>
#include "ComplexColorMap.h"
#include "PlottablesManager.h"
#include "JsonSerializable.h"

inline double ffmod(double x, double y);

const double BASIC_HUE_SET[32] = {
	1, 6, 12, 18, 24, 30, 36, 43,
	49, 55, 62, 70, 77, 84, 92, 100,
	109, 119, 130, 142, 155, 168, 177, 200,
	220, 238, 242, 260, 285, 310, 335, 355 };

// todo:
// remove colormap from this class; this class should be used with chaining...
// add orbit traps
class BasinFormula : public ComplexColorMap
{
	_INHERITS_ABSTRACT(BasinFormula, ComplexColorMap);

protected:
	int m_nIterations;
	double m_fBailout;

	// final orbit position colormap
	ptrtype m_pColorMap;		// ComplexColorMap

public:
	BasinFormula(): m_pColorMap(NULL), m_fBailout(0.0001), ComplexColorMap() {}
	BasinFormula(int iter, double bailout, ptrtype pColorMap): 
		m_nIterations(iter), m_pColorMap(pColorMap), 
		m_fBailout(bailout),
		ComplexColorMap()
		{ _ASSERT_VALID(this); }

	virtual void AssertValid() const {
		ComplexColorMap::AssertValid();
		ASSERT(dynamic_cast<ComplexColorMap*>(m_pColorMap.get()));
	}

	virtual Json::Value getJSONValue() const;
	virtual void fromJSONValue(const Json::Value &root, JsonSerializableFactory &factory);

	virtual std::string getRequiredMemberClass(std::string path) const;

public:
	virtual void getDefaultRange(Complex range[]) const { range[0] = Complex(-2,-1.5); range[1] = Complex(2, 1.5); }
	virtual COLORREF getColor(Complex z) = 0;
	virtual int whichRoot(Complex z) const = 0;
	virtual COLORREF getIterColor(Complex z, int iter) const;

};

// ln
class LnFormula : public BasinFormula
{
	_INHERITS(LnFormula, BasinFormula);

public:
	LnFormula(): BasinFormula() {m_nIterations=100;}
	LnFormula(int iter, double bailout, ptrtype colorMap): 
		BasinFormula(iter, bailout, colorMap) { _ASSERT_VALID(this); }

	virtual COLORREF getColor(Complex c);
	int whichRoot(Complex z) const;
};

// TODO: yet another intermediate class? FiniteRootBasinFormula? numroots, rootflags...
// base class for unit, ringsine, (to be created) polynomial..

/** The classic Newton basin fractal for x^n - 1 = 0
 *
 */
class UnitBasinFormula : public BasinFormula
{
	_INHERITS(UnitBasinFormula, BasinFormula);

protected:
	// number of roots
	int m_nRoots;
	// notes:
	// bailout with 3 roots: bands begin to touch at 0.26806... or more for geometric-looking things
	int m_nRootFlags;

public:
	UnitBasinFormula() { }
	UnitBasinFormula(int n, int iter, double bailout, int rootflags, ptrtype colorMap) : 
		BasinFormula(iter, bailout, colorMap),
		m_nRoots(n), m_nRootFlags(rootflags) 
		{
			_ASSERT_VALID(this);
		}

	virtual void AssertValid() const { BasinFormula::AssertValid(); ASSERT(m_nRoots > 0); }

	virtual Json::Value getJSONValue() const;
	void fromJSONValue(const Json::Value &root, JsonSerializableFactory &factory);

	virtual void getDefaultRange(Complex range[]) const { range[0] = Complex(-2,-1.5); range[1] = Complex(2, 1.5); }
	virtual COLORREF getColor(Complex z);
	virtual int whichRoot(Complex z) const;
};

class TracedUnitBasinFormula : public UnitBasinFormula
{
	_INHERITS(TracedUnitBasinFormula, BasinFormula);

public:
	TracedUnitBasinFormula() 
		: UnitBasinFormula() { }
	TracedUnitBasinFormula(int n, int iter, double cutoff, int rootflags, ptrtype colorMap) 
		: UnitBasinFormula(n, iter, cutoff, rootflags, colorMap) { }

	virtual COLORREF getColor(Complex z);
	
	virtual Json::Value getJSONValue() const 
	{
		Json::Value root = UnitBasinFormula::getJSONValue();
		root["tracer"] = -42;
		return root;
	}

	virtual void fromJSONValue(const Json::Value &root, JsonSerializableFactory &factory)
	{
		UnitBasinFormula::fromJSONValue(root, factory);
	}

};

class SineBasinFormula : public BasinFormula
{
	_INHERITS(SineBasinFormula, BasinFormula);

public:
	SineBasinFormula();
	SineBasinFormula(int iter, double bailout, ptrtype colormap):
		BasinFormula(iter, bailout, colormap)
		{ _ASSERT_VALID(this); }

	virtual Json::Value getJSONValue() const;
	virtual void fromJSONValue(const Json::Value &root, JsonSerializableFactory &factory);

	virtual COLORREF getColor(Complex z);
	virtual int whichRoot(Complex z) const;
	virtual COLORREF getIterColor(Complex z, int iter) const;
};

/**
	Function: sin(ni*ln(z))

	Roots lie on unit circle
**/
class RingSineBasinFormula : public BasinFormula
{
	_INHERITS(RingSineBasinFormula, BasinFormula);

private:
	double m_fRoots;

public:
	RingSineBasinFormula() {  }
	RingSineBasinFormula(int iter, double bailout, double fRoots, ptrtype colorMap):
		BasinFormula(iter, bailout, colorMap),
		m_fRoots(fRoots) { _ASSERT_VALID(this); }

	virtual Json::Value getJSONValue() const;
	virtual void fromJSONValue(const Json::Value &root, JsonSerializableFactory &factory);

	virtual COLORREF getColor(Complex z);
	virtual int whichRoot(Complex z) const;
};

/**
	Function: sin(z)*sinh(z)

	Roots lie on grid: m*pi + i*n*pi, where m and n are all integers
**/
class GridSineBasinFormula : public BasinFormula
{
	_INHERITS(GridSineBasinFormula, BasinFormula);

private:
	double _r;

public:
	GridSineBasinFormula():
		_r(10.0) {  }
	GridSineBasinFormula(int iter, double bailout, ptrtype colorMap):
		_r(10.0), BasinFormula(iter, bailout, colorMap) { _ASSERT_VALID(this); }

	//virtual Json::Value getJSONValue() const;
	//virtual void fromJSONValue(const Json::Value &root, JsonSerializableFactory &factory);

	virtual COLORREF getColor(Complex z);
	virtual COLORREF getIterColor(Complex z, int iter) const;
	virtual int whichRoot(Complex z) const;
};





#endif