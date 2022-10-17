
"""
Create the .h and .cc files for defining LabStor singletons
USAGE:
    cd /path/to/labstor
    python3 src/singleton_generator.py
"""

import re

def ToCamelCase(string):
    if string is None:
        return
    words = re.sub(r"(_|-)+", " ", string).split()
    words = [word.capitalize() for word in words]
    return "".join(words)

def ToSnakeCase(string):
    if string is None:
        return
    string = re.sub('(\.|-)+', '_', string)
    words = re.split('([A-Z][^A-Z]*)', string)
    words = [word for word in words if len(word)]
    string = "_".join(words)
    return string.lower()

class SingletonDefinition:
    def __init__(self, namespace, class_name, include):
        snake = ToSnakeCase(class_name).upper()
        self.macro_name = f"LABSTOR_{snake}"
        self.type_name = f"{self.macro_name}_T"
        if namespace is not None:
            self.class_name = f"{namespace}::{class_name}"
        else:
            self.class_name = class_name
        self.include = include

class SingletonGenerator:
    def __init__(self):
        self.defs = []

    def Add(self, namespace, class_name, include):
        self.defs.append(SingletonDefinition(
            namespace, class_name, include
        ))

    def Generate(self, cc_file, h_file):
        self._GenerateCC(cc_file)
        self._GenerateH(h_file)

    def _GenerateCC(self, path):
        lines = []
        lines.append("#include <labstor/constants/macros.h>")
        lines.append("#include <labstor/util/singleton.h>")
        lines.append("#include <labstor/constants/singleton_macros.h>")
        lines.append("")
        for defn in self.defs:
            lines.append(f"template<> std::unique_ptr<{defn.class_name}> scs::Singleton<{defn.class_name}>::obj_ = nullptr;")
        self._SaveLines(lines, path)

    def _GenerateH(self, path):
        lines = []
        lines.append("#ifndef LABSTOR_SINGLETON_MACROS_H")
        lines.append("#define LABSTOR_SINGLETON_MACROS_H")
        lines.append("")
        lines.append("#include <labstor/util/singleton.h>")
        lines.append("")
        for defn in self.defs:
            lines.append(f"#include <{defn.include}>")
            lines.append(f"#define {defn.macro_name} scs::Singleton<{defn.class_name}>::GetInstance()")
            lines.append(f"#define {defn.type_name} {defn.class_name}*")
            lines.append("")
        lines.append("#endif  // LABSTOR_SINGLETON_MACROS_H")
        self._SaveLines(lines, path)

    def _SaveLines(self, lines, path):
        text = "\n".join(lines) + "\n"
        if path is None:
            print(text)
            return
        with open(path, 'w') as fp:
            fp.write(text)

gen = SingletonGenerator()

gen.Add("labstor", "IpcManager", "labstor/ipc_manager/ipc_manager.h")
gen.Add("labstor", "ConfigurationManager", "labstor/runtime/configuration_manager.h")

gen.Generate("src/singleton.cc", "include/labstor/constants/singleton_macros.h")