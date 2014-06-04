.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightinglang:: lua

SMTP protocol
-------------
SMTP stands for Simple Mail Transfer Protocol and was designed to deliver mail
reliabily. It is a command/reponses protocol that starts with a session initiation
during which the server sends first a welcoming message together with a status
code indicating if the transaction (220) has succeed or not (554). Then, the
client identifies himself using the commands EHLO ot HELO and awaits for server
response server response to proceed::

    S: 220 foo.com Simple Mail Transfer Service Ready
    C: EHLO bar.com
    S: 250-foo.com greets bar.com
    S: 250-8BITMIME
    S: 250-SIZE
    S: 250-DSN

Mail transaction starts after the above initiation phase. At each step, the
client sends a smtp command and receives one or multiple response messages
from the server. Three steps are required to transfert a mail, namely, through
MAIL, RCPT and DATA commands::

    C: MAIL FROM:<Smith@bar.com>
    S: 250 OK
    C: RCPT TO:<Jones@foo.com>
    S: 250 OK
    C: DATA
    S: 354 Start mail input; end with <CRLF>.<CRLF>
    C: some data...
    C: ...etc. etc. etc.
    C: .
    S: 250 OK

Finally, the connection ends with a QUIT message::

    C: QUIT
    S: 221 foo.com Service closing transmission channel
