
.. highlightlang:: lua

Defining rules
==============

This section introduces how to define security rules.

Single rule :lua:mod:`haka.rule`
--------------------------------

As detailed hereater, a rule is made of a list of hooks and an evaluation function:

.. lua:class:: rule

    .. lua:data:: hooks

        An array of hook names where the rule should be installed.

    .. lua:method:: eval(self, d)

        The function to call to evaluate the rule.
        
        :param d: The dissector data.
        :paramtype d: :lua:class:`dissector_data`

.. lua:function:: rule(r)

    Register a new rule.

    :param r: Rule description.
    :paramtype r: :lua:class:`rule`

Example: ::

    haka.rule {
        hooks = { "ipv4-up" },
        eval = function (self, pkt)
            if pkt.src == ipv4.addr("192.168.1.2") then
                pkt:drop()
            end
        end
    }

Rule group :lua:mod:`haka.rule_group`
-------------------------------------

Rule group allow to customize the rule evaluation.

.. lua:class:: rule_group

    .. lua:data:: name

        Name of the group.

    .. lua:method:: init(self, d)

        This function is called whenever a group start to be evaluated. `d` is the
        dissector data for the current hook (:lua:class:`dissector_data`).

    .. lua:method:: fini(self, d)

        If all the rules of the group have been evaluated, this callback is
        called at the end. `d` is the dissector data for the current hook
        (:lua:class:`dissector_data`).

    .. lua:method:: continue(self, d, ret)

        After each rule evaluation, the function is called to know if the evaluation
        of the other rules should continue. If not, the other rules will be skipped.
        
        :param ret: Value returned by the evaluated rule.
        :param d: Data that where given to the evaluated rule.
        :paramtype d: :lua:class:`dissector_data`

    .. lua:method:: rule(self, r)

        Register a rule for this group.

        .. seealso:: :lua:func:`haka.rule`.

.. lua:function:: rule_group(rg)

    Register a new rule group. `rg` should be a table that will be used to initialize the
    rule group. It can contains `name`, `init`, `fini` and `continue`.

    :returns: The new group.
    :rtype: :lua:class:`rule_group`

Example: ::

    local group = haka.rule_group {
        name = "group",
        init = function (self, pkt)
            haka.log.debug("filter", "Entering packet filtering rules : %d --> %d", pkt.tcp.srcport, pkt.tcp.dstport)
        end,
        fini = function (self, pkt)
            haka.alert{
                description = "Packet dropped : drop by default",
                targets = { haka.alert.service("tcp", pkt.tcp.dstport) }
            }
            pkt:drop()
        end,
        continue = function (self, pkt, ret)
            return not ret
        end
    }

    group:rule {
        hooks = { 'tcp-connection-new' },
        eval = function (self, pkt)
            -- Accept connection to TCP port 80
            if pkt.tcp.dstport == 80 then
                haka.log("Filter", "Authorizing traffic on port 80")
                return true
            end
        end
    }

    group:rule {
        hooks = { 'tcp-connection-new' },
        eval = function (self, pkt)
            -- Accept connection to TCP port 22
            if pkt.tcp.dstport == 22 then
                haka.log("Filter", "Authorizing traffic on port 22")
                return true
            end
        end
    }
