
.. highlightlang:: c

Log & Alert
===========

Log
---

.. c:type:: log_level

    Logging level constants.

    .. c:member:: HAKA_LOG_FATAL
                  HAKA_LOG_ERROR
                  HAKA_LOG_WARNING
                  HAKA_LOG_INFO
                  HAKA_LOG_DEBUG

.. c:function:: const char *level_to_str(log_level level)

    Convert a logging level to a human readable string.

    :returns: A string representing the logging level. This string is constant and should
        not be freed.

.. c:function:: void message(log_level level, const wchar_t *module, const wchar_t *message)

    Log a message without string formating.

.. c:function:: void messagef(log_level level, const wchar_t *module, const wchar_t *fmt, ...)

    Log a message with string formating.

.. c:function:: void setlevel(log_level level, const wchar_t *module)

    Set the logging level to display for a given module name. The `module` parameter can be
    `NULL` in which case it will set the default level.

.. c:function:: log_level getlevel(const wchar_t *module)

    Get the logging level for a given module name.

Alert
-----

.. c:type:: alert_level

    .. c:member:: HAKA_ALERT_LOW
                  HAKA_ALERT_MEDIUM
                  HAKA_ALERT_HIGH
                  HAKA_ALERT_NUMERIC

.. c:type:: alert_completion

    .. c:member:: HAKA_ALERT_FAILED
                  HAKA_ALERT_SUCCESSFUL

.. c:type:: alert_node_type

    .. c:member:: HAKA_ALERT_NODE_ADDRESS
                  HAKA_ALERT_NODE_SERVICE

.. c:type:: struct alert

    Structure used to describe the alert.

    .. c:member:: time_us start_time
    .. c:member:: time_us end_time

        Alert times.

    .. c:member:: wchar_t *description

        Description of the alert.

    .. c:member:: alert_level severity

        Severity of the alert.

    .. c:member:: alert_level confidence

        Confidence of the detection.

    .. c:member:: double confidence_num

        If `confidence` is HAKA_ALERT_NUMERIC, set this value as custom
        confidence value.

    .. c:member:: alert_completion completion

        Completion of the alert.

    .. c:member:: wchar_t *method_description

         Description of the method used.

    .. c:member:: wchar_t **method_ref

        NULL terminated list of references.

    .. c:member:: struct alert_node **sources

        NULL terminated list of alert sources.

    .. c:member:: struct alert_node **targets

        NULL terminated list of alert targets.

    .. c:member:: size_t alert_ref_count

        Number of external alert references.

    .. c:member:: uint64 *alert_ref

        Array of alert ids.

.. c:macro:: ALERT(name, nsrc, ntgt)

    Construct a static alert description.

    :param name: Name of the variable to create.
    :param nsrc: Number of sources.
    :param ntgt: Number of targets.

    Example: ::

        ALERT(invalid_packet, 1, 1)
            description: L"invalid tcp packet, size is too small",
            severity: HAKA_ALERT_LOW,
        ENDALERT

.. c:macro:: ALERT_NODE(alert, mode, index, type, ...)

    Append an alert node.

    :param alert: Alert name.
    :param mode: ``sources`` or ``target``.
    :param index: Index of the node.
    :param type: Type of node (see :c:type:`alert_node_type`).
    :param ...: List of strings (:c:type:`wchar_t *`).

    Example: ::

        ALERT_NODE(invalid_packet, sources, 0, HAKA_ALERT_NODE_ADDRESS, "127.0.0.1");

.. c:macro:: ALERT_REF(alert, count, ...)

    Append a list of alert references.

    :param alert: Alert name.
    :param count: Number of references.
    :param ...: List of alert ids.

.. c:macro:: ALERT_METHOD_REF(alert, ...)

    Append a list of method references.

    :param alert: Alert name.
    :param ...: List of strings (:c:type:`wchar_t *`).

.. c:function:: uint64 alert(const struct alert *alert)

    Raise a new alert.

    :returns: A unique alert id.

.. c:function:: bool alert_update(uint64 id, const struct alert *alert)

    Update an existing alert.
