#include "stdafx.h"
#include "PlottablesManager.h"
#include "ComplexMap.h"
#include "ComplexColorMap.h"
#include "ColorSet.h"
#include "Formulas.h"
#include "SketchPath.h"
#include "SketchFormulas.h"
#include <stdio.h>
#include <boost/algorithm/string/trim_all.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/lexical_cast.hpp>

/////////////////////////////////////////////////////////////
//
// a class to wrap an array of Plottables.
// it assumes ownership and deletes its owned objects
// on destruction.
//

// first instantiation of all BasinFormula-derived classes
// is done here.


PlottablesManager PlottablesManager::_instance;
bool PlottablesManager::_initializing = false;

/**
	Uniquely adds function or resource reference to library. 
	If reference already exists in library, no change is made.
**/
int PlottablesManager::add(ptrtype pfn, std::string instanceName)
{
	_ASSERT_VALID(pfn);

	if(!instanceName.empty()) {
		pfn->setInstanceName(instanceName);
	}

	if(getIndexOf(pfn) < 0) {

		std::string name = pfn->getInstanceName();

		int existingIndex;
		if(!name.empty() && (existingIndex = this->getIndexByName(name.c_str())) >= 0) {
			ptrtype x = this->getAt(existingIndex);
			TRACE(" *** adding item with duplicate name ***\n *** existing: %s : %s\n *** new: %s : %s\n",
				x->getInstanceName().c_str(),
				x->getClassName(),
				pfn->getInstanceName().c_str(),
				pfn->getClassName());
		}
		
		TRACE("Library.add %s : %s\n", pfn->getInstanceName().c_str(), pfn->getClassName());
		_library.push_back(pfn);
	}

	return _library.size()-1;
}

int PlottablesManager::replace(ptrtype oldFn, ptrtype newFn)
{
	int n = getIndexOf(oldFn);
	if(n>=0) {
		bool b = _readOnlyMap[oldFn];
		(_readOnlyMap.erase(oldFn));
		setAt(n, newFn);
		setReadOnly(newFn, b);
	}
	return n;
}

void PlottablesManager::setAt(int n, ptrtype pfn)
{
	_library[n] = pfn;
}

