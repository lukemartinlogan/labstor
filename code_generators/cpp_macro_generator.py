from code_generators.c_macro.macro_generator import CppMacroGenerator
import pathlib, os

PROJECT_ROOT=pathlib.Path(__file__).parent.parent.resolve()

CppMacroGenerator().generate(
    os.path.join(PROJECT_ROOT,
        'include/labstor/data_structures/internal/shm_container_template.h'),
    os.path.join(PROJECT_ROOT,
        'include/labstor/data_structures/internal/shm_container_macro.h'),
    "SHM_CONTAINER_TEMPLATE",
    ["CLASS_NAME", "TYPED_CLASS", "TYPED_HEADER"],
    "LABSTOR_DATA_STRUCTURES_INTERNAL_SHM_CONTAINER_MACRO_H_")