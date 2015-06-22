#ifndef _SKETCH_FORMULAS_H
#define _SKETCH_FORMULAS_H 1

#include "Plottable.h"
#include "PlottablesManager.h"
#include "SketchPath.h"

// Sketch classes

class SketchFormula : public Plottable
{
	_INHERITS_ABSTRACT(SketchFormula, Plottable);

protected:
	ptrtype m_pPath;	// Path

public:
	SketchFormula() : Plottable() {
		//m_pPath = new RingPath(1.4, 1.5, 3, 0.5, 7.25);
	}
	SketchFormula(ptrtype path): Plottable(), m_pPath(path) { _ASSERT_VALID(this); }

	virtual void AssertValid() const {
		Plottable::AssertValid();
		ASSERT(m_pPath);
		ASSERT(dynamic_cast<Path*>(m_pPath.get()));
		ASSERT(((Path*)(m_pPath.get()))->getGroupSize() > 0);
	}

	virtual Json::Value getJSONValue() const 
	{
		Json::Value root = Plottable::getJSONValue();
		root["path"] = m_pPath->getJSONValue();
		return root;
	}

	virtual void fromJSONValue(const Json::Value &root, JsonSerializableFactory &factory)
	{
		Plottable::fromJSONValue(root, factory);
		m_pPath = factory.createFromJSON(root["path"]);
		_ASSERT_VALID(this);
	}

	virtual std::string getRequiredMemberClass(std::string path) const
	{
		if(!path.compare("[path]")) {
			return "Path";
		}
		if(!path.substr(0, strlen("[path]")).compare("[path]")) {
			return m_pPath->getRequiredMemberClass(path.substr(strlen("[path]"), path.length()-strlen("[path]")).c_str());
		}
		return "";
	}

	const ptrtype getPath(void) const { return m_pPath; }

	void setPath(ptrtype path) { 
		ASSERT(dynamic_cast<Path*>(path.get()));
		m_pPath = path;
	}

	virtual int GetGroupSize(void) const { return ((Path*)(m_pPath.get()))->getGroupSize(); }
	virtual int GetGroup(double th, Complex *pts, GLcolor *cols, int n);

	virtual void Fold(Complex *pz, int n) const;
	virtual Complex Fold(Complex z) const = 0;
};

class NewtonSketch : public SketchFormula
{
	_INHERITS(NewtonSketch, SketchFormula);

protected:
	int m_nIterations;

public:
	NewtonSketch() : SketchFormula(),
		m_nIterations(2) { }
	NewtonSketch(int iter, ptrtype path): SketchFormula(path), m_nIterations(iter) { _ASSERT_VALID(this); }

	virtual Json::Value getJSONValue() const 
	{
		Json::Value root = SketchFormula::getJSONValue();
		root["iterations"] = m_nIterations;
		return root;
	}

	virtual void fromJSONValue(const Json::Value &root, JsonSerializableFactory &factory)
	{
		SketchFormula::fromJSONValue(root, factory);
		m_nIterations = root["iterations"].asInt();
	}

	Complex Fold(Complex z) const;
};

class BuddhabrotSketch : public SketchFormula
{
	_INHERITS(BuddhabrotSketch, SketchFormula);

protected:
	int m_nMinIter, m_nMaxIter;
	double m_fBrightness;
	int m_nColorScheme;
	double m_fSmartBailout;

public:
	BuddhabrotSketch() {}
	BuddhabrotSketch(ptrtype path) : 
		SketchFormula(path),
		m_nMinIter(5), 
		m_nMaxIter(10), 
		m_fBrightness(0.1),
		m_nColorScheme(0) { _ASSERT_VALID(this); }

	virtual Json::Value getJSONValue() const 
	{
		Json::Value root = SketchFormula::getJSONValue();
		root["MinIter"] = m_nMinIter;
		root["MaxIter"] = m_nMaxIter;
		root["Brightness"] = m_fBrightness;
		root["ColorScheme"] = m_nColorScheme;
		return root;
	}

	virtual void fromJSONValue(const Json::Value &root, JsonSerializableFactory &factory)
	{
		SketchFormula::fromJSONValue(root, factory);
		m_nMinIter = root["MinIter"].asInt();
		m_nMaxIter = root["MaxIter"].asInt();
		m_fBrightness = root["Brightness"].asDouble();
		m_nColorScheme = root["ColorScheme"].asInt();
	}

	// override
	virtual int GetGroupSize(void) const;
	virtual int GetGroup(double th, Complex *pts, GLcolor *cols, int n);
	Complex Fold(Complex z) const;
};

#endif
