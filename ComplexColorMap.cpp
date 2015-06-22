#include "stdafx.h"
#include "ComplexColorMap.h"
#include "Formulas.h"
#include "complex.h"

//
//	ComplexColorMaps are functions of the form: Z -> RGB
//

void ChainableColorMap::AssertValid() const
{
	ComplexColorMap::AssertValid();

	ASSERT(_complexMap);
	ASSERT(dynamic_cast<ComplexMap*>(_complexMap.get()));
	_ASSERT_VALID(_complexMap);

	ASSERT(_colormap);
	ASSERT(dynamic_cast<ComplexColorMap*>(_colormap.get()));
	_ASSERT_VALID(_colormap);
	ASSERT(_colormap.get() != this);

}

Json::Value ChainableColorMap::getJSONValue() const 
{
	Json::Value root = ComplexColorMap::getJSONValue();
	root["map"] = _complexMap->getJSONValue();
	root["colormap"] = _colormap->getJSONValue();

	return root;
}

std::string ChainableColorMap::getRequiredMemberClass(std::string path) const
{
	if(!path.compare("[map]")) {
		return "ComplexMap";
	}
	if(!path.substr(0, strlen("[map]")).compare("[map]")) {
		return _complexMap->getRequiredMemberClass(path.substr(strlen("[map]"), path.length()-strlen("[map]")).c_str());
	}
	if(!path.compare("[colormap]")) {
		return "ComplexColorMap";
	}
	if(!path.substr(0, strlen("[colormap]")).compare("[colormap]")) {
		return _colormap->getRequiredMemberClass(path.substr(strlen("[colormap]"), path.length()-strlen("[colormap]")).c_str());
	}
	return "";
}

void ChainableColorMap::fromJSONValue(const Json::Value &root, JsonSerializableFactory &factory)
{
	ComplexColorMap::fromJSONValue(root, factory);

	ptrtype p;
	p = factory.createFromJSON(root["map"]);
	ASSERT(dynamic_cast<ComplexMap*>(p.get()));
	_complexMap = p;

	p = factory.createFromJSON(root["colormap"]);
	ASSERT(dynamic_cast<ComplexColorMap*>(p.get()));
	_colormap = p;

	_ASSERT_VALID(this);
}

COLORREF ChainableColorMap::getColor(Complex z)
{
	// _complexMap:	C -> C
	// _colormap:	C -> RGB
	ComplexColorMap *p1 = (ComplexColorMap*)(_colormap.get());
	ComplexMap *p2 = (ComplexMap*)(_complexMap.get());
	return p1->getColor(p2->map(z));
}



//
//	TileColorMap
//

TileColorMap::TileColorMap(double periodX, double periodY, int tileCountX, int tileCountY, int tileType, ptrtype colorset):
	_periodX(periodX),
	_periodY(periodY),
	_tileCountX(tileCountX),
	_tileCountY(tileCountY),
	_offsetX(0),
	_offsetY(0),
	_tileType(tileType),
	_spotSize(0.45),
	_colorset(colorset)
{
	_ASSERT_VALID(this);
}

void TileColorMap::AssertValid() const
{
	ASSERT(_periodX > 0 && _periodY > 0);
	ASSERT(_tileCountX > 0 && _tileCountY > 0);
	ASSERT(dynamic_cast<ColorSetBase*>(_colorset.get()));
	ASSERT((dynamic_cast<ColorSetBase*>(_colorset.get()))->size() > 0);
}

