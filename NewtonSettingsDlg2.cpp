// NewtonSettingsDlg2.cpp : implementation file
//

#include "stdafx.h"
#include "NewtonBasin.h"
#include "NewtonSettingsDlg2.h"
#include "formulas.h"
#include "json/writer.h"
#include "PlottablesManager.h"
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/foreach.hpp>

// CNewtonSettingsDlg dialog

IMPLEMENT_DYNAMIC(CNewtonSettingsDlg, CDialog)
CNewtonSettingsDlg::CNewtonSettingsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNewtonSettingsDlg::IDD, pParent)
{
	m_pfn = NULL;
	m_json = NULL;
	m_editParamValueChanged = false;
	m_activeParameter = "";
	ASSERT(m_activeParameter.empty());
}

CNewtonSettingsDlg::~CNewtonSettingsDlg()
{
}

void CNewtonSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	CString sz;
	if(pDX->m_bSaveAndValidate)
	{
	}
	else
	{
	}
	DDX_Control(pDX, IDC_PARAM_LIST, m_listParams);
	DDX_Control(pDX, IDC_EDIT_PARAMVALUE, m_editParamValue);
	DDX_Control(pDX, IDC_STATIC_PARAMNAME, m_staticParamName);
	DDX_Control(pDX, IDC_EDIT_FUNCTION, m_editFunction);
	DDX_Control(pDX, IDC_CLASS_LIST, m_classList);
	DDX_Control(pDX, IDC_EDIT_INSTANCENAME, m_editInstanceName);
	DDX_Control(pDX, IDOK, m_applyButton);
	DDX_Control(pDX, IDC_STATIC_MODIFIED, m_staticModified);
	DDX_Control(pDX, IDC_STATIC_LIBRARY_TITLE, m_staticLibraryTitle);
	DDX_Control(pDX, IDC_STATIC_INVALID_JSON, m_staticInvalidJson);
}


BEGIN_MESSAGE_MAP(CNewtonSettingsDlg, CDialog)
	ON_BN_CLICKED(IDC_ADD, OnBnClickedSaveAsCopy)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_LBN_SETFOCUS(IDC_PARAM_LIST, OnLbnSetfocusJsonList)
	ON_LBN_SELCHANGE(IDC_PARAM_LIST, OnLbnSelchangeJsonList)
	ON_LBN_DBLCLK(IDC_PARAM_LIST, OnLbnDblclkJsonList)
	ON_EN_CHANGE(IDC_EDIT_FUNCTION, OnEnChangeEditFunction)
	ON_EN_KILLFOCUS(IDC_EDIT_PARAMVALUE, OnEnKillfocusEdit1)
	ON_EN_CHANGE(IDC_EDIT_PARAMVALUE, OnEnChangeEditParameter)
	ON_LBN_SELCHANGE(IDC_CLASS_LIST, &CNewtonSettingsDlg::OnLbnSelchangeClassList)
	ON_EN_CHANGE(IDC_EDIT_INSTANCENAME, &CNewtonSettingsDlg::OnEnChangeEditInstancename)
	ON_BN_CLICKED(IDCANCEL, &CNewtonSettingsDlg::OnBnClickedRevert)
	ON_LBN_DBLCLK(IDC_CLASS_LIST, &CNewtonSettingsDlg::OnLbnDblclkClassList)
END_MESSAGE_MAP()


// CNewtonSettingsDlg message handlers

BOOL CNewtonSettingsDlg::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_LBUTTONDBLCLK 
		&& pMsg->hwnd == m_editFunction.m_hWnd)
	{
		return OnEditFunctionDoubleClicked();
	}

	return FALSE;
}

void CNewtonSettingsDlg::OnCancel()
{
	TRACE("OnCancel\n");
}

// 'revert'
void CNewtonSettingsDlg::OnBnClickedRevert()
{
	//CDialog::OnCancel();
	revert();
}

void CNewtonSettingsDlg::revert()
{
	this->setFunction(m_pfn, m_readOnly);
}

// 'apply'
void CNewtonSettingsDlg::OnOK()
{
}

BOOL CNewtonSettingsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

