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
# Generated from file `PermissionsVerifierF.ice'
#
# Warning: do not edit this file.
#
# </auto-generated>
#

import Ice, IcePy

# Start of module Glacier2
_M_Glacier2 = Ice.openModule('Glacier2')
__name__ = 'Glacier2'

if 'PermissionsVerifier' not in _M_Glacier2.__dict__:
    _M_Glacier2._t_PermissionsVerifier = IcePy.declareClass('::Glacier2::PermissionsVerifier')
    _M_Glacier2._t_PermissionsVerifierPrx = IcePy.declareProxy('::Glacier2::PermissionsVerifier')

if 'SSLPermissionsVerifier' not in _M_Glacier2.__dict__:
    _M_Glacier2._t_SSLPermissionsVerifier = IcePy.declareClass('::Glacier2::SSLPermissionsVerifier')
    _M_Glacier2._t_SSLPermissionsVerifierPrx = IcePy.declareProxy('::Glacier2::SSLPermissionsVerifier')

# End of module Glacier2
