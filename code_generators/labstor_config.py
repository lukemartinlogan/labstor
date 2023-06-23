"""
USAGE:
    cd code_generators/bin
    python3 labstor_config.py

OUTPUT:
    ${HERMES}/src/config_client_default.h (if client)
    ${HERMES}/src/config_server_default.h (if server)
"""

from code_generators.labstor_config.generator import create_config
from code_generators.util.paths import LABSTOR_ROOT

create_config(
    path=f"{LABSTOR_ROOT}/config/labstor_client_default.yaml",
    var_name="kClientDefaultConfigStr",
    config_path=f"{LABSTOR_ROOT}/include/labstor/config/config_client_default.h",
    macro_name="CLIENT"
)

create_config(
    path=f"{LABSTOR_ROOT}/config/labstor_server_default.yaml",
    var_name="kServerDefaultConfigStr",
    config_path=f"{LABSTOR_ROOT}/include/labstor/config/config_server_default.h",
    macro_name="SERVER"
)