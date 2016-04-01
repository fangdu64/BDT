#!__PYTHON_BIN_PATH__

"""
bdvd-ruv row selection
"""
import sys, traceback
import getopt
import subprocess
import os
import shutil
import time
from datetime import datetime, date
import logging
from copy import deepcopy

BDT_HomeDir=os.path.abspath(os.path.dirname(os.path.abspath(__file__))+"../../..")

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

import bdtUtil
import iBSDefines
import iBSFCDClient as fcdc
import bigMatUtil
import iBS
import Ice

use_message = '''
bdvd-row-selection
'''

gParams=None
gRunner=None

class BdvdExportParams:
    def __init__(self):
        self.output_dir = None
        self.bigmat_dir = None
        self.max_mem = 2000
        self.num_threads = 4
        self.fcdc_fvworker_size=2
        self.fcdc_tcp_port=16000
        self.fcdc_threadpool_size=2
        self.workflow_node = "rowselection"
        self.result_dumpfile = None
        self.design_file = None

    def parse_options(self, argv):
        try:
            opts, args = getopt.getopt(argv[1:], "",
                ["out=",
                    "num-threads=",
                    "max-mem=",
                    "node=",
                    "bigmat-dir="])
        except getopt.error as msg:
            raise iBSDefines.BdtUsage(msg)

        for option, value in opts:
            if option in ("-p", "--num-threads"):
                self.num_threads = int(value)
                self.fcdc_fvworker_size = 2
            if option in ("-m", "--max-mem"):
                self.max_mem = int(value)
            if option in ("-o", "--out"):
                self.output_dir = value
            if option == "--node":
                self.workflow_node = value
            if option =="--bigmat-dir":
                self.bigmat_dir = value
        
        self.result_dumpfile = "{0}.pickle".format(self.workflow_node)
        self.output_dir = os.path.abspath(self.output_dir)

        if self.bigmat_dir is None:
            raise iBSDefines.BdtUsage("--bigmat-dir is required")
        self.bigmat_dir = os.path.abspath(self.bigmat_dir)

        if len(args) < 1:
            raise iBSDefines.BdtUsage("design file is required")
        self.design_file = args[0]
        return args

def prepare_output_dir():
    shutil.copy(gParams.design_file,
            os.path.abspath("{0}/bdvdRuvRowSelectionDesign.py".format(gRunner.script_dir)))

def dumpOutput(obj):
    fn = "{0}/{1}".format(gParams.output_dir, gParams.result_dumpfile)
    iBSDefines.dumpPickle(obj,fn)


def launchRowSelectionTask(fcdcPrx, bdvdFacetAdminPrx, computePrx):
    # configuration by user
    designPath=os.path.abspath(gRunner.script_dir)
    if designPath not in sys.path:
         sys.path.append(designPath)
    import bdvdRuvRowSelectionDesign as design

    facetID=1
    ruvPrx=bdvdFacetAdminPrx.GetRUVFacet(facetID)
    (rt, rfi)=bdvdFacetAdminPrx.GetRUVFacetInfo(facetID)
    ruvPrx.SetOutputWorkerNum(design.OutputWorkerNum)
    ruvPrx.SetOutputMode(design.RUVOutputMode)
    ruvPrx.SetOutputScale(design.RUVOutputScale)
    ks=   design.Ks
    extWs=design.Ns
    sampleIDs = rfi.SampleIDs
    if design.ColIds is not None:
        sampleIDs = []
        # sampleIDs in design start from 1
        for si in design.ColIds:
            sampleIDs.append(rfi.SampleIDs[si-1])

    (rt,ofis)=fcdcPrx.GetFeatureObservers(sampleIDs)
    outpath=os.path.abspath(gParams.output_dir)
    task = computePrx.GetBlankWithSignalFeaturesTask()
    task.reader=ruvPrx
    task.SampleIDs= sampleIDs
    rowCnt = ofis[0].DomainSize
    task.FeatureIdxFrom = 0
    task.FeatureIdxTo = rowCnt
    if design.FeatureIdxFrom is not None:
        task.FeatureIdxFrom = design.FeatureIdxFrom
    if design.FeatureIdxTo is not None:
        task.FeatureIdxTo = design.FeatureIdxTo
    task.SignalThreshold = design.WithSignalThreshold
    if design.WithSignalColCnt is not None:
        task.SampleCntAboveThreshold = design.WithSignalColCnt
    if design.WithSignalSamplingRowCnt is not None:
        task.SamplingFeatureCnt = design.WithSignalSamplingRowCnt

    (rt, featureIdxs) = computePrx.WithSignalFeatures(task)

    outFile = design.RowIdxsOutFile
    with open(outFile, 'w') as f:
        for idx in featureIdxs:
            rt = f.writelines(str(idx)+'\n')


def saveResults(outObj):
    fn = os.path.abspath("{0}/{1}".format(gParams.output_dir, gParams.result_dumpfile))
    iBSDefines.dumpPickle(outObj,fn)

def main(argv=None):
    global gParams
    global gRunner
    gParams = BdvdExportParams()
    gRunner = bigMatUtil.bigMatRunner(iBSConfig.BDT_HomeDir, 'bdvd')

    run_argv = sys.argv[:]

    try:
        if argv is None:
            argv = sys.argv
        args = gParams.parse_options(argv)

        start_time = datetime.now()

        gRunner.prepare_dirs(gParams.output_dir, gParams.bigmat_dir)
        prepare_output_dir()
        gRunner.init_logger("bdvd-rowselection.log")

        gRunner.logp()
        gRunner.log("Beginning bdvd-rowselection run (v"+bdtUtil.get_version()+")")
        gRunner.logp("-----------------------------------------------")

        gParams.fcdc_tcp_port = bdtUtil.getUsableTcpPort()
        gRunner.prepare_bigmat_config(gParams.fcdc_tcp_port, 
                                  gParams.fcdc_fvworker_size, 
                                  gParams.fcdc_threadpool_size)

        gRunner.launch_bigMat()
        fcdc.Init()

        fcdcHost="localhost -p "+str(gParams.fcdc_tcp_port)
        fcdcPrx = None

        tryCnt=0
        while (tryCnt<20) and (fcdcPrx is None):
            try:
                fcdcPrx=fcdc.GetFCDCProxy(fcdcHost)
            except Ice.ConnectionRefusedException as ex:
                tryCnt=tryCnt+1
                time.sleep(1)

        if fcdcPrx is None:
            raise iBSDefines.BdtUsage("connection timeout")

        bdvdFacetAdminPrx = fcdc.GetBdvdFacetAdminProxy(fcdcHost)
        computePrx=fcdc.GetComputeProxy(fcdcHost)
  
        gRunner.log("bdtCore activated")

        launchRowSelectionTask(fcdcPrx, bdvdFacetAdminPrx, computePrx)

        gRunner.shutdown_bigMat()

        finish_time = datetime.now()
        duration = finish_time - start_time
        gRunner.logp("-----------------------------------------------")
        gRunner.log("Run complete: %s elapsed" %  bdtUtil.formatTD(duration))

    except iBSDefines.BdtUsage as err:
        gRunner.shutdown_bigMat()
        gRunner.logp(sys.argv[0].split("/")[-1] + ": " + str(err.msg))
        return 2
    
    except:
        gRunner.logp(traceback.format_exc())
        gRunner.die()


if __name__ == "__main__":
    sys.exit(main())
