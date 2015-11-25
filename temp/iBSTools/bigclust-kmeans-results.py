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
import random

import iBSConfig
import iBSUtil
import iBSDefines
import iBSFCDClient as fcdc
import iBS
import Ice
import iBSFcdcHelper as fcdcHelper

use_message = '''
KMeans (multihreads, local node)

Usage:
    bigclust-kmeans-results.py [options] <--kmeansout kmeans_out_file> <design_file>

Options:
    -v/--version
    -o/--output-dir                <string>    [ default: ./bam2mat_out       ]
    -p/--num-threads               <int>       [ default: 4                   ]
    -m/--max-mem                   <int>       [ default: 20000               ]
    --tmp-dir                      <dirname>   [ default: <output_dir>/tmp ]

Advanced Options:
    --place-holder

'''

class Usage(Exception):
    def __init__(self, msg):
        self.msg = msg

output_dir = "./kmeans_out/"
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
        self.fcdc_fvworker_size=self.num_threads
        self.fcdc_tcp_port=16000
        self.fcdc_threadpool_size=2
        self.workflow_node = "seeds"
        self.result_dumpfile = None
        self.datacentral_dir = None
        self.kmeansout_pickle = None
        self.design_file = None
        self.bdt_home = None
        self.R_bin = iBSConfig.R_BinDir
        self.R_configfile = "./iBS/iBS.R/BigClust/BigClust_Plot_KMeansResult_Config.R"
        self.kmeansdistance=None

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
                                         "kmeansout=",
                                         "tmp-dir=",
                                         "R-config="])
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
                self.fcdc_fvworker_size = self.num_threads
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
            if option =="--kmeansout":
                self.kmeansout_pickle = value
            if option =="--R-config":
                self.R_configfile = value

        
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

        if self.kmeansout_pickle is None:
            raise Usage(use_message)

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

def attachBigMatrix(bigmat,fcdcPrx):
    bdvd_log("attach bigmat: {0} x {1} from {2}".format(bigmat.RowCnt,bigmat.ColCnt,bigmat.StorePathPrefix))
    (rt, outOIDs)=fcdcPrx.AttachBigMatrix(bigmat.ColCnt,bigmat.RowCnt,bigmat.ColNames,bigmat.StorePathPrefix)
    bdvd_log("assigned colIDs: {0}".format(str(outOIDs)))
    return outOIDs

def attachBigVec(bigVec, fcdcPrx):
    bdvd_log("attach bigvec: {0} from {1}".format(bigVec.RowCnt,bigVec.StorePathPrefix))
    (rt, outOID)=fcdcPrx.AttachBigVector(bigVec.RowCnt,bigVec.ColName,bigVec.StorePathPrefix)
    bdvd_log("assigned colID: {0}".format(outOID))
    return outOID

def exportKMembers(fcdcPrx, computePrx, kmembersOID, kmembersVec):
    task=computePrx.GetBlankExportRowMatrixTask()
    task.reader=fcdcPrx
    task.SampleIDs = [kmembersOID]
    task.FeatureIdxFrom=0
    task.FeatureIdxTo=kmembersVec.RowCnt
    task.OutID = 10001
    task.OutPath=os.path.abspath(output_dir)
    task.ConvertToType = iBS.FeatureValueEnum.FeatureValueInt32
    bdvd_log("Export KMembers ...")
    fcdcHelper.exportMatByRange(bdvd_log,fcdcPrx,computePrx,task)
    bfvFile = "{0}/gid_{1}.bfv".format(task.OutPath,task.OutID)
    return bfvFile

def exportKCentroids(fcdcPrx, computePrx, centroidsOIDs, kmeansout):
    centroidsMat=kmeansout.CentroidsMat
    task=computePrx.GetBlankExportRowMatrixTask()
    task.reader=fcdcPrx
    task.SampleIDs = centroidsOIDs
    task.FeatureIdxFrom=0
    task.FeatureIdxTo=centroidsMat.RowCnt
    task.OutID = 10002
    task.OutPath=os.path.abspath(output_dir)
    if kmeansout.Project.Distance == iBS.KMeansDistEnum.KMeansDistCorrelation:
        task.RowAdjust=iBS.RowAdjustEnum.RowAdjustZeroMeanUnitSD
    bdvd_log("Export KCentroids ...")
    fcdcHelper.exportMatByRange(bdvd_log,fcdcPrx,computePrx,task)

    bfvFile = "{0}/gid_{1}.bfv".format(task.OutPath,task.OutID)
    fn="{0}kcentroids_mat.txt".format(output_dir);
    outf = open(fn, "w")
    outf.write("{0}\t{1}\t{2}\t{3}\n".format("MatrixID","DataFile","RowCnt","ColCnt"))
    outf.write("{0}\t{1}\t{2}\t{3}\n".format(1,bfvFile,centroidsMat.RowCnt,len(centroidsOIDs)))
    outf.close()

    #output colNames
    colnames_fn="{0}kcentroids_colnames.txt".format(output_dir);
    outf = open(colnames_fn, "w")
    for colname in centroidsMat.ColNames:
        outf.write("{0}\n".format(colname))
    outf.close()

def exportKSeeds(fcdcPrx, computePrx, seedsOIDs, kmeansout):
    centroidsMat=kmeansout.CentroidsMat
    seedsFeatureIdxFrom=kmeansout.Project.SeedsFeatureIdxFrom
    task=computePrx.GetBlankExportRowMatrixTask()
    task.reader=fcdcPrx
    task.SampleIDs = seedsOIDs
    task.FeatureIdxFrom=seedsFeatureIdxFrom
    task.FeatureIdxTo=seedsFeatureIdxFrom+centroidsMat.RowCnt
    task.OutID = 10004
    task.OutPath=os.path.abspath(output_dir)
    if kmeansout.Project.Distance == iBS.KMeansDistEnum.KMeansDistCorrelation:
        task.RowAdjust=iBS.RowAdjustEnum.RowAdjustZeroMeanUnitSD
    bdvd_log("Export KSeeds ...")
    fcdcHelper.exportMatByRange(bdvd_log,fcdcPrx,computePrx,task)

    bfvFile = "{0}/gid_{1}.bfv".format(task.OutPath,task.OutID)
    fn="{0}kseeds_mat.txt".format(output_dir);
    outf = open(fn, "w")
    outf.write("{0}\t{1}\t{2}\t{3}\n".format("MatrixID","DataFile","RowCnt","ColCnt"))
    outf.write("{0}\t{1}\t{2}\t{3}\n".format(1,bfvFile,centroidsMat.RowCnt,len(seedsOIDs)))
    outf.close()

def exportKCnts(fcdcPrx, computePrx, kcntsOID, kcntsVec):
    task=computePrx.GetBlankExportRowMatrixTask()
    task.reader=fcdcPrx
    task.SampleIDs = [kcntsOID]
    task.FeatureIdxFrom=0
    task.FeatureIdxTo=kcntsVec.RowCnt
    task.OutID = 10003
    task.OutPath=os.path.abspath(output_dir)
    task.ConvertToType = iBS.FeatureValueEnum.FeatureValueInt32
    bdvd_log("Export KCnts ...")
    fcdcHelper.exportMatByRange(bdvd_log,fcdcPrx,computePrx,task)
    
    bfvFile = "{0}/gid_{1}.bfv".format(task.OutPath,task.OutID)
    fn="{0}kcnts_mat.txt".format(output_dir);
    outf = open(fn, "w")
    outf.write("{0}\t{1}\t{2}\t{3}\n".format("MatrixID","DataFile","RowCnt","ColCnt"))
    outf.write("{0}\t{1}\t{2}\t{3}\n".format(1,bfvFile,kcntsVec.RowCnt,1))
    outf.close()

