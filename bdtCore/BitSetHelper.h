
#ifndef __BitSetHelper_h__
#define __BitSetHelper_h__

#include <IceUtil/IceUtil.h>
#include <Ice/Ice.h>
#include <bitset>

class CLargeBitSet
{
public:
	CLargeBitSet()
		:m_p3GBitSet(0),m_p300MBitSet(0)
	{
		
	}

	~CLargeBitSet()
	{
		if(m_p3GBitSet)
		{
			delete m_p3GBitSet;
		}
		if(m_p300MBitSet)
		{
			delete m_p300MBitSet;
		}
	}

public:
	bool Initialize(Ice::Long bitCnt);
	bool IsInitialized() const
	{
		return m_p3GBitSet||m_p3GBitSet;
	}

	bool Test(Ice::Long pos) const
	{
		if(m_p300MBitSet)
			return m_p300MBitSet->test(pos);
		else
			return m_p3GBitSet->test(pos);
	}

	void Set(Ice::Long pos, bool val=true)
	{
		if(m_p300MBitSet)
			 m_p300MBitSet->set(pos,val);
		else
			 m_p3GBitSet->set(pos,val);
	}
private:
	typedef std::bitset<3000000000> BitSet3G_T;
	typedef std::bitset<300000000>  BitSet300M_T;
	BitSet3G_T   *m_p3GBitSet;
	BitSet300M_T *m_p300MBitSet;
	
};

#endif
