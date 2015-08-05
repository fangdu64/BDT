#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <IceUtil/Time.h>
#include <IceUtil/FileUtil.h>
#include <FeatureValueStoreMgr.h>
#include <FeatureValueRAM.h>
#include <algorithm>    // std::numeric_limits
#include <limits>       // std::numeric_limits
#include <bdtUtil/SortHelper.h>

#ifdef _WIN32
#   include <io.h>
#else
#   include <unistd.h>
#endif

CFeatureValueStoreMgr::CFeatureValueStoreMgr(Ice::Long maxFeatureValueFileSize, const string& rootdir)
:m_theMaxFeatureValueFileSize(maxFeatureValueFileSize), m_rootDir(rootdir)
{
	m_theMaxFeatureValueCntDouble = m_theMaxFeatureValueFileSize / sizeof(Ice::Double);
	m_theMaxFeatureValueCntInt32 = m_theMaxFeatureValueFileSize / sizeof(Ice::Int);
}


CFeatureValueStoreMgr::~CFeatureValueStoreMgr()
{
}

void CFeatureValueStoreMgr::Initilize()
{
	//cout<<"CFeatureValueStoreMgr Initilize begin ..."<<endl; 

	//cout<<"CFeatureValueStoreMgr Initilize End"<<endl;
}


void CFeatureValueStoreMgr::UnInitilize()
{
	//cout<<"CFeatureValueStoreMgr UnInitilize begin ..."<<endl; 
	
	//cout<<"CFeatureValueStoreMgr UnInitilize End"<<endl;
}


bool CFeatureValueStoreMgr::StoreFileExist(const iBS::FeatureObserverSimpleInfoPtr& foi)
{
	string filename=GetStoreFilePath(foi);
	return IceUtilInternal::fileExists(filename);
}

size_t CFeatureValueStoreMgr::GetSizeOfValueType(iBS::FeatureValueEnum valueType)
{
	if(valueType==iBS::FeatureValueDouble)
	{
		return sizeof(::Ice::Double);
	}
	else if(valueType==iBS::FeatureValueFloat)
	{
		return sizeof(::Ice::Float);
	}
	else if(valueType==iBS::FeatureValueInt32)
	{
		return sizeof(::Ice::Int);
	}
	else if(valueType==iBS::FeatureValueInt64)
	{
		return sizeof(::Ice::Long);
	}
	else 
	{
		return 0;
	}
}

string CFeatureValueStoreMgr::GetStoreFilePath(const iBS::FeatureObserverSimpleInfoPtr& foi)
{
	if (foi->StoreLocation == iBS::FeatureValueStoreLocationSpecified)
	{
		return foi->SpecifiedPathPrefix + ".bfv";
	}

	if(foi->StorePolicy==iBS::FeatureValueStorePolicyBinaryFilesSingleObserver)
	{
		//m_rootDir should be absolute path

		 ostringstream os;
		 os<<m_rootDir<<"/oid_"<<foi->ObserverID<<".bfv";
		
		 return os.str();
	}
	else if(foi->StorePolicy==iBS::FeatureValueStorePolicyBinaryFilesObserverGroup)
	{
		
		 ostringstream os;
		 os<<m_rootDir<<"/gid_"<<foi->ObserverGroupID<<".bfv";
		
		 return os.str();
	}
	else
	{
		return "";
	}
}



//for foi  >1G
bool CFeatureValueStoreMgr::StoreFileExist(const iBS::FeatureObserverSimpleInfoPtr& foi, 
		Ice::Long featureIdxFrom, Ice::Long featureIdxTo)
{
	return false;
}


string CFeatureValueStoreMgr::GetBatchStoreFilePath(
		const iBS::FeatureObserverSimpleInfoPtr& foi, 
		Ice::Long batchIdx)
{
	if (foi->StoreLocation == iBS::FeatureValueStoreLocationSpecified)
	{
		// attached matrix with multiple batches must follow the _batchIdx.bfv name convention
		ostringstream os;
		os << foi->SpecifiedPathPrefix << "_" << batchIdx << ".bfv";
		return os.str();
	}

	if(foi->StorePolicy==iBS::FeatureValueStorePolicyBinaryFilesSingleObserver)
	{
		//m_rootDir should be absolute path

		 ostringstream os;
		 os<<m_rootDir<<"/oid_"<<foi->ObserverID<<"_"<<batchIdx<<".bfv";
		
		 return os.str();
	}
	else if(foi->StorePolicy==iBS::FeatureValueStorePolicyBinaryFilesObserverGroup)
	{
		
		 ostringstream os;
		 os<<m_rootDir<<"/gid_"<<foi->ObserverID<<"_"<<batchIdx<<".bfv";
		
		 return os.str();
	}
	else
	{
		return "";
	}
}

//for foi  >1G
int CFeatureValueStoreMgr::GetStoreFilePath(const iBS::FeatureObserverSimpleInfoPtr& foi, 
	Ice::Long featureIdxFrom, Ice::Long featureIdxTo, 
	Ice::Long& featureCntPerBatch,
	StringVec& files, LongVec& batchFileIdxs, LongVec& posFroms, LongVec& posTos)
{
	size_t szValueType = GetSizeOfValueType(foi->ValueType);

	//in bytes
	Ice::Long batchFileSize= m_theMaxFeatureValueFileSize;
	Ice::Long rowBytesSize = foi->ObserverGroupSize* szValueType;

	if(batchFileSize%rowBytesSize!=0)
	{
		batchFileSize-=(batchFileSize%rowBytesSize); //a data row will not split in two batch files
	}
	Ice::Long valueCntPerBatchFile = batchFileSize / szValueType; //each value is a double
	featureCntPerBatch=valueCntPerBatchFile;
	Ice::Long batchFileIdxFrom=featureIdxFrom/valueCntPerBatchFile;
	Ice::Long batchFileIdxTo=(featureIdxTo-1)/valueCntPerBatchFile;

	for(Ice::Long i=batchFileIdxFrom;i<=batchFileIdxTo;i++)
	{
		batchFileIdxs.push_back(i);
		files.push_back(GetBatchStoreFilePath(foi,i));
		//position is in double*
		if(i==batchFileIdxFrom)
		{
			posFroms.push_back(featureIdxFrom%valueCntPerBatchFile);
		}
		else
		{
			posFroms.push_back(0);
		}

		if(i==batchFileIdxTo)
		{
			Ice::Long posEnd=((featureIdxTo-1)%valueCntPerBatchFile)+1;
			posTos.push_back(posEnd); //not including
		}
		else
		{
			posTos.push_back(valueCntPerBatchFile); //not including
		}
	}

	return 0;
}

string CFeatureValueStoreMgr::GetRootDir()
{ 
	string rootDir = m_rootDir;
	if (!IceUtilInternal::isAbsolutePath(m_rootDir))
	{
		string strCwd;
		IceUtilInternal::getcwd(strCwd);
		ostringstream os;
		os << strCwd << "/" << m_rootDir;
		rootDir = os.str();
	}
	return rootDir;
}

string CFeatureValueStoreMgr::GetStoreFilePathPrefix(const iBS::FeatureObserverSimpleInfoPtr& foi)
{
	string rootDir = m_rootDir;
	if (!IceUtilInternal::isAbsolutePath(m_rootDir))
	{
		string strCwd;
		IceUtilInternal::getcwd(strCwd);
		ostringstream os;
		os << strCwd << "/" << m_rootDir;
		rootDir = os.str();
	}
	if (foi->StoreLocation == iBS::FeatureValueStoreLocationSpecified)
	{
		ostringstream os;
		os << foi->SpecifiedPathPrefix;
		return os.str();
	}

	if (foi->StorePolicy == iBS::FeatureValueStorePolicyBinaryFilesSingleObserver)
	{
		//m_rootDir should be absolute path

		ostringstream os;
		os << rootDir << "/oid_" << foi->ObserverID;

		return os.str();
	}
	else if (foi->StorePolicy == iBS::FeatureValueStorePolicyBinaryFilesObserverGroup)
	{

		ostringstream os;
		os << rootDir << "/gid_" << foi->ObserverGroupID;

		return os.str();
	}
	else
	{
		return "";
	}
}

