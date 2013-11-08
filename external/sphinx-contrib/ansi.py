# -*- coding: utf-8 -*-
# Copyright (c) 2010, Sebastian Wiesner <lunaryorn@googlemail.com>
# All rights reserved.

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:

# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.


"""
    sphinxcontrib.ansi
    ==================

    This extension parses ANSI color codes in literal blocks.

    .. moduleauthor::  Sebastian Wiesner  <lunaryorn@googlemail.com>
"""


import re
import sys
from os import path

from docutils import nodes
from docutils.parsers import rst
from docutils.parsers.rst.directives import flag
from sphinx.util.osutil import copyfile
from sphinx.util.console import bold


class ansi_literal_block(nodes.literal_block):
    """
    Represent a literal block, that contains ANSI color codes.
    """
    pass


#: the pattern to find ANSI color codes
COLOR_PATTERN = re.compile('\x1b\\[([^m]+)m')

#: map ANSI color codes to class names
CODE_CLASS_MAP = {
    1: 'bold',
    4: 'underscore',
    30: 'black',
    31: 'red',
    32: 'green',
    33: 'yellow',
    34: 'blue',
    35: 'magenta',
    36: 'cyan',
    37: 'white',
    40: 'bg_black',
    41: 'bg_red',
    42: 'bg_green',
    43: 'bg_yellow',
    44: 'bg_blue',
    45: 'bg_magenta',
    46: 'bg_cyan',
    47: 'bg_white',
    }


class ANSIColorParser(object):
    """
    Traverse a document, look for ansi_literal_block nodes, parse these
    nodes, and replace them with literal blocks, containing proper child
    nodes for ANSI color sequences.
    """

    def _finalize_pending_nodes(self):
        """
        Finalize all pending nodes.

        Pending nodes will be append to the new nodes.
        """
        self.new_nodes.extend(self.pending_nodes)
        self.pending_nodes = []

    def _add_text(self, text):
        """
        If ``text`` is not empty, append a new Text node to the most recent
        pending node, if there is any, or to the new nodes, if there are no
        pending nodes.
        """
        if text:
            if self.pending_nodes:
                self.pending_nodes[-1].append(nodes.Text(text))
            else:
                self.new_nodes.append(nodes.Text(text))

    def _colorize_block_contents(self, block):
        raw = block.rawsource
        # create the "super" node, which contains to while block and all it
        # sub nodes, and replace the old block with it
        literal_node = nodes.literal_block()
        literal_node['classes'].append('ansi-block')
        block.replace_self(literal_node)

        # this contains "pending" nodes.  A node representing an ANSI
        # color is "pending", if it has not yet seen a reset
        self.pending_nodes = []
        # these are the nodes, that will finally be added to the
        # literal_node
        self.new_nodes = []
        # this holds the end of the last regex match
        last_end = 0
        # iterate over all color codes
        for match in COLOR_PATTERN.finditer(raw):
            # add any text preceeding this match
            head = raw[last_end:match.start()]
            self._add_text(head)
            # update the match end
            last_end = match.end()
            # get the single format codes
            codes = [int(c) for c in match.group(1).split(';')]
            if codes[-1] == 0:
                # the last code is a reset, so finalize all pending
                # nodes.
                self._finalize_pending_nodes()
            else:
                # create a new color node
                code_node = nodes.inline()
                self.pending_nodes.append(code_node)
                # and set the classes for its colors
                for code in codes:
                    code_node['classes'].append(
                        'ansi-%s' % CODE_CLASS_MAP[code])
        # add any trailing text
        tail = raw[last_end:]
        self._add_text(tail)
        # move all pending nodes to new_nodes
        self._finalize_pending_nodes()
        # and add the new nodes to the block
        literal_node.extend(self.new_nodes)

    def _strip_color_from_block_content(self, block):
        content = COLOR_PATTERN.sub('', block.rawsource)
        literal_node = nodes.literal_block(content, content)
        block.replace_self(literal_node)

    def __call__(self, app, doctree, docname):
        """
        Extract and parse all ansi escapes in ansi_literal_block nodes.
        """
        handler = self._colorize_block_contents
        if app.builder.name != 'html':
            # strip all color codes in non-html output
            handler = self._strip_color_from_block_content
        for ansi_block in doctree.traverse(ansi_literal_block):
            handler(ansi_block)


def add_stylesheet(app):
    if app.config.html_ansi_stylesheet:
        app.add_stylesheet('ansi.css')


def copy_stylesheet(app, exception):
    if app.builder.name != 'html' or exception:
        return
    stylesheet = app.config.html_ansi_stylesheet
    if stylesheet:
        app.info(bold('Copying ansi stylesheet... '), nonl=True)
        dest = path.join(app.builder.outdir, '_static', 'ansi.css')
        source = path.abspath(path.dirname(__file__))
        copyfile(path.join(source, stylesheet), dest)
        app.info('done')


def unescape(text):
    regex = re.compile('\\\\(\\\\|[0-7]{1,3}|x[0-9a-f]{1,2}|[\'"abfnrt]|.|$)')
    def replace(m):
        b = m.group(1)
        if len(b) == 0:
            raise ValueError("Invalid character escape: '\\'.")
        if b[0] == 'a': return '\a'
        elif b[0] == 'b': return '\b'
        elif b[0] == 'f': return '\f'
        elif b[0] == 'n': return '\n'
        elif b[0] == 'r': return '\r'
        elif b[0] == 't': return '\t'
        elif b[0] == 'x': return bytes((int(b[1:], 16), )).decode('ascii')
        elif b[0] >= '0' and b[0] <= 7: return bytes((int(b[1:], 8), )).decode('ascii')
        else: return b
    return regex.sub(replace, text)


class ANSIBlockDirective(rst.Directive):
    """
    This directive interprets its content as literal block with ANSI color
    codes.

    The content is decoded using ``string-escape`` to allow symbolic names
    as \x1b being used instead of the real escape character.
    """

    has_content = True

    option_spec = dict(string_escape=flag)

    def run(self):
        text = '\n'.join(self.content)
        if 'string_escape' in self.options:
            if sys.version_info < (3, 0): text = text.decode('string_escape')
            else: text = unescape(text)
        return [ansi_literal_block(text, text)]


def setup(app):
    app.require_sphinx('1.0')
    app.add_config_value('html_ansi_stylesheet', None, 'env')
    app.add_directive('ansi-block', ANSIBlockDirective)
    app.connect('builder-inited', add_stylesheet)
    app.connect('build-finished', copy_stylesheet)
    app.connect('doctree-resolved', ANSIColorParser())
