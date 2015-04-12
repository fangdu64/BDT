#ifndef BASIC_SLICE_DEFINE_ICE
#define BASIC_SLICE_DEFINE_ICE

module iBS
{
	//vectors
	sequence<float>		FloatVec;
	sequence<int>		IntVec;
	sequence<string>	StringVec;
	sequence<double>	DoubleVec;
	sequence<byte>		ByteVec;
	sequence<bool>		BoolVec;
	sequence<long>		LongVec;

	//List of vectors
	sequence<IntVec>	IntVecVec;
	sequence<StringVec> StringVecVec;
	sequence<FloatVec>	FloatVecVec;
	sequence<DoubleVec> DoubleVecVec;
	sequence<ByteVec>	ByteVecVec;
	sequence<LongVec>	LongVecVec;

	//List of list of vectors
	sequence<IntVecVec>	  IntVecVecVec;
	sequence<FloatVecVec> FloatVecVecVec;
	sequence<DoubleVecVec> DoubleVecVecVec;
	sequence<ByteVecVec>  ByteVecVecVec;
	sequence<StringVecVec> StringVecVecVec;
	
	enum AMDTaskStatusEnum
	{
		AMDTaskStatusNormal= 0,
		AMDTaskStatusFinished= 1,
		AMDTaskStatusFailure = 2,
		AMDTaskStatusNotExist = 3
	};

	struct AMDTaskInfo
	{
		long TaskID;
		string TaskName;
		long TotalCnt;
		long FinishedCnt;
		AMDTaskStatusEnum Status;
	};

	dictionary<string, string> Str2StrMap;

	struct IntPair
	{
		int int1;
		int int2;
	};
	sequence<IntPair> IntPairVec;

	enum RecordUpdateAction
	{ 
		RecordActionUnkown,
		RecordActionDelete,
		RecordActionInsert,
		RecordActionUpdate
	};

	struct IntFloat
	{
		int ival;
		float fval;
	};
	sequence<IntFloat> IntFloatVec;
	sequence<IntFloatVec> IntFloatVecVec;

	//generic node access exception
	exception ArgumentException
	{
		string reason;
	};
};

#endif