bool CFeatureValueStoreMgr::SaveFeatureValueToStoreDouble(
		const iBS::FeatureObserverSimpleInfoPtr& foi,
		::Ice::Long localFeatureIdxFrom,
		::Ice::Long localFeatureIdxTo,
		::Ice::Double *values,
		const string& batchFileName, Ice::Long featureCntPerBatch)
{
	if(!IceUtilInternal::fileExists(batchFileName) && featureCntPerBatch>(localFeatureIdxTo-localFeatureIdxFrom) )
	{
		//first time write, fill with full values of NaN
		::IceUtil::ScopedArray<Ice::Double> saNanValues(new ::Ice::Double[featureCntPerBatch]);
		if(!saNanValues.get())
		{
			return false;
		}
		std::pair<Ice::Double*, Ice::Double*> nanValues(
			saNanValues.get(), saNanValues.get()+featureCntPerBatch);

		::Ice::Double nanVal = std::numeric_limits< ::Ice::Double >::quiet_NaN();

		std::fill(nanValues.first, nanValues.second, nanVal);

		std::copy(values, values+(localFeatureIdxTo-localFeatureIdxFrom), nanValues.first+localFeatureIdxFrom);

		return SaveFeatureValueToStoreDouble(foi,0,featureCntPerBatch,nanValues.first,batchFileName,featureCntPerBatch);

	}

	string mode="r+b";
	if(localFeatureIdxTo-localFeatureIdxFrom==featureCntPerBatch)
	{
		mode="wb";
	}

    FILE* fp = IceUtilInternal::fopen(batchFileName, mode);
    if(fp == 0)
    {
        return  false;
    }

	if(localFeatureIdxFrom>0)
	{
		if(fseek(fp,localFeatureIdxFrom*sizeof(Ice::Double),SEEK_SET )!=0)
		{
			fclose(fp);
			//report problem
			return 0;
		}
	}

	size_t remainSize=localFeatureIdxTo - localFeatureIdxFrom;
	size_t batchSize=1024*1024; //1M
	cout<<IceUtil::Time::now().toDateTime()<<" oid="<<foi->ObserverID<<" SaveFeatureValueToStoreDouble begin ..."<<endl; 

	while(remainSize>0)
	{
		if(remainSize>batchSize)
		{
			fwrite(values,sizeof(::Ice::Double),batchSize,fp);
			values+=batchSize;
			remainSize-=batchSize;
		}
		else
		{
			fwrite(values,sizeof(::Ice::Double),remainSize,fp);
			remainSize=0;
		}
		
	}
    fclose(fp);
	cout<<IceUtil::Time::now().toDateTime()<<" oid="<<foi->ObserverID<<" SaveFeatureValueToStoreDouble End"<<endl; 
	return true;
}

bool CFeatureValueStoreMgr::SaveFeatureValueToStoreInt32(
	const iBS::FeatureObserverSimpleInfoPtr& foi,
	::Ice::Long localFeatureIdxFrom,
	::Ice::Long localFeatureIdxTo,
	::Ice::Double *values,
	const string& batchFileName, Ice::Long featureCntPerBatch)
{
	if (!IceUtilInternal::fileExists(batchFileName) && featureCntPerBatch>(localFeatureIdxTo - localFeatureIdxFrom))
	{
		//first time write, fill with full values of NaN
		::IceUtil::ScopedArray<Ice::Double> saNanValues(new ::Ice::Double[featureCntPerBatch]);
		if (!saNanValues.get())
		{
			return false;
		}
		std::pair<Ice::Double*, Ice::Double*> nanValues(
			saNanValues.get(), saNanValues.get() + featureCntPerBatch);

		::Ice::Double nanVal = std::numeric_limits< ::Ice::Double >::quiet_NaN();

		//::Ice::Double nanVal = 0;

		std::fill(nanValues.first, nanValues.second, nanVal);

		std::copy(values, values + (localFeatureIdxTo - localFeatureIdxFrom), nanValues.first + localFeatureIdxFrom);

		return SaveFeatureValueToStoreInt32(foi, 0, featureCntPerBatch, nanValues.first, batchFileName, featureCntPerBatch);

	}

	::Ice::Long TotalValueCnt = localFeatureIdxTo - localFeatureIdxFrom;
	::IceUtil::ScopedArray<Ice::Int> saIntValues(new ::Ice::Int[TotalValueCnt]);
	if (!saIntValues.get())
	{
		return false;
	}

	::Ice::Int *intValues = saIntValues.get();
	//convert Double to Int32
	for (Ice::Long i = 0; i < TotalValueCnt; i++)
	{
		intValues[i] = (::Ice::Int)values[i];
	}

	string mode = "r+b";
	if (localFeatureIdxTo - localFeatureIdxFrom == featureCntPerBatch)
	{
		mode = "wb";
	}

	FILE* fp = IceUtilInternal::fopen(batchFileName, mode);
	if (fp == 0)
	{
		cout << IceUtil::Time::now().toDateTime() << " Error: file cannot open " << batchFileName << endl;
		return  false;
	}

	if (localFeatureIdxFrom>0)
	{
		if (fseek(fp, localFeatureIdxFrom*sizeof(Ice::Int), SEEK_SET) != 0)
		{
			fclose(fp);
			//report problem
			return 0;
		}
	}

	size_t remainSize = localFeatureIdxTo - localFeatureIdxFrom;
	size_t batchSize = 1024 * 1024*2; //2M value cnt
	cout << IceUtil::Time::now().toDateTime() << " oid=" << foi->ObserverID << " SaveFeatureValueToStoreInt32 begin ..." << endl;

	while (remainSize>0)
	{
		if (remainSize>batchSize)
		{
			fwrite(intValues, sizeof(::Ice::Int), batchSize, fp);
			intValues += batchSize;
			remainSize -= batchSize;
		}
		else
		{
			fwrite(intValues, sizeof(::Ice::Int), remainSize, fp);
			remainSize = 0;
		}

	}
	fclose(fp);
	cout << IceUtil::Time::now().toDateTime() << " oid=" << foi->ObserverID << " SaveFeatureValueToStoreInt32 End" << endl;
	return true;
}

