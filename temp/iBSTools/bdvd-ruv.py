#!/dcs01/gcode/fdu1/BDT/python/bin/python3.3

"""
bdvd-bam2mat.py

Created by Fang Du on 2014-07-24.
Copyright (c) 2014 Fang Du. All rights reserved.
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

import iBSConfig
import iBSUtil
import iBSDefines
import iBSFCDClient as fcdc
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

class Usage(Exception):
    def __init__(self, msg):
        self.msg = msg

output_dir = "./ruvs_out/"
logging_dir = output_dir + "logs/"
fcdcentral_dir = output_dir + "fcdcentral/"
script_dir = output_dir + "script/"
tmp_dir = output_dir + "tmp/"
bdvd_log_handle = None #main log file handle
bdvd_logger = None # main logging object

fcdc_popen = None
fcdc_log_file=None
gParams=None

def init_logger(log_fname):
    global bdvd_logger
    bdvd_logger = logging.getLogger('project')
    formatter = logging.Formatter('%(asctime)s %(message)s', '[%Y-%m-%d %H:%M:%S]')
    bdvd_logger.setLevel(logging.DEBUG)

    # output logging information to stderr
    hstream = logging.StreamHandler(sys.stderr)
    hstream.setFormatter(formatter)
    bdvd_logger.addHandler(hstream)
    
    #
    # Output logging information to file
    if os.path.isfile(log_fname):
        os.remove(log_fname)
    global bdvd_log_handle
    logfh = logging.FileHandler(log_fname)
    logfh.setFormatter(formatter)
    bdvd_logger.addHandler(logfh)
    bdvd_log_handle=logfh.stream

class BDVDParams:

    def __init__(self):

        #max mem allowed in Mb
        self.max_mem = 2000
        self.num_threads = 4
        self.fcdc_fvworker_size=2
        self.fcdc_tcp_port=16000
        self.fcdc_threadpool_size=2
        self.workflow_node = "ruv"
        self.result_dumpfile = None
        self.datacentral_dir = None
        self.design_file = None
        self.input_pickle = None
        self.quantile_pickle = None
        self.ctrl_rows_pickle = None


    def parse_options(self, argv):
        try:
            opts, args = getopt.getopt(argv[1:], "hvp:m:o:",
                                        ["version",
                                         "help",
                                         "output-dir=",
                                         "num-threads=",
                                         "max-mem=",
                                         "node=",
                                         "datacentral-dir=",
                                         "tmp-dir=",
                                         "bigmat=",
                                         "quantile=",
                                         "ctrl-rows="])
        except getopt.error as msg:
            raise Usage(msg)

        global output_dir
        global logging_dir
        global tmp_dir
        global fcdcentral_dir
        global script_dir

        custom_tmp_dir = None
        custom_out_dir = None

        # option processing
        for option, value in opts:
            if option in ("-v", "--version"):
                print("BDVD v",iBSUtil.get_version())
                sys.exit(0)
            if option in ("-h", "--help"):
                raise Usage(use_message)
            if option in ("-p", "--num-threads"):
                self.num_threads = int(value)
                self.fcdc_fvworker_size = 2
            if option in ("-m", "--max-mem"):
                self.max_mem = int(value)
            if option in ("-o", "--output-dir"):
                custom_out_dir = value + "/"
                self.resume_dir = value
            if option == "--tmp-dir":
                custom_tmp_dir = value + "/"
            if option == "--node":
                self.workflow_node = value
            if option =="--datacentral-dir":
                self.datacentral_dir = value
            if option =="--bigmat":
                self.input_pickle = value
            if option =="--quantile":
                self.quantile_pickle = value
            if option == "--ctrl-rows":
                self.ctrl_rows_pickle = value
        
        self.result_dumpfile = "{0}.pickle".format(self.workflow_node)
        if custom_out_dir:
            output_dir = custom_out_dir
            logging_dir = output_dir + "logs/"
            tmp_dir = output_dir + "tmp/"
            fcdcentral_dir = output_dir + "fcdcentral/"
            script_dir = output_dir + "script/"
        if custom_tmp_dir:
            tmp_dir = custom_tmp_dir

        if self.datacentral_dir is not None:
            fcdcentral_dir = self.datacentral_dir+"/fcdcentral/"

        if self.input_pickle is None:
            raise Usage(use_message)
        if len(args) < 1:
            raise Usage(use_message)
        self.design_file = args[0]
        return args

# The BDVD logging formatter
def bdvd_log(out_str):
  if bdvd_logger:
       bdvd_logger.info(out_str)

# error msg
def bdvd_logp(out_str=""):
    print(out_str,file=sys.stderr)
    if bdvd_log_handle:
        print(out_str, file=bdvd_log_handle)

def die(msg=None):
    global fcdc_popen
    if msg is not None:
        bdvd_logp(msg)
    shutdownFCDCentral()
    sys.exit(1)

def shutdownFCDCentral():
    global fcdc_popen
    if fcdc_popen is not None:
        fcdc_popen.terminate()
        fcdc_popen.wait()
        fcdc_popen = None
        bdvd_log("FCDCentral shutdown")
    if fcdc_log_file is not None:
        fcdc_log_file.close()

# Ensures that the output, logging, and temp directories are present. If not,
# they are created
def prepare_output_dir():

    bdvd_log("Preparing output location "+output_dir)

    if not os.path.exists(output_dir):
        os.mkdir(output_dir)       

    if not os.path.exists(logging_dir):
        os.mkdir(logging_dir)
         
    if not os.path.exists(script_dir):
        os.mkdir(script_dir)

    #shutil.copy("./iBS/config/bdvdRUVsDesign.py",script_dir)
    shutil.copy(gParams.design_file,script_dir)

    if not os.path.exists(fcdcentral_dir):
        os.mkdir(fcdcentral_dir)

    fcdc_db_dir=fcdcentral_dir+"FCDCentralDB"
    if not os.path.exists(fcdc_db_dir):
        os.mkdir(fcdc_db_dir)

    fcdc_fvstore_dir=fcdcentral_dir+"FeatureValueStore"
    if not os.path.exists(fcdc_fvstore_dir):
        os.mkdir(fcdc_fvstore_dir)

    if not os.path.exists(tmp_dir):
        try:
          os.makedirs(tmp_dir)
        except OSError as o:
          die("\nError creating directory %s (%s)" % (tmp_dir, o))
       

def prepare_fcdcentral_config(tcpPort,fvWorkerSize, iceThreadPoolSize ):
    infile = open("./iBS/config/FCDCentralServer.config")
    outfile = open(fcdcentral_dir+"FCDCentralServer.config", "w")

    replacements = {"__FCDCentral_TCP_PORT__":str(tcpPort), 
                    "__FeatureValueWorker.Size__":str(fvWorkerSize), 
                    "__Ice.ThreadPool.Server.Size__":str(iceThreadPoolSize)}

    for line in infile:
        for src, target in replacements.items():
            line = line.replace(src, target)
        outfile.write(line)
    infile.close()
    outfile.close()

def launchFCDCentral():
    global fcdc_popen
    global fcdc_log_fhandle
    fcdcentral_path=os.getcwd()+"/iBS/bin/FCDCentral"
    fcdc_cmd = [fcdcentral_path]
    bdvd_log("Launching FCDCentral ...")
    fcdc_log_file = open(logging_dir + "fcdc.log","w")
    fcdc_popen = subprocess.Popen(fcdc_cmd, cwd=fcdcentral_dir, stdout=fcdc_log_file)

def getTaskConfig():
    designPath=os.path.abspath(script_dir)
    if designPath not in sys.path:
         sys.path.append(designPath)
    import bdvdRUVDesign

    sampleList=bdvdBam2MatDesign.define_samples()
    binInfo=bdvdBam2MatDesign.define_genomicBins()
    #check configs
    return sampleList,binInfo

def dumpOutput(bam2mat):
    fn = "{0}{1}".format(output_dir,gParams.result_dumpfile)
    iBSDefines.dumpPickle(bam2mat,fn)

def attachInputBigMatrix(bigmat,fcdcPrx):
    bdvd_log("attach bigmat: {0} x {1} from {2}".format(bigmat.RowCnt,bigmat.ColCnt,bigmat.StorePathPrefix))
    (rt, outOIDs)=fcdcPrx.AttachBigMatrix(bigmat.ColCnt,bigmat.RowCnt,bigmat.ColNames,bigmat.StorePathPrefix)
    #bdvd_log("assigned colIDs: {0}".format(str(outOIDs)))
    minSampleID = min(outOIDs)
    osis=bigmat.ColStats
    for i in range(len(osis)):
        osi=osis[i]
        osis[i].ObserverID=outOIDs[i]
        bdvd_logp("Sample {0}: Max = {1}, Min = {2}, Sum = {3}".format(osi.ObserverID - minSampleID +1, int(osi.Max), int(osi.Min), int(osi.Sum)))
    rt=fcdcPrx.SetObserverStats(osis)
    return outOIDs

def setupCtrlQuantiles(facetAdminPrx,  rfi,quantile,fraction):
    ruvPrx=facetAdminPrx.GetRUVFacet(rfi.FacetID)
    inObj = iBSDefines.loadPickle(gParams.quantile_pickle)
    qIdx=0;
    for i in range(len(inObj.Quantiles)):
        if quantile == inObj.Quantiles[i]:
            qIdx = i
            break
    if len(inObj.qFeatureIdxs) != len(rfi.SampleIDs):
        bdvd_log(" sample cnt not the same")

    qFeatureIdxs = []
    for i in range(len(rfi.SampleIDs)):
        qFeatureIdxs.append(inObj.qFeatureIdxs[i][qIdx])
    bdvd_log("ctrl-featureIdxs: {0}".format(qFeatureIdxs))

    (rt,qRows)=ruvPrx.SampleRowMatrix(rfi.SampleIDs,qFeatureIdxs,iBS.RowAdjustEnum.RowAdjustNone)
    qValues=[]
    for i in range(len(rfi.SampleIDs)):
        idx = i*len(rfi.SampleIDs)+i
        qValues.append(qRows[idx])

    bdvd_log("ctrl-quantile: {0}, all-in-fraction: {1}".format(quantile,fraction))
    bdvd_log("ctrl-qValues: {0}".format(qValues))
    #import code
    #code.interact(local=locals())
    ruvPrx.SetCtrlQuantileValues(quantile,qValues,fraction)


def setupRUVFacet(fcdcPrx, facetAdminPrx, sampleIDs, ctrlOIDs):
    # configuration by user
    designPath=os.path.abspath(script_dir)
    if designPath not in sys.path:
         sys.path.append(designPath)
    import bdvdRUVDesign

    (rt,rfi)=facetAdminPrx.RqstNewRUVFacet() #facetID = 1
    rfi.FacetStatus=iBS.RUVFacetStatusEnum.RUVFacetStatusFeatureFiltered
    rfi.FeatureFilterMaxCntLowThreshold=-1 #no filtering
    rfi.FacetName="bdvd-ruv"
    rfi.CommonLibrarySize=0 #no library size adjustment
    rfi.ControlFeatureMaxCntLowBound =1
    rfi.ControlFeatureMaxCntUpBound = 1000
    #check 
    if bdvdRUVDesign.CommonLibrarySize is not None:
        rfi.CommonLibrarySize=bdvdRUVDesign.CommonLibrarySize
        if rfi.CommonLibrarySize == -1:
            (rt,osis)=fcdcPrx.GetObserversStats(sampleIDs)
            sums=[v.Sum for v in osis]
            sums.sort()
            rfi.CommonLibrarySize = sums[int(len(sums)/2)] #median
            bdvd_log("Using median column-sum: {0} ".format(rfi.CommonLibrarySize ))
    
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
    facetAdminPrx.SetRUVFacetInfo(rfi)

    if (rfi.ControlFeaturePolicy ==iBS.RUVControlFeaturePolicyEnum.RUVControlFeaturePolicyAllInUpperQuantile) or (rfi.ControlFeaturePolicy ==iBS.RUVControlFeaturePolicyEnum.RUVControlFeaturePolicyAllInLowerQuantile):
        quantile = bdvdRUVDesign.CtrlQuantile
        fraction = bdvdRUVDesign.AllInQuantileFraction
        setupCtrlQuantiles(facetAdminPrx, rfi,quantile,fraction)
    return rfi

def rebuildRUV(fcdcPrx,facetAdminPrx,rfi):
    ruvPrx=facetAdminPrx.GetRUVFacet(rfi.FacetID)
    (rt, amdTaskID)=ruvPrx.RebuildRUVModel(gParams.num_threads,gParams.max_mem)

    preMsg=""
    amdTaskFinished=False
    bdvd_log("")
    bdvd_log("Running RUV with {0} threads, {1} Mb RAM ...".format(gParams.num_threads,gParams.max_mem ))
    while (not amdTaskFinished):
        (rt,amdTaskInfo)=fcdcPrx.GetAMDTaskInfo(amdTaskID)
        thisMsg="task: {0}, batch processed: {1}/{2}".format(amdTaskInfo.TaskName, amdTaskInfo.FinishedCnt, amdTaskInfo.TotalCnt)
        if preMsg!=thisMsg:
            preMsg = thisMsg
            bdvd_log(thisMsg)
        if amdTaskInfo.Status!=iBS.AMDTaskStatusEnum.AMDTaskStatusNormal:
            amdTaskFinished = True;
        else:
            time.sleep(4)
    bdvd_log("Running RUV [done]")

def main(argv=None):

    # Initialize default parameter values
    global gParams
    gParams = BDVDParams()
    run_argv = sys.argv[:]

    global fcdc_popen
    fcdc_popen = None

    try:
        if argv is None:
            argv = sys.argv
        args = gParams.parse_options(argv)
       
        print("design file = ",gParams.design_file)

        start_time = datetime.now()

        prepare_output_dir()
        init_logger(logging_dir + "bdvd.log")

        bdvd_logp()
        bdvd_log("Beginning BDVD RUV run (v"+iBSUtil.get_version()+")")
        bdvd_logp("-----------------------------------------------")

        gParams.fcdc_tcp_port = iBSUtil.getUsableTcpPort()
        prepare_fcdcentral_config(gParams.fcdc_tcp_port, 
                                  gParams.fcdc_fvworker_size, 
                                  gParams.fcdc_threadpool_size)
        print("fcdc_tcp_port = ",gParams.fcdc_tcp_port)

        inObj = iBSDefines.loadPickle(gParams.input_pickle)
        bigmat = inObj.BigMat
        print("sample cnt = ",bigmat.ColCnt)

        launchFCDCentral()

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
    
        bdvd_log("FCDCentral activated")
        
        sampleIDs = attachInputBigMatrix(bigmat, fcdcPrx)

        ctrlOIDs = None
        if gParams.ctrl_rows_pickle is not None:
            ctrObj = iBSDefines.loadPickle(gParams.ctrl_rows_pickle)
            ctrlMat = ctrObj.BigMat
            bdvd_log("ctrl row cnt = {0}".format(ctrlMat.RowCnt))
            ctrlOIDs = attachInputBigMatrix(ctrlMat, fcdcPrx)

        rfi=setupRUVFacet(fcdcPrx, facetAdminPrx, sampleIDs, ctrlOIDs)

        # if MaxK==0, no need to run RUV
        if rfi.MaxK>0:
            rebuildRUV(fcdcPrx,facetAdminPrx,rfi)

        #import code
        #code.interact(local=locals())

        shutdownFCDCentral()

        finish_time = datetime.now()
        duration = finish_time - start_time
        bdvd_logp("-----------------------------------------------")
        bdvd_log("Run complete: %s elapsed" %  iBSUtil.formatTD(duration))

    except Usage as err:
        shutdownFCDCentral()
        bdvd_logp(sys.argv[0].split("/")[-1] + ": " + str(err.msg))
        bdvd_logp("    for detailed help see url ...")
        return 2
    
    except:
        bdvd_logp(traceback.format_exc())
        die()


if __name__ == "__main__":
    sys.exit(main())
