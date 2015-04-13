#ifndef __KMeanProjectMgr_h__
#define __KMeanProjectMgr_h__

#include <IceUtil/IceUtil.h>
#include <Ice/Ice.h>
#include <FCDCentralService.h>
#include <KMeanService.h>
#include <KMeanWorkItem.h>
#include <KMeanProject.h>

class CGlobalVars;
class CKMeanProjectMgr
{
public:
	CKMeanProjectMgr(CGlobalVars& gv);
	~CKMeanProjectMgr();

public:

	void Initilize();
	
	void UnInitilize();

	::Ice::Int CreateProjectAndWaitForContractors(
			const ::iBS::KMeanProjectInfoPtr& rqstProjectInfo,
            ::iBS::KMeanProjectInfoPtr& retProjectInfo);

	CKMeanProjectL2Ptr GetKMeanProjectL2ByProjectID(int projectID);

	::Ice::Int DestroyProject(int projectID);

private:

	
private:
	CGlobalVars&	m_gv;
	typedef std::map<int, CKMeanProjectL2Ptr> KMeanProjectL2PtrMap_T;

	KMeanProjectL2PtrMap_T m_l2Projects;
	int m_currProjectMaxID;
	IceUtil::Mutex				m_mutex;
};
#endif

