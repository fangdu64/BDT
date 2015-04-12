#include <GlobalVars.h>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/uniform_01.hpp>

CGlobalVars* iBSInternal::GlobalVars::_ptrGV=0;

boost::random::mt19937 _gen_mt19937;

CGlobalVars*
CGlobalVars::get()
{
    return iBSInternal::GlobalVars::_ptrGV;
}

int CGlobalVars::GetUniformInt(int valFrom, int valTo)
{
	boost::random::uniform_int_distribution<> dist(valFrom, valTo);
	return dist(_gen_mt19937);
}

