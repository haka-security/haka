# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import re

from docutils import nodes
from docutils.parsers.rst import directives

from sphinx import addnodes
from sphinx.roles import XRefRole
from sphinx.locale import l_, _
from sphinx.domains import Domain, ObjType, Index
from sphinx.directives import ObjectDescription
from sphinx.util.nodes import make_refnode
from sphinx.util.compat import Directive
from sphinx.util.docfields import Field, GroupedField, TypedField
from sphinx.writers.html import HTMLTranslator


# REs
lua_signature_re = re.compile(
    r'''^ ([\w\./\-]+[:.])?       # class name(s)
          ([\w/\-/]+)  \s*        # thing name
          (?: ([({])(.*)([)}]))?  # optional: arguments
          (?:\s* -> \s* (.*))?    # optional: return annotation
          $                       # and nothing more
          ''', re.VERBOSE)

def _desc_parameterlist(argstart, argend):
    node = addnodes.desc_parameterlist()
    node.param_start = argstart
    node.param_end = argend
    return node

def _pseudo_parse_arglist(signode, argstart, arglist, argend):
    """"Parse" a list of arguments separated by commas.

    Arguments can have "optional" annotations given by enclosing them in
    brackets.  Currently, this will split at any comma, even if it's inside a
    string literal (e.g. default argument value).
    """
    paramlist = _desc_parameterlist(argstart, argend)
    stack = [paramlist]
    for argument in arglist.split(','):
        argument = argument.strip()
        ends_open = ends_close = 0
        while argument.startswith('['):
            stack.append(addnodes.desc_optional())
            stack[-2] += stack[-1]
            argument = argument[1:].strip()

        while argument.startswith(']'):
            stack.pop()
            argument = argument[1:].strip()

        while argument.endswith(']'):
            ends_close += 1
            argument = argument[:-1].strip()

        while argument.endswith('['):
            ends_open += 1
            argument = argument[:-1].strip()

        if argument:
            stack[-1] += addnodes.desc_parameter(argument, argument)
            while ends_open:
                stack.append(addnodes.desc_optional())
                stack[-2] += stack[-1]
                ends_open -= 1

            while ends_close:
                stack.pop()
                ends_close -= 1

    if len(stack) != 1:
        raise IndexError

    signode += paramlist


# Haka objects

class HakaObject(ObjectDescription):
    """
    Description of a general Haka object.
    """
    option_spec = {
        'noindex': directives.flag,
        'annotation': directives.unchanged,
    }

    def get_signature_prefix(self, sig):
        """May return a prefix to put before the object name in the
        signature."""
        return ''

    def needs_arglist(self):
        """May return true if an empty argument list is to be generated even if
        the document contains none."""
        return False

    def needs_module(self):
        """May return true if the module name should be displayed."""
        return self.context == None

    def handle_signature(self, sig, signode):
        m = lua_signature_re.match(sig)
        if m is None:
            raise ValueError

        context, name, argstart, arglist, argend, retann = m.groups()
        self.context = (context and context[:1]) or None
        self.module = self.options.get('module', self.env.temp_data.get('haka:module'))

        add_module = True
        fullname = name

        signode['module'] = self.module
        signode['class'] = self.context
        signode['fullname'] = fullname

        sig_prefix = self.get_signature_prefix(sig)
        if sig_prefix:
            signode += addnodes.desc_annotation(sig_prefix, sig_prefix)

        if context:
            signode += addnodes.desc_addname(context, context)
            context = context[:-1]

        anno = self.options.get('annotation')

        signode += addnodes.desc_name(name, name)
        if not arglist:
            if self.needs_arglist():
                # for callables, add an empty parameter list
                listnode = _desc_parameterlist(argstart, argend)
                signode += listnode
        else:
            _pseudo_parse_arglist(signode, argstart, arglist, argend)

        if retann:
            signode += addnodes.desc_returns(retann, retann)
        if anno:
            signode += addnodes.desc_annotation(' ' + anno, ' ' + anno)

        if self.module and self.needs_module():
            modname = ' (in module %s)' % (self.module)
            signode += addnodes.desc_annotation(modname, modname)

        return {'fullname':fullname, 'context':context}

    def add_target_and_index(self, names, sig, signode):
        modname = self.module or ''
        fullname = (modname and modname + '.' or '') + names['fullname']
        fullid = 'haka-' + fullname

        if fullid not in self.state.document.ids:
            signode['names'].append(fullname)
            signode['ids'].append(fullid)
            signode['first'] = (not self.names)
            self.state.document.note_explicit_target(signode)
            objects = self.env.domaindata['haka']['objects']
            if fullname in objects:
                self.state_machine.reporter.warning(
                    'duplicate object description of %s, ' % fullname +
                    'other instance in ' +
                    self.env.doc2path(objects[fullname]) +
                    ', use :noindex: for one of them',
                    line=self.lineno)
            objects[fullname] = (self.env.docname, self.objtype)

        indextext = self.get_index_text(names)
        if indextext:
            self.indexnode['entries'].append(('single', indextext,
                                              fullid, ''))

    def get_index_text(self, names):
        ret = []
        if names['context']: ret.append("%s" % (names['context']))
        if hasattr(self.__class__, 'fulltypename'):
            ret.append("%s" % (self.__class__.fulltypename))
        else:
            ret.append("%s" % (self.__class__.typename))
        if self.module: ret.append("in module %s" % (self.module))
        return "%s (%s)" % (names['fullname'], ' '.join(ret))

    def get_signature_prefix(self, sig):
        return "%s " % (self.__class__.typename)

