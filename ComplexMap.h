#ifndef _COMPLEX_MAP_H
#define _COMPLEX_MAP_H 1

#include "complex.h"
#include "json/json.h"
#include "JsonSerializable.h"


#define INV_TWO_PI 0.15915494309

/**
	ComplexMap: base class for all functions which map complex plane to itself,
	i.e. C -> C
**/
class ComplexMap : public JsonSerializable
{
	_INHERITS_ABSTRACT(ComplexMap, JsonSerializable);

public:
	virtual void AssertValid() const {}

	virtual Complex map(Complex z) = 0;
};

/**
	Chainable! Creates function composition.
	Given f: X->Y and g: Y->Z,
	creates g(f(x)).
	f, g represent ComplexMap-derived class instances;
	X, Y, Z represent complex numbers.
**/
class ChainableComplexMap : public ComplexMap
{
	_INHERITS(ChainableComplexMap, ComplexMap);

protected:
	ptrtype _map1;		// ComplexMap
	ptrtype _map2;		// ComplexMap

public:
	ChainableComplexMap(): _map1(NULL), _map2(NULL) {}
	ChainableComplexMap(ptrtype map1, ptrtype map2) :
		_map1(map1),
		_map2(map2)
	{
		_ASSERT_VALID(this);
	}

	virtual void AssertValid() const;

	virtual Json::Value getJSONValue() const;
	virtual void fromJSONValue(const Json::Value &value, JsonSerializableFactory &factory);
	virtual std::string getRequiredMemberClass(std::string path) const;

	virtual Complex map(Complex z);
};

class WavyMap : public ComplexMap
{
	_INHERITS(WavyMap, ComplexMap);

protected:
	double _xfreq;
	double _xmag;
	double _xoff;
	double _yfreq;
	double _ymag;
	double _yoff;

public:
	WavyMap() {}
	WavyMap(double xfreq, double xmag, double xoff, double yfreq, double ymag, double yoff):
		_xfreq(xfreq), _xmag(xmag), _xoff(xoff),
		_yfreq(yfreq), _ymag(ymag), _yoff(yoff)
	{ _ASSERT_VALID(this); }

	virtual Json::Value getJSONValue() const 
	{
		Json::Value root = ComplexMap::getJSONValue();
		root["xfreq"] = _xfreq;
		root["xmag"] = _xmag;
		root["xoff"] = _xoff;
		root["yfreq"] = _yfreq;
		root["ymag"] = _ymag;
		root["yoff"] = _yoff;
		return root;
	}

	virtual void fromJSONValue(const Json::Value &root, JsonSerializableFactory &factory)
	{
		ComplexMap::fromJSONValue(root, factory);
		_xfreq = root["xfreq"].asDouble();
		_xmag =  root["xmag"].asDouble(); 
		_xoff =  root["xoff"].asDouble(); 
		_yfreq = root["yfreq"].asDouble();
		_ymag =  root["ymag"].asDouble(); 
		_yoff =  root["yoff"].asDouble(); 
	}

	Complex map(Complex z)
	{
		return Complex(
			z.real() + _ymag*sin(_yoff+_yfreq*z.imag()),
			z.imag() + _xmag*sin(_xoff+_xfreq*z.real()));
	}
};

/**
	f(z) = (a*z^2 + b*z + c)^-1
**/
class InvertMap : public ComplexMap
{
	_INHERITS(InvertMap, ComplexMap);

protected:
	double _a;
	double _b;
	double _c;

public:

	InvertMap(): _a(0.1), _b(0), _c(-0.1) {}

	virtual void AssertValid() const { ASSERT(_a || _b || _c); }

	virtual Json::Value getJSONValue() const 
	{
		Json::Value root = ComplexMap::getJSONValue();
		root["a"] = _a;
		root["b"] = _b;
		root["c"] = _c;
		return root;
	}

	virtual void fromJSONValue(const Json::Value &root, JsonSerializableFactory &factory)
	{
		ComplexMap::fromJSONValue(root, factory);
		_a = root["a"].asDouble();
		_b = root["b"].asDouble();
		_c = root["c"].asDouble();
	}

	Complex map(Complex z) { return inv(_a*sq(z) + _b*z + _c); }
};

/**
	Mobius transformation map, aka linear fractional transformation
	Formula:
	z <- (az+b) / (cz+d)
**/
class MobiusMap : public ComplexMap
{
	_INHERITS(MobiusMap, ComplexMap);

private:
	Complex _a, _b, _c, _d;

public:
	MobiusMap():
		_a(1), _b(0), _c(0), _d(1) {}
	MobiusMap(Complex a, Complex b, Complex c, Complex d):
		_a(a), _b(b), _c(c), _d(d) {}

	virtual Json::Value getJSONValue() const 
	{
		Json::Value root = ComplexMap::getJSONValue();
		root["a.real"] = Json::Value(_a.real());
		root["a.imag"] = Json::Value(_a.imag());
		root["b.real"] = Json::Value(_b.real());
		root["b.imag"] = Json::Value(_b.imag());
		root["c.real"] = Json::Value(_c.real());
		root["c.imag"] = Json::Value(_c.imag());
		root["d.real"] = Json::Value(_d.real());
		root["d.imag"] = Json::Value(_d.imag());
		return root;
	}

	virtual void fromJSONValue(const Json::Value &root, JsonSerializableFactory &factory)
	{
		ComplexMap::fromJSONValue(root, factory);
		_a = Complex(root["a.real"].asDouble(), root["a.imag"].asDouble());
		_b = Complex(root["b.real"].asDouble(), root["b.imag"].asDouble());
		_c = Complex(root["c.real"].asDouble(), root["c.imag"].asDouble());
		_d = Complex(root["d.real"].asDouble(), root["d.imag"].asDouble());
	}

	Complex map(Complex z) { return (_a*z + _b) / (_c*z + _d); }
};



/**
	Base class for colormaps based on a system of two alternate logarithmic spirals.
	Spirals define a rectangular tiling.
	periodX and periodY define the number of each spiral.
	This facilitates square tile-based patterns. Because this is a conformal mapping,
	angles are preserved. There is a singularity at the origin, but due to the self-similar
	nature of logarithmic spirals, the pattern will look similar at any scale.

	Fact: Bernoulli named the logarithmic spiral "spiral mirabilis".
**/
class LogSpiralTileMap : public ComplexMap
{
	_INHERITS(LogSpiralTileMap, ComplexMap);

	double _periodX;
	double _periodY;

public:
	LogSpiralTileMap():
		_periodX(5), _periodY(3) {}
	LogSpiralTileMap(double periodX, double periodY):
		_periodX(periodX), _periodY(periodY) { _ASSERT_VALID(this); }

	virtual Json::Value getJSONValue() const 
	{
		Json::Value root = ComplexMap::getJSONValue();
		root["periodX"] = _periodX;
		root["periodY"] = _periodY;
		return root;
	}

	virtual void fromJSONValue(const Json::Value &root, JsonSerializableFactory &factory)
	{
		ComplexMap::fromJSONValue(root, factory);
		_periodX = root["periodX"].asDouble();
		_periodY = root["periodY"].asDouble();
	}

	Complex map(Complex z);
};

#endif
