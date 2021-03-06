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
# Generated from file `Metrics.ice'
#
# Warning: do not edit this file.
#
# </auto-generated>
#

import Ice, IcePy
import Ice_Metrics_ice

# Included module Ice
_M_Ice = Ice.openModule('Ice')

# Included module IceMX
_M_IceMX = Ice.openModule('IceMX')

# Start of module IceMX
__name__ = 'IceMX'

if 'SessionMetrics' not in _M_IceMX.__dict__:
    _M_IceMX.SessionMetrics = Ice.createTempClass()
    class SessionMetrics(_M_IceMX.Metrics):
        '''Provides information on Glacier2 sessions.'''
        def __init__(self, id='', total=0, current=0, totalLifetime=0, failures=0, forwardedClient=0, forwardedServer=0, routingTableSize=0, queuedClient=0, queuedServer=0, overriddenClient=0, overriddenServer=0):
            _M_IceMX.Metrics.__init__(self, id, total, current, totalLifetime, failures)
            self.forwardedClient = forwardedClient
            self.forwardedServer = forwardedServer
            self.routingTableSize = routingTableSize
            self.queuedClient = queuedClient
            self.queuedServer = queuedServer
            self.overriddenClient = overriddenClient
            self.overriddenServer = overriddenServer

        def ice_ids(self, current=None):
            return ('::Ice::Object', '::IceMX::Metrics', '::IceMX::SessionMetrics')

        def ice_id(self, current=None):
            return '::IceMX::SessionMetrics'

        def ice_staticId():
            return '::IceMX::SessionMetrics'
        ice_staticId = staticmethod(ice_staticId)

        def __str__(self):
            return IcePy.stringify(self, _M_IceMX._t_SessionMetrics)

        __repr__ = __str__

    _M_IceMX.SessionMetricsPrx = Ice.createTempClass()
    class SessionMetricsPrx(_M_IceMX.MetricsPrx):

        def checkedCast(proxy, facetOrCtx=None, _ctx=None):
            return _M_IceMX.SessionMetricsPrx.ice_checkedCast(proxy, '::IceMX::SessionMetrics', facetOrCtx, _ctx)
        checkedCast = staticmethod(checkedCast)

        def uncheckedCast(proxy, facet=None):
            return _M_IceMX.SessionMetricsPrx.ice_uncheckedCast(proxy, facet)
        uncheckedCast = staticmethod(uncheckedCast)

    _M_IceMX._t_SessionMetricsPrx = IcePy.defineProxy('::IceMX::SessionMetrics', SessionMetricsPrx)

    _M_IceMX._t_SessionMetrics = IcePy.defineClass('::IceMX::SessionMetrics', SessionMetrics, -1, (), False, False, _M_IceMX._t_Metrics, (), (
        ('forwardedClient', (), IcePy._t_int, False, 0),
        ('forwardedServer', (), IcePy._t_int, False, 0),
        ('routingTableSize', (), IcePy._t_int, False, 0),
        ('queuedClient', (), IcePy._t_int, False, 0),
        ('queuedServer', (), IcePy._t_int, False, 0),
        ('overriddenClient', (), IcePy._t_int, False, 0),
        ('overriddenServer', (), IcePy._t_int, False, 0)
    ))
    SessionMetrics._ice_type = _M_IceMX._t_SessionMetrics

    _M_IceMX.SessionMetrics = SessionMetrics
    del SessionMetrics

    _M_IceMX.SessionMetricsPrx = SessionMetricsPrx
    del SessionMetricsPrx

# End of module IceMX

Ice.sliceChecksums["::IceMX::SessionMetrics"] = "221020be2c80301fb4dbb779e21b190"