void PlottablesManager::initializeDefaults()
{
	// first run (?):

	// register all classes

	{
		registerClass(ptrtype(new BlackHoleColorMap()));
		registerClass(ptrtype(new BuddhabrotSketch()));
		registerClass(ptrtype(new ChainableComplexMap()));
		registerClass(ptrtype(new ChainableColorMap()));
		registerClass(ptrtype(new ColorSet()));
		registerClass(ptrtype(new HueSpinColorSet()));
		registerClass(ptrtype(new HexTileColorMap()));
		registerClass(ptrtype(new InvertMap()));
		registerClass(ptrtype(new LnFormula()));
		registerClass(ptrtype(new LogSpiralTileMap()));
		registerClass(ptrtype(new MobiusMap()));
		registerClass(ptrtype(new NewtonSketch()));
		registerClass(ptrtype(new RingPath()));
		registerClass(ptrtype(new RingSineBasinFormula()));
		registerClass(ptrtype(new GridSineBasinFormula()));
		registerClass(ptrtype(new LogRingsColorMap()));
		registerClass(ptrtype(new SineBasinFormula()));
		registerClass(ptrtype(new TileColorMap()));
		registerClass(ptrtype(new TracedUnitBasinFormula()));
		registerClass(ptrtype(new UnitBasinFormula()));
		registerClass(ptrtype(new WavyMap()));
	}

	// generate set of defaults

	removeAll();

	// first, some colorsets. these are not (yet) stored in the library,
	// but can be used to build some maps

	ColorSet *pSpectralColorSet = new ColorSet();
	pSpectralColorSet->fromHues(BASIC_HUE_SET, 32, 0.9, 1.0);
	pSpectralColorSet->offsetHues(90);	//
	ptrtype spectralColorSet(pSpectralColorSet);
	add(spectralColorSet, "spectralColorSet");

	ColorSet *pOrangeColorSet = new ColorSet();
	pOrangeColorSet->fromHues(BASIC_HUE_SET, 10, 0.9, 1.0);
	pOrangeColorSet->offsetHues(30);	// orange
	ptrtype orangeColorSet(pOrangeColorSet);
	add(orangeColorSet, "orangeColorSet");

	ColorSet *pThreeColorSet = new ColorSet(*pOrangeColorSet);
	pThreeColorSet->resampleBiased(3, 1.0);
	ptrtype threeColorSet(pThreeColorSet);
	add(threeColorSet, "threeColorSet");

	HueSpinColorSet *pHueSpinColorSet = new HueSpinColorSet(8, 0, 360.0/8, 0.8, 1.0);
	add(ptrtype(pHueSpinColorSet), "hueSpinColorSet");

	// now some color maps

	ptrtype hexTile = ptrtype(new HexTileColorMap(1, 1.732, 1, 3, TileColorMap::SPOTS, threeColorSet));
	add(hexTile, "hexTile");

	ptrtype orangeTile = ptrtype(new TileColorMap(2, 7, 2, 7, TileColorMap::SPOTS, orangeColorSet));
	add(orangeTile, "orangeTile");

	ptrtype blackHole = ptrtype(new BlackHoleColorMap());
	add(blackHole, "blackHole");

	// complex maps (C->C)

	ptrtype invertMap = createInstance("InvertMap");
	add(invertMap, "invertMap");

	ptrtype spiralMap_2_7 = ptrtype(new LogSpiralTileMap(2, 7));
	add(spiralMap_2_7, "spiralMap_2_7");

	ptrtype mobiusMap = ptrtype(new MobiusMap(1.0, 1.5, 1.0, 1.0));
	add(mobiusMap, "mobiusMap");

	ptrtype wavyMapX = ptrtype(new WavyMap(1.0, 1.0, 0.0, 0.0, 0.0, 0.0));
	add(wavyMapX, "wavyMapX");

	ptrtype wavyMapY = ptrtype(new WavyMap(0.0, 0.0, 0.0, 1.0, -1.0, 0.0));
	add(wavyMapY, "wavyMapY");

	ptrtype wavyMap2D = ptrtype(new WavyMap(1.0, 1.0, 0.0, 1.0, -1.0, 0.0));
	add(wavyMap2D, "wavyMap2D");

	// chaining C->C->C

	ptrtype wavyChainMap = ptrtype(new ChainableComplexMap(wavyMapX, wavyMapY));
	add(wavyChainMap, "wavyChainMap");

	// finally some chained color maps: C->C->RGB

	ptrtype orangeSpiral = ptrtype(new ChainableColorMap(spiralMap_2_7, orangeTile));
	add(orangeSpiral, "orangeSpiral");

	// previously HarlequinColorMap.
	// we're pairing a periodic color function (tilecolormap) with a complex map (logspiral).
	// ensuring that the periods match (periodX and periodY in both) means that the spiral transform
	// preserves periodicity.
	ptrtype spectralSpots = ptrtype(new TileColorMap(7, 18, 7, 18, TileColorMap::SPOTS, spectralColorSet));
	add(spectralSpots, "spectralSpots");
	ptrtype spiralMap_7_18 = ptrtype(new LogSpiralTileMap(7, 18));
	add(spiralMap_7_18, "spiralMap_7_18");
	ptrtype spectralSpiralSpots = ptrtype(new ChainableColorMap(spiralMap_7_18, spectralSpots));
	add(spectralSpiralSpots, "spectralSpiralSpots");
		
	ptrtype logRingsMap = ptrtype(new LogRingsColorMap());
	add(logRingsMap, "logRingsMap");

	ptrtype steel = ptrtype(new TileColorMap(13, 8, 13, 8, TileColorMap::STEEL, spectralColorSet));
	add(steel, "steel");

	ptrtype chainTest = ptrtype(new ChainableColorMap(invertMap, steel));
	add(chainTest, "chainTest");

	add(ptrtype(new ChainableColorMap(mobiusMap, steel)), "steelMobius");

	// paths

	ptrtype defaultSketchPath = ptrtype(new RingPath(1.4, 1.5, 3, 0.5, 7.25));
	add(defaultSketchPath, "defaultSketchPath");

	// source function bank
	ptrtype fns[] = { 
		ptrtype(new GridSineBasinFormula(12, 0.00001, spectralSpiralSpots)),
		ptrtype(new RingSineBasinFormula(1, 0.00001, 5, spectralSpiralSpots)),
		ptrtype(new RingSineBasinFormula(1, 0.00001, 5, blackHole)),
		ptrtype(new RingSineBasinFormula(1, 0.00001, 5, logRingsMap)),
		ptrtype(new UnitBasinFormula(3, 25, 0.00001, -1, orangeTile)),
		ptrtype(new UnitBasinFormula(3, 115, 0.26806, 1, orangeTile)),
		ptrtype(new TracedUnitBasinFormula(6, 25, 0.0001, 21, orangeTile)),
		ptrtype(new SineBasinFormula(10, 0.0001, blackHole)), 
		ptrtype(new LnFormula(100, 0.00001, orangeTile)),
		ptrtype(new NewtonSketch(2, defaultSketchPath)),
		ptrtype(new BuddhabrotSketch(defaultSketchPath)),
		NULL };

	add(fns);

	// set all read-only
	for(int i=0; i<getSize(); ++i) {
		ptrtype p = getAt(i);
		setReadOnly(p, true);
	}
}

