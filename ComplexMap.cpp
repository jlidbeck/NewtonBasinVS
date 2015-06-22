#include "stdafx.h"
#include "ComplexMap.h"
#include "complex.h"

//
//	ComplexMaps are functions of the form: C -> C
//

void ChainableComplexMap::AssertValid() const
{
	ComplexMap::AssertValid();

	ASSERT(_map1);
	ASSERT(dynamic_cast<ComplexMap*>(_map1.get()));
	_ASSERT_VALID(_map1);

	ASSERT(_map2);
	ASSERT(dynamic_cast<ComplexMap*>(_map2.get()));
	_ASSERT_VALID(_map2);
	ASSERT(_map2.get() != this);

}

Json::Value ChainableComplexMap::getJSONValue() const 
{
	Json::Value root = ComplexMap::getJSONValue();
	root["map1"] = _map1->getJSONValue();
	root["map2"] = _map2->getJSONValue();

	return root;
}

std::string ChainableComplexMap::getRequiredMemberClass(std::string path) const
{
	if(!path.compare("[map1]")) {
		return "ComplexMap";
	}
	if(!path.substr(0, strlen("[map1]")).compare("[map1]")) {
		return _map1->getRequiredMemberClass(path.substr(strlen("[map1]"), path.length()-strlen("[map1]")).c_str());
	}
	if(!path.compare("[map2]")) {
		return "ComplexColorMap";
	}
	if(!path.substr(0, strlen("[map2]")).compare("[map2]")) {
		return _map2->getRequiredMemberClass(path.substr(strlen("[map2]"), path.length()-strlen("[map2]")).c_str());
	}
	return "";
}

void ChainableComplexMap::fromJSONValue(const Json::Value &root, JsonSerializableFactory &factory)
{
	ComplexMap::fromJSONValue(root, factory);

	ptrtype p;
	p = factory.createFromJSON(root["map1"]);
	ASSERT(dynamic_cast<ComplexMap*>(p.get()));
	_map1 = p;

	p = factory.createFromJSON(root["map2"]);
	ASSERT(dynamic_cast<ComplexMap*>(p.get()));
	_map2 = p;

	_ASSERT_VALID(this);
}

Complex ChainableComplexMap::map(Complex z)
{
	// _map1:	C -> C
	// _map2:	C -> RGB
	ComplexMap *p1 = (ComplexMap*)(_map1.get());
	ComplexMap *p2 = (ComplexMap*)(_map2.get());
	return p2->map(p1->map(z));
}




// a tricky effect to achieve:
// This is a self-similar pattern created by 2 orthogonal logarithmic spirals.
// When paired with a periodic colormap, and periodX and periodY of the LogSpiralTileMap
// are the same as (or multiples of) the periodic colormap's X and Y periods,
// the seam along the negative real axis is invisible.
//
// Degenerate (though legal) cases occur when periodX or periodY are zero. Instead of 2 spirals,
// patterns will repeat radially and angularly, along radii and along circles.
// 
Complex LogSpiralTileMap::map(Complex z)
{
	ASSERT(_periodX>0 || _periodY>0);

	Complex lnz(0, 0);
	if(isFinite(z)) {
		lnz = INV_TWO_PI*ln(z);
	}
	
	return Complex(
		_periodX*(lnz.imag()) + _periodY*(lnz.real()),
		_periodX*(lnz.real()) - _periodY*(lnz.imag()));
}