COLORREF TileColorMap::getColor(Complex z)
{
	ASSERT(_tileCountX>0 && _tileCountY>0);

	double tileWidth = _periodX / _tileCountX;
	double tileHeight = _periodY / _tileCountY;

	if(!isFinite(z)) {
		return 0;
	}

	z += Complex(_offsetX, _offsetY);

	// identify z's position within repeating rectangular region (periodX x periodY)
	double uu = ffmod(z.real(), _periodX);
	double vv = ffmod(z.imag(), _periodY);

	ASSERT(uu>=0 && vv>=0);

	// identify unit subtile
	int ui = (int)(uu / tileWidth);	// 0 <= ui < _tileCountX
	int vi = (int)(vv / tileHeight);	// 0 <= vi < _tileCountY

	// correct floating point error
	if(ui == _tileCountX) --ui;
	if(vi == _tileCountY) --vi;
	ASSERT(ui>=0 && ui<_tileCountX && vi>=0 && vi<_tileCountY);

	// get position within tile
	double u = ffmod(uu, tileWidth) / tileWidth;
	double v = ffmod(vv, tileHeight) / tileHeight;
	ASSERT(0<=u && u<=1 && 0<=v && v<=1);

	// scramble ui and vi a bit to mix up the colors
	int hueindex = 
		//(ui*_tileCountY + vi);
		//(ui*vi+ui+vi);
		(ui*_tileCountY + vi) * (ui*_tileCountY + vi);
	
	ColorSetBase &colorset = *(dynamic_cast<ColorSetBase*>(_colorset.get()));
	GLcolor c = colorset.getAt(hueindex % colorset.size());

	bool darkenEdges = false;
	bool bevel = false;

	switch(_tileType)
	{
	case TileColorMap::TILES:
	default:
		return c.GetColorref();

	case TileColorMap::TILESBLACK:
		{
			darkenEdges = true;
			break;
		}

	case TileColorMap::TILES3D:
		{
			bevel = true;
			break;
		}

	case TileColorMap::SPOTS:
		{
			// mask shape in each unit UV tile...

			// polkadots: get distance from center of region
			double ctr = (u-0.5) * (u-0.5) + (v-0.5) * (v-0.5);
			// diamonds
			//double ctr = ::fabs(uu-ui-0.5) + ::fabs(vv-vi-0.5);

			if(ctr > _spotSize*_spotSize)
				c.SetGray(0);		// outside the spot

			break;
		}

	case TileColorMap::TIEDYE:
		// rainbows vs. black stripes (tie-dye)
		c.SetHSV(360*uu/_periodX, 0.75, 0.5+0.5*sin(6.283*vv/_periodY));
		break;

	case TileColorMap::IONS:
		// tighter stripes (ion)
		c.SetHSV(360*uu/tileWidth, 0.9, 0.5+0.5*sin(6.283*vv/tileHeight));
		break;

	case TileColorMap::STEEL:
		{
			int a = (ui*ui-3*vi) % 7;
			int b = (ui*vi+ui+4) % 7;
			c.SetGray(0.5*(1+sin(a*uu + b*vv)));
				//(uu+vv)/(_tileCountX+_tileCountY));
			darkenEdges = true;
			break;
		}
	}

	// apply edges

	if(bevel) {
		// square tiles, soft borders

		// position within unit tile, [0...1)
		double left = u;
		double right = 1-u;
		double top = v;
		double bottom = 1-v;
		double whiteness = 1-20*min(left, top);
		double blackness = 1-20*min(right, bottom);
		if(whiteness > blackness) {
			if(whiteness > 0) {
				c.Whiten(whiteness);
			}
		}
		else if(blackness > 0) {
			c.Darken(1-blackness);
		}
	}
	else if(darkenEdges) {
		// square tiles, soft borders
	
		double xedginess = fabs(fabs(u)-0.5);
		double yedginess = fabs(fabs(v)-0.5);
		double edginess = max(xedginess, yedginess);	// 0.5 is maximum edginess
		double val = (edginess>0.45 ? (0.5-edginess)*20 : 1);

		c.Darken(val);
	}

	return c.GetColorref();

	// steel-blue blocks. dark and modern
//	c.SetHSV(218, 0.5, min(uu/_xover, vv/_yover));

	//return c.GetColorref();


	return 0xFFFFFF;
}

Json::Value TileColorMap::getJSONValue() const 
{
	Json::Value root = ComplexColorMap::getJSONValue();
	root["periodX"] = _periodX;
	root["periodY"] = _periodY;
	root["offsetX"] = _offsetX;
	root["offsetY"] = _offsetY;
	root["tileType"] = _tileType;
	root["spotSize"] = _spotSize;
	root["tileCountX"] = _tileCountX;
	root["tileCountY"] = _tileCountY;
	root["colorset"] = _colorset->getJSONValue();
	return root;
}