class HakaClass(HakaObject):
    doc_field_types = [
        Field('extend', label=l_('Extends'), has_arg=False,
              names=('extend',)),
    ]

    typename = l_("object")

    def before_content(self):
        HakaObject.before_content(self)
        if self.names:
            self.env.temp_data['haka:class'] = self.names[0]['fullname']

    def after_content(self):
        HakaObject.after_content(self)
        if self.names:
            self.env.temp_data['haka:class'] = None

class HakaDissector(HakaClass):
    typename = l_("dissector")

class HakaFunction(HakaObject):
    typename = l_("function")

    doc_field_types = [
        TypedField('parameter', label=l_('Parameters'),
                   names=('param', 'parameter', 'arg', 'argument'),
                   typerolename='obj', typenames=('paramtype', 'type')),
        Field('returnvalue', label=l_('Returns'), has_arg=False,
              names=('returns', 'return')),
        Field('returntype', label=l_('Return type'), has_arg=False,
              names=('rtype',)),
    ]

    def needs_arglist(self):
        return True

class HakaMethod(HakaFunction):
    typename = l_("method")

class HakaEvent(HakaObject):
    typename = l_("event")

class HakaData(HakaObject):
    typename = l_("data")

    doc_field_types = [
        Field('type', label=l_('Type'), has_arg=False,
              names=('type',)),
    ]

class HakaAttribute(HakaObject):
    typename = l_("attr")
    fulltypename = l_("attribute")

    doc_field_types = [
        Field('type', label=l_('Type'), has_arg=False,
              names=('type',)),
    ]


class HakaModule(Directive):
    """
    Directive to mark description of a new module.
    """

    has_content = False
    required_arguments = 1
    optional_arguments = 0
    final_argument_whitespace = False
    option_spec = {
        'platform': lambda x: x,
        'synopsis': lambda x: x,
        'noindex': directives.flag,
        'deprecated': directives.flag,
    }

    def run(self):
        env = self.state.document.settings.env
        modname = self.arguments[0].strip()
        noindex = 'noindex' in self.options
        env.temp_data['haka:module'] = modname
        ret = []
        if not noindex:
            env.domaindata['haka']['modules'][modname] = \
                (env.docname, self.options.get('synopsis', ''),
                 self.options.get('platform', ''), 'deprecated' in self.options)
            # make a duplicate entry in 'objects' to facilitate searching for
            # the module in LuaDomain.find_obj()
            env.domaindata['haka']['objects'][modname] = (env.docname, 'module')
            targetnode = nodes.target('', '', ids=['module-' + modname],
                                      ismod=True)
            self.state.document.note_explicit_target(targetnode)
            # the platform and synopsis aren't printed; in fact, they are only
            # used in the modindex currently
            ret.append(targetnode)
            indextext = _('%s (module)') % modname
            inode = addnodes.index(entries=[('single', indextext,
                                             'module-' + modname, '')])
            ret.append(inode)
        return ret