void PlottablesManager::writeDefaults()
{
	FILE *fout;
	fopen_s(&fout, "DefaultPlots.txt", "wt");

	int noNameCount = 1;

	for(unsigned int n=0; n<_library.size(); ++n)
	{
		ptrtype fn = _library[n];

		//ASSERT(dynamic_cast<Plottable*>(fn.get()));

		Json::Value json = fn->getJSONValue();
		if(!fn->getInstanceName().empty()) {
			//json["_id"] = p->getInstanceName();
			fputs(fn->getInstanceName().c_str(), fout);
			fputs("\n", fout);
		}
		else {
			fprintf(fout, "noname%d\n", noNameCount++);
		}

		std::string sz = json.toStyledString();
		//fputs("SomeName: ", fout);
		fputs(sz.c_str(), fout);
		fputs("\n\n", fout);
		fflush(fout);
	}

	fclose(fout);

}

int PlottablesManager::readDefaults()
{
	_library.clear();

	FILE *fin = NULL;
	fopen_s(&fin, "DefaultPlots.txt", "rt");
	if(!fin) {
		return 0;
	}

	char buf[10005];
	std::string blockName = "";
	std::string block = "";

	while(fgets(buf, 10000, fin))
	{
		::StrTrimA(buf, " \r\n\t");
		if(strlen(buf) > 0) {
			if(blockName.empty()) {
				// store first line as name
				blockName = buf;
			}
			else {
				// append line to JSON block to parse
				block += buf;
			}
		}
		else {
			// empty line found: attempt to deserialize block
			if(block.size() > 0) {
				Json::Value json;
				Json::Reader reader;
				if(reader.parse(block, json, true)) {
					try {
						ptrtype p = createFromJSON(json);
						p->setInstanceName(blockName.c_str());
						add(p);
					}
					catch(const char *e) {
						TRACE("Exception: %s\n", e);
					}
				}
				else {
					// unable to parse JSON block
					TRACE("Unable to parse JSON value in defaults file: %s\n", block.c_str());

					// controversial: one error means bail on all objects in file, 
					// signaling a revert to default set...
					_library.clear();
					break;
				}
			}

			block = "";
			blockName = "";
		}

	}
	fclose(fin);

	return _library.size();
}

ptrtype PlottablesManager::createInstance(LPCSTR className)
{
	// instantiate named class

	ptrtype p = JsonSerializableFactory::createInstance(className);
	if(p) {
		return p;
	}

	std::string sz = "Unregistered JsonSerializable class: '"+std::string(className)+"'";
	throw(sz);
	return NULL;
}

ptrtype PlottablesManager::createFromJSON(const Json::Value &root)
{
	ptrtype pfn = JsonSerializableFactory::createFromJSON(root);

	if(pfn) {
		//this->add(pfn);
	}

	return pfn;
}

std::string incrementName(const std::string &oldName) {
	// determine whether string already ends in a sequence of characters

	int digit = (int)(oldName.length()) - 1;
	while(digit>=0 && isdigit(oldName[digit])) {
		--digit;
	}
	++digit;

	int n = 1;
	if(digit < oldName.length()) {
		// oldName ends with a number string--increment that
		n = boost::lexical_cast<int>(oldName.data() + digit);
		return (oldName.substr(0, digit) + boost::lexical_cast<std::string>(n+1));
	}

	return oldName + "1";
}

std::string PlottablesManager::getNextUnusedName(std::string oldName) const
{
	std::string newName = boost::algorithm::trim_copy(oldName);
	
	if(newName.empty()) {
		newName = "noname";
	}

	// increment name
	while(getIndexByName(newName.c_str()) >= 0) {
		newName = incrementName(newName);
	}

	return newName;
}