void TileColorMap::fromJSONValue(const Json::Value &root, JsonSerializableFactory &factory)
{
	_tileCountX = root["tileCountX"].asInt();
	_tileCountY = root["tileCountY"].asInt();
	_periodX = root["periodX"].isDouble() ? root["periodX"].asDouble() : _tileCountX;
	_periodY = root["periodY"].isDouble() ? root["periodY"].asDouble() : _tileCountY;
	_offsetX = root["offsetX"].isDouble() ? root["offsetX"].asDouble() : 0;
	_offsetY = root["offsetY"].isDouble() ? root["offsetY"].asDouble() : 0;
	_tileType = root["tileType"].asInt();
	_spotSize = root["spotSize"].asDouble();
	_colorset = factory.createFromJSON(root["colorset"]);
}

std::string TileColorMap::getRequiredMemberClass(std::string path) const 
{ 
	if(!path.compare("[colorset]")) 
		return "ColorSetBase"; 
	return ComplexColorMap::getRequiredMemberClass(path);
}




/**
	3-color-enabled hex tiling
	tiles are regular hexagons when _periodX/_periodY == 3/sqrt(3) and _tileCountX/_tileCountY == 2/1

	\__/
	/  \__

	 >--<
	<    >--<
	 >--<
**/
COLORREF HexTileColorMap::getColor(Complex z)
{
	ASSERT(_tileCountX>0 && _tileCountY>0);

	if(!isFinite(z)) {
		return 0;
	}

	z += Complex(_offsetX, _offsetY);

	double blockWidth = _periodX / _tileCountX;
	double blockHeight = _periodY / _tileCountY;

	// identify z's position within repeating rectangular region (periodX x periodY)
	double uu = ffmod(z.real(), _periodX);
	double vv = ffmod(z.imag(), _periodY);

	ASSERT(uu>=0 && vv>=0);

	// identify block (we will refine this to get tile indexes; there's 2 per block)
	int ui = 2 * (int)(uu / blockWidth);		// 0 <= ui < 2*_tileCountX
	int vi = 2 * (int)(vv / blockHeight);		// 0 <= vi < 2*_tileCountY

	// correct floating point error
	if(ui == 2*_tileCountX) ui=0;
	if(vi == 2*_tileCountY) vi=0;
	ASSERT(ui>=0 && ui<2*_tileCountX && vi>=0 && vi<2*_tileCountY);

	// get position within block 
	// (a block is 2 tiles wide by 1 tile high; it contains parts of 4 hex tiles)
	double sq3 = sqrt(3.0);
	double u = ffmod(uu, blockWidth) * 3.0 / blockWidth;
	double v = ffmod(vv, blockHeight) * sq3 / blockHeight;
	ASSERT(0<=u && u<=3 && 0<=v && v<=sq3);

	//		This is one block.
	//
	//		------ 3 ----->
	//		                  ^
	//		\   d   /         |
	//		a\_____/   b      | sqrt(3)
	//		 /     \          |
	//		/   c   \_____    |
	//
	//		ui,vi references hex tiles within the periodic region.
	//		Going into this next block of code, ui and vi are both even, and
	//			0 <= ui < 2*tileCountX; 0 <= vi < 2*tileCountY.
	//		Once this code is complete, ui and vi will uniquely index a hex tile
	//		within the periodic region. Here are the ui, vi offsets within a block:
	//		
	//	    \  1,2  /         |
	//	0,1  \_____/  2,1     | 
	//	     /     \          |
	//	    /  1,0  \_____    |
	//
	//		Note that only half of the possible addresses are used: ui+vi is always odd.
	//
	//		dx,dy will indicate the hextile-relative position of the given complex value.
	//		(0,0) is the center of the tile; (-1/2, -sqrt(3)/2) is the lower-left corner, etc.
	double dx, dy;
	if(v < 0.5*sq3) {
		if(v > u*sq3) {
			// a
			++vi;
			dx = u + 0.5;
			dy = v - 0.5*sq3;
		}
		else if(v > (2.0-u)*sq3) {
			// b
			ui+=2;
			++vi;
			dx = u - 2.5;
			dy = v - 0.5*sq3;
		}
		else {
			// c
			++ui;
			dx = u - 1.0;
			dy = v;
		}
	}
	else {
		if(v < (1.0-u)*sq3) {
			// a
			++vi;
			dx = u + 0.5;
			dy = v - 0.5*sq3;
		}
		else if(v < (u-1.0)*sq3) {
			// b
			ui+=2;
			++vi;
			dx = u - 2.5;
			dy = v - 0.5*sq3;
		}
		else {
			// d
			++ui;
			vi+=2;
			dx = u - 1.0;
			dy = v - sq3;
		}
	}

	ASSERT(-1.001<dx && dx<1.001);
	ASSERT(-0.867<dy && dy<0.867);
	ASSERT(ui >= 0 && vi >= 0);
	ASSERT(((ui+vi)&1) == 1);

	// ensure tiles crossing the periodic rectangle's boundaries have the same colors
	ui %= (_tileCountX*2);
	vi %= (_tileCountY*2);

	// we'll need to enumerate the hex tiles to make an interesting color index.
	// hextiles are numbered bottom to top, then left to right. there are _tileCountY rows.
	// columns are already offset by 1/2 a tile height from their neighbors;
	// this next line offsets each column up an extra tile height, so
	// a hex tile is less likely to share a color with its 4 horizontally-adjacent neighbors.
	// works best when tileCountY is a multiple of 3.
	int vijitter = ((3*ui+vi-1) / 2) % _tileCountY;
	int hueindex = ui*_tileCountY + vijitter;
	
	ColorSetBase &colorset = *(dynamic_cast<ColorSetBase*>(_colorset.get()));
	GLcolor c = colorset.getAt(hueindex % colorset.size());

	bool darkenEdges = false;
	bool bevel = false;

	switch(_tileType)
	{
	case TileColorMap::TILES:
		return c.GetColorref();

	//case TileColorMap::TILESBLACK:
	//	{
	//		darkenEdges = true;
	//		break;
	//	}

	//case TileColorMap::TILES3D:
	//	{
	//		bevel = true;
	//		break;
	//	}

	default:
	case TileColorMap::SPOTS:
		{
			// mask shape in each unit UV tile...

			// polkadots: get distance from center of region
			//double dx2 = u<ctrx ? min(ctrx-u, u-ctrx+blockWidth) : min(u-ctrx, ctrx-u+blockWidth);
			double ctr = dx*dx + dy*dy;
			// diamonds
			//double ctr = ::fabs(uu-ui-0.5) + ::fabs(vv-vi-0.5);

			if(ctr > _spotSize*_spotSize)
				c.SetGray(0);		// outside the spot

			break;
		}

	//case TileColorMap::TIEDYE:
	//	// rainbows vs. black stripes (tie-dye)
	//	c.SetHSV(360*uu/_periodX, 0.75, 0.5+0.5*sin(6.283*vv/_periodY));
	//	break;

	//case TileColorMap::IONS:
	//	// tighter stripes (ion)
	//	c.SetHSV(360*uu/tileWidth, 0.9, 0.5+0.5*sin(6.283*vv/tileHeight));
	//	break;

	//case TileColorMap::STEEL:
	//	{
	//		int a = (ui*ui-3*vi) % 7;
	//		int b = (ui*vi+ui+4) % 7;
	//		c.SetGray(0.5*(1+sin(a*uu + b*vv)));
	//			//(uu+vv)/(_tileCountX+_tileCountY));
	//		darkenEdges = true;
	//		break;
	//	}
	}

	// apply edges

	/*if(bevel) {
		// square tiles, soft borders

		// position within unit tile, [0...1)
		double left = u;
		double right = 1-u;
		double top = v;
		double bottom = 1-v;
		double whiteness = 1-20*min(left, top);
		double blackness = 1-20*min(right, bottom);
		if(whiteness > blackness) {
			if(whiteness > 0) {
				c.Whiten(whiteness);
			}
		}
		else if(blackness > 0) {
			c.Darken(1-blackness);
		}
	}
	else if(darkenEdges) {
		// square tiles, soft borders
	
		double xedginess = fabs(fabs(u)-0.5);
		double yedginess = fabs(fabs(v)-0.5);
		double edginess = max(xedginess, yedginess);	// 0.5 is maximum edginess
		double val = (edginess>0.45 ? (0.5-edginess)*20 : 1);

		c.Darken(val);
	}*/

	return c.GetColorref();
}





