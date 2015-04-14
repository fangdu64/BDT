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
# Generated from file `ObjectFactory.ice'
#
# Warning: do not edit this file.
#
# </auto-generated>
#

import Ice, IcePy

# Start of module Ice
_M_Ice = Ice.openModule('Ice')
__name__ = 'Ice'

if 'ObjectFactory' not in _M_Ice.__dict__:
    _M_Ice.ObjectFactory = Ice.createTempClass()
    class ObjectFactory(object):
        '''A factory for objects. Object factories are used in several
places, for example, when receiving "objects by value" and
when Freeze restores a persistent object. Object factories
must be implemented by the application writer, and registered
with the communicator.'''
        def __init__(self):
            if Ice.getType(self) == _M_Ice.ObjectFactory:
                raise RuntimeError('Ice.ObjectFactory is an abstract class')

        def create(self, type):
            '''Create a new object for a given object type. The type is the
absolute Slice type id, i.e., the id relative to the
unnamed top-level Slice module. For example, the absolute
Slice type id for interfaces of type Bar in the module
Foo is ::Foo::Bar.

The leading "::" is required.

Arguments:
    type The object type.

Returns:
    The object created for the given type, or nil if the
factory is unable to create the object.'''
            pass

        def destroy(self):
            '''Called when the factory is removed from the communicator, or if
the communicator is destroyed.'''
            pass

        def __str__(self):
            return IcePy.stringify(self, _M_Ice._t_ObjectFactory)

        __repr__ = __str__

    _M_Ice._t_ObjectFactory = IcePy.defineClass('::Ice::ObjectFactory', ObjectFactory, -1, (), True, False, None, (), ())
    ObjectFactory._ice_type = _M_Ice._t_ObjectFactory

    _M_Ice.ObjectFactory = ObjectFactory
    del ObjectFactory

# End of module Ice