class HakaCurrentModule(Directive):
    """
    This directive is just to tell Sphinx that we're documenting
    stuff in module foo, but links to module foo won't lead here.
    """
    has_content = False
    required_arguments = 1
    optional_arguments = 0
    final_argument_whitespace = False
    option_spec = {}

    def run(self):
        env = self.state.document.settings.env
        modname = self.arguments[0].strip()
        if modname == 'None':
            env.temp_data['haka:module'] = None
        else:
            env.temp_data['haka:module'] = modname
        return []


class HakaXRefRole(XRefRole):
    def process_link(self, env, refnode, has_explicit_title, title, target):
        refnode['haka:module'] = env.temp_data.get('haka:module')
        refnode['haka:class'] = env.temp_data.get('haka:class')
        if not has_explicit_title:
            title = title.lstrip('.')   # only has a meaning for the target
            target = target.lstrip('~') # only has a meaning for the title
            # if the first character is a tilde, don't display the module/class
            # parts of the contents
            if title[0:1] == '~':
                title = title[1:]
                dot = max(title.rfind('.'), title.rfind(':'))
                if dot != -1:
                    title = title[dot+1:]
        # if the first character is a dot, search more specific namespaces first
        # else search builtins first
        if target[0:1] == '.':
            target = target[1:]
            refnode['refspecific'] = True
        return title, target

class HakaModuleIndex(Index):
    """
    Index subclass to provide the Haka module index.
    """
    name = 'modindex'
    localname = l_('Haka Module Index')
    shortname = l_('modules')

    def generate(self, docnames=None):
        content = {}
        # list of prefixes to ignore
        ignores = self.domain.env.config['modindex_common_prefix']
        ignores = sorted(ignores, key=len, reverse=True)
        # list of all modules, sorted by module name
        modules = sorted(self.domain.data['modules'].items(),
                         key=lambda x: x[0].lower())
        # sort out collapsable modules
        prev_modname = ''
        num_toplevels = 0
        for modname, (docname, synopsis, platforms, deprecated) in modules:
            if docnames and docname not in docnames:
                continue

            for ignore in ignores:
                if modname.startswith(ignore):
                    modname = modname[len(ignore):]
                    stripped = ignore
                    break
            else:
                stripped = ''

            # we stripped the whole module name?
            if not modname:
                modname, stripped = stripped, ''

            entries = content.setdefault(modname[0].lower(), [])

            package = modname.split('.')[0]
            if package != modname:
                # it's a submodule
                if prev_modname == package:
                    # first submodule - make parent a group head
                    entries[-1][1] = 1
                elif not prev_modname.startswith(package):
                    # submodule without parent in list, add dummy entry
                    entries.append([stripped + package, 1, '', '', '', '', ''])
                subtype = 2
            else:
                num_toplevels += 1
                subtype = 0

            qualifier = deprecated and _('Deprecated') or ''
            entries.append([stripped + modname, subtype, docname,
                            'module-' + stripped + modname, platforms,
                            qualifier, synopsis])
            prev_modname = modname

        # apply heuristics when to collapse modindex at page load:
        # only collapse if number of toplevel modules is larger than
        # number of submodules
        collapse = len(modules) - num_toplevels < num_toplevels

        # sort by first letter
        content = sorted(content.items())

        return content, collapse


# Haka domain

