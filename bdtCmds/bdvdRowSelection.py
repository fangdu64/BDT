#!__PYTHON_BIN_PATH__

"""
command line tool for bdvd row selection
"""
import os
import sys, traceback
import getopt
import subprocess
import shutil
import time
from datetime import datetime, date
import random

BDT_HomeDir=os.path.dirname(os.path.abspath(__file__))

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

gParams = None
gRunner = None
gSteps = ['1-run-selection']
gRowSelectors = ['with-signal', 'high-variation']
gComponents = ['signal', 'artifact', 'random', 'signal+random']
gScales = ['mlog', 'original']
gArtifactDetections = ['aggressive', 'conservative']

class BDVDParams:
    def __init__(self):
        self.output_dir = None
        self.logging_dir = None
        self.start_from = gSteps[0]
        self.dry_run = False
        self.remove_before_run = True
        self.pipeline_rundir = None
        self.workercnt = 4
        self.memory_size = 2000
        self.row_selector = gRowSelectors[0]
        self.output_file = None
        self.with_signal_threshold = None
        self.with_signal_col_cnt = None
        self.with_signal_sampling_row_cnt = None
        self.rowidx_from = None
        self.rowidx_to = None
        self.column_ids = None
        self.bdvd_dir = None
        self.evaluate_component = None
        self.evaluate_scale =gScales[0]
        self.artifact_detection =gArtifactDetections[1]
        self.unwanted_factors = None
        self.known_factors = None

    def parse_options(self, argv):
        args = argv[1:]
        bdvdArgv = args
        try:
            opts, args = getopt.getopt(bdvdArgv, "",
                ["thread-num=",
                "memory-size=",
                "out=",
                "column-ids=",
                "row-selector=",
                "output-file=",
                "rowidx-from=",
                "rowidx-to=",
                "bdvd-dir=",
                "with-signal-threshold=",
                "with-signal-col-cnt=",
                "with-signal-sampling-row-cnt=",
                "component=",
                "scale=",
                "artifact-detection=",
                "unwanted-factors=",
                "known-factors="])

        except getopt.error as msg:
            raise iBSDefines.BdtUsage(msg)

        for option, value in opts:
            if option == "--thread-num":
                self.workercnt = int(value)
            if option == "--memory-size":
                self.memory_size = int(value)
            if option in ("-o", "--out"):
                self.output_dir = value
            if option == "--row-selector":
                allowedValues = gRowSelectors;
                if value not in allowedValues:
                    raise iBSDefines.BdtUsage('--row-selector should be one of the {0}'.format(allowedValues))
                self.row_selector = value
            if option == "--rowidx-from":
                self.rowidx_from = int(value)
            if option == "--rowidx-to":
                self.rowidx_to = int(value)
            if option =="--column-ids":
                self.column_ids = bdtUtil.parseIntSeq(value)
                if len(self.column_ids)<1:
                    raise iBSDefines.BdtUsage('invalid column-ids: {0}'.format(value))
            if option =="--bdvd-dir":
                self.bdvd_dir = value
            if option == "--component":
                allowedValues = gComponents;
                if value not in allowedValues:
                    raise iBSDefines.BdtUsage('--component should be one of the {0}'.format(allowedValues))
                self.evaluate_component = value
            if option == "--scale":
                allowedValues = gScales;
                if value not in allowedValues:
                    raise iBSDefines.BdtUsage('--scale should be one of the {0}'.format(allowedValues))
                self.evaluate_scale = value
            if option == "--artifact-detection":
                allowedValues = gArtifactDetections;
                if value not in allowedValues:
                    raise iBSDefines.BdtUsage('--artifact-detection should be one of the {0}'.format(allowedValues))
                self.artifact_detection = value
            if option =="--unwanted-factors":
                self.unwanted_factors = bdtUtil.parseIntSeq(value)
                if len(self.unwanted_factors)<1:
                    raise iBSDefines.BdtUsage('invalid unwanted_factors: {0}'.format(value))
            if option =="--known-factors":
                self.known_factors = bdtUtil.parseIntSeq(value)
                if len(self.known_factors)<1:
                    raise iBSDefines.BdtUsage('invalid known-factors: {0}'.format(value))
            if option == "--with-signal-threshold":
                self.with_signal_threshold = float(value)
            if option == "--with-signal-col-cnt":
                self.with_signal_col_cnt = int(value)
            if option == "--with-signal-sampling-row-cnt":
                self.with_signal_sampling_row_cnt = int(value)
            if option == "--output-file":
                self.output_file = value


        requiredNames = ['--out', '--bdvd-dir', '--component', '--unwanted-factors']
        providedValues = [self.output_dir, self.bdvd_dir, self.evaluate_component, self.unwanted_factors]
        noneIdx = bdtUtil.getFirstNone(providedValues)
        if noneIdx != -1:
            raise iBSDefines.BdtUsage("{0} is required".format(requiredNames[noneIdx]))

        self.bdvd_dir = os.path.abspath(self.bdvd_dir)
        self.output_dir = os.path.abspath(self.output_dir)
        self.logging_dir = os.path.abspath(self.output_dir + "/logs")
        self.pipeline_rundir=os.path.abspath(self.output_dir + "/run")

        if self.output_file is None:
            self.output_file = os.path.abspath(self.output_dir + "/rowIdxs.txt")
        if self.known_factors is None:
            self.known_factors =[0]*len(self.unwanted_factors)
        if len(self.known_factors) != len(self.unwanted_factors):
            raise iBSDefines.BdtUsage("unwanted-factors and known-factors must have equal lenght")
        return args

def s01_run_selection():
    nodeName = gSteps[0]
    return bdvdUtil.run_bdvd_ruv_rowselection(
        gRunner,
        Platform,
        BDT_HomeDir,
        nodeName,
        gParams.pipeline_rundir,
        gParams.dry_run,
        gParams.remove_before_run,
        gParams.row_selector,
        gParams.rowidx_from,
        gParams.rowidx_to,
        gParams.column_ids,
        gParams.workercnt,
        gParams.memory_size,
        gParams.bdvd_dir,
        gParams.evaluate_component,
        gParams.evaluate_scale,
        gParams.artifact_detection,
        gParams.unwanted_factors,
        gParams.known_factors,
        gParams.with_signal_threshold,
        gParams.with_signal_col_cnt,
        gParams.with_signal_sampling_row_cnt,
        gParams.output_file);

def main(argv=None):
    global gParams
    global gRunner
    gParams = BDVDParams()
    gRunner = bdtUtil.bdtRunner()
    run_argv = sys.argv[:]

    try:
        if argv is None:
            argv = sys.argv
        args = gParams.parse_options(argv)
       
        start_time = datetime.now()

        gRunner.prepare_dirs(gParams.output_dir, gParams.logging_dir, gParams.pipeline_rundir)
        gRunner.init_logger(os.path.abspath(gParams.logging_dir + "/bdvd-rowselection.log"))

        gRunner.logp()
        gRunner.log("Beginning bdvd row selection run v({0})".format(bdtUtil.get_version()))
        gRunner.logp("-----------------------------------------------")

        s01_run_selection()

        finish_time = datetime.now()
        duration = finish_time - start_time
        gRunner.logp("-----------------------------------------------")
        gRunner.log("Run complete: %s elapsed" %  bdtUtil.formatTD(duration))

    except iBSDefines.BdtUsage as err:
        gRunner.logp(sys.argv[0].split("/")[-1] + ": " + str(err.msg))
        return 2
    
    except:
        gRunner.logp(traceback.format_exc())
        gRunner.die()

if __name__ == "__main__":
    sys.exit(main())