Ice::Long CFeatureValueStoreMgr::GetMaxFeatureValueCntPerStoreFile(
	iBS::FeatureValueEnum valueStoredType)
{
	Ice::Long MaxFeatureValueCntPerFile = m_theMaxFeatureValueCntDouble;
	if (valueStoredType == ::iBS::FeatureValueInt32)
	{
		MaxFeatureValueCntPerFile = m_theMaxFeatureValueCntInt32;
	}
	return MaxFeatureValueCntPerFile;
}


bool CFeatureValueStoreMgr::RemoveFeatureValueStore(const iBS::FeatureObserverSimpleInfoPtr& foi)
{
	Ice::Long MaxFeatureValueCntPerFile = GetMaxFeatureValueCntPerStoreFile(foi->ValueType);

	if (foi->DomainSize <= MaxFeatureValueCntPerFile)
	{
		string fileName = GetStoreFilePath(foi);
		if (IceUtilInternal::fileExists(fileName))
		{
			IceUtilInternal::remove(fileName);
		}
	}
	else
	{
		StringVec files;
		LongVec batchFileIdxs;
		LongVec posFroms, posTos;
		::Ice::Long featureCntPerBatch;
		::Ice::Long featureIdxFrom = 0;
		::Ice::Long featureIdxTo = foi->DomainSize;
		GetStoreFilePath(foi, featureIdxFrom, featureIdxTo, featureCntPerBatch, files, batchFileIdxs, posFroms, posTos);
		Ice::Long batchFileCnt = files.size();
		for (Ice::Long i = 0; i < batchFileCnt; i++)
		{
			if (IceUtilInternal::fileExists(files[i]))
			{
				IceUtilInternal::remove(files[i]);
			}
		}
	}

	return true;
}

bool CFeatureValueStoreMgr::SaveFeatureValueToStore(const iBS::FeatureObserverSimpleInfoPtr& foi,
		::Ice::Long featureIdxFrom,
		::Ice::Long featureIdxTo,
		const std::pair<const Ice::Double*, const Ice::Double*>& values)
{
	Ice::Long MaxFeatureValueCntPerFile = GetMaxFeatureValueCntPerStoreFile(foi->ValueType);

	if (foi->DomainSize <= MaxFeatureValueCntPerFile)
	{
		//single file, not exist
		::Ice::Double * valueBegin=const_cast<Ice::Double* >(values.first);
		string fileName = GetStoreFilePath(foi);
		Ice::Long featureCntPerBatch=foi->DomainSize;

		bool rt = false;
		if (foi->ValueType == ::iBS::FeatureValueInt32)
		{
			rt = SaveFeatureValueToStoreInt32(
					foi, featureIdxFrom, featureIdxTo,
					valueBegin, fileName, featureCntPerBatch);
		}
		else
		{
			rt = SaveFeatureValueToStoreDouble(
				foi, featureIdxFrom, featureIdxTo,
				valueBegin, fileName, featureCntPerBatch);
		}
		return rt;
	}
	else
	{
		StringVec files;
		LongVec batchFileIdxs;
		LongVec posFroms, posTos;
		Ice::Long featureCntPerBatch;
		GetStoreFilePath(foi, featureIdxFrom,featureIdxTo,featureCntPerBatch, files,batchFileIdxs,posFroms,posTos);
		Ice::Long batchFileCnt=files.size();
		::Ice::Double * valueBegin=const_cast<Ice::Double*>(values.first);
		for(Ice::Long i=0;i<batchFileCnt;i++)
		{
			::Ice::Long localFeatureIdxFrom=posFroms[i];
			::Ice::Long localFeatureIdxTo =posTos[i];

			if (foi->ValueType == ::iBS::FeatureValueInt32)
			{
				SaveFeatureValueToStoreInt32(
					foi, localFeatureIdxFrom, localFeatureIdxTo,
					valueBegin, files[i], featureCntPerBatch);
			}
			else
			{
				SaveFeatureValueToStoreDouble(
					foi, localFeatureIdxFrom, localFeatureIdxTo,
					valueBegin, files[i], featureCntPerBatch);
			}

			valueBegin+=(localFeatureIdxTo-localFeatureIdxFrom);
		}
		return true;
	}

}


