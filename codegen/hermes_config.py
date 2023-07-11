"""
USAGE:
    cd code_generators/bin
    python3 hermes_config.py

OUTPUT:
    ${HERMES}/src/config_client_default.h (if client)
    ${HERMES}/src/config_server_default.h (if server)
"""

from codegen.labstor_config.generator import create_config
from codegen.util.paths import LABSTOR_ROOT

HERMES_ROOT = f'{LABSTOR_ROOT}/tasks/hermes'
create_config(
    path=f"{HERMES_ROOT}/config/hermes_client_default.yaml",
    var_name="kClientDefaultConfigStr",
    config_path=f"{HERMES_ROOT}/include/hermes/config_client_default.h",
    macro_name="CLIENT"
)

create_config(
    path=f"{HERMES_ROOT}/config/hermes_server_default.yaml",
    var_name="kServerDefaultConfigStr",
    config_path=f"{HERMES_ROOT}/include/hermes/config_server_default.h",
    macro_name="SERVER"
)
