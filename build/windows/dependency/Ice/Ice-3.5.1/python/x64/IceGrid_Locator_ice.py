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
# Generated from file `Locator.ice'
#
# Warning: do not edit this file.
#
# </auto-generated>
#

import Ice, IcePy
import Ice_Locator_ice

# Included module Ice
_M_Ice = Ice.openModule('Ice')

# Start of module IceGrid
_M_IceGrid = Ice.openModule('IceGrid')
__name__ = 'IceGrid'

if 'Registry' not in _M_IceGrid.__dict__:
    _M_IceGrid._t_Registry = IcePy.declareClass('::IceGrid::Registry')
    _M_IceGrid._t_RegistryPrx = IcePy.declareProxy('::IceGrid::Registry')

if 'Query' not in _M_IceGrid.__dict__:
    _M_IceGrid._t_Query = IcePy.declareClass('::IceGrid::Query')
    _M_IceGrid._t_QueryPrx = IcePy.declareProxy('::IceGrid::Query')

if 'Locator' not in _M_IceGrid.__dict__:
    _M_IceGrid.Locator = Ice.createTempClass()
    class Locator(_M_Ice.Locator):
        '''The IceGrid locator interface provides access to the Query
and Registry object of the IceGrid registry.'''
        def __init__(self):
            if Ice.getType(self) == _M_IceGrid.Locator:
                raise RuntimeError('IceGrid.Locator is an abstract class')

        def ice_ids(self, current=None):
            return ('::Ice::Locator', '::Ice::Object', '::IceGrid::Locator')

        def ice_id(self, current=None):
            return '::IceGrid::Locator'

        def ice_staticId():
            return '::IceGrid::Locator'
        ice_staticId = staticmethod(ice_staticId)

        def getLocalRegistry(self, current=None):
            '''Get the proxy of the registry object hosted by this IceGrid
registry.

Returns:
    The proxy of the registry object.'''
            pass

        def getLocalQuery(self, current=None):
            '''Get the proxy of the query object hosted by this IceGrid
registry.

Returns:
    The proxy of the query object.'''
            pass

        def __str__(self):
            return IcePy.stringify(self, _M_IceGrid._t_Locator)

        __repr__ = __str__

    _M_IceGrid.LocatorPrx = Ice.createTempClass()
    class LocatorPrx(_M_Ice.LocatorPrx):

        '''Get the proxy of the registry object hosted by this IceGrid
registry.

Returns:
    The proxy of the registry object.'''
        def getLocalRegistry(self, _ctx=None):
            return _M_IceGrid.Locator._op_getLocalRegistry.invoke(self, ((), _ctx))

        '''Get the proxy of the registry object hosted by this IceGrid
registry.

Returns:
    The proxy of the registry object.'''
        def begin_getLocalRegistry(self, _response=None, _ex=None, _sent=None, _ctx=None):
            return _M_IceGrid.Locator._op_getLocalRegistry.begin(self, ((), _response, _ex, _sent, _ctx))

        '''Get the proxy of the registry object hosted by this IceGrid
registry.

Returns:
    The proxy of the registry object.'''
        def end_getLocalRegistry(self, _r):
            return _M_IceGrid.Locator._op_getLocalRegistry.end(self, _r)

        '''Get the proxy of the query object hosted by this IceGrid
registry.

Returns:
    The proxy of the query object.'''
        def getLocalQuery(self, _ctx=None):
            return _M_IceGrid.Locator._op_getLocalQuery.invoke(self, ((), _ctx))

        '''Get the proxy of the query object hosted by this IceGrid
registry.

Returns:
    The proxy of the query object.'''
        def begin_getLocalQuery(self, _response=None, _ex=None, _sent=None, _ctx=None):
            return _M_IceGrid.Locator._op_getLocalQuery.begin(self, ((), _response, _ex, _sent, _ctx))

        '''Get the proxy of the query object hosted by this IceGrid
registry.

Returns:
    The proxy of the query object.'''
        def end_getLocalQuery(self, _r):
            return _M_IceGrid.Locator._op_getLocalQuery.end(self, _r)

        def checkedCast(proxy, facetOrCtx=None, _ctx=None):
            return _M_IceGrid.LocatorPrx.ice_checkedCast(proxy, '::IceGrid::Locator', facetOrCtx, _ctx)
        checkedCast = staticmethod(checkedCast)

        def uncheckedCast(proxy, facet=None):
            return _M_IceGrid.LocatorPrx.ice_uncheckedCast(proxy, facet)
        uncheckedCast = staticmethod(uncheckedCast)

    _M_IceGrid._t_LocatorPrx = IcePy.defineProxy('::IceGrid::Locator', LocatorPrx)

    _M_IceGrid._t_Locator = IcePy.defineClass('::IceGrid::Locator', Locator, -1, (), True, False, None, (_M_Ice._t_Locator,), ())
    Locator._ice_type = _M_IceGrid._t_Locator

    Locator._op_getLocalRegistry = IcePy.Operation('getLocalRegistry', Ice.OperationMode.Idempotent, Ice.OperationMode.Idempotent, False, None, (), (), (), ((), _M_IceGrid._t_RegistryPrx, False, 0), ())
    Locator._op_getLocalQuery = IcePy.Operation('getLocalQuery', Ice.OperationMode.Idempotent, Ice.OperationMode.Idempotent, False, None, (), (), (), ((), _M_IceGrid._t_QueryPrx, False, 0), ())

    _M_IceGrid.Locator = Locator
    del Locator

    _M_IceGrid.LocatorPrx = LocatorPrx
    del LocatorPrx

# End of module IceGrid

Ice.sliceChecksums["::IceGrid::Locator"] = "816e9d7a3cb39b8c80fe342e7f18ae"
