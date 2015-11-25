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
jabs-armfinder

Usage:
    jabs-armfinder [options] <--cpgmap cpgmap_pickle_file> <design_file>

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

class JABSParams:

    def __init__(self):

        #max mem allowed in Mb
        self.max_mem = 2000
        self.num_threads = 4
        self.fcdc_fvworker_size=2
        self.fcdc_tcp_port=16000
        self.fcdc_threadpool_size=2
        self.workflow_node = "amrfinder"
        self.result_dumpfile = None
        self.datacentral_dir = None
        self.design_file = None
        self.input_pickle = None

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
                                         "cpgmap="])
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
                print("AMR Finder v",iBSUtil.get_version())
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
            if option =="--cpgmap":
                self.input_pickle = value
        
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

def attachCpGMap(fcdcPrx, amrPrx):
    designPath=os.path.abspath(script_dir)
    if designPath not in sys.path:
         sys.path.append(designPath)
    import jabsAMRFinderDesign as design

    inObj = iBSDefines.loadPickle(gParams.input_pickle)
    bigvec = inObj.CpGIdx2BpIdxBigVec
    cpgmap = inObj.CpGMapInfo
    cpgmap.MergeWatsonCrick = design.MergeWatsonCrick

    bdvd_log("attach CpG site map, MergeWatsonCrick={0}".format(cpgmap.MergeWatsonCrick))
    bdvd_log("attach bigvec: {0} from {1}".format(bigvec.RowCnt,bigvec.StorePathPrefix))
    (rt, outOID)=fcdcPrx.AttachBigVector(bigvec.RowCnt,bigvec.ColName,bigvec.StorePathPrefix)
    bdvd_log("assigned colID: {0}".format(outOID))
    amrPrx.SetCpGSiteMap(cpgmap)
    #import code
    #code.interact(local=locals())

def runSinglAMRFinder(fcdcPrx, amrPrx):
    designPath=os.path.abspath(script_dir)
    if designPath not in sys.path:
         sys.path.append(designPath)
    import jabsAMRFinderDesign as design
    
    (rt,task)=amrPrx.GetBlankAMRFinderTask(True)
    task.EvTblFiles = design.EvTblFiles
    task.WinSize = design.WinSize
    task.ThreadCnt = gParams.num_threads
    task.RamMb = gParams.max_mem
    task.MinMAPQ = design.MinMAPQ
    task.MinMUCntPerCpGSite = design.MinMUCntPerCpGSite
    task.MinEVCntPerRead = design.MinEVCntPerRead
    task.MinReadsPerWindow = design.MinReadsPerWindow
    task.BatchEVCnt = design.BatchEVCnt
    task.SlidingWin = design.SlidingWin
    task.RunMode = design.RunMode

    bdvd_log("result IDs: h0LL {0}, h1LL {1}, winCnt {2}, stats {3}"
             .format(task.Result.OIDH0LogLikelihood,
                     task.Result.OIDH1LogLikelihood,
                     task.Result.WinCnt,
                     task.Result.GIDProperty))

    (rt,taskID)=amrPrx.RunAMRFinder(task)
    return (rt,taskID,task)

def dumpOutput(fcdcPrx, task):
    armfinder=iBSDefines.JABSAMRFinderOutputDefine()
    if task.Result.OIDH0LogLikelihood>0:
        OIDs=[task.Result.OIDH0LogLikelihood,
              task.Result.OIDH1LogLikelihood]
        (rt,fois)=fcdcPrx.GetFeatureObservers(OIDs)
        (rt,bigvec_store_pathprefix)=fcdcPrx.GetFeatureValuePathPrefix(task.Result.OIDH0LogLikelihood)
        bdvd_log("h0 bigvec store: {0}".format(bigvec_store_pathprefix))
        bigvec = iBSDefines.BigVecMetaInfo()
        foi=fois[0]
        bigvec.Name = foi.ObserverName
        bigvec.StorePathPrefix = bigvec_store_pathprefix
        bigvec.ColID = foi.ObserverID
        bigvec.ColName= foi.ObserverName
        bigvec.RowCnt = foi.DomainSize
        armfinder.H0LLVec=bigvec

        (rt,bigvec_store_pathprefix)=fcdcPrx.GetFeatureValuePathPrefix(task.Result.OIDH1LogLikelihood)
        bdvd_log("h1 bigvec store: {0}".format(bigvec_store_pathprefix))
        bigvec = iBSDefines.BigVecMetaInfo()
        foi=fois[1]
        bigvec.Name = foi.ObserverName
        bigvec.StorePathPrefix = bigvec_store_pathprefix
        bigvec.ColID = foi.ObserverID
        bigvec.ColName= foi.ObserverName
        bigvec.RowCnt = foi.DomainSize
        armfinder.H1LLVec=bigvec

    if task.Result.GIDProperty>0:
        OIDs=[task.Result.GIDProperty]
        (rt,fois)=fcdcPrx.GetFeatureObservers(OIDs)
        OIDs=list(range(OIDs[0],OIDs[0]+fois[0].ObserverGroupSize))
        (rt,fois)=fcdcPrx.GetFeatureObservers(OIDs)

        (rt,bigmat_store_pathprefix)=fcdcPrx.GetFeatureValuePathPrefix(task.Result.GIDProperty)
        bdvd_log("stats store: {0}".format(bigmat_store_pathprefix))
        bigmat = iBSDefines.BigMatrixMetaInfo()
        bigmat.Name = gParams.workflow_node
        bigmat.ColStats=None
        bigmat.StorePathPrefix = bigmat_store_pathprefix
        bigmat.ColIDs = OIDs
        bigmat.ColNames= [foi.ObserverName for foi in fois]
        bigmat.RowCnt = fois[0].DomainSize
        bigmat.ColCnt=len(bigmat.ColIDs)
        armfinder.StatsMat=bigmat
    
    task.Result.fcdc=None
    armfinder.Task=task
    fn = "{0}{1}".format(output_dir,gParams.result_dumpfile)
    iBSDefines.dumpPickle(armfinder,fn)
    bdvd_log("output pickle: {0}".format(fn))

def main(argv=None):

    # Initialize default parameter values
    global gParams
    gParams = JABSParams()
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
        bdvd_log("Beginning AMRFinder run (v"+iBSUtil.get_version()+")")
        bdvd_logp("-----------------------------------------------")

        gParams.fcdc_tcp_port = iBSUtil.getUsableTcpPort()
        prepare_fcdcentral_config(gParams.fcdc_tcp_port, 
                                  gParams.fcdc_fvworker_size, 
                                  gParams.fcdc_threadpool_size)
        print("fcdc_tcp_port = ",gParams.fcdc_tcp_port)

        

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
        amrPrx = fcdc.GetJointAMRProxy(fcdcHost)
    
        bdvd_log("FCDCentral activated")
        
        attachCpGMap(fcdcPrx, amrPrx)

        (rt, amdTaskID, amrTask)=runSinglAMRFinder(fcdcPrx, amrPrx)

        preMsg=""
        amdTaskFinished=False
        bdvd_log("")
        bdvd_log("Running AMRFinder with {0} threads, {1} Mb RAM ...".format(gParams.num_threads,gParams.max_mem ))
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
        
        dumpOutput(fcdcPrx,amrTask)
        
        bdvd_log("Running AMRFinder [done]")

        
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
