#include <bdt/bdtBase.h>
#include <BitSetHelper.h>


bool CLargeBitSet::Initialize(Ice::Long bitCnt)
{
	if(bitCnt>3000000000)
	{
		return false;
	}
	else if(bitCnt<=300000000)
	{
		m_p300MBitSet = new BitSet300M_T();
	}
	else
	{
		m_p3GBitSet = new BitSet3G_T();
	}

	return true;
}