#ifndef __RUVDB_h__
#define __RUVDB_h__

#include <IceUtil/Mutex.h>
#include <Ice/Ice.h>
#include <Freeze/Freeze.h>
#include <FCDCentralService.h>
#include <FCDCentralFreeze.h>
#include <RUVFreezeMap.h>

typedef iBS::CRUVFreezeMap RUVFreezeMap_T;

class CRUVFacetDB
{
public:
	CRUVFacetDB(Ice::CommunicatorPtr communicator, const std::string& strEnvName, 
		const std::string& strDBName);

	~CRUVFacetDB();

public:
	::Ice::Int RqstNewRUVFacet(::iBS::RUVFacetInfo& rfi);
	::Ice::Int RemoveRUVFacetInfo(Ice::Int facetID);
	::Ice::Int GetRUVFacetInfo(Ice::Int facetID, ::iBS::RUVFacetInfo& rfi);
	::Ice::Int SetRUVFacetInfo(Ice::Int facetID, const ::iBS::RUVFacetInfo& rfi);
	void SetRUVFacetInfoBlank(::iBS::RUVFacetInfo& rfi);

private:
	void Initilize();

private:
	Ice::CommunicatorPtr		m_communicator;
	Freeze::ConnectionPtr		m_connection;
	const int theMaxThreadRandomIdx;
	int							m_facetIDMax; //current max facet ID
	RUVFreezeMap_T	m_map;
	IceUtil::Mutex				m_mutex;
};

#endif

