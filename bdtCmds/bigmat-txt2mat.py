#!__PYTHON_BIN_PATH__

"""
import matrix from txt file
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
Creats matrix from txt data.

Usage:
    bdvd-ext2mat.py [options] <design_file>

Options:
    -v/--version
    -p/--num-threads               <int>       [ default: 4                   ]
    -m/--max-mem                   <int>       [ default: 20000               ]

Advanced Options:
    --place-holder

'''

gParams=None
gRunner=None

class Usage(Exception):
    def __init__(self, msg):
        self.msg = msg

class BDVDParams:

    def __init__(self):
        self.output_dir = None
        self.bigmat_dir = None
        self.max_mem = 2000
        self.num_threads = 4
        self.fcdc_fvworker_size=self.num_threads
        self.fcdc_tcp_port=16000
        self.fcdc_threadpool_size=2
        self.workflow_node = "txt2mat"
        self.result_dumpfile = None
        self.design_file = None
        self.calc_statistics = False

    def parse_options(self, argv):
        try:
            opts, args = getopt.getopt(argv[1:], "hvp:m:o:",
                                        ["version",
                                         "help",
                                         "out=",
                                         "num-threads=",
                                         "max-mem=",
                                         "node=",
                                         "bigmat-dir=",
                                         "tmp-dir="])
        except getopt.error as msg:
            raise Usage(msg)

        # option processing
        for option, value in opts:
            if option in ("-v", "--version"):
                print("BDT v",bdtUtil.get_version())
                sys.exit(0)
            if option in ("-h", "--help"):
                raise Usage(use_message)
            if option in ("-p", "--num-threads"):
                self.num_threads = int(value)
                self.fcdc_fvworker_size = self.num_threads
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
            self.bigmat_dir = self.output_dir+"/bigmat"
        self.bigmat_dir = os.path.abspath(self.bigmat_dir)

        if len(args) < 1:
            raise Usage(use_message)
        self.design_file = args[0]
        return args

def prepare_output_dir():
    shutil.copy(gParams.design_file,
                os.path.abspath("{0}/txt2MatDesign.py".format(gRunner.script_dir)))

def uploadData(fcdcPrx):
    designPath=os.path.abspath(gRunner.script_dir)
    if designPath not in sys.path:
         sys.path.append(designPath)
    import txt2MatDesign as design
    sampleNames = design.SampleNames
    (rt, observerIDs) = fcdcPrx.RqstNewFeatureObserversInGroup(len(sampleNames),False)
    observerGroupID=observerIDs[0]
    if design.RowCnt is None:
        design.RowCnt = design.getRowCount()
    DomainSize = design.RowCnt
    gRunner.log("input data: rowCnt {0}, colCnt {1}".format(DomainSize,len(sampleNames)))

    if design.CalcStatistics is not None:
        gParams.calc_statistics = design.CalcStatistics
    (rt,fois)=fcdcPrx.GetFeatureObservers(observerIDs)
    for i in range(len(sampleNames)):
        observerID = observerIDs[i]
        foi = fois[i]
        foi.ObserverName = sampleNames[i]
        foi.DomainSize=DomainSize
        foi.SetPolicy = iBS.FeatureValueSetPolicyEnum.FeatureValueSetPolicyNoRAMImmediatelyToStore
        foi.DomainID=0

    fcdcPrx.SetFeatureObservers(fois)
    batchRowCnt= int(240*1024*1024/8/len(sampleNames))
    design.uploadData(fcdcPrx, observerGroupID, batchRowCnt)
    return (sampleNames,observerIDs,DomainSize)

def dumpOutput(ext2mat):
    fn = os.path.abspath("{0}/{1}".format(gRunner.output_dir, gParams.result_dumpfile))
    iBSDefines.dumpPickle(ext2mat,fn)

def main(argv=None):
    global gParams
    global gRunner
    gParams = BDVDParams()
    gRunner = bigMatUtil.bigMatRunner(iBSConfig.BDT_HomeDir)

    run_argv = sys.argv[:]

    try:
        if argv is None:
            argv = sys.argv
        args = gParams.parse_options(argv)
       
        start_time = datetime.now()

        gRunner.prepare_dirs(gParams.output_dir, gParams.bigmat_dir)
        prepare_output_dir()
        gRunner.init_logger("txt2mat.log")

        gRunner.logp()
        gRunner.log("Beginning txt2mat run v({0})".format(bdtUtil.get_version()))
        gRunner.logp("-----------------------------------------------")

        gParams.fcdc_tcp_port = bdtUtil.getUsableTcpPort()
        gRunner.prepare_bigmat_config(gParams.fcdc_tcp_port, 
                                  gParams.fcdc_fvworker_size, 
                                  gParams.fcdc_threadpool_size)
        #print("fcdc_tcp_port = ",gParams.fcdc_tcp_port)
        gRunner.launch_bigMat()
        fcdc.Init();
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
            raise Usage("connection timeout")

        facetAdminPrx=fcdc.GetFacetAdminProxy(fcdcHost)
        computePrx=fcdc.GetComputeProxy(fcdcHost)
        samplePrx=fcdc.GetSeqSampleProxy(fcdcHost)
    
        gRunner.log("bigMat activated")
        
        (sampleNames,outOIDs, rowCnt)= uploadData(fcdcPrx)
        
        osis=None
        if gParams.calc_statistics:
            #recalculate statistics
            bigMatrixID= outOIDs[0]
            print(outOIDs)
            bmPrx=facetAdminPrx.GetBigMatrixFacet(bigMatrixID)
            (rt, amdTaskID)=bmPrx.RecalculateObserverStats(250)
            preFinishedCnt=0
            amdTaskFinished=False
            gRunner.log("")
            gRunner.log("Calculting statistics for matrix ...")
            while (not amdTaskFinished):
                (rt,amdTaskInfo)=fcdcPrx.GetAMDTaskInfo(amdTaskID)
                if preFinishedCnt<amdTaskInfo.FinishedCnt:
                    preFinishedCnt = amdTaskInfo.FinishedCnt
                    gRunner.log("batch processed: {0}/{1}".format(preFinishedCnt, amdTaskInfo.TotalCnt))
                if amdTaskInfo.TotalCnt==amdTaskInfo.FinishedCnt:
                    amdTaskFinished = True;
                else:
                    time.sleep(2)

            (rt, osis)=fcdcPrx.GetObserversStats(outOIDs)
            gRunner.logp("Statistics")
            binCount = int(osis[0].Cnt)
            gRunner.logp("Row Count: "+str(rowCnt))
            for osi in osis:
                gRunner.logp("Sample {0}: Max = {1:.2f}, Min = {2:.2f}, Sum = {3:.2f}".format(osi.ObserverID, osi.Max, osi.Min, osi.Sum))
            gRunner.log("Calculting statistics for matrix [done]")

        #now prepare output infomation
        (rt,bigmat_store_pathprefix)=fcdcPrx.GetFeatureValuePathPrefix(outOIDs[0])
        bigmat_store_pathprefix = os.path.abspath(bigmat_store_pathprefix)
        gRunner.log("bigmat store: {0}".format(bigmat_store_pathprefix))
        bigmat = iBSDefines.BigMatrixMetaInfo()
        bigmat.Name = gParams.workflow_node
        bigmat.ColStats=osis
        bigmat.StorePathPrefix = bigmat_store_pathprefix
        bigmat.ColIDs = outOIDs
        bigmat.ColNames= sampleNames
        bigmat.RowCnt = rowCnt
        bigmat.ColCnt=len(outOIDs)

        csv2mat=iBSDefines.Csv2MatOutputDefine(bigmat)
        
        dumpOutput(csv2mat)

        gRunner.shutdown_bigMat()

        finish_time = datetime.now()
        duration = finish_time - start_time
        gRunner.logp("-----------------------------------------------")
        gRunner.log("Run complete: %s elapsed" %  bdtUtil.formatTD(duration))

    except Usage as err:
        gRunner.shutdown_bigMat()
        gRunner.logp(sys.argv[0].split("/")[-1] + ": " + str(err.msg))
        gRunner.logp("    for detailed help see url ...")
        return 2
    
    except:
        gRunner.logp(traceback.format_exc())
        gRunner.die()

if __name__ == "__main__":
    sys.exit(main())
