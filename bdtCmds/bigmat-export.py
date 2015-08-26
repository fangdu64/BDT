#!__PYTHON_BIN_PATH__

"""
bigmat export data
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
bigmat-export
'''

gParams=None
gRunner=None

class BigmatExportParams:
    def __init__(self):
        self.output_dir = None
        self.bigmat_dir = None
        self.fcdc_fvworker_size=2
        self.fcdc_tcp_port=16000
        self.fcdc_threadpool_size=2
        self.workflow_node = "export"
        self.result_dumpfile = None
        self.design_file = None
        self.input_pickle = None
        self.export_rows_pickle = None

    def parse_options(self, argv):
        try:
            opts, args = getopt.getopt(argv[1:], "",
                ["out=",
                    "node=",
                    "bigmat-dir=",
                    "data-mat=",
                    "export-rows="])
        except getopt.error as msg:
            raise iBSDefines.BdtUsage(msg)

        for option, value in opts:
            if option in ("-o", "--out"):
                self.output_dir = value
            if option == "--node":
                self.workflow_node = value
            if option =="--bigmat-dir":
                self.bigmat_dir = value
            if option == "--data-mat":
                self.input_pickle = value
            if option == "--export-rows":
                self.export_rows_pickle = value
        
        self.result_dumpfile = "{0}.pickle".format(self.workflow_node)
        self.output_dir = os.path.abspath(self.output_dir)

        if self.bigmat_dir is None:
            self.bigmat_dir = self.output_dir+"/bigmat"
        self.bigmat_dir = os.path.abspath(self.bigmat_dir)

        if self.input_pickle is None:
            raise iBSDefines.BdtUsage("--data-mat is required")

        if len(args) < 1:
            raise iBSDefines.BdtUsage("design file is required")
        self.design_file = args[0]
        return args

def prepare_output_dir():
    shutil.copy(gParams.design_file,
            os.path.abspath("{0}/bigmatExportDesign.py".format(gRunner.script_dir)))

def dumpOutput(obj):
    fn = "{0}/{1}".format(gParams.output_dir,gParams.result_dumpfile)
    iBSDefines.dumpPickle(obj,fn)

def attachInputBigMatrix(bigmat,fcdcPrx, requireStats):
    gRunner.log("attach mat: {0} x {1} from {2}".format(bigmat.RowCnt,bigmat.ColCnt,bigmat.StorePathPrefix))
    (rt, outOIDs)=fcdcPrx.AttachBigMatrix(bigmat.ColCnt,bigmat.RowCnt,bigmat.ColNames,bigmat.StorePathPrefix)
    #bdvd_log("assigned colIDs: {0}".format(str(outOIDs)))
    if requireStats:
        minSampleID = min(outOIDs)
        if bigmat.ColStats is None:
            raise iBSDefines.BdtUsage("calc-statistics required")
        osis=bigmat.ColStats
        for i in range(len(osis)):
            osi=osis[i]
            osis[i].ObserverID=outOIDs[i]
            gRunner.logp("Sample {0}: Max = {1}, Min = {2}, Sum = {3}".format(osi.ObserverID - minSampleID +1, int(osi.Max), int(osi.Min), int(osi.Sum)))
        rt=fcdcPrx.SetObserverStats(osis)
    return outOIDs

def launchExportTask(fcdcPrx, computePrx, exportRowsOid, rowCnt, allColIds):
    # configuration by user
    designPath=os.path.abspath(gRunner.script_dir)
    if designPath not in sys.path:
         sys.path.append(designPath)
    import bigmatExportDesign as design

    sampleIDs = allColIds
    if design.ColIds is not None:
        sampleIDs = []
        # sampleIDs in design start from 1
        for si in design.ColIds:
            sampleIDs.append(rfi.SampleIDs[si-1])

    (rt,ofis)=fcdcPrx.GetFeatureObservers(sampleIDs)

    if exportRowsOid is not None:
            task=computePrx.GetBlankExportByRowIdxsTask()
            task.FeatureIdxsOid=exportRowsOid
    else:
        task=computePrx.GetBlankExportRowMatrixTask()
        task.FeatureIdxFrom = design.FeatureIdxFrom
        task.FeatureIdxTo = design.FeatureIdxTo
        rowCnt = task.FeatureIdxTo - task.FeatureIdxFrom

    task.TaskName = design.OutMatName
    task.reader = fcdcPrx
    task.SampleIDs = sampleIDs
    task.OutID = 10001+i
    task.OutPath = os.path.abspath(gParams.output_dir)
    task.OutFile = os.path.abspath("{0}/{1}".format(gParams.output_dir, design.OutMatName))

    gRunner.log("Export data ...")
    bigMatUtil.exportMatByRange(gRunner.log,fcdcPrx,computePrx,task)
    bfvFile = task.OutFile
    gRunner.log("Exported at: {0}\n".format(bfvFile))

    osis = None
    bigmat = iBSDefines.BigMatrixMetaInfo()
    bigmat.Name = design.OutMatName
    bigmat.ColStats=osis
    bigmat.StorePathPrefix = bfvFile
    bigmat.ColIDs = sampleIDs
    bigmat.ColNames= [si.ObserverName for si in ofis]
    bigmat.RowCnt = rowCnt
    bigmat.ColCnt=len(sampleIDs)

    nd_outobj=iBSDefines.Bfv2MatOutputDefine(bigmat)

    return (rt,nd_outobj)

def saveResults(outObj):
    fn = os.path.abspath("{0}/{1}".format(gParams.output_dir,gParams.result_dumpfile))
    iBSDefines.dumpPickle(outObj,fn)

def main(argv=None):
    global gParams
    global gRunner
    gParams = BigmatExportParams()
    gRunner = bigMatUtil.bigMatRunner(iBSConfig.BDT_HomeDir, 'bigmat')

    run_argv = sys.argv[:]

    try:
        if argv is None:
            argv = sys.argv
        args = gParams.parse_options(argv)

        start_time = datetime.now()

        gRunner.prepare_dirs(gParams.output_dir, gParams.bigmat_dir)
        prepare_output_dir()
        gRunner.init_logger("bigmat-export.log")

        gRunner.logp()
        gRunner.log("Beginning bigmat-export run (v"+bdtUtil.get_version()+")")
        gRunner.logp("-----------------------------------------------")

        gParams.fcdc_tcp_port = bdtUtil.getUsableTcpPort()
        gRunner.prepare_bigmat_config(gParams.fcdc_tcp_port, 
                                  gParams.fcdc_fvworker_size, 
                                  gParams.fcdc_threadpool_size)

        inObj = iBSDefines.loadPickle(gParams.input_pickle)
        bigmat = inObj.BigMat
        print("col cnt = ", bigmat.ColCnt)

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

        computePrx=fcdc.GetComputeProxy(fcdcHost)
  
        gRunner.log("bdtCore activated")
        
        sampleIDs = attachInputBigMatrix(bigmat, fcdcPrx, False)

        exportRowsOid = None
        rowCnt = None
        if gParams.export_rows_pickle is not None:
            ctrObj = iBSDefines.loadPickle(gParams.export_rows_pickle)
            ctrlMat = ctrObj.BigMat
            gRunner.log("export row cnt = {0}".format(ctrlMat.RowCnt))
            exportRowsOid = attachInputBigMatrix(ctrlMat, fcdcPrx, False)[0]
            rowCnt = ctrlMat.RowCnt

        (rt, outObj) = launchExportTask(fcdcPrx, computePrx, exportRowsOid, rowCnt, sampleIDs)
        
        # -----------------------------------------------------------
        # Output Results
        # -----------------------------------------------------------
        saveResults(outObj)

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
