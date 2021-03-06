// ======================================================================
//
// ObjvarNameManager.h
// copyright (c) 2003 Sony Online Entertainment
//
// ======================================================================

#ifndef INCLUDED_ObjvarNameManager_H
#define INCLUDED_ObjvarNameManager_H

// ======================================================================

namespace DB
{
	class TaskRequest;
}

// ======================================================================

class ObjvarNameManager
{
  public:
	static void install();
	static ObjvarNameManager &getInstance();
	static ObjvarNameManager &getGoldInstance();
		
  public:
	int getOrAddNameId(const std::string &name);
	int getNameId(const std::string &name) const;
	bool getName(int id, std::string &name) const;
	void addNameMapping(int id, const std::string &name);
	void allNamesLoaded();

	DB::TaskRequest *saveNewNames();
	
  private:
	typedef stdhash_map<std::string, int>::fwd NameToIdMapType;
	NameToIdMapType *m_nameToIdMap;

	typedef stdhash_map<int, std::string>::fwd IdToNameMapType;
	IdToNameMapType *m_idToNameMap;

	typedef stdvector<std::pair<int, std::string> >::fwd NewNamesType;
	NewNamesType *m_newNames;
	
	int m_nextAvailableId;
	bool m_namesLoaded;

  private:
	static void remove();
	ObjvarNameManager();
	~ObjvarNameManager();
	
  private:
	static ObjvarNameManager *ms_instance;
	static ObjvarNameManager *ms_goldInstance;
};

// ======================================================================

inline ObjvarNameManager &ObjvarNameManager::getInstance()
{
	NOT_NULL(ms_instance);
	return *ms_instance;
}

// ----------------------------------------------------------------------

inline ObjvarNameManager &ObjvarNameManager::getGoldInstance()
{
	NOT_NULL(ms_goldInstance);
	return *ms_goldInstance;
}

// ======================================================================

#endif
