.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Class
=====

.. note:: This section contains advanced feature of Haka.

.. haka:module:: class

.. haka:class:: Class
    :module:

    The Haka class allows to define custom Lua object.

    .. haka:function:: class(name, parent=nil)
    
        :param name: Class name.
        :paramtype name: string
        :param parent: Parent class to inherit from.
        :paramtype parent: :haka:class:`Class`

        Create a new class.
        
    .. haka:method:: Class:new(...) -> instance
    
        :return instance: New instance of the class.
        :rtype instance: :haka:class:̀`Class` instance
        
        Create a new instance of the class.

    .. haka:function:: isa(instance, class)
    
        :param instance: Class instance to test.
        :paramtype instance: :haka:class:`Class` instance
        :param class: Class.
        :paramtype class: :haka:class:`Class`
        
        Check if the given instance is a *class*.

    .. haka:function:: classof(instance) -> class
    
        :param instance: Class instance to test.
        :paramtype instance: :haka:class:`Class` instance
        :return class: Class of this instance.
        :rtype class: :haka:class:`Class`
        
        Get the class of a given instance.
        
    .. haka:data:: Class.method
    
        :type: table
        
        List of methods available on instances of this class.
        
        Special methods:
        
        * *__init*: constructor function called every time a new instance of the class is created.
        * *__tostring*: string conversion function for the class.
        * *__index*: meta method __index
        * *__newindex*: meta method __newindex
        
        **Example:**
        
        ::
        
            local A = class("A")
            
            function A.method:f()
                print("f()")
            end
            
            local a = A:new()
            
            a:f() -- will print 'f()'
            
    .. haka:data:: Class.property
    
        :type: table
        
        List of properties on this class. A property is a table containing two
        functions:
        
        .. haka:function:: get(self) -> value
        
            Getter for the property. The function is called every time the user access
            the property on the object.
        
        .. haka:function:: set(self, newvalue)
        
            Setter for the property. It is called every time the user sets a new value on
            the property.
        
        **Example:**
        
        ::
        
            local A = class("A")
            
            function A.method:__init()
                self._prop = "my prop"
            end

            A.property:prop = {
                function get(self) return self._prop end,
                function set(self, value) self._prop = value end
            end
            
            local a = A:new()
            
            a.prop -- will print "my prop"
            a.prop = "foo bar"
    
    .. haka:method:: Class:addproperty(get, set)
    
        :param get: Getter for the property.
        :paramtype get: function
        :param set: Setter for the property.
        :paramtype set: function
        
        Add a dynamic property to the class instance. This property can be accessed like any other
        property but only exists on this instance.