//caller need to delete memory
::Ice::Double* CFeatureValueStoreMgr::LoadFeatureValuesFromStore(const iBS::FeatureObserverSimpleInfoPtr& foi)
{

	::Ice::Long featureIdxFrom=0;
	::Ice::Long featureIdxTo =foi->DomainSize;
	return LoadFeatureValuesFromStore(foi, featureIdxFrom, featureIdxTo);
}

//caller need to delete memory
::Ice::Double* CFeatureValueStoreMgr::LoadFeatureValuesFromStore(
	const iBS::FeatureObserverSimpleInfoPtr& foi,
	::Ice::Long featureIdxFrom,
	::Ice::Long featureIdxTo)
{
	Ice::Long featureValueCnt = featureIdxTo - featureIdxFrom;
	::Ice::Double *values = new ::Ice::Double[featureValueCnt];
	if (values == NULL)
	{
		//report problem
		return 0;
	}
	LoadFeatureValuesFromStore(foi, featureIdxFrom, featureIdxTo, values);
	return values;
}

bool CFeatureValueStoreMgr::LoadFeatureValuesFromStore(
	const iBS::FeatureObserverSimpleInfoPtr& foi,
	::Ice::Long featureIdxFrom,
	::Ice::Long featureIdxTo, ::Ice::Double* values)
{
	if(foi->DomainSize<= m_theMaxFeatureValueCntDouble)
	{
		Ice::Long featureValueCnt= featureIdxTo -featureIdxFrom;
		//single file, not exist
		if(!StoreFileExist(foi))
		{
			::Ice::Double nanVal = std::numeric_limits< ::Ice::Double >::quiet_NaN();
			std::fill(values, values+featureValueCnt, nanVal);
			return true;
		}
		else
		{
			//fill default values
			::Ice::Double nanVal = std::numeric_limits< ::Ice::Double >::quiet_NaN();
			std::fill(values, values+featureValueCnt, nanVal);

			LoadBatchFeatureValuesDouble(
				foi,featureIdxFrom,featureIdxTo,
				values, GetStoreFilePath(foi));

			return true;
		}
	}
	else
	{   
		StringVec files;
		LongVec batchFileIdxs;
		LongVec posFroms, posTos;
		Ice::Long featureCntPerBatch;
		GetStoreFilePath(foi, featureIdxFrom,featureIdxTo,featureCntPerBatch, files,batchFileIdxs,posFroms,posTos);
		Ice::Long batchFileCnt=files.size();

		Ice::Long featureValueCnt= featureIdxTo -featureIdxFrom;
		::Ice::Double nanVal = std::numeric_limits< ::Ice::Double >::quiet_NaN();
			std::fill(values, values+featureValueCnt, nanVal);

		::Ice::Double * valueBegin=values;
		for(Ice::Long i=0;i<batchFileCnt;i++)
		{
			::Ice::Long localFeatureIdxFrom=posFroms[i];
			::Ice::Long localFeatureIdxTo =posTos[i];
			LoadBatchFeatureValuesDouble(
				foi,localFeatureIdxFrom,localFeatureIdxTo,
				valueBegin, files[i]);
			valueBegin+=(localFeatureIdxTo-localFeatureIdxFrom);
		}
		//the caller need to delete the RAM
		return true;
	}
}

int CFeatureValueStoreMgr::LoadBatchFeatureValuesDouble(
		const iBS::FeatureObserverSimpleInfoPtr& foi,
		::Ice::Long localFeatureIdxFrom,
		::Ice::Long localFeatureIdxTo,
		::Ice::Double *values,
		const string& batchFileName)
{
	::Ice::Long readValueCnt=(localFeatureIdxTo-localFeatureIdxFrom);

	if(!IceUtilInternal::fileExists(batchFileName))
	{
		::Ice::Double nanVal = std::numeric_limits< ::Ice::Double >::quiet_NaN();
			std::fill(values, values+readValueCnt, nanVal);
		return 1;
	}

	//cout<<IceUtil::Time::now().toDateTime()<<" oid="<<foi->ObserverID<<" LoadBatchFeatureValuesDouble begin ..."<<endl; 

	int fd = IceUtilInternal::open(batchFileName, O_RDONLY|O_BINARY);
	if(fd == -1)
    {
        //FileAccessException ex;
        //ex.reason = "cannot open `" + path + "' for reading: " + strerror(errno);
        //cb->ice_exception(ex);
        return 0;
    }

	//values are already allocated in RAM
	if(values==NULL)
	{
		//report problem
		return 0;
	}

	::Ice::Long pos=localFeatureIdxFrom*sizeof(Ice::Double);
	

	if(localFeatureIdxFrom>0)
	{
	    if(
			#if defined(_MSC_VER)
			_lseek(fd, static_cast<off_t>(pos), SEEK_SET)
			#else
			lseek(fd, static_cast<off_t>(pos), SEEK_SET)
			#endif
			!= static_cast<off_t>(pos))
		{
			IceUtilInternal::close(fd);
			//report problem
			return 0;
		}
	}


#ifdef _WIN32
    int r;
	unsigned int max_charCnt=(unsigned int)readValueCnt*sizeof(::Ice::Double);
    if((r = _read(fd, values, static_cast<unsigned int>(max_charCnt))) == -1)
#else
    ssize_t r;
	size_t max_charCnt=(size_t)readValueCnt*sizeof(::Ice::Double);
    if((r = read(fd, values, max_charCnt)) == -1)
#endif
    {
        IceUtilInternal::close(fd);

        //FileAccessException ex;
        //ex.reason = "cannot read `" + path + "': " + strerror(errno);
        //cb->ice_exception(ex);
        return 0;
    }

    IceUtilInternal::close(fd);

	//cout<<IceUtil::Time::now().toDateTime()<<" oid="<<foi->ObserverID<< " LoadBatchFeatureValuesDouble end"<<endl; 
	return 1;
}