def prepare_RScripts():
    global gParams
   
    RDir="{0}/R".format(output_dir)
    if not os.path.exists(RDir):
        os.mkdir(RDir)
    
    RScriptDir=os.path.abspath(RDir)

    # ==============================
    # Common.R
    infile = open("./iBS/iBS.R/BDVD/Common.R")
    outfile = open(RScriptDir+"/Common.R", "w")

    replacements = {"__NOTHING__":"__SOMETHING__"}

    for line in infile:
        for src, target in replacements.items():
            line = line.replace(src, target)
        outfile.write(line)
    infile.close()
    outfile.close()

    # ==============================
    # BigClust_Plot_KMeansResult_Config.R
    infile = open(gParams.R_configfile)
    outfile = open(RScriptDir+"/BigClust_Plot_KMeansResult_Config.R", "w")

    replacements = {"__NOTHING__":"__SOMETHING__"}

    for line in infile:
        for src, target in replacements.items():
            line = line.replace(src, target)
        outfile.write(line)
    infile.close()
    outfile.close()

    # ==============================
    # BigClust_Plot_KMeansResult.R
    infile = open("./iBS/iBS.R/BigClust/BigClust_Plot_KMeansResult.R")
    outfile = open(RScriptDir+"/BigClust_Plot_KMeansResult.R", "w")
    replacements = {"__RSCRIPT_DIR__":RScriptDir,
                    "__DATA_DIR__":output_dir[:-1],
                    "__OUT_DIR__":output_dir[:-1],
                    "__DISTANCE__":gParams.kmeansdistance
                    }
    for line in infile:
        for src, target in replacements.items():
            line = line.replace(src, target)
        outfile.write(line)
    infile.close()
    outfile.close()

##
## run BigClust_KCentroids_Hclust.R
##
def runKCentroidsHclust():
    RDir="{0}/R".format(output_dir)
    RScriptDir=os.path.abspath(RDir)

    cmdpath="{0}/Rscript".format(gParams.R_bin)
    r_cmd = [cmdpath,
                "--no-restore",
                "--no-save",
                "{0}/{1}".format(RScriptDir,"BigClust_Plot_KMeansResult.R")]
      
    shell_cmd=""
    for str in r_cmd:
        shell_cmd=shell_cmd+str+" "
    print(shell_cmd)
    subnode = "R"
    bdvd_logp("run subtask at: {0}{1}".format(output_dir,subnode))
    proc = subprocess.call(r_cmd)

def dumpOutput(kmeansOut):
    fn = "{0}{1}".format(output_dir,gParams.result_dumpfile)
    iBSDefines.dumpPickle(kmeansOut,fn)
    bdvd_log("output: {0}".format(fn))

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
        bdvd_log("Beginning kmeans results run (v"+iBSUtil.get_version()+")")
        bdvd_logp("-----------------------------------------------")

        # -----------------------------------------------------------
        # launch FCDCentral
        # -----------------------------------------------------------
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
    
        bdvd_log("FCDCentral activated")
        kmeansout = iBSDefines.loadPickle(gParams.kmeansout_pickle)
        dataOIDs = attachBigMatrix(kmeansout.DataMat,fcdcPrx)
        seedOIDs = attachBigMatrix(kmeansout.SeedsMat,fcdcPrx)
        centroidsOIDs = attachBigMatrix(kmeansout.CentroidsMat,fcdcPrx)
        kmembersOID = attachBigVec(kmeansout.KMembersVec, fcdcPrx)
        kcntsOID = attachBigVec(kmeansout.KCntsVec, fcdcPrx)

        gParams.kmeansdistance=str(kmeansout.Project.Distance)
        # -----------------------------------------------------------
        # Output Results
        # -----------------------------------------------------------
        
        exportKMembers(fcdcPrx, computePrx, kmembersOID, kmeansout.KMembersVec)
        exportKCentroids(fcdcPrx, computePrx, centroidsOIDs, kmeansout)
        exportKCnts(fcdcPrx, computePrx, kcntsOID, kmeansout.KCntsVec)
        exportKSeeds(fcdcPrx, computePrx, seedOIDs, kmeansout)
        

        prepare_RScripts()
        runKCentroidsHclust()

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