class HakaDomain(Domain):
    """Haka language domain."""
    name = 'haka'
    label = 'Haka'
    object_types = {
        'class':         ObjType(l_('class'),      'class',  'obj'),
        'dissector':     ObjType(l_('dissector'),  'class',  'obj'),
        'event':         ObjType(l_('event'),      'event',  'obj'),
        'attribute':     ObjType(l_('attribute'),  'data',   'obj'),
        'function':      ObjType(l_('function'),   'func',   'obj'),
        'method':        ObjType(l_('method'),     'func',   'obj'),
        'module':        ObjType(l_('module'),     'mod',    'obj'),
        'data':          ObjType(l_('data'),       'data',   'obj'),
    }

    directives = {
        'class':           HakaClass,
        'dissector':       HakaDissector,
        'function':        HakaFunction,
        'method':          HakaMethod,
        'data':            HakaData,
        'event':           HakaEvent,
        'attribute':       HakaAttribute,
        'module':          HakaModule,
        'currentmodule':   HakaCurrentModule,
    }
    roles = {
        'data':  HakaXRefRole(),
        'func':  HakaXRefRole(fix_parens=True),
        'class': HakaXRefRole(),
        'mod':   HakaXRefRole(),
    }
    initial_data = {
        'objects': {},     # fullname -> docname, objtype
        'modules': {},     # modname -> docname, synopsis, platform, deprecated
        'inheritance': {}, # class -> [ derived ]
    }
    indices = [
        HakaModuleIndex,
    ]

    def clear_doc(self, docname):
        for fullname, (fn, _) in list(self.data['objects'].items()):
            if fn == docname:
                del self.data['objects'][fullname]
        for modname, (fn, _, _, _) in list(self.data['modules'].items()):
            if fn == docname:
                del self.data['modules'][modname]

    def find_obj(self, env, modname, classname, name, type, searchmode=0):
        # skip parens
        if name[-2:] == '()':
            name = name[:-2]

        if not name:
            return []

        objects = self.data['objects']
        matches = []

        newname = None
        if searchmode == 1:
            objtypes = self.objtypes_for_role(type)
            if modname and classname:
                fullname = modname + '.' + classname + '.' + name
                if fullname in objects and objects[fullname][1] in objtypes:
                    newname = fullname
            if not newname:
                if modname and modname + '.' + name in objects and \
                   objects[modname + '.' + name][1] in objtypes:
                    newname = modname + '.' + name
                elif name in objects and objects[name][1] in objtypes:
                    newname = name
                else:
                    # "fuzzy" searching mode
                    searchname = '.' + name
                    matches = [(oname, objects[oname]) for oname in objects
                               if oname.endswith(searchname)
                               and objects[oname][1] in objtypes]
        else:
            # NOTE: searching for exact match, object type is not considered
            if name in objects:
                newname = name
            elif type == 'mod':
                # only exact matches allowed for modules
                return []
            elif classname and classname + '.' + name in objects:
                newname = classname + '.' + name
            elif modname and modname + '.' + name in objects:
                newname = modname + '.' + name
            elif modname and classname and \
                     modname + '.' + classname + '.' + name in objects:
                newname = modname + '.' + classname + '.' + name
            # special case: builtin exceptions have module "exceptions" set
            elif type == 'exc' and '.' not in name and \
                 'exceptions.' + name in objects:
                newname = 'exceptions.' + name
            # special case: object methods
            elif type in ('func', 'meth') and '.' not in name and \
                 'object.' + name in objects:
                newname = 'object.' + name
        if newname is not None:
            matches.append((newname, objects[newname]))
        return matches

    def resolve_xref(self, env, fromdocname, builder,
                     type, target, node, contnode):
        modname = node.get('haka:module')
        clsname = node.get('haka:class')
        searchmode = node.hasattr('refspecific') and 1 or 0
        matches = self.find_obj(env, modname, clsname, target,
                                type, searchmode)
        if not matches:
            return None
        elif len(matches) > 1:
            env.warn_node(
                'more than one target found for cross-reference '
                '%r: %s' % (target, ', '.join(match[0] for match in matches)),
                node)
        name, obj = matches[0]

        if obj[1] == 'module':
            # get additional info for modules
            docname, synopsis, platform, deprecated = self.data['modules'][name]
            assert docname == obj[0]
            title = name
            if synopsis:
                title += ': ' + synopsis
            if deprecated:
                title += _(' (deprecated)')
            if platform:
                title += ' (' + platform + ')'
            return make_refnode(builder, fromdocname, docname,
                                'module-' + name, contnode, title)
        else:
            return make_refnode(builder, fromdocname, obj[0], 'haka-' + name,
                                contnode, name)

    def get_objects(self):
        for modname, info in self.data['modules'].items():
            yield (modname, modname, 'module', info[0], 'module-' + modname, 0)
        for refname, (docname, type) in self.data['objects'].items():
            yield (refname, refname, type, docname, refname, 1)


def setup(app):
    app.add_domain(HakaDomain)