COLORREF HSVRingColorMap::getColor(Complex z)
{
	double r = abs(z);
	double ang = arg(z);
	bool smallband = (r>0.19 && r<0.21);
	double h = arg(z);
	double s = smallband ? 0.1 : 1.0;
	double v = (r>0.98&&r<1.02) ? 0.65-0.25*cos(5*ang) : 0.9;
	GLcolor c;
	if(!c.SetHSV(360/6.283*h, s, v))
		return 0x000000;
	ASSERT(c.Valid());
	return c.GetColorref();
}



// zeroes are black holes; infinity is white.
// 0-1 radius ring is candystriped.
COLORREF BlackHoleColorMap::getColor(Complex z)
{
	Complex lnz = ln(z);

	double val, sat;
	if(lnz.real() < _innerRadius)
	{
		// fade to black
		val = tanh(lnz.real() - _innerRadius) + 1;
		sat = 1.0;	// sunburst: hue/gray
	}
	else if(lnz.real() > _outerRadius)
	{
		// fade to white
		val = 1.0;
		sat = 1-tanh(lnz.real() - _outerRadius);
	}
	else
	{
		// wash from black to hue
		val = (lnz.real() - _innerRadius) / (_outerRadius - _innerRadius);
		// add white sunburst rays
		double v = sin(_sunburst*lnz.imag()*0.5);
		sat =  v * v;
	}

	GLcolor c;
	if(!c.SetHSV(lnz.imag()*360/6.283, sat, val))
		return 0x000000;
	return c.GetColorref();
}

