.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: c

Modules
=======

.. c:type:: struct module

    Module structure.

    .. c:type:: type

        Module type.

        .. c:var:: MODULE_UNKNOWN
                   MODULE_PACKET
                   MODULE_LOG
                   MODULE_EXTENSION

    .. c:var:: const wchar_t *name
    .. c:var:: const wchar_t *description
    .. c:var:: const wchar_t *author

    .. c:function:: int (*init)(struct parameters *args)

        Initialize the module. This function is called by the application.

        :returns: Non zero in case of an error. In this case the module will be
            unloaded but cleanup is not going to be called.

    .. c:function:: void (*cleanup)()

        Cleanup the module. This function is called by the application when the
        module is unloaded.


.. c:type:: struct packet_module

    Packet module used to interact with the low-level packets. The module
    will be used to receive packets and set a verdict on them. It also define
    an interface to access the packet fields.

    .. c:var:: struct module module

    .. c:function:: bool (*multi_threaded)()

        Does this module supports multi-threading.

    .. c:function:: struct packet_module_state *(*init_state)(int thread_id)

        Initialize the packet module state. This function will be called to create
        multiple states if the module supports multi-threading.

    .. c:function:: void (*cleanup_state)(struct packet_module_state *state)

        Cleanup the packet module state.

    .. c:function:: int (*receive)(struct packet_module_state *state, struct packet **pkt)

        Callback used to receive a new packet. This function should block until
        a packet is received.

        :returns: Non zero in case of error.

    .. c:function:: void (*verdict)(struct packet *pkt, filter_result result)

        Apply a verdict on a received packet. The module should then apply this
        verdict on the underlying packet.

        :param pkt: The received packet. After calling this function the packet address
            is never used again by the application but allow the module to free it if needed.
        :param result: The verdict to apply to this packet.

    .. c:function:: size_t (*get_length)(struct packet *pkt)

        Get the length of a packet.

    .. c:function:: uint8 *(*make_modifiable)(struct packet *pkt)

        Make the packet modifiable.

    .. c:function:: int (*resize)(struct packet *pkt, size_t size)

        Resize the packet to a new size.

    .. c:function:: uint64 (*get_id)(struct packet *pkt)

        Get the id fo the packet.

    .. c:function:: const uint8 *(*get_data)(struct packet *pkt)

        Get the data of a packet.

    .. c:function:: const char *(*get_dissector)(struct packet *pkt)

        Get the packet dissector.


.. c:function:: struct module *module_load(const char *module_name,...)

    Load a module given its name. It is not needed to call `module_addref` on the result
    as this is done before returning.

    :returns: The loaded module structure or NULL in case of an error.

.. c:function:: void module_addref(struct module *module)

    Keep the module. Must match with a call to `module_release`
    otherwise the module will not be able to be removed correctly
    when unused.

.. c:function:: void module_release(struct module *module)

    Release a module.

.. c:function:: void module_set_path(const char *path)

    Set the path used to load haka modules. This path must be in the form:

    .. code-block:: bash

        path/to/modules/*;another/path/*

.. c:function:: const char *module_get_path()

    Get the modules path.