::Ice::Double* CFeatureValueStoreMgr::LoadNonOverlapRangesFromStore(
	const iBS::FeatureObserverSimpleInfoPtr& foi,
	const ::iBS::LongVec& featureIdxFroms,
	const ::iBS::LongVec& featureIdxTos)
{
	Ice::Long cnt = featureIdxFroms.size();
	Ice::Long totalValueCnt = 0;
	for (Ice::Long i = 0; i < cnt; i++)
	{
		totalValueCnt += (featureIdxTos[i] - featureIdxFroms[i]);
	}

	::IceUtil::ScopedArray<Ice::Double>  values(new ::Ice::Double[totalValueCnt]);
	if (!values.get())
	{
		return 0;
	}
	LoadNonOverlapRangesFromStore(foi, featureIdxFroms, featureIdxTos, values.get());
	return values.release();
}


bool CFeatureValueStoreMgr::LoadNonOverlapRangesFromStore(
	const iBS::FeatureObserverSimpleInfoPtr& foi,
	const ::iBS::LongVec& featureIdxFroms,
	const ::iBS::LongVec& featureIdxTos,
	::Ice::Double* values)
{
	bool alreadySorted = true;
	Ice::Long cnt = featureIdxFroms.size();
	for (Ice::Long i = 1; i < cnt; i++)
	{
		if (featureIdxFroms[i - 1]>featureIdxFroms[i])
		{
			alreadySorted = false;
			break;
		}
	}

	if (alreadySorted)
	{
		Ice::Long totalValueCnt = 0;
		for (Ice::Long i = 0; i < cnt; i++)
		{
			totalValueCnt += (featureIdxTos[i] - featureIdxFroms[i]);
		}

		return LoadNonOverlapSortedRangesFromStore(foi, featureIdxFroms,
			featureIdxTos, totalValueCnt, values);
	}
	else
	{
		::iBS::LongVec sorted_featureIdxFroms(cnt);
		::iBS::LongVec sorted_featureIdxTos(cnt);
		::iBS::LongVec sorted_originalIdxs(cnt);
		::iBS::LongVec originalIdx2SortedValueBegin(cnt);

		Ice::Long totalValueCnt = 0;
		CSortHelper::GetSortedValuesAndIdxs(featureIdxFroms, sorted_featureIdxFroms, sorted_originalIdxs);
		for (Ice::Long i = 0; i < cnt; i++)
		{
			Ice::Long originalIdx = sorted_originalIdxs[i];
			originalIdx2SortedValueBegin[originalIdx] = totalValueCnt;
			sorted_featureIdxTos[i] = featureIdxTos[originalIdx];
			totalValueCnt += (sorted_featureIdxTos[i] - sorted_featureIdxFroms[i]);
		}

		::IceUtil::ScopedArray<Ice::Double>  sortedValues(new ::Ice::Double[totalValueCnt]);
		if (!sortedValues.get())
		{
			return false;
		}

		LoadNonOverlapSortedRangesFromStore(foi, sorted_featureIdxFroms,
			sorted_featureIdxTos, totalValueCnt, sortedValues.get());

		totalValueCnt = 0;
		for (Ice::Long i = 0; i < cnt; i++)
		{
			Ice::Long originalIdx = i;
			Ice::Double * valueBegin = values + totalValueCnt;
			Ice::Double * sortedValueBegin = sortedValues.get() + originalIdx2SortedValueBegin[originalIdx];
			Ice::Long rangeValCnt = featureIdxTos[originalIdx] - featureIdxFroms[originalIdx];
			std::copy(sortedValueBegin, sortedValueBegin + rangeValCnt, valueBegin);
			totalValueCnt += rangeValCnt;
		}
		return true;
	}
}

