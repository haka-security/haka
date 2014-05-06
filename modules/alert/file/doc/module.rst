.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

File alert `alert/file`
===========================

Description
^^^^^^^^^^^

This module will output all alerts to a given file.

Parameters
^^^^^^^^^^

.. describe:: file

    Absolute file path where alert will end up.

.. describe:: format

    String determining output format of alert.
    The following format are allowed :

    - `oneline`
    - `pretty`

    Default to `pretty`.

Example :

.. code-block:: ini

    # Ignore alerts
    file = "/dev/null"
    format = "oneliner"