//	UpdateData(FALSE);
	ASSERT_VALID(this);

	//setFunction(m_pfn);
	m_classList.SetTabStops(80);
	populateLibrary();

	m_staticInvalidJson.ShowWindow(SW_HIDE);
	m_staticModified.ShowWindow(SW_HIDE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CNewtonSettingsDlg::setFunction(ptrtype pfn, bool readOnly)
{
	m_pfn = pfn;
	m_readOnly = readOnly;

	m_applyButton.EnableWindow(!readOnly);

	if(!pfn) {
		//this->EnableWindow(FALSE);	// don't do this
		return;
	}

	ASSERT(dynamic_cast<Plottable*>(pfn.get()));

	std::string instanceName = pfn->getInstanceName().empty() ? "" : pfn->getInstanceName();

	m_title = instanceName + " : " + ((pfn->getClassName()));
	SetWindowText(S2WS(m_title));

	m_json = pfn->getJSONValue();
	ASSERT(m_json.isObject());
	updateJsonList();
	updateJsonEdit();

	m_editInstanceName.SetWindowText(S2WS(instanceName));

}

/*ptrtype CNewtonSettingsDlg::applyChanges()
{
	if(m_readOnly) {
		::MessageBoxA(this->GetSafeHwnd(), "Function is read-only.", "Read-only", MB_OK);
	}
	else {
		setFunction(this->saveAsCopy());

		UpdateData(TRUE);
	}

	ASSERT_VALID(this);

	EndDialog(IDC_ADD);
}*/

ptrtype CNewtonSettingsDlg::saveAsCopy()
{
	commitChanges();

	ptrtype pfn = theApp.getFunctionFactory()->createFromJSON(m_json);
	ASSERT(pfn);

	CString sz;
	m_editInstanceName.GetWindowText(sz);
	if(!sz.IsEmpty()) {
		pfn->setInstanceName(WS2S(sz));
	}

	return pfn;
}

void CNewtonSettingsDlg::updateJsonList(void)
{
	int n = m_listParams.GetCurSel();

	m_listParams.ResetContent();
	m_listParamsData.clear();

	JsonToListR(NULL, m_json, "", EXPAND_FIRST_ONLY, 0);

	m_listParams.SetCurSel(n);
	OnLbnSelchangeJsonList();
}

void CNewtonSettingsDlg::updateJsonEdit(void)
{
	Json::StyledWriter writer;
	writer.setLinebreak("\r\n");

	std::string sz = writer.write(m_json);

	m_jsonMap = writer.getMapping();

	m_editFunction.SetWindowText(S2WS(sz));
}

/**
	Populates a list control with a JSON object, giving each name/value pair its own row.
	Indentation added, and extra punctuation omitted, for easier reading.

	key and value refer to this object's JSON key and value
	jsonPath, e.g. "[somekey][otherkey]" represents this value's path within the main JSON object
	expandNamedInstances: whether to show full list for sub-objects with an "_id" key
*/
void CNewtonSettingsDlg::JsonToListR(const char* key, const Json::Value &value, 
	std::string jsonPath, int expandNamedInstances, int level )
{
	// objects aren't required to have a key
	ASSERT(key || value.isObject());

	bool expandNamedObjectReferences = (expandNamedInstances==EXPAND_FIRST_ONLY || expandNamedInstances==EXPAND_ALL);
	if(expandNamedInstances==EXPAND_FIRST_ONLY) {
		expandNamedInstances = EXPAND_NONE;
	}

	std::string prefix = "  ";
	for(int i=0; i<level; ++i) {
		prefix += "   ";
	}

	switch ( value.type() )
	{
	case Json::nullValue:
		AddJsonListEntry(prefix + key +": null", jsonPath);
		break;
	case Json::intValue:
		AddJsonListEntry(prefix + key +": "+Json::valueToString( value.asInt() ), jsonPath);
		break;
	case Json::uintValue:
		AddJsonListEntry(prefix + key +": "+Json::valueToString( value.asUInt() ), jsonPath);
		break;
	case Json::realValue:
		AddJsonListEntry(prefix + key +": "+Json::valueToString( value.asDouble() ), jsonPath);
		break;
	case Json::stringValue:
		AddJsonListEntry(prefix + key +": "+Json::valueToQuotedString( value.asCString() ), jsonPath);
		break;
	case Json::booleanValue:
		AddJsonListEntry(prefix + key +": "+Json::valueToString( value.asBool() ), jsonPath);
		break;
	case Json::arrayValue:
		{
			AddJsonListEntry(prefix + key +":", jsonPath);
			int size = value.size();
			for ( int index =0; index < size; ++index )
			{
				JsonToListR("", value[index], jsonPath+"["+Json::valueToString(index)+"]", expandNamedInstances, level+1 );
			}
		}
		break;
	case Json::objectValue:
		{
			if(value["_class"].isString() && value["_id"].isString() && !expandNamedObjectReferences) {
				// object is a named reference--show only the reference name
				std::string className = value["_class"].asCString();
				std::string instanceName = value["_id"].asCString();
				AddJsonListEntry(prefix + key + ": {" + instanceName + ": " + className + "}", jsonPath);
			}
			else {
				// list all object members

				Json::Value::Members members( value.getMemberNames() );
				if(key) {
					AddJsonListEntry(prefix + key +":", jsonPath);
				}

				// for each key in the set of key/value pairs...
				for ( Json::Value::Members::iterator it = members.begin(); 
					it != members.end(); 
					++it )
				{
					const std::string &key = *it;
					JsonToListR(key.c_str(), value[key], jsonPath+"["+key+"]", expandNamedInstances, level+1);
				}
			}
		}
		break;
	}
}

void CNewtonSettingsDlg::AddJsonListEntry( const std::string &sz, const std::string &jsonPath )
{
	ASSERT(m_listParams.GetCount() == m_listParamsData.size());
	m_listParams.AddString(S2WS(sz));
	m_listParamsData.push_back(jsonPath);
}


/////////////


void CNewtonSettingsDlg::OnLbnSetfocusJsonList()
{
	commitChanges();
}

// different parameter selected: put the param name
// and current value into the edit control
void CNewtonSettingsDlg::OnLbnSelchangeJsonList()
{
	int i = m_listParams.GetCurSel();


	if(i<0)
	{
		populateLibrary();
		m_staticParamName.SetWindowText(L"---");
		m_editParamValue.SetWindowText(L"---");
		m_editParamValue.EnableWindow(FALSE);
		m_editParamValueChanged = false;
		return;
	}

	std::string jsonPath = m_listParamsData[i];

	setActiveParameter(jsonPath);
}

void CNewtonSettingsDlg::OnLbnDblclkJsonList()
{
	if(m_listParams.GetCurSel() >= 0)
		m_editParamValue.SetFocus();
}

// user finished editing value: get new value
// and store it in m_newValues
//
// problem: this notification is called AFTER
// the other control is activated: so if OK is
// activated, say with the enter key,
// this message arrives afterward, too late.
void CNewtonSettingsDlg::OnEnKillfocusEdit1()
{
}

// user finished by selecting "Save as Copy"
void CNewtonSettingsDlg::OnBnClickedSaveAsCopy()
{
	commitChanges();

	theApp.applyNewFunctionSettings(true);
}

// user finished by selecting "Apply"
void CNewtonSettingsDlg::OnBnClickedOk()
{
	commitChanges();
	
	theApp.applyNewFunctionSettings(false);
}



void CNewtonSettingsDlg::OnEnChangeEditParameter()
{
	m_editParamValueChanged = true;
}

void CNewtonSettingsDlg::OnEnChangeEditFunction()
{
	CString sz;
	m_editFunction.GetWindowTextW(sz);

	Json::Reader reader;
	Json::Value json;
	if(reader.parse(WS2S(sz), json, true)) {
		m_json = json;
		try {
		}
		catch(const char *e) {
			TRACE("Exception: %s\n", e);
		}
		m_staticInvalidJson.ShowWindow(SW_HIDE);
	}
	else {
		TRACE("JSON edits are not valid...\n");
		m_staticInvalidJson.ShowWindow(SW_SHOW);
	}

	this->updateJsonList();
}


BOOL CNewtonSettingsDlg::OnEditFunctionDoubleClicked()
{
	int start, end;
	m_editFunction.GetSel(start, end);
	CString sz;
	m_editFunction.GetWindowTextW(sz);

	while(start > 0 && ::isalnum(sz[start-1])) --start;
	while(end < sz.GetLength() && ::isalnum(sz[end])) ++end;

	// innermost PAIR or VALUE that the user has selected (we don't care about NAMEs)
	Json::ValueStringMapping innermost = {Json::ValueStringMapping::NAME, "", NULL, -1, INT_MAX};
	std::string path = "";
	for(std::vector<Json::ValueStringMapping>::iterator it = m_jsonMap.begin(); it != m_jsonMap.end(); ++it) {
		const Json::Value &value = (*it).value;

		// for each JSON map element whose string range overlaps the selection...
		if((*it).start <= start && (*it).end >= end) {

			// if the map element is a name/value pair, prepend it to our path as "[42]" or "[somename]" as applicable.
			if((*it).type == Json::ValueStringMapping::PAIR) {
				
				std::string name = "";
				switch((*it).name.type()) {
				case Json::intValue:
				case Json::uintValue:
					name = Json::valueToString((*it).name.asInt());
					break;

				case Json::stringValue:
					name = (*it).name.asCString();
					break;

				default:
					ASSERT(false);
				}

				path = "[" + name + "]" + path;
			}

			// if it's a better fit for innermost, replace it.
			if((*it).type != Json::ValueStringMapping::NAME) {
				if((*it).start >= innermost.start && (*it).end <= innermost.end) {
					innermost = *it;
				}
			}
		}

		switch (value.type()) {
			case Json::ValueType::objectValue:
			{
				const Json::Value& className = value["_class"];
				TRACE("  %d-%d: %s\n", (*it).start, (*it).end, className.asCString());
			}
			break;

			case Json::ValueType::arrayValue:
				value[Json::UInt(0)];
				TRACE("  %d-%d: array[%d]\n", (*it).start, (*it).end, (value.size()));
				break;

			case Json::ValueType::realValue:
			case Json::ValueType::stringValue:
				break;

			default:
				TRACE("  %d-%d: unknown type: %d\n", (*it).start, (*it).end, (value.type()));
		}

	}

	CString x = sz.Mid(innermost.start, innermost.end-innermost.start);

	m_editFunction.SetSel(innermost.start, innermost.end);
	
	if(setActiveParameter(path.c_str())) {
		m_editParamValue.SetFocus();
	}

	return TRUE;	// handled
}



/////////////////////////////////////////////////////////
// data


bool CNewtonSettingsDlg::setActiveParameter(std::string jsonPath)
{
	ASSERT(m_pfn);
	ASSERT(!jsonPath.empty());

	if(!m_activeParameter.compare(jsonPath))
		return false;	// no change

	commitChanges();

	m_activeParameter = jsonPath;

	m_staticParamName.SetWindowText(S2WS(m_activeParameter + ":"));

	const Json::Value& value = GetValueAtJsonPath(&m_json, jsonPath);

	m_editParamValue.EnableWindow(TRUE);

	populateLibrary();

	// the selected parameter might be:
	// - a simple value: string, int, double
	// - an object/custom value: complex, color, colorset

	bool isClass = false;
	std::string classObjectPath = jsonPath;
	if(jsonPath.length() >= 8 && !jsonPath.substr(jsonPath.length()-8).compare("[_class]")) {
		isClass = true;
		classObjectPath = jsonPath.substr(0, jsonPath.length()-8);
	}
	if(value.type() == Json::objectValue) {
		isClass = true;
	}

	if(isClass) {
		m_editParamValue.EnableWindow(FALSE);
		
		// ask JsonSerializable-derived object what type this path requires
		std::string requiredClass = m_pfn->getRequiredMemberClass(classObjectPath.c_str());

		populateLibrary(requiredClass);
	}
	else {
		switch(value.type()) {
		case Json::stringValue:
			m_editParamValue.SetWindowText(S2WS(value.asCString()));
			break;

		case Json::intValue:
		case Json::uintValue:
			m_editParamValue.SetWindowText(S2WS(Json::valueToString(value.asInt())));
			break;

		case Json::realValue:
			m_editParamValue.SetWindowText(S2WS(Json::valueToString(value.asDouble()).c_str()));
			break;

		default:
			m_editParamValue.SetWindowText(L"NA");
		}
	}

	m_editParamValue.SetSel(0, -1);
	m_editParamValueChanged = false;

	return true;
}

void CNewtonSettingsDlg::populateLibrary(std::string className)
{
	if(className.empty()) {
		m_staticLibraryTitle.SetWindowText(L"");
		m_classList.ResetContent();
		m_classListData.clear();
	}
	else {
		CString title;
		title.Format(L"Library - objects of type '%s'", S2WS(className));
		m_staticLibraryTitle.SetWindowText(title);

		const std::vector<ptrtype> *pLibrary = theApp.getFunctionFactory()->getLibrary();
		
		for(unsigned i=0; i<pLibrary->size(); ++i) {
			ptrtype p = (*pLibrary)[i];
			if(p->instanceOf(className.c_str())) {
				int n = m_classList.AddString(S2WS(
					(p->getInstanceName() + std::string("\t") + p->getClassName()).c_str()));
				m_classListData.push_back(p);
			}
		}
	}
}

bool CNewtonSettingsDlg::commitChanges(void)
{
	if(m_activeParameter.empty() || !m_editParamValueChanged) {
		return false;
	}

	CString sz;
	m_editParamValue.GetWindowText(sz);
	//Json::Value newValue = WS2S(sz);
	SetValueAtJsonPath(&m_json, m_activeParameter, WS2S(sz).c_str());

	m_editParamValueChanged = false;
	updateJsonList();
	updateJsonEdit();
	return true;
}

//
//	these helper functions should really go in json libs.
//	however, first investigate the "Path" class there, see if this is redundant.
//

Json::Value* dereferenceJsonPath(Json::Value *p, std::string jsonPath)
{
	Json::UInt n;
	std::string keyName;

	while(!jsonPath.empty()) {
		// get next keyname and remove from string
		int a = (jsonPath.find_first_of("["));
		int b = (jsonPath.find_first_of("]"));
		keyName = jsonPath.substr(a+1, b-a-1);
		jsonPath = jsonPath.substr(b+1);

		// do some checking when dereferencing an object vs an array
		switch(p->type()) {
		case Json::arrayValue:
			
			n = atoi(keyName.c_str());
			p = &((*p)[n]);
			break;

		case Json::nullValue:
		case Json::objectValue:
			p = &((*p)[keyName.c_str()]);
			break;

		default:
			TRACE("Dead end.");
			return NULL;
		}
	}

	return p;
}

Json::Value GetValueAtJsonPath(Json::Value *p, std::string jsonPath)
{
	p = ::dereferenceJsonPath(p, jsonPath);

	return *p;
}

bool SetValueAtJsonPath(Json::Value *p, std::string jsonPath, const char* value)
{
	Json::UInt n;

	p = ::dereferenceJsonPath(p, jsonPath);

	// convert string to match existing type
	switch(p->type()) {
	case Json::stringValue:
		*p = Json::Value(value);
		break;

	case Json::intValue:
	case Json::uintValue:
		*p = Json::Value(atoi(value));
		break;

	case Json::realValue:
		*p = Json::Value(atof(value));
		break;

	default:
		TRACE("Can't change value of type %d\n", p->type());
		return false;
	}

	return true;
}

bool SetJsonValueAtJsonPath(Json::Value *p, std::string jsonPath, Json::Value value)
{
	Json::UInt n;

	p = ::dereferenceJsonPath(p, jsonPath);
	ASSERT(p);

	// stupid test to make sure json type remains constant
	ASSERT(p->isArray() == value.isArray());
	ASSERT(p->isBool() == value.isBool());
	ASSERT(p->isDouble() == value.isDouble());
	ASSERT(p->isInt() == value.isInt());
	ASSERT(p->isUInt() == value.isUInt());
	ASSERT(p->isNumeric() == value.isNumeric());
	ASSERT(p->isObject() == value.isObject());
	ASSERT(p->isString() == value.isString());
	ASSERT(p->type() == value.type());
	*p = value;
	return true;
}

void CNewtonSettingsDlg::OnLbnSelchangeClassList()
{
	// TODO: Add your control notification handler code here
}

void CNewtonSettingsDlg::OnLbnDblclkClassList()
{
	int paramIndex = m_listParams.GetCurSel();
	std::string path = m_listParamsData[paramIndex];

	int libraryIndex = m_classList.GetCurSel();
	ptrtype pfn = m_classListData[libraryIndex];

	std::string requiredClass = m_pfn->getRequiredMemberClass(path);
	TRACE("  ** setting %s to %s : %s **\n", path.c_str(), pfn->getInstanceName().c_str(), pfn->getClassName());
	TRACE("  ** required type: %s\n", requiredClass.c_str());

	if(!path.empty() && pfn) {
		if(pfn->instanceOf(requiredClass.c_str())) {
			::SetJsonValueAtJsonPath(&m_json, path, pfn->getJSONValue());
			updateJsonEdit();
			updateJsonList();
		}
		else {
			CString sz;
			sz.Format(L"Unable to set param %s to value of type %s. Type %s is required.",
				S2WS(path), S2WS(pfn->getClassName()), S2WS(requiredClass));
			MessageBox(sz);
		}
	}
	else {
		TRACE("Nothing selected.");
	}
}

void CNewtonSettingsDlg::OnEnChangeEditInstancename()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}




