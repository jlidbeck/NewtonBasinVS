#ifndef PLOTTABLES_MANAGER_H
#define PLOTTABLES_MANAGER_H 1

#include "stdafx.h"
#include "json/json.h"
#include "Plottable.h"
#include "JsonSerializable.h"

/////////////////////////////////////////////////////////////
//
// a class to wrap an array of Plottables.
// it assumes ownership and deletes its owned objects
// on destruction.
//
class PlottablesManager : public JsonSerializableFactory
{
	static PlottablesManager _instance;
	static bool _initializing;
	std::vector<ptrtype> _library;

public:

	static PlottablesManager& getInstance() 
	{ 
		if(!_initializing && _instance.getSize() == 0) {
			_initializing = true;
			
			//int n = _instance.readDefaults();
			//if(n == 0)
			{
				_instance.initializeDefaults();
			}
			_instance.writeDefaults();

			_initializing = false;

		}
		return _instance;
	}

	const std::vector<ptrtype>* getLibrary() const { 
		return &_library; }

private:
	void initializeDefaults();
	void writeDefaults();
	int readDefaults();

public:
	PlottablesManager() {}
	~PlottablesManager() { removeAll(); }

	// overrides
	virtual ptrtype createFromJSON(const Json::Value &root);
	virtual ptrtype createInstance(LPCSTR className);

	// last member of vpfn MUST be NULL
	void add(ptrtype vpfn[])
	{
		for(int n=0; vpfn[n]; ++n)
		{
			if(!vpfn[n]) break;
			add(vpfn[n]);
			ASSERT(n<9999);
		}
	}

	/**
		Uniquely adds function or resource reference to library. 
		If reference already exists in library, no change is made.
	**/
	int add(ptrtype pfn, std::string instanceName = "");

	int replace(ptrtype oldFn, ptrtype newFn);
	void setAt(int index, ptrtype pfn);

	void removeAll(void)
	{
		for(unsigned n=0; n<_library.size(); ++n)
		{
			_library[n].reset();
		}

		_library.clear();
	}

	int getSize(void) const { return _library.size(); }
	ptrtype getAt(int n) const { return _library[n]; }
	
	int getIndexOf(ptrtype pfn) const
	{
		int n;
		for(n=0; n<getSize(); ++n) {
			if(_library[n] == pfn) {
				return n;
			}
		}
		return -1;
	}

	std::map<ptrtype, bool> _readOnlyMap;

	void setReadOnly(ptrtype pfn, bool readOnly)
	{
		_readOnlyMap[pfn] = readOnly;
		ASSERT(_readOnlyMap[pfn] == readOnly);
	}

	bool isReadOnly(ptrtype pfn)
	{
		return _readOnlyMap[pfn];
	}

	std::string getNextUnusedName(std::string baseName) const;

	int getIndexByName(LPCSTR name) const
	{
		int n;
		for(n=0; n<getSize(); ++n) {
			if(!_library[n]->getInstanceName().compare(name)) {
				return n;
			}
		}
		return -1;
	}

	ptrtype getNextPlottable(ptrtype pfn) const
	{
		int startIndex = _library.size()-1;
		if(pfn) {
			ASSERT(dynamic_cast<Plottable*>(pfn.get()));
			startIndex = getIndexOf(pfn);
			ASSERT(startIndex >= 0);
		}

		int n = startIndex;
		do {
			n = (n+1) % _library.size();
			ptrtype p = _library[n];
			if(dynamic_cast<Plottable*>(p.get())) {
				return p;
			}
		} while(n != startIndex);

		return pfn;
	}

	ptrtype getPreviousPlottable(ptrtype pfn) const
	{
		int startIndex = 0;
		if(pfn) {
			ASSERT(dynamic_cast<Plottable*>(pfn.get()));
			startIndex = getIndexOf(pfn);
			ASSERT(startIndex >= 0);
		}

		int n = startIndex;
		do {
			n = (n+_library.size()-1) % _library.size();
			ptrtype p = _library[n];
			if(dynamic_cast<Plottable*>(p.get())) {
				return p;
			}
		} while(n != startIndex);

		return pfn;
	}

};


#endif
