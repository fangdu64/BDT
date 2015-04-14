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
# Generated from file `EndpointF.ice'
#
# Warning: do not edit this file.
#
# </auto-generated>
#

import Ice, IcePy

# Start of module Ice
_M_Ice = Ice.openModule('Ice')
__name__ = 'Ice'

if 'EndpointInfo' not in _M_Ice.__dict__:
    _M_Ice._t_EndpointInfo = IcePy.declareClass('::Ice::EndpointInfo')

if 'TCPEndpointInfo' not in _M_Ice.__dict__:
    _M_Ice._t_TCPEndpointInfo = IcePy.declareClass('::Ice::TCPEndpointInfo')

if 'UDPEndpointInfo' not in _M_Ice.__dict__:
    _M_Ice._t_UDPEndpointInfo = IcePy.declareClass('::Ice::UDPEndpointInfo')

if 'Endpoint' not in _M_Ice.__dict__:
    _M_Ice._t_Endpoint = IcePy.declareClass('::Ice::Endpoint')

if '_t_EndpointSeq' not in _M_Ice.__dict__:
    _M_Ice._t_EndpointSeq = IcePy.defineSequence('::Ice::EndpointSeq', (), _M_Ice._t_Endpoint)

# End of module Ice
