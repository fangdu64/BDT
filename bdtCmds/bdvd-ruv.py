#!__PYTHON_BIN_PATH__

"""
bdvd - perform ruv
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
BDVD-RUVs run RUV for big matrix.

Usage:
    bdvd-ruv [options] <--bigmat bigmat_file> <design_file>

Options:
    -v/--version
    -o/--output-dir                <string>    [ default: ./ruvs_out       ]
    -p/--num-threads               <int>       [ default: 4                ]
    -m/--max-mem                   <int>       [ default: 20000            ]
    --tmp-dir                      <dirname>   [ default: <output_dir>/tmp ]

Advanced Options:
    --place-holder
'''

gParams=None
gRunner=None

class BdvdRuvParams:
    def __init__(self):
        self.output_dir = None
        self.bigmat_dir = None
        self.max_mem = 2000
        self.num_threads = 4
        self.fcdc_fvworker_size=2
        self.fcdc_tcp_port=16000
        self.fcdc_threadpool_size=2
        self.workflow_node = "ruv"
        self.result_dumpfile = None
        self.design_file = None
        self.input_pickle = None
        self.quantile_pickle = None
        self.ctrl_rows_pickle = None

    def parse_options(self, argv):
        try:
            opts, args = getopt.getopt(argv[1:], "hv",
                ["version",
                    "help",
                    "out=",
                    "num-threads=",
                    "max-mem=",
                    "node=",
                    "bigmat-dir=",
                    "data-mat=",
                    "quantile=",
                    "ctrl-rows="])
        except getopt.error as msg:
            raise iBSDefines.BdtUsage(msg)

        for option, value in opts:
            if option in ("-v", "--version"):
                print("bdvd-ruv v",bdtUtil.get_version())
                sys.exit(0)
            if option in ("-h", "--help"):
                raise iBSDefines.BdtUsage(use_message)
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
            if option == "--data-mat":
                self.input_pickle = value
            if option =="--quantile":
                self.quantile_pickle = value
            if option == "--ctrl-rows":
                self.ctrl_rows_pickle = value
        
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
            os.path.abspath("{0}/bdvdRUVDesign.py".format(gRunner.script_dir)))

def dumpOutput(bam2mat):
    fn = "{0}/{1}".format(gParams.output_dir,gParams.result_dumpfile)
    iBSDefines.dumpPickle(bam2mat,fn)

def attachInputBigMatrix(bigmat,fcdcPrx, requireStats):
    gRunner.log("attach bigmat: {0} x {1} from {2}".format(bigmat.RowCnt,bigmat.ColCnt,bigmat.StorePathPrefix))
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

def setupCtrlQuantiles(bdvdFacetAdminPrx,  rfi,quantile,fraction):
    ruvPrx=bdvdFacetAdminPrx.GetRUVFacet(rfi.FacetID)
    inObj = iBSDefines.loadPickle(gParams.quantile_pickle)
    qIdx=0;
    for i in range(len(inObj.Quantiles)):
        if quantile == inObj.Quantiles[i]:
            qIdx = i
            break
    if len(inObj.qFeatureIdxs) != len(rfi.SampleIDs):
        gRunner.log(" sample cnt not the same")

    qFeatureIdxs = []
    for i in range(len(rfi.SampleIDs)):
        qFeatureIdxs.append(inObj.qFeatureIdxs[i][qIdx])
    gRunner.log("ctrl-featureIdxs: {0}".format(qFeatureIdxs))

    (rt,qRows)=ruvPrx.SampleRowMatrix(rfi.SampleIDs,qFeatureIdxs,iBS.RowAdjustEnum.RowAdjustNone)
    qValues=[]
    for i in range(len(rfi.SampleIDs)):
        idx = i*len(rfi.SampleIDs)+i
        qValues.append(qRows[idx])

    gRunner.log("ctrl-quantile: {0}, all-in-fraction: {1}".format(quantile,fraction))
    gRunner.log("ctrl-qValues: {0}".format(qValues))
    ruvPrx.SetCtrlQuantileValues(quantile,qValues,fraction)

def setupRUVFacet(fcdcPrx, bdvdFacetAdminPrx, sampleIDs, ctrlOIDs):
    # configuration by user
    designPath=os.path.abspath(gRunner.script_dir)
    if designPath not in sys.path:
         sys.path.append(designPath)
    import bdvdRUVDesign

    (rt,rfi)=bdvdFacetAdminPrx.RqstNewRUVFacet() #facetID = 1
    rfi.FacetStatus=iBS.RUVFacetStatusEnum.RUVFacetStatusFeatureFiltered
    rfi.FeatureFilterMaxCntLowThreshold=-1 #no filtering
    rfi.FacetName="bdvd-ruv"
    rfi.CommonLibrarySize=0 #no library size adjustment
    rfi.ControlFeatureMaxCntLowBound =1
    rfi.ControlFeatureMaxCntUpBound = 1000
    rfi.PermutationCnt = bdvdRUVDesign.PermutationCnt
    #check 
    if bdvdRUVDesign.CommonLibrarySize is not None:
        rfi.CommonLibrarySize=bdvdRUVDesign.CommonLibrarySize
        if rfi.CommonLibrarySize == -1:
            (rt,osis)=fcdcPrx.GetObserversStats(sampleIDs)
            sums=[v.Sum for v in osis]
            sums.sort()
            rfi.CommonLibrarySize = sums[int(len(sums)/2)] #median
            gRunner.log("Using median column-sum: {0} ".format(rfi.CommonLibrarySize ))
    
    if bdvdRUVDesign.KnownFactors is not None:
        rfi.KnownFactors=bdvdRUVDesign.KnownFactors

    if bdvdRUVDesign.ControlFeaturePolicy is not None:
        rfi.ControlFeaturePolicy=bdvdRUVDesign.ControlFeaturePolicy
    if  rfi.ControlFeaturePolicy==iBS.RUVControlFeaturePolicyEnum.RUVControlFeaturePolicyMaxCntLow:
        if bdvdRUVDesign.ControlFeatureMaxCntLowBound is not None:
            rfi.ControlFeatureMaxCntLowBound=bdvdRUVDesign.ControlFeatureMaxCntLowBound
        if bdvdRUVDesign.ControlFeatureMaxCntUpBound is not None:
            rfi.ControlFeatureMaxCntUpBound=bdvdRUVDesign.ControlFeatureMaxCntUpBound
    if ctrlOIDs is not None:
        rfi.ObserverIDforControlFeatureIdxs = ctrlOIDs[0]

    if bdvdRUVDesign.RUVMode is not None:
        rfi.RUVMode=bdvdRUVDesign.RUVMode

    if bdvdRUVDesign.RUVScale is not None:
        rfi.InputAdjust = bdvdRUVDesign.RUVScale

    if bdvdRUVDesign.MaxK is not None:
        rfi.MaxK=bdvdRUVDesign.MaxK

    if bdvdRUVDesign.FeatureIdxFrom is not None:
        rfi.FeatureIdxFrom=bdvdRUVDesign.FeatureIdxFrom

    if bdvdRUVDesign.FeatureIdxTo is not None:
        rfi.FeatureIdxTo=bdvdRUVDesign.FeatureIdxTo

    conditionIdxSampleIDs=[]
    # sampleIDs in bdvdRUVDesign.sampleGroups starts from 1
    for sg in bdvdRUVDesign.sampleGroups:
        group=[]
        for si in sg:
            group.append(sampleIDs[si-1])
        conditionIdxSampleIDs.append(group)

    rfi.SampleIDs=sampleIDs
    rfi.ReplicateSampleIDs=conditionIdxSampleIDs
    rfi.RawCountObserverIDs=sampleIDs
    rfi.ConditionObserverIDs=conditionIdxSampleIDs
    bdvdFacetAdminPrx.SetRUVFacetInfo(rfi)

    if (rfi.ControlFeaturePolicy ==iBS.RUVControlFeaturePolicyEnum.RUVControlFeaturePolicyAllInUpperQuantile) or (rfi.ControlFeaturePolicy ==iBS.RUVControlFeaturePolicyEnum.RUVControlFeaturePolicyAllInLowerQuantile):
        quantile = bdvdRUVDesign.CtrlQuantile
        fraction = bdvdRUVDesign.AllInQuantileFraction
        setupCtrlQuantiles(bdvdFacetAdminPrx, rfi,quantile,fraction)
    return rfi

def rebuildRUV(fcdcPrx,bdvdFacetAdminPrx,rfi):
    ruvPrx=bdvdFacetAdminPrx.GetRUVFacet(rfi.FacetID)
    (rt, amdTaskID)=ruvPrx.RebuildRUVModel(gParams.num_threads,gParams.max_mem)

    preMsg=""
    amdTaskFinished=False
    gRunner.log("")
    gRunner.log("Running RUV with {0} threads, {1} Mb RAM ...".format(gParams.num_threads,gParams.max_mem ))
    while (not amdTaskFinished):
        (rt,amdTaskInfo)=fcdcPrx.GetAMDTaskInfo(amdTaskID)
        thisMsg="task: {0}, batch processed: {1}/{2}".format(amdTaskInfo.TaskName, amdTaskInfo.FinishedCnt, amdTaskInfo.TotalCnt)
        if preMsg!=thisMsg:
            preMsg = thisMsg
            gRunner.log(thisMsg)
        if amdTaskInfo.Status!=iBS.AMDTaskStatusEnum.AMDTaskStatusNormal:
            amdTaskFinished = True;
        else:
            time.sleep(4)
    gRunner.log("Running RUV [done]")

def saveResults(fcdcPrx, bdvdFacetAdminPrx):
    facetId = 1
    (rt, rfi) = bdvdFacetAdminPrx.GetRUVFacetInfo(facetId)
    fn = os.path.abspath("{0}/{1}".format(gParams.output_dir,gParams.result_dumpfile))
    outObj = iBSDefines.BdvdRuvOutDefine()
    
    outObj.BigMatDir = gParams.bigmat_dir
    outObj.RuvFaceInfo = rfi

    # eigen values
    (rt,bigvec_store_pathprefix)=fcdcPrx.GetFeatureValuePathPrefix(rfi.OIDforEigenValue)
    bigvec_store_pathprefix = os.path.abspath(bigvec_store_pathprefix)
    foi = fcdc.GetFeatureObserver(fcdcPrx,rfi.OIDforEigenValue)
    bigvec = iBSDefines.BigVecMetaInfo()
    bigvec.Name = "Eigen Values"
    bigvec.StorePathPrefix = bigvec_store_pathprefix
    bigvec.ColID = foi.ObserverID
    bigvec.ColName= foi.ObserverName
    bigvec.RowCnt = foi.DomainSize
    outObj.EigenValues = bigvec

    # eigen vectors
    foi = fcdc.GetFeatureObserver(fcdcPrx,rfi.OIDforEigenVectors)
    (rt, fois) = fcdcPrx.GetFeatureObservers(list(range(foi.ObserverID, foi.ObserverID + foi.ObserverGroupSize)))
    (rt,bigmat_store_pathprefix)=fcdcPrx.GetFeatureValuePathPrefix(foi.ObserverID)
    bigmat_store_pathprefix = os.path.abspath(bigmat_store_pathprefix)
    bigmat = iBSDefines.BigMatrixMetaInfo()
    bigmat.Name = "Eigen Vectors"
    bigmat.StorePathPrefix = bigmat_store_pathprefix
    bigmat.ColIDs = [v.ObserverID for v in fois]
    bigmat.ColNames= [v.ObserverName for v in fois]
    bigmat.RowCnt = foi.DomainSize
    bigmat.ColCnt=len(fois)
    outObj.EigenVectors = bigmat

    # permutated eigen values
    
    if rfi.OIDforPermutatedEigenValues > 0:
        foi = fcdc.GetFeatureObserver(fcdcPrx,rfi.OIDforPermutatedEigenValues)
        (rt, fois) = fcdcPrx.GetFeatureObservers(list(range(foi.ObserverID, foi.ObserverID + foi.ObserverGroupSize)))
        (rt,bigmat_store_pathprefix)=fcdcPrx.GetFeatureValuePathPrefix(foi.ObserverID)
        bigmat_store_pathprefix = os.path.abspath(bigmat_store_pathprefix)
        bigmat = iBSDefines.BigMatrixMetaInfo()
        bigmat.Name = "Permutated eigen values"
        bigmat.StorePathPrefix = bigmat_store_pathprefix
        bigmat.ColIDs = [v.ObserverID for v in fois]
        bigmat.ColNames= [v.ObserverName for v in fois]
        bigmat.RowCnt = foi.DomainSize
        bigmat.ColCnt=len(fois)
        outObj.PermutatedEigenValues = bigmat
    else:
        bigmat = iBSDefines.BigMatrixMetaInfo()
        bigmat.as_emtpy();
        outObj.PermutatedEigenValues = bigmat

    # WT
    foi = fcdc.GetFeatureObserver(fcdcPrx,rfi.ObserverIDforWts)
    (rt, fois) = fcdcPrx.GetFeatureObservers(list(range(foi.ObserverID, foi.ObserverID + foi.ObserverGroupSize)))
    (rt,bigmat_store_pathprefix)=fcdcPrx.GetFeatureValuePathPrefix(foi.ObserverID)
    bigmat_store_pathprefix = os.path.abspath(bigmat_store_pathprefix)
    bigmat = iBSDefines.BigMatrixMetaInfo()
    bigmat.Name = "Wts"
    bigmat.StorePathPrefix = bigmat_store_pathprefix
    bigmat.ColIDs = [v.ObserverID for v in fois]
    bigmat.ColNames= [v.ObserverName for v in fois]
    bigmat.RowCnt = foi.DomainSize
    bigmat.ColCnt=len(fois)
    outObj.Wt = bigmat

    iBSDefines.dumpPickle(outObj,fn)

def main(argv=None):
    global gParams
    global gRunner
    gParams = BdvdRuvParams()
    gRunner = bigMatUtil.bigMatRunner(iBSConfig.BDT_HomeDir, 'bdvd')

    run_argv = sys.argv[:]

    try:
        if argv is None:
            argv = sys.argv
        args = gParams.parse_options(argv)

        start_time = datetime.now()

        gRunner.prepare_dirs(gParams.output_dir, gParams.bigmat_dir)
        prepare_output_dir()
        gRunner.init_logger("bdvd-ruv.log")

        gRunner.logp()
        gRunner.log("Beginning bdvd-ruv run (v"+bdtUtil.get_version()+")")
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

        bdvdFacetAdminPrx = fcdc.GetBdvdFacetAdminProxy(fcdcHost)
        computePrx=fcdc.GetComputeProxy(fcdcHost)
  
        gRunner.log("bdtCore activated")
        
        sampleIDs = attachInputBigMatrix(bigmat, fcdcPrx, True)

        ctrlOIDs = None
        if gParams.ctrl_rows_pickle is not None:
            ctrObj = iBSDefines.loadPickle(gParams.ctrl_rows_pickle)
            ctrlMat = ctrObj.BigMat
            gRunner.log("ctrl row cnt = {0}".format(ctrlMat.RowCnt))
            ctrlOIDs = attachInputBigMatrix(ctrlMat, fcdcPrx, False)

        rfi=setupRUVFacet(fcdcPrx, bdvdFacetAdminPrx, sampleIDs, ctrlOIDs)

        # if MaxK==0, no need to run RUV
        if rfi.MaxK>0:
            rebuildRUV(fcdcPrx,bdvdFacetAdminPrx,rfi)

        # -----------------------------------------------------------
        # Output Results
        # -----------------------------------------------------------
        saveResults(fcdcPrx, bdvdFacetAdminPrx)

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
