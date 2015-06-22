#pragma once
#include "afxwin.h"
#include <vector>
#include "json/writer.h"
#include "PlottablesManager.h"
#include "JsonSerializable.h"	// for ptrtype

// CNewtonSettingsDlg dialog

class Plottable;

class CNewtonSettingsDlg : public CDialog
{
	DECLARE_DYNAMIC(CNewtonSettingsDlg)

public:
	CNewtonSettingsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CNewtonSettingsDlg();

// Dialog Data
	enum { IDD = IDD_NEWTONSETTINGS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);

protected:
	ptrtype m_pfn;			// source object; contents not modified by this class
	bool m_readOnly;		// whether dialog is allowed to make changes to m_pfn
	Json::Value m_json;		// json, reflects current state of edit. in sync with UI, not with pfn.
	std::string m_title;

	std::string m_activeParameter;


protected:
	// UI components and related data

	CEdit m_editInstanceName;

	CListBox m_listParams;
	std::vector<std::string> m_listParamsData;			// json paths for each entry in the list

	CEdit m_editFunction;
	std::vector<Json::ValueStringMapping> m_jsonMap;	// maps json objects to offsets in serialized string shown in m_editFunction

	CStatic m_staticParamName;

	CEdit m_editParamValue;
	bool m_editParamValueChanged;

	CListBox m_classList;
	std::vector<ptrtype> m_classListData;

	CButton m_applyButton;

protected:
	void updateJsonList(void);
	void updateJsonEdit(void);

	enum { EXPAND_ALL, EXPAND_FIRST_ONLY, EXPAND_NONE };
	void JsonToListR(const char* name, const Json::Value &value, std::string jsonPath, int expandNamedInstances, int level);
	void AddJsonListEntry(const std::string &sz, const std::string &jsonPath);

	virtual BOOL OnInitDialog();
	void OnCancel();
	void OnOK();


	BOOL OnEditFunctionDoubleClicked();

public:
	bool setActiveParameter(std::string jsonPath);

	void setFunction(ptrtype pfn, bool readOnly);
	ptrtype getSourceFunction() const { return m_pfn; }

	ptrtype saveAsCopy();
	//ptrtype applyChanges();
	void revert();

protected:
	bool commitChanges();
	void populateLibrary(std::string className = "");

public:
	afx_msg void OnBnClickedSaveAsCopy();
	afx_msg void OnBnClickedOk();
	afx_msg void OnLbnSelchangeJsonList();
	afx_msg void OnEnKillfocusEdit1();
	afx_msg void OnLbnDblclkJsonList();
	afx_msg void OnLbnSetfocusJsonList();
	afx_msg void OnEnChangeEditParameter();
	afx_msg void OnEnChangeEditFunction();
	afx_msg void OnLbnSelchangeClassList();
	afx_msg void OnEnChangeEditInstancename();
	afx_msg void OnBnClickedRevert();
	afx_msg void OnLbnDblclkClassList();
	CStatic m_staticModified;
	CStatic m_staticLibraryTitle;
	CStatic m_staticInvalidJson;
};

// misplaced Json helper functions
Json::Value* dereferenceJsonPath(Json::Value *p, std::string jsonPath);
Json::Value GetValueAtJsonPath(Json::Value *root, std::string path);
bool SetValueAtJsonPath(Json::Value *root, std::string path, const char* value);
bool SetJsonValueAtJsonPath(Json::Value *root, std::string path, Json::Value value);
