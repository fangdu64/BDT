#!__PYTHON_BIN_PATH__

"""
command line tools to generate a matrix
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
import iBSFCDClient as fcdc
import bigMatUtil
import iBS
import Ice

use_message = '''
bigMat

Usage:
    bigMat [options] <--data data_file> <--nrow row_cnt> <--ncol col_cnt> <--k cluster_num> <--out out_dir>

Advanced Options:

'''

gParams=None
gRunner=None
gSteps = ['1-input-mat', 'end']
gInputTypes = [
    'text-mat',
    'binary-mat',
    'kmeans-seeds-mat',
    'kmeans-centroids-mat']

class Usage(Exception):
    def __init__(self, msg):
        self.msg = msg

class BDVDParams:
    def __init__(self):
        self.output_dir = None
        self.logging_dir = None
        self.start_from = gSteps[0]
        self.dry_run = True
        self.remove_before_run = True
        self.pipeline_rundir = None
        self.input_type = None
        self.data_dir = None
        self.data_file = None
        self.row_cnt = None
        self.col_cnt = None

    def parse_options(self, argv):
        try:
            opts, args = getopt.getopt(argv[1:], "hvp:m:o:",
                ["version",
                "help",
                "input-type=",
                "data=",
                "in-dir=",
                "out=",
                "ncol=",
                "nrow="])

        except getopt.error as msg:
            raise Usage(msg)

        # option processing
        for option, value in opts:
            if option in ("-v", "--version"):
                print("bigKmeans v",bdtUtil.get_version())
                sys.exit(0)
            if option in ("-h", "--help"):
                raise Usage(use_message)
            if option in ("-o", "--out"):
                self.output_dir = value
            if option =="--start-from":
                allowedValues = gSteps;
                if value not in allowedValues:
                    raise Usage('--start-from should be one of the {0}'.format(allowedValues))
                self.start_from = value
            if option =="--input-type":
                allowedValues = gInputTypes;
                if value not in allowedValues:
                    raise Usage('--input-type should be one of the {0}'.format(allowedValues))
                self.input_type = value
            if option == "--data":
                self.data_file = os.path.abspath(value)
            if option == "--in-dir":
                self.data_dir = os.path.abspath(value)
            if option in ("--ncol"):
                self.col_cnt = int(value)
            if option in ("--nrow"):
                self.row_cnt = int(value)
        if self.input_type in  ['text-mat', 'binary-mat']:
            requiredNames = ['--data', '--out', '--ncol', '--nrow']
            providedValues = [self.data_file, self.output_dir, self.col_cnt, self.row_cnt]
            providedFiles = [self.data_file]
        elif self.input_type in ['kmeans-seeds-mat', 'kmeans-centroids-mat']:
            requiredNames = ['--in-dir', '--out']
            providedValues = [self.data_dir, self.output_dir]
            providedFiles = [self.data_dir]

        noneIdx = bdtUtil.getFirstNone(providedValues)
        if noneIdx != -1:
            raise Usage("{0} is required".format(requiredNames[noneIdx]))

        noneIdx = bdtUtil.getFirstNotExistFile(providedFiles)
        if noneIdx != -1:
            raise Usage("{0} not exist".format(providedFiles[noneIdx]))

        self.output_dir = os.path.abspath(self.output_dir)
        self.logging_dir = os.path.abspath(self.output_dir + "/logs")
        self.pipeline_rundir=os.path.abspath(self.output_dir + "/run")

        return args

# -----------------------------------------------------------
# import data matrix from txt format
# -----------------------------------------------------------
def s01_txt2mat():
    nodeName = gSteps[0]
    calcStatistics = False
    colNames = None
    field_sep = None
    return bigMatUtil.run_txt2Mat(
        gRunner,
        Platform,
        BDT_HomeDir,
        nodeName,
        gParams.pipeline_rundir,
        gParams.dry_run,
        gParams.remove_before_run,
        calcStatistics,
        gParams.col_cnt,
        gParams.row_cnt,
        gParams.data_file,
        colNames,
        field_sep)

# -----------------------------------------------------------
# import data matrix from txt format (*.bfv)
# -----------------------------------------------------------
def s01_bfv2mat():
    nodeName = gSteps[0]
    calcStatistics = False
    colNames = None
    return bigMatUtil.run_bfv2Mat(
        gRunner,
        Platform,
        BDT_HomeDir,
        nodeName,
        gParams.pipeline_rundir,
        gParams.dry_run,
        gParams.remove_before_run,
        calcStatistics,
        gParams.col_cnt,
        gParams.row_cnt,
        gParams.data_file,
        colNames)

# -----------------------------------------------------------
# import data matrix from bigKmeans output
# -----------------------------------------------------------
def s01_fromKmeansResult():
    nodeName = gSteps[0]
    inputPickle = bdtUtil.derivePickleFile(gParams.data_dir)
    kmeansOutObj = iBSDefines.loadPickle(inputPickle)
    nodeDir = os.path.abspath("{0}/{1}".format(gParams.pipeline_rundir, nodeName))
    out_picke_file = os.path.abspath("{0}/{1}.pickle".format(nodeDir,nodeName))
    if gParams.dry_run:
        return out_picke_file
    if gParams.remove_before_run and os.path.exists(nodeDir):
        shutil.rmtree(nodeDir)
    if not os.path.exists(nodeDir):
        os.mkdir(nodeDir)

    if gParams.input_type == 'kmeans-seeds-mat':
        iBSDefines.dumpPickle(kmeansOutObj.SeedsMat, out_picke_file)
    elif gParams.input_type == 'kmeans-centroids-mat':
        iBSDefines.dumpPickle(kmeansOutObj.CentroidsMat, out_picke_file)

    return out_picke_file

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
        gRunner.init_logger(os.path.abspath(gParams.logging_dir + "/bigMat.log"))

        gRunner.logp()
        gRunner.log("Beginning bigKmeans run v({0})".format(bdtUtil.get_version()))
        gRunner.logp("-----------------------------------------------")

        # -----------------------------------------------------------
        # launch bigMat
        # -----------------------------------------------------------     
        if gParams.dry_run and gParams.start_from == gSteps[0]:
            gParams.dry_run = False

        datamatPickle = None
        if (gParams.input_type == gInputTypes[0]):
            datamatPickle = s01_txt2mat()
        elif (gParams.input_type == gInputTypes[1]):
            datamatPickle = s01_bfv2mat()
        elif (gParams.input_type in [gInputTypes[2], gInputTypes[3]]):
            datamatPickle = s01_fromKmeansResult()

        if gParams.dry_run:
            gRunner.log("retrieve existing result for: {0}".format(gSteps[0]))
            gRunner.log("from: {0}".format(datamatPickle))
            if not os.path.exists(datamatPickle):
                gRunner.die('file not exist')
            gRunner.log("")

        finish_time = datetime.now()
        duration = finish_time - start_time
        gRunner.logp("-----------------------------------------------")
        gRunner.log("Run complete: %s elapsed" %  bdtUtil.formatTD(duration))

    except Usage as err:
        gRunner.logp(sys.argv[0].split("/")[-1] + ": " + str(err.msg))
        return 2
    
    except:
        gRunner.logp(traceback.format_exc())
        gRunner.die()

if __name__ == "__main__":
    sys.exit(main())
