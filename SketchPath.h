#ifndef _SKETCH_PATH_H
#define _SKETCH_PATH_H 1

#include "json/json.h"
#include "JsonSerializable.h"

// Path classes
// these pair with Sketch classes

class Path : public JsonSerializable
{
	_INHERITS_ABSTRACT(Path, JsonSerializable);

public:
	Path() {}
	Path(const Path *pp) {}

	virtual void AssertValid() const { }

	virtual int getGroupSize(void) const = 0;
	virtual Complex get(double th, int j) const = 0;
};

class RingPath : public Path
{
	_INHERITS(RingPath, Path);

protected:
	int m_rows;
	double m_rmin, m_rmax;
	double m_waveamp, m_wavefreq;
	double m_blur;
	Complex m_ctr;
	// calc
	double m_a, m_b;
public:
	RingPath(double rmin=0.5, double rmax=1.5, int rows=100, double waveamp=0.5, double wavefreq=7.5, double blur=0.0, Complex ctr=Complex(0,0)):
		m_rmin(rmin), m_rmax(rmax), m_rows(rows), 
		m_waveamp(waveamp), m_wavefreq(wavefreq),
		m_blur(blur), m_ctr(ctr)
		{ init(); }

	//virtual LPCSTR getClassName(void) const { return "RingPath"; }

	void init(void) {
		m_a = ::log(m_rmin) / m_rows;
		m_b = ::log(m_rmax) / m_rows; }

	virtual Json::Value getJSONValue() const 
	{
		Json::Value root = Path::getJSONValue();
		root["Rows"] = m_rows;
		root["RMin"] = m_rmin;
		root["RMax"] = m_rmax;
		root["WaveAmp"] = m_waveamp;
		root["WaveFreq"] = m_wavefreq;
		root["Blur"] = m_blur;
		root["Center.real"] = m_ctr.real();
		root["Center.imag"] = m_ctr.imag();
		return root;
	}

	virtual void fromJSONValue(const Json::Value &root, JsonSerializableFactory &factory)
	{
		Path::fromJSONValue(root, factory);
		m_rows = root["Rows"].asInt();
		m_rmin = root["RMin"].asDouble();
		m_rmax = root["RMax"].asDouble();
		m_waveamp = root["WaveAmp"].asDouble();
		m_wavefreq = root["WaveFreq"].asDouble();
		m_blur = root["Blur"].asDouble();
		m_ctr.real(root["Center.real"].asDouble());
		m_ctr.imag(root["Center.imag"].asDouble());

		init();
	}

	int getGroupSize(void) const { return m_rows; }
	Complex get(double th, int j) const;

public:
	friend class SketchFormula;
};

#endif