Json::Value BlackHoleColorMap::getJSONValue() const 
{
	Json::Value root = ComplexColorMap::getJSONValue();
	root["innerRadius"] = _innerRadius;
	root["outerRadius"] = _outerRadius;
	root["sunburst"] = _sunburst;
	return root;
}

void BlackHoleColorMap::fromJSONValue(const Json::Value &root, JsonSerializableFactory &factory)
{
	_sunburst = root["sunburst"].asInt();
	_innerRadius = root["innerRadius"].isDouble() ? root["innerRadius"].asDouble() : 0;
	_outerRadius = root["outerRadius"].isDouble() ? root["outerRadius"].asDouble() : 1;
}



COLORREF LogRingsColorMap::getColor(Complex z)
{
	Complex lnz = ln(z);
	double val = _minVal + (_maxVal-_minVal) * fabs(sin(5 * lnz.real()));	// soft black stripes
	//val = 1;

	GLcolor c;
	if(!c.SetHSV(lnz.imag() * 180 / 3.14159, _sat, val))
		return 0;
	return c.GetColorref();
}

Json::Value LogRingsColorMap::getJSONValue() const 
{
	Json::Value root = ComplexColorMap::getJSONValue();
	root["minVal"] = _minVal;
	root["maxVal"] = _maxVal;
	root["sat"] = _sat;
	return root;
}

void LogRingsColorMap::fromJSONValue(const Json::Value &root, JsonSerializableFactory &factory)
{
	ComplexColorMap::fromJSONValue(root, factory);
	_minVal = root["minVal"].asDouble();
	_maxVal = root["maxVal"].asDouble();
	_sat = root["sat"].asDouble();
}



//               HSV
//  1   white                   color
//
//
//  0   <------ black ------>
// val
// sat  0                           1

// basin 0 is assumed to be at (1,0)
// only <nBasins> distinct hues are used. black at zero, white at infinity
COLORREF BasinColorMap::getColor(Complex z)
{
	Complex lnz = ln(z);
	int n = (int)((lnz.imag()/6.283) * _numBasins + _numBasins + 0.5);
	GLcolor c;
	double sat = (lnz.real()<0?1.0:1.0-tanh(lnz.real()));
	double val = (lnz.real()<0?tanh(lnz.real())+1.0:1.0);
	c.SetHSV(n*360.0/_numBasins, sat, val);
	ASSERT(c.Valid());
	return c.GetColorref();
}


// fixed floating-point mod function
// unlike fmod, return value is always non-negative (provided y is positive)
// if y is not positive, results are undefined
inline double ffmod(double x, double y)
{
	double f = fmod(x, y);
	return (f<0 ? f+y : f);
}

std::string stringReplaceAll(std::string sz, LPCSTR source, LPCSTR target)
{
	int sourceLen = ::strlen(source);
	int targetLen = ::strlen(target);
	int i = sz.find_first_of(source);
	while(i >= 0) {
		sz.replace(i, sourceLen, target);
		i = sz.find_first_of(source, i+targetLen+1);
	}
	return sz;
}

