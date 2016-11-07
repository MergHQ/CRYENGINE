import imp
import os
import sys
from types import ModuleType, FunctionType

import SandboxBridge
import sandbox


def _indent(lines, amount, ch=' '):
    padding = amount * ch
    return padding + ('\n' + padding).join(lines.split('\n'))


def _get_param_info(text):
    params = []
    try:
        base_split = text.split("->")
        param_info = base_split[0]
        rtype = base_split[1][:-1].strip()
        types_info = param_info[param_info.find("(") + 1: param_info.rfind(")")]
        params_docstrings = []
        if types_info:
            for one_part in types_info.split(","):
                st = one_part.strip()
                arg_split = st.split(")")
                params.append(arg_split[1])
                params_docstrings.append(":param %s %s:" % (arg_split[0][1:], arg_split[1]))
        if rtype and rtype != "None":
            params_docstrings.append(":rtype: %s" % rtype)
        params_docstring = "\n".join(params_docstrings)
    except Exception:
        params_docstring = "Error during generating autocomplete docstring"
        params = []
    return params_docstring, params


def _extract_docstring(docstring):
    stripped = docstring.strip()
    split_lines = stripped.splitlines()
    params_info = ""
    signature = ""
    if "->" in split_lines[0]:
        params_info = _get_param_info(split_lines[0])
        signature = ", ".join(params_info[1])
        params_info = params_info[0]
        split_lines = split_lines[1:]

    stripped = "\n".join(split_lines)
    stripped = stripped.strip()
    stripped = stripped.strip("\"")
    if params_info:
        stripped += "\n\n" + params_info

    return signature, stripped


def generate_auto_complete_dummy_files():
    """
    Iterates trough sandbox python namespace and generates dummy modules used later for console autocomplete

    """
    # Generate dummy python packages for autocomplete
    docs_root_path = os.path.join(sandbox.user_folder, "python", "autocomplete", "sandbox")
    if not os.path.exists(docs_root_path):
        os.makedirs(docs_root_path)

    # create empty file
    init_path = os.path.join(docs_root_path, "__init__.py")
    with file(init_path, "w+"):
        pass

    for name, attrib in sandbox.__dict__.iteritems():
        if isinstance(attrib, ModuleType):
            mod_path = os.path.join(docs_root_path, name + ".py")
            with file(mod_path, "w+") as mod_file:
                for content_name, content in attrib.__dict__.iteritems():
                    if type(content).__name__ == "function":
                        signature, docstring = _extract_docstring(content.__doc__)

                        func_str = "def %s(%s):\n" % (content_name, signature)
                        docstring = '"""\n%s\n"""\n' % docstring
                        func_str += _indent(docstring, 4)
                        func_str += "pass\n\n"
                        mod_file.write(func_str)
