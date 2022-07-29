
import argparse
from enum import Enum
from jarvis_cd.serialize.yaml_file import YAMLFile
import yaml
import inspect, pathlib
import os

class TestCase(Enum):
    Anatomy = "io_anatomy",
    LiveUpgrade = "live_upgrade",
    WorkOrchCPU = "work_orch_cpu",
    WorkOrchReq = "work_orch_req",
    StorageAPI = "storage_api",
    IOSchedLabStor = "iosched_labstor"
    IOSchedBlksw = "iosched_blksw"

    def __str__(self):
        return str(self.value)

parser = argparse.ArgumentParser(description="LabStor Benchmark")
parser.add_argument('test_case', metavar="case", choices=list(TestCase), type=TestCase, help="The LabStor test case to run")
parser.parse_args()

bench_root = pathlib.Path(__file__).parent.resolve()
conf = YAMLFile(os.path.join(bench_root, 'config.yaml')).Load()

