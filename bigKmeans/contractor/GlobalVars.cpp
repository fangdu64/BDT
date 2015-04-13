#include <GlobalVars.h>

CGlobalVars* iBSInternal::GlobalVars::_ptrGV=0;

CGlobalVars*
CGlobalVars::get()
{
    return iBSInternal::GlobalVars::_ptrGV;
}
