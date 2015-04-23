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
import re
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

gParams=None
gRunner=None
gSteps = bigMatUtil.bigmat_steps
gInputHandlers = None

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
        gParams.input_location,
        colNames,
        field_sep)

# -----------------------------------------------------------
# import data matrix from binary format (*.bfv)
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
        gParams.input_location,
        colNames)

# -----------------------------------------------------------
# import data matrix from bam files (*.bam)
# -----------------------------------------------------------
def s01_bam2mat():
    nodeName = gSteps[0]
    calcStatistics = True
    colNames = None
    return bigMatUtil.run_bam2Mat(
        gRunner,
        Platform,
        BDT_HomeDir,
        nodeName,
        gParams.pipeline_rundir,
        gParams.dry_run,
        gParams.remove_before_run,
        calcStatistics,
        colNames,
        gParams.thread_cnt,
        gParams.chromosomes,
        gParams.bin_width,
        gParams.input_location)

# -----------------------------------------------------------
# import data matrix from bigKmeans output
# -----------------------------------------------------------
def s01_fromKmeansResult():
    nodeName = gSteps[0]
    inputPickle = iBSDefines.derivePickleFile(gParams.input_location)
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
    elif gParams.input_type == 'kmeans-data-mat':
        iBSDefines.dumpPickle(kmeansOutObj.DataMat, out_picke_file)

    return out_picke_file

def main(argv=None):
    global gParams
    global gRunner
    global gInputHandlers

    gParams = bigMatUtil.BigMatParams()
    gRunner = bdtUtil.bdtRunner()
    gInputHandlers = {
        'text-mat': s01_txt2mat,
        'binary-mat': s01_bfv2mat,
        'bams': s01_bam2mat,
        'kmeans-seeds-mat': s01_fromKmeansResult,
        'kmeans-centroids-mat': s01_fromKmeansResult,
        'kmeans-data-mat': s01_fromKmeansResult}

    run_argv = sys.argv[:]

    try:
        if argv is None:
            argv = sys.argv
        gParams.parse_options("", argv[1:])
       
        start_time = datetime.now()

        gRunner.prepare_dirs(gParams.output_dir, gParams.logging_dir, gParams.pipeline_rundir)
        gRunner.init_logger(os.path.abspath(gParams.logging_dir + "/bigMat.log"))

        gRunner.logp()
        gRunner.log("Beginning bigMat run v({0})".format(bdtUtil.get_version()))
        gRunner.logp("-----------------------------------------------")

        # -----------------------------------------------------------
        # launch bigMat
        # -----------------------------------------------------------     
        if gParams.dry_run and gParams.start_from == gSteps[0]:
            gParams.dry_run = False

        datamatPickle = None
        datamatPickle = gInputHandlers[gParams.input_type]()

        if gParams.dry_run:
            gRunner.log("retrieve existing result for: {0}".format(gSteps[0]))
            gRunner.log("from: {0}".format(datamatPickle))
            if not os.path.exists(datamatPickle):
                gRunner.die('file not exist')
            gRunner.log("")

        runSummary = iBSDefines.NodeRunSummaryDefine()
        runSummary.NodeDir = gParams.output_dir
        runSummary.NodeType = "bigMat"
        runSummaryPicke = "{0}/logs/runSummary.pickle".format(gParams.output_dir)
        iBSDefines.dumpPickle(runSummary, runSummaryPicke)

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