bool CFeatureValueStoreMgr::LoadNonOverlapSortedRangesFromStore(
	const iBS::FeatureObserverSimpleInfoPtr& foi,
	const ::iBS::LongVec& featureIdxFroms,
	const ::iBS::LongVec& featureIdxTos, 
	::Ice::Long totalValueCnt,
	::Ice::Double *values)
{
	if (foi->DomainSize <= m_theMaxFeatureValueCntDouble)
	{
		//single file, not exist
		if (!StoreFileExist(foi))
		{
			::Ice::Double nanVal = std::numeric_limits< ::Ice::Double >::quiet_NaN();
			std::fill(values, values + totalValueCnt, nanVal);
			return true;
		}
		else
		{
			//fill default values
			::Ice::Double nanVal = std::numeric_limits< ::Ice::Double >::quiet_NaN();
			std::fill(values, values + totalValueCnt, nanVal);

			multipleLoadBatchFeatureValuesDouble(
				foi, featureIdxFroms, featureIdxTos,
				values, GetStoreFilePath(foi));

			return true;
		}
	}
	else
	{
		::iBS::LongVec g_featureIdxFroms(featureIdxFroms);
		::iBS::LongVec g_featureIdxTos(featureIdxTos);

		::Ice::Double nanVal = std::numeric_limits< ::Ice::Double >::quiet_NaN();
		std::fill(values, values + totalValueCnt, nanVal);

		StringVec files;
		LongVec batchFileIdxs;
		LongVec posFroms, posTos;
		Ice::Long featureCntPerBatch;
		Ice::Long rangeCnt = g_featureIdxFroms.size();
		Ice::Long featureIdxFrom_min = g_featureIdxFroms[0];
		Ice::Long featureIdxFrom_max = g_featureIdxTos[rangeCnt - 1];

		GetStoreFilePath(foi, featureIdxFrom_min, featureIdxFrom_max, featureCntPerBatch, files, batchFileIdxs, posFroms, posTos);
		Ice::Long batchFileCnt = files.size();

		Ice::Long rangeIdx = 0;
		Ice::Long fetchedValueCnt = 0;
		Ice::Long thisBatchFetchedValueCnt = 0;
		for (Ice::Long i = 0; i < batchFileCnt; i++)
		{
			Ice::Long thisBatch_globalFeatureIdxFrom = batchFileIdxs[i] * featureCntPerBatch;
			::iBS::LongVec thisBatch_localFeatureIdxFroms;
			thisBatch_localFeatureIdxFroms.reserve(rangeCnt);
			::iBS::LongVec thisBatch_localFeatureIdxTos;
			thisBatch_localFeatureIdxTos.reserve(rangeCnt);

			thisBatchFetchedValueCnt = 0;
			for (Ice::Long j = rangeIdx; j < rangeCnt; j++)
			{
				if ((g_featureIdxFroms[j] >= thisBatch_globalFeatureIdxFrom)
					&& (g_featureIdxTos[j] <= thisBatch_globalFeatureIdxFrom + featureCntPerBatch))
				{
					//completely in the batch
					thisBatch_localFeatureIdxFroms.push_back(g_featureIdxFroms[j] - thisBatch_globalFeatureIdxFrom);
					thisBatch_localFeatureIdxTos.push_back(g_featureIdxTos[j] - thisBatch_globalFeatureIdxFrom);
					thisBatchFetchedValueCnt += (g_featureIdxTos[j] - g_featureIdxFroms[j]);

					//range j be processed in this batch
					rangeIdx = j+1;
				}
				else if ((g_featureIdxFroms[j] >=thisBatch_globalFeatureIdxFrom)
					&&(g_featureIdxFroms[j] < thisBatch_globalFeatureIdxFrom + featureCntPerBatch)
					&& (g_featureIdxTos[j] > thisBatch_globalFeatureIdxFrom + featureCntPerBatch))
				{
					//overlap, needs to create new range

					Ice::Long splittingIdx = thisBatch_globalFeatureIdxFrom + featureCntPerBatch;

					thisBatch_localFeatureIdxFroms.push_back(g_featureIdxFroms[j] - thisBatch_globalFeatureIdxFrom);
					thisBatch_localFeatureIdxTos.push_back(featureCntPerBatch);
					g_featureIdxFroms[j] = splittingIdx;
					thisBatchFetchedValueCnt += (splittingIdx - g_featureIdxFroms[j]);
					//range j still need be processed
					rangeIdx = j;
					break;
				}
				else if ((g_featureIdxFroms[j] >= thisBatch_globalFeatureIdxFrom + featureCntPerBatch))
				{
					//range in a larger batch
					//range j still need be processed
					rangeIdx = j;
					break;
				}
				else
				{
					//should not reach here
					cout << IceUtil::Time::now().toDateTime() << "Error: LoadNonOverlapSortedRangesFromStore" << endl;
				}
			}

			if (thisBatchFetchedValueCnt>0)
			{
				::Ice::Double * valueBegin = values + fetchedValueCnt;

				multipleLoadBatchFeatureValuesDouble(
					foi, thisBatch_localFeatureIdxFroms, thisBatch_localFeatureIdxTos,
					valueBegin, files[i]);

				fetchedValueCnt += thisBatchFetchedValueCnt;
			}
		}

		return true;
	}
}

