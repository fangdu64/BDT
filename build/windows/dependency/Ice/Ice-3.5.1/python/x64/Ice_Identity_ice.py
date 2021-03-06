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
# Generated from file `Identity.ice'
#
# Warning: do not edit this file.
#
# </auto-generated>
#

import Ice, IcePy

# Start of module Ice
_M_Ice = Ice.openModule('Ice')
__name__ = 'Ice'

if 'Identity' not in _M_Ice.__dict__:
    _M_Ice.Identity = Ice.createTempClass()
    class Identity(object):
        '''The identity of an Ice object. In a proxy, an empty Identity#name denotes a nil
proxy. An identity with an empty Identity#name and a non-empty Identity#category
is illegal. You cannot add a servant with an empty name to the Active Servant Map.'''
        def __init__(self, name='', category=''):
            self.name = name
            self.category = category

        def __hash__(self):
            _h = 0
            _h = 5 * _h + Ice.getHash(self.name)
            _h = 5 * _h + Ice.getHash(self.category)
            return _h % 0x7fffffff

        def __compare(self, other):
            if other is None:
                return 1
            elif not isinstance(other, _M_Ice.Identity):
                return NotImplemented
            else:
                if self.name is None or other.name is None:
                    if self.name != other.name:
                        return (-1 if self.name is None else 1)
                else:
                    if self.name < other.name:
                        return -1
                    elif self.name > other.name:
                        return 1
                if self.category is None or other.category is None:
                    if self.category != other.category:
                        return (-1 if self.category is None else 1)
                else:
                    if self.category < other.category:
                        return -1
                    elif self.category > other.category:
                        return 1
                return 0

        def __lt__(self, other):
            r = self.__compare(other)
            if r is NotImplemented:
                return r
            else:
                return r < 0

        def __le__(self, other):
            r = self.__compare(other)
            if r is NotImplemented:
                return r
            else:
                return r <= 0

        def __gt__(self, other):
            r = self.__compare(other)
            if r is NotImplemented:
                return r
            else:
                return r > 0

        def __ge__(self, other):
            r = self.__compare(other)
            if r is NotImplemented:
                return r
            else:
                return r >= 0

        def __eq__(self, other):
            r = self.__compare(other)
            if r is NotImplemented:
                return r
            else:
                return r == 0

        def __ne__(self, other):
            r = self.__compare(other)
            if r is NotImplemented:
                return r
            else:
                return r != 0

        def __str__(self):
            return IcePy.stringify(self, _M_Ice._t_Identity)

        __repr__ = __str__

    _M_Ice._t_Identity = IcePy.defineStruct('::Ice::Identity', Identity, (), (
        ('name', (), IcePy._t_string),
        ('category', (), IcePy._t_string)
    ))

    _M_Ice.Identity = Identity
    del Identity

if '_t_ObjectDict' not in _M_Ice.__dict__:
    _M_Ice._t_ObjectDict = IcePy.defineDictionary('::Ice::ObjectDict', (), _M_Ice._t_Identity, IcePy._t_Object)

if '_t_IdentitySeq' not in _M_Ice.__dict__:
    _M_Ice._t_IdentitySeq = IcePy.defineSequence('::Ice::IdentitySeq', (), _M_Ice._t_Identity)

# End of module Ice
