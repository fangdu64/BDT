#/dcs01/gcode/fdu1/install/gcc-4.4.7/bdt-0.1.1/ThirdParty/python/bin/python3.3
import os
import sys, traceback
import getopt
import subprocess
import shutil
import time
from datetime import datetime, date
import random

BDT_HomeDir='/dcs01/gcode/fdu1/install/gcc-4.4.7/bdt'

Platform = None
if Platform == "Windows":
    # this file will be at install\
    bdtInstallDir = BDT_HomeDir
    icePyDir = os.path.abspath(bdtInstallDir+"/dependency/IcePy")
    bdtPyDir = os.path.abspath(bdtInstallDir+"/bdt/bdtPy")
    for dir in [icePyDir, bdtPyDir]:
        if dir not in sys.path:
            sys.path.append(dir)

import iBSConfig
iBSConfig.BDT_HomeDir = BDT_HomeDir
import iBSDefines
import bdtUtil
import bigKmeansUtil
import iBSFCDClient as fcdc
import bigMatUtil
import bdvdUtil
import iBS
import Ice

out_dir = '/dcs01/gcode/fdu1/repositories/BDT/examples/analysis/DukeUwDnase/s02-bdvd/out_fdu_home/run/1-data-mat'
out_obj = iBSDefines.loadPickle(
        iBSDefines.derivePickleFile(out_dir))

