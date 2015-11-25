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

use_message = '''
KMeans (multihreads, local node)

Usage:
    bigclust-kmeans-singlenode [options] <--datamat bigmat_file> <--seedsmat bigmat_file> <design_file>

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
kmeanss_dir = output_dir + "kmeanss/"
kmeansc_dir = output_dir + "kmeansc/"
script_dir = output_dir + "script/"
tmp_dir = output_dir + "tmp/"
bdvd_log_handle = None #main log file handle
bdvd_logger = None # main logging object

fcdc_popen = None
fcdc_log_file=None

kmeanss_popen = None
kmeanss_log_file=None

kmeansc_popen = None
kmeansc_log_file=None

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
        self.datamat_pickle = None
        self.seedsmat_pickle = None
        self.design_file = None
        self.kmeanss_tcp_port=16010
        self.kmeansc_tcp_port=16011
        self.kmeansc_workercnt =4
        self.kmeansc_ramsize =4000

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
                                         "datamat=",
                                         "seedsmat=",
                                         "tmp-dir="])
        except getopt.error as msg:
            raise Usage(msg)

        global output_dir
        global logging_dir
        global tmp_dir
        global fcdcentral_dir
        global kmeanss_dir
        global kmeansc_dir
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
            if option =="--datamat":
                self.datamat_pickle = value
            if option =="--seedsmat":
                self.seedsmat_pickle = value
        
        self.result_dumpfile = "{0}.pickle".format(self.workflow_node)
        if custom_out_dir:
            output_dir = custom_out_dir
            logging_dir = output_dir + "logs/"
            tmp_dir = output_dir + "tmp/"
            fcdcentral_dir = output_dir + "fcdcentral/"
            script_dir = output_dir + "script/"
            kmeanss_dir = output_dir +"kmeanss/"
            kmeansc_dir = output_dir +"kmeansc/"
        if custom_tmp_dir:
            tmp_dir = custom_tmp_dir

        if self.datacentral_dir is not None:
            fcdcentral_dir = self.datacentral_dir+"/fcdcentral/"

        if self.datamat_pickle is None:
            raise Usage(use_message)
        if self.seedsmat_pickle is None:
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
    if msg is not None:
        bdvd_logp(msg)
    shutdownKMeansC()
    shutdownKMeansS()
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

def shutdownKMeansS():
    global kmeanss_popen
    if kmeanss_popen is not None:
        kmeanss_popen.terminate()
        kmeanss_popen.wait()
        kmeanss_popen = None
        bdvd_log("KMeansS shutdown")
    if kmeanss_log_file is not None:
        kmeanss_log_file.close()

def shutdownKMeansC():
    global kmeansc_popen
    if kmeansc_popen is not None:
        kmeansc_popen.terminate()
        kmeansc_popen.wait()
        kmeansc_popen = None
        bdvd_log("KMeansC shutdown")
    if kmeansc_log_file is not None:
        kmeansc_log_file.close()

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

    if not os.path.exists(kmeanss_dir):
        os.mkdir(kmeanss_dir)

    if not os.path.exists(kmeansc_dir):
        os.mkdir(kmeansc_dir)

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

def prepare_kmeansserver_config(fcdc_tcp_port,kmeanss_tcp_port):
    infile = open("./iBS/config/KMeansServer.config")
    outfile = open(kmeanss_dir+"KMeansServer.config", "w")
    replacements = {"__FCDCentral_TCP_PORT__":str(fcdc_tcp_port),
                    "__KMeansServer_TCP_PORT__":str(kmeanss_tcp_port)}
    for line in infile:
        for src, target in replacements.items():
            line = line.replace(src, target)
        outfile.write(line)
    infile.close()
    outfile.close()

def launchKMeansServer():
    global kmeanss_popen
    global kmeanss_log_fhandle
    kmeansserver_path=os.getcwd()+"/iBS/bin/KMeansServer"
    kmeanss_cmd = [kmeansserver_path]
    bdvd_log("Launching KMeansServer ...")
    kmeanss_log_file = open(logging_dir + "kmeanss.log","w")
    kmeanss_popen = subprocess.Popen(kmeanss_cmd, cwd=kmeanss_dir, stdout=kmeanss_log_file)

def prepare_kmeansc_config(fcdc_tcp_port,kmeansc_tcp_port):
    global gParams
    designPath=os.path.abspath(script_dir)
    if designPath not in sys.path:
         sys.path.append(designPath)
    import bigclustKMeansSingleDesign as design
    gParams.kmeansc_workercnt =design.KMeansContractorWorkerCnt
    gParams.kmeansc_ramsize =design.KMeansContractorRAMSize

    infile = open("./iBS/config/KMeansContractor.config")
    outfile = open(kmeansc_dir+"KMeansContractor.config", "w")
    replacements = {"__FCDCentral_TCP_PORT__":str(fcdc_tcp_port),
                    "__FCDCentral_HOST__":"localhost",
                    "__KMeansContractor_TCP_PORT__":str(kmeansc_tcp_port),
                    "__KMeansContractor_WorkerCnt__":str(design.KMeansContractorWorkerCnt),
                    "__KMeansContractor_RAMSIZE__":str(design.KMeansContractorRAMSize)}
    for line in infile:
        for src, target in replacements.items():
            line = line.replace(src, target)
        outfile.write(line)
    infile.close()
    outfile.close()

def launchKMeansContractor():
    global kmeansc_popen
    global kmeansc_log_fhandle
    kmeanscontractor_path=os.getcwd()+"/iBS/bin/KMeansContractor"
    kmeansc_cmd = [kmeanscontractor_path]
    bdvd_log("Launching KMeansContractor ...")
    kmeansc_log_file = open(logging_dir + "kmeansc.log","w")
    kmeansc_popen = subprocess.Popen(kmeansc_cmd, cwd=kmeansc_dir, stdout=kmeansc_log_file)

def attachBigMatrix(bigmat,fcdcPrx):
    bdvd_log("attach bigmat: {0} x {1} from {2}".format(bigmat.RowCnt,bigmat.ColCnt,bigmat.StorePathPrefix))
    (rt, outOIDs)=fcdcPrx.AttachBigMatrix(bigmat.ColCnt,bigmat.RowCnt,bigmat.ColNames,bigmat.StorePathPrefix)
    bdvd_log("assigned colIDs: {0}".format(str(outOIDs)))
    #osis=bigmat.ColStats
    #rt=fcdcPrx.SetObserverStats(osis)
    return outOIDs

def prepareKMeansProject(facetAdminPrx, fcdcPrx, kmeansSAdminPrx, dataOIDs, seedOIDs, dataMat):
    designPath=os.path.abspath(script_dir)
    if designPath not in sys.path:
         sys.path.append(designPath)
    import bigclustKMeansSingleDesign as design
    domainSize = dataMat.RowCnt
    RowCnt=dataMat.RowCnt
    (rt, rqstProj)=kmeansSAdminPrx.GetBlankProject()
    rqstProj.ProjectName = "KMeans"
    rqstProj.K=design.K
    rqstProj.Distance = design.Distance
    rqstProj.MaxIteration = design.MaxIteration
    rqstProj.MinChangeCnt = design.MinChangeCnt
    rqstProj.FcdcReader = facetAdminPrx.GetBigMatrixFacet(dataOIDs[0])
    rqstProj.ObserverIDs = dataOIDs
    rqstProj.FeatureIdxFrom = 0
    rqstProj.FeatureIdxTo= domainSize
    if design.FeatureIdxFrom is not None:
        rqstProj.FeatureIdxFrom=design.FeatureIdxFrom
    if design.FeatureIdxTo is not None:
        rqstProj.FeatureIdxTo=design.FeatureIdxTo

    rqstProj.FcdcForKMeans = fcdcPrx
    rqstProj.GIDForClusterSeeds=seedOIDs[0]
    rqstProj.SeedsFeatureIdxFrom=design.SeedsFeatureIdxFrom
    rqstProj.GIDForKClusters=0
    rqstProj.OIDForFeature2ClusterIdx=0
    return rqstProj

def saveKMeansResult(fcdcPrx, kmeansServerPrx,proj, dataMat, seedsMat):
    dataRowCnt = proj.FeatureIdxTo - proj.FeatureIdxFrom
    dataColCnt = len(proj.ObserverIDs)
    projectID = proj.ProjectID
    K = proj.K

    outObj=iBSDefines.BigClustKMeansOutputDefine()
    #
    # Save KMembers
    #
    (rt, OID_KMembers) = fcdcPrx.RqstNewFeatureObserverID(False)
    foi = fcdc.GetFeatureObserver(fcdcPrx,OID_KMembers)
    foi.ObserverName = "KMembers"
    foi.DomainSize=dataRowCnt
    foi.SetPolicy = iBS.FeatureValueSetPolicyEnum.FeatureValueSetPolicyNoRAMImmediatelyToStore
    rt=fcdcPrx.SetFeatureObservers([foi])

    bdvd_log("save cluster membership for {0} rows, oid {1} ...".format(dataRowCnt, OID_KMembers))
    remainCnt =dataRowCnt
    batchRowCnt=(1024*1024*128)/(8*1)
    featureIdxFrom=0
    batchId=0
    while (remainCnt>0):
        thisBatchCnt=0
        if remainCnt>batchRowCnt:
            thisBatchCnt=batchRowCnt
        else:
            thisBatchCnt = remainCnt
        batchId+=1
        (rt,values)=kmeansServerPrx.GetKMembers(projectID,featureIdxFrom,featureIdxFrom+thisBatchCnt)
        rt=fcdcPrx.SetDoublesColumnVector(OID_KMembers,featureIdxFrom,featureIdxFrom+thisBatchCnt,values)
        featureIdxFrom+=thisBatchCnt
        remainCnt-=thisBatchCnt
    
    #now prepare output infomation
    (rt,bigvec_store_pathprefix)=fcdcPrx.GetFeatureValuePathPrefix(OID_KMembers)
    bdvd_log("k-members store: {0}".format(bigvec_store_pathprefix))
    bigvec = iBSDefines.BigVecMetaInfo()
    bigvec.Name = "k-members"
    bigvec.StorePathPrefix = bigvec_store_pathprefix
    bigvec.ColID = foi.ObserverID
    bigvec.ColName= foi.ObserverName
    bigvec.RowCnt = foi.DomainSize
    outObj.KMembersVec = bigvec
    bdvd_log("save cluster membership for {0} rows [done]".format(dataRowCnt))

    #
    # Save K Clusters
    #
    (rt, dataFois) = fcdcPrx.GetFeatureObservers(proj.ObserverIDs)
    (rt, OIDs_KClusters) = fcdcPrx.RqstNewFeatureObserversInGroup(dataColCnt,False)
    observerGroupID=OIDs_KClusters[0];
    (rt,fois)=fcdcPrx.GetFeatureObservers(OIDs_KClusters)
    for i in range(len(OIDs_KClusters)):
        observerID = OIDs_KClusters[i]
        foi = fois[i]
        foi.ObserverName = dataFois[i].ObserverName
        foi.DomainSize= K
        foi.SetPolicy = iBS.FeatureValueSetPolicyEnum.FeatureValueSetPolicyNoRAMImmediatelyToStore
        foi.DomainID=0
    fcdcPrx.SetFeatureObservers(fois);

    (rt, kclusters)=kmeansServerPrx.GetKClusters(projectID)
    fcdcPrx.SetDoublesRowMatrix(observerGroupID,0,K,kclusters)
    (rt,bigmat_store_pathprefix)=fcdcPrx.GetFeatureValuePathPrefix(observerGroupID)
    bdvd_log("k-clusters store: {0}".format(bigmat_store_pathprefix))
    bigmat = iBSDefines.BigMatrixMetaInfo()
    bigmat.Name = "k-clusters"
    bigmat.StorePathPrefix = bigmat_store_pathprefix
    bigmat.ColIDs = OIDs_KClusters
    bigmat.ColNames= [v.ObserverName for v in dataFois]
    bigmat.RowCnt = K
    bigmat.ColCnt=dataColCnt
    outObj.CentroidsMat = bigmat
    bdvd_log("save cluster centroids, {0} rows {1} cols".format(K,dataColCnt))

    #
    # Save K Cnts
    #
    (rt, OID_KCnts) = fcdcPrx.RqstNewFeatureObserverID(False)
    foi = fcdc.GetFeatureObserver(fcdcPrx,OID_KCnts)
    foi.ObserverName = "KCnts"
    foi.DomainSize=K
    foi.SetPolicy = iBS.FeatureValueSetPolicyEnum.FeatureValueSetPolicyNoRAMImmediatelyToStore
    rt=fcdcPrx.SetFeatureObservers([foi])

    (rt,values)=kmeansServerPrx.GetKCnts(projectID)
    featureIdxFrom=0
    featureIdxTo=foi.DomainSize
    rt=fcdcPrx.SetDoublesColumnVector(OID_KCnts,featureIdxFrom,featureIdxTo,values)
    (rt,bigvec_store_pathprefix)=fcdcPrx.GetFeatureValuePathPrefix(OID_KCnts)
    bdvd_log("k-cnts store: {0}".format(bigvec_store_pathprefix))
    bigvec = iBSDefines.BigVecMetaInfo()
    bigvec.Name = "k-cnts"
    bigvec.StorePathPrefix = bigvec_store_pathprefix
    bigvec.ColID = foi.ObserverID
    bigvec.ColName= foi.ObserverName
    bigvec.RowCnt = foi.DomainSize
    outObj.KCntsVec = bigvec
    bdvd_log("save cluster member counts")

    proj.FcdcReader = None
    proj.FcdcForKMeans = None
    outObj.Project =proj
    outObj.DataMat = dataMat
    outObj.SeedsMat = seedsMat;
    dumpOutput(outObj)

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
    global kmeanss_popen
    kmeanss_popen=None
    global kmeansc_popen
    kmeansc_popen=None

    try:
        if argv is None:
            argv = sys.argv
        args = gParams.parse_options(argv)
       
        print("design file = ",gParams.design_file)

        start_time = datetime.now()

        prepare_output_dir()
        init_logger(logging_dir + "bdvd.log")

        bdvd_logp()
        bdvd_log("Beginning bigclust seeds run (v"+iBSUtil.get_version()+")")
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
        dataMat = iBSDefines.loadPickle(gParams.datamat_pickle).BigMat
        dataOIDs = attachBigMatrix(dataMat,fcdcPrx)
        seedsMat = iBSDefines.loadPickle(gParams.seedsmat_pickle).BigMat
        seedOIDs = attachBigMatrix(seedsMat,fcdcPrx)

        # -----------------------------------------------------------
        # launch KMeans Server
        # -----------------------------------------------------------
        gParams.kmeanss_tcp_port = iBSUtil.getUsableTcpPort()
        prepare_kmeansserver_config(gParams.fcdc_tcp_port, 
                                  gParams.kmeanss_tcp_port)
        print("kmeanss_tcp_port = ",gParams.kmeanss_tcp_port)
        launchKMeansServer()

        kmeanssHost="KMeanServerAdminService:default -h localhost -p {0}".format(gParams.kmeanss_tcp_port)
        kmeansSAdminPrx = None
        tryCnt=0
        while (tryCnt<20) and (kmeansSAdminPrx is None):
            try:
                kmeansSAdminPrx=fcdc.GetKMeanSAdminProxy(kmeanssHost)
            except Ice.ConnectionRefusedException as ex:
                tryCnt=tryCnt+1
                time.sleep(1)

        if kmeansSAdminPrx is None:
            raise Usage("connection timeout")

        kmeansServerPrx=kmeansSAdminPrx.GetKMeansSeverProxy()
    
        bdvd_log("KMeans Server activated")

        rqstProj = prepareKMeansProject(facetAdminPrx, fcdcPrx, kmeansSAdminPrx, dataOIDs, seedOIDs, dataMat)
        (rt,retProj) = kmeansSAdminPrx.CreateProjectAndWaitForContractors(rqstProj)

        # -----------------------------------------------------------
        # launch KMeans Contractor
        # -----------------------------------------------------------
        gParams.kmeansc_tcp_port = iBSUtil.getUsableTcpPort()
        prepare_kmeansc_config(gParams.fcdc_tcp_port, 
                                  gParams.kmeansc_tcp_port)
        print("kmeansc_tcp_port = ",gParams.kmeansc_tcp_port)
        launchKMeansContractor()

        kmeanscHost="KMeanContractorAdminService:default -h localhost -p {0}".format(gParams.kmeansc_tcp_port)
        kmeansCAdminPrx = None
        tryCnt=0
        while (tryCnt<20) and (kmeansCAdminPrx is None):
            try:
                kmeansCAdminPrx=fcdc.GetKMeanCAdminProxy(kmeanscHost)
            except Ice.ConnectionRefusedException as ex:
                tryCnt=tryCnt+1
                time.sleep(1)

        if kmeansCAdminPrx is None:
            raise Usage("connection timeout")  
        bdvd_log("KMeans Contractor activated")

        # -----------------------------------------------------------
        # Run KMeans
        # -----------------------------------------------------------
        kmeansCAdminPrx.StartNewContract(retProj.ProjectID,kmeansServerPrx)
        time.sleep(4)
        (rt, amdTaskID)=kmeansSAdminPrx.LaunchProjectWithCurrentContractors(retProj.ProjectID)

        preMsg=""
        amdTaskFinished=False
        bdvd_log("")
        bdvd_log("Running KMeans with {0} threads, {1} Mb RAM ...".format(gParams.kmeansc_workercnt,gParams.kmeansc_ramsize))
        while (not amdTaskFinished):
            (rt,amdTaskInfo)=kmeansSAdminPrx.GetAMDTaskInfo(amdTaskID)
            thisMsg="task: {0}, batch processed: {1}/{2}".format(amdTaskInfo.TaskName, amdTaskInfo.FinishedCnt, amdTaskInfo.TotalCnt)
            if preMsg!=thisMsg:
                preMsg = thisMsg
                bdvd_log(thisMsg)
            if amdTaskInfo.Status!=iBS.AMDTaskStatusEnum.AMDTaskStatusNormal:
                amdTaskFinished = True;
            else:
                time.sleep(4)
        
        bdvd_log("Running KMeans [done]")

        # -----------------------------------------------------------
        # Output Results
        # -----------------------------------------------------------
        saveKMeansResult(fcdcPrx, kmeansServerPrx,retProj, dataMat, seedsMat)

        shutdownKMeansC()
        shutdownKMeansS()
        shutdownFCDCentral()

        finish_time = datetime.now()
        duration = finish_time - start_time
        bdvd_logp("-----------------------------------------------")
        bdvd_log("Run complete: %s elapsed" %  iBSUtil.formatTD(duration))

    except Usage as err:
        shutdownKMeansC()
        shutdownKMeansS()
        shutdownFCDCentral()
        bdvd_logp(sys.argv[0].split("/")[-1] + ": " + str(err.msg))
        bdvd_logp("    for detailed help see url ...")
        return 2
    
    except:
        bdvd_logp(traceback.format_exc())
        die()


if __name__ == "__main__":
    sys.exit(main())