Ice::Long CFeatureValueStoreMgr::getValueCntByMultipleRanges(
		const ::iBS::LongVec& featureIdxFroms,
		const ::iBS::LongVec& featureIdxTos)
{
	Ice::Long cnt=0;
	if(featureIdxFroms.size()!=featureIdxTos.size())
		return 0;

	for(int i=0;i<featureIdxFroms.size();i++)
	{
		cnt+=(featureIdxTos[i]-featureIdxFroms[i]);
	}
	return cnt;
}

int CFeatureValueStoreMgr::multipleLoadBatchFeatureValuesDouble(
		const iBS::FeatureObserverSimpleInfoPtr& foi,
		const ::iBS::LongVec& localFeatureIdxFroms,
		const ::iBS::LongVec& localFeatureIdxTos,
		::Ice::Double *values,
		const string& batchFileName)
{
	::Ice::Long readValueCnt=getValueCntByMultipleRanges(localFeatureIdxFroms,localFeatureIdxTos);

	if(!IceUtilInternal::fileExists(batchFileName))
	{
		::Ice::Double nanVal = std::numeric_limits< ::Ice::Double >::quiet_NaN();
			std::fill(values, values+readValueCnt, nanVal);
		return 1;
	}

	//cout<<IceUtil::Time::now().toDateTime()<<" oid="<<foi->ObserverID<<" LoadBatchFeatureValuesDouble begin ..."<<endl; 

	int fd = IceUtilInternal::open(batchFileName, O_RDONLY|O_BINARY);
	if(fd == -1)
    {
        //FileAccessException ex;
        //ex.reason = "cannot open `" + path + "' for reading: " + strerror(errno);
        //cb->ice_exception(ex);
        return 0;
    }

	//values are already allocated in RAM
	if(values==NULL)
	{
		//report problem
		return 0;
	}

	::Ice::Double *readValues = 0;
	::Ice::Long fetchedCnt = 0;
	for(size_t i=0;i<localFeatureIdxFroms.size();i++)
	{
		::Ice::Long localFeatureIdxFrom=localFeatureIdxFroms[i];
		::Ice::Long localFeatureIdxTo=localFeatureIdxTos[i];
		::Ice::Long thisReadValueCnt=(localFeatureIdxTo-localFeatureIdxFrom);

		::Ice::Long pos=localFeatureIdxFrom*sizeof(Ice::Double);
		readValues = values + fetchedCnt;
	
		if(localFeatureIdxFrom>0)
		{
			if(
				#if defined(_MSC_VER)
				_lseek(fd, static_cast<off_t>(pos), SEEK_SET)
				#else
				lseek(fd, static_cast<off_t>(pos), SEEK_SET)
				#endif
				!= static_cast<off_t>(pos))
			{
				IceUtilInternal::close(fd);
				//report problem
				return 0;
			}
		}


	#ifdef _WIN32
		int r;
		unsigned int max_charCnt=(unsigned int)thisReadValueCnt*sizeof(::Ice::Double);
		if((r = _read(fd, readValues, static_cast<unsigned int>(max_charCnt))) == -1)
	#else
		ssize_t r;
		size_t max_charCnt=(size_t)thisReadValueCnt*sizeof(::Ice::Double);
		if((r = read(fd, readValues, max_charCnt)) == -1)
	#endif
		{
			IceUtilInternal::close(fd);

			//FileAccessException ex;
			//ex.reason = "cannot read `" + path + "': " + strerror(errno);
			//cb->ice_exception(ex);
			return 0;
		}

		fetchedCnt += thisReadValueCnt;
	}

    IceUtilInternal::close(fd);

	//cout<<IceUtil::Time::now().toDateTime()<<" oid="<<foi->ObserverID<< " LoadBatchFeatureValuesDouble end"<<endl; 
	return 1;
}