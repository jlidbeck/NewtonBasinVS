#ifndef _COMPLEX_COLOR_MAP_H
#define _COMPLEX_COLOR_MAP_H 1

#include "ColorSet.h"
#include "complex.h"
#include <fstream>
#include "math.h"
#include "ComplexMap.h"
#include "Plottable.h"
#include "PlottablesManager.h"
#include <algorithm>
#include "JsonSerializable.h"

// map functions: complex -> RGB


/**
		Base class for all functions mapping the complex plane to RGB values.
		C -> RGB
**/

class ComplexColorMap : public Plottable
{
	_INHERITS_ABSTRACT(ComplexColorMap, Plottable);

public:
	ComplexColorMap() {}

	virtual Json::Value getJSONValue() const 
	{
		Json::Value root = Plottable::getJSONValue();
		return root;
	}

	virtual void fromJSONValue(const Json::Value &root, JsonSerializableFactory &factory)
	{
		Plottable::fromJSONValue(root, factory);
	}

	virtual void getDefaultRange(Complex range[]) const { range[0] = Complex(-2,-1.5); range[1] = Complex(2, 1.5); }
	virtual COLORREF getColor(Complex z) = 0;
	virtual void AssertValid() const {}
};

/**
	Chainable!
**/
class ChainableColorMap : public ComplexColorMap
{
	_INHERITS(ChainableColorMap, ComplexColorMap);

protected:
	ptrtype _complexMap;	// ComplexMap
	ptrtype _colormap;		// ComplexColorMap

public:
	ChainableColorMap(): _complexMap(NULL), _colormap(NULL) {}
	ChainableColorMap(ptrtype complexMap, ptrtype colormap) :
		_complexMap(complexMap),
		_colormap(colormap)
	{
		_ASSERT_VALID(this);
	}

	virtual void AssertValid() const;

	virtual Json::Value getJSONValue() const;
	virtual void fromJSONValue(const Json::Value &value, JsonSerializableFactory &factory);
	virtual std::string getRequiredMemberClass(std::string path) const;

	COLORREF getColor(Complex z);

};


/**
	This colormap creates a repeating rectangular pattern.
	The repeated region ("tile") has the specified width and height.
	The region is divided into (width*height) subtiles, each of size 1x1,
	each subtile is colored independently,
	based on the given colorset (and some secret sauce).
**/
class TileColorMap : public ComplexColorMap
{
	_INHERITS(TileColorMap, ComplexColorMap);

protected:
	double _periodX;
	double _periodY;
	double _offsetX;
	double _offsetY;
	int _tileCountX;
	int _tileCountY;
	int _tileType;
	double _spotSize;
	ptrtype _colorset;	// ColorSet

public:
	// various colormaps based on this transformation
	static const int TILES = 1;
	static const int TILESBLACK = 2;
	static const int TILES3D = 3;
	static const int SPOTS = 4;
	static const int IONS = 5;
	static const int TIEDYE = 6;
	static const int STEEL = 7;

	TileColorMap():
		_periodX(-1),
		_periodY(-1),
		_tileCountX(-1),
		_tileCountY(-1),
		_tileType(-1),
		_spotSize(0.45) { }

	TileColorMap(double periodX, double periodY, int tileCountX, int tileCountY, int tileType, ptrtype colorset);
	
	virtual COLORREF getColor(Complex z);

	virtual void AssertValid() const;

	virtual Json::Value getJSONValue() const;
	virtual void fromJSONValue(const Json::Value &root, JsonSerializableFactory &factory);

	virtual std::string getRequiredMemberClass(std::string path) const;
};

/**
	This colormap creates a repeating rectangular pattern.
	The number of hexagonal tiles within the rectangular region is controlled by tileCountX and tileCountY.
	There will be 2*tileCountX*tileCountY unique hex tiles, as 2 columns of tiles are generated for each
	tileCountX. Odd columns are offset vertically by half a tile height.
	Indexing of the tiles is jittered. tileCountY works best as a multiple of 3.
**/
class HexTileColorMap : public TileColorMap
{
	_INHERITS(HexTileColorMap, TileColorMap);

protected:
public:
	HexTileColorMap() {_spotSize = 0.67; }
	HexTileColorMap(double periodX, double periodY, int tileCountX, int tileCountY, int tileType, ptrtype colorset):
		TileColorMap(periodX, periodY, tileCountX, tileCountY, tileType, colorset)
	{
		_spotSize = 0.67;
		_ASSERT_VALID(this);
	}

	virtual COLORREF getColor(Complex z);
};

class HSVRingColorMap : public ComplexColorMap
{
	_INHERITS(HSVRingColorMap, ComplexColorMap);

public:
	COLORREF getColor(Complex z);
};

class BlackHoleColorMap : public ComplexColorMap
{
	_INHERITS(BlackHoleColorMap, ComplexColorMap);

protected:
	double _innerRadius;
	double _outerRadius;
	int _sunburst;

public:
	BlackHoleColorMap(): _innerRadius(0), _outerRadius(1), _sunburst(5) {}

	virtual Json::Value getJSONValue() const;
	virtual void fromJSONValue(const Json::Value &root, JsonSerializableFactory &factory);

	COLORREF getColor(Complex z);
};

class LogRingsColorMap : public ComplexColorMap
{
	_INHERITS(LogRingsColorMap, ComplexColorMap);

	double _sat;
	double _minVal;
	double _maxVal;
	int _numRoots;

public:
	LogRingsColorMap(): _sat(0.9), _minVal(0), _maxVal(1) {}
	//LogRingsColorMap(int nRoots) : _numRoots(nRoots) {}

	COLORREF getColor(Complex z);

	virtual Json::Value getJSONValue() const;
	virtual void fromJSONValue(const Json::Value &root, JsonSerializableFactory &factory);
};

class BasinColorMap : public ComplexColorMap
{
	_INHERITS(BasinColorMap, ComplexColorMap);

	int _numBasins;

public:
	BasinColorMap() : _numBasins(5) {}
	BasinColorMap(int nBasins) : _numBasins(nBasins) {}
	COLORREF getColor(Complex z);
};

#endif