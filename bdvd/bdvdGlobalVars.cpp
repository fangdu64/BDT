#include <bdvdGlobalVars.h>
CBdvdGlobalVars* iBSInternal::bdvdGlobalVars::_ptrGV = 0;

CBdvdGlobalVars*
CBdvdGlobalVars::get()
{
	return iBSInternal::bdvdGlobalVars::_ptrGV;
}
