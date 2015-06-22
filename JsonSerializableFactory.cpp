#include "stdafx.h"
#include "JsonSerializable.h"


void JsonSerializableFactory::registerClass(ptrtype p)
{
	ASSERT(p);
	ptrtype pTemp = p->createInstance();
	ASSERT(pTemp);

	_registeredClasses[pTemp->getClassName()] = p;
}

ptrtype JsonSerializableFactory::createFromJSON(const Json::Value &root)
{
	const char* className = NULL;

	if(!root || !root.isObject()) {
		throw("Invalid JSON: object expected");
		return NULL;
	}
	if(!root["_class"].isString()) {
		throw("Invalid JSON: no classname");
		return NULL;
	}

	className = root["_class"].asCString();

	if(!className || !strcmp(className, "")) {
		throw("Invalid JSON: no classname");
		return NULL;
	}
	
	ptrtype pfn = createInstance(className);

	if(pfn) {
		pfn->fromJSONValue(root, *this);

		if(root["_id"].isString()) {
			const char *instanceName = 	className = root["_id"].asCString();
			pfn->setInstanceName(instanceName);
		}

	}

	return pfn;
}

ptrtype JsonSerializableFactory::createInstance(LPCSTR className)
{
	// instantiate named class

	ptrtype registeredClassInstance = _registeredClasses[className];
	if(registeredClassInstance) {
		return registeredClassInstance->createInstance();
	}
	return NULL;
}
