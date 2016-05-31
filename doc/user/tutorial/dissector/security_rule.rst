.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Filtering spam
^^^^^^^^^^^^^^
As usual, we load first the smtp module disector. Then, we install our newly
created dissector on flow addressed to port 25.

Next, we attach our security rule on event `command` which will pass to the
evaluaton function the parsed command message. The rest of the rule is basic:
we get the parmeter of MAIL command and check if the dommain name is banned.

.. literalinclude:: ../../../../sample/smtp_dissector/smtp_spam_filter.lua
    :language: lua
    :tab-width: 4

Dumping mail content
^^^^^^^^^^^^^^^^^^^^

The second security rule is attached on `mail` event and allows us to get the
content of a mail in a streamed mode.

.. literalinclude:: ../../../../sample/smtp_dissector/smtp_mail_content.lua
    :language: lua
    :tab-width: 4

.. note:: These security rules are available in <haka_install_path>/share/haka/sample/smtp_dissetor. A smtp capture sample ``smtp.pcap`` is also provided in order to replay the above rules.
