#include "stdafx.h"
#include "Formulas.h"
#include "SketchFormulas.h"
#include "json/writer.h"
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

using namespace std;


//////////////////////////////////////////////////////////
// streaming

void Plottable::Serialize(CArchive& ar)
{
	if(ar.IsStoring())
	{
		Json::Value root = getJSONValue();

		// this is stupid: CArchive does not play nice with iostream.
		// either do this:

		CFile *pOutFile = ar.GetFile();
		ofstream out(pOutFile->GetFileName());
		Json::StyledStreamWriter writer;
		writer.write(out, root);

		// or create my own ArchiveStreamWriter class...
		//Json::BoogerWriter writer(ar);

	}
	else
	{
		// TODO
		/*CString sz, json;
		while(ar.ReadString(sz)) {
			json += sz;
		}
		Json::Value root(json);
		PlottablesManager::GetInstance().createFromJSON(root);*/
	}
}

std::string Plottable::serialize(std::vector<Json::ValueStringMapping> *pMapping) const
{ 
	Json::StyledWriter writer;
	writer.setLinebreak("\r\n");

	Json::Value obj = getJSONValue();
		
	std::string sz = writer.write(obj);

	if(pMapping) {
		*pMapping = writer.getMapping();
	}

	return sz;
}

void Plottable::getDefaultRange(Complex range[]) const
{
	range[0] = Complex(-2.0, -1.5);
	range[1] = Complex(2.0, 1.5);
}
