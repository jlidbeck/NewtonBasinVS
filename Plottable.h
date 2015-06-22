#ifndef _PLOTTABLE_H
#define _PLOTTABLE_H 1

#include <fstream>
#include "json/json.h"
#include "JsonSerializable.h"
#include "Complex.h"

// How I missed you, C++... how long has it been
// since I got to implement my own string replace?
std::string stringReplaceAll(std::string sz, LPCSTR source, LPCSTR target);

class Plottable : public JsonSerializable
{
	_INHERITS_ABSTRACT(Plottable, JsonSerializable);

protected:
	bool m_bModified;

public:
	friend class Canvas;

public:
	Plottable() : m_bModified(false) { }
	virtual ~Plottable() {}

	virtual void AssertValid() const {}

	bool IsModified(void) const { return m_bModified; }
	void SetModified(bool b = true) { m_bModified = b; }

	virtual void getDefaultRange(Complex range[]) const;

	std::string serialize(std::vector<Json::ValueStringMapping> *pMapping) const;

	// oops, we ended up with 2 fns called "serialize"... get this sorted out TODO
	void Serialize(CArchive& ar);
};



#endif