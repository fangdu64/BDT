# **********************************************************************
#
# Copyright (c) 2003-2013 ZeroC, Inc. All rights reserved.
#
# This copy of Ice is licensed to you under the terms described in the
# ICE_LICENSE file included in this distribution.
#
# **********************************************************************
#
# Ice version 3.5.1
#
# <auto-generated>
#
# Generated from file `InstrumentationF.ice'
#
# Warning: do not edit this file.
#
# </auto-generated>
#

import Ice, IcePy

# Start of module Ice
_M_Ice = Ice.openModule('Ice')
__name__ = 'Ice'

# Start of module Ice.Instrumentation
_M_Ice.Instrumentation = Ice.openModule('Ice.Instrumentation')
__name__ = 'Ice.Instrumentation'

if 'Observer' not in _M_Ice.Instrumentation.__dict__:
    _M_Ice.Instrumentation._t_Observer = IcePy.declareClass('::Ice::Instrumentation::Observer')

if 'CommunicatorObserver' not in _M_Ice.Instrumentation.__dict__:
    _M_Ice.Instrumentation._t_CommunicatorObserver = IcePy.declareClass('::Ice::Instrumentation::CommunicatorObserver')

# End of module Ice.Instrumentation

__name__ = 'Ice'

# End of module Ice
