#!__PYTHON_BIN_PATH__

"""
bigclust-kmeans++
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
import bigKmeansUtil
import iBSDefines
import iBSFCDClient as fcdc
import iBS
import Ice
import iBSFcdcHelper as fcdcHelper

use_message = '''
KMeans++ (multihreads, local node)

Usage:
    bigclust-ppseeds-singlenode [options] <--datamat bigmat_file> <design_file>

Options:
    -v/--version
    -o/--output-dir                <string>    [ default: ./bam2mat_out       ]
    -p/--num-threads               <int>       [ default: 4                   ]
    -m/--max-mem                   <int>       [ default: 20000               ]
    --tmp-dir                      <dirname>   [ default: <output_dir>/tmp ]

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
        self.workflow_node = "kmeans++"
        self.result_dumpfile = None
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
                                         "bigmat-dir=",
                                         "datamat=",
                                         "seedsmat=",
                                         "tmp-dir="])
        except getopt.error as msg:
            raise Usage(msg)

        # option processing
        for option, value in opts:
            if option in ("-v", "--version"):
                print("bigKmeans v",bdtUtil.get_version())
                sys.exit(0)
            if option in ("-h", "--help"):
                raise Usage(use_message)
            if option in ("-p", "--num-threads"):
                self.num_threads = int(value)
                self.fcdc_fvworker_size = self.num_threads
            if option in ("-m", "--max-mem"):
                self.max_mem = int(value)
            if option in ("-o", "--output-dir"):
                self.output_dir = value
            if option == "--tmp-dir":
                custom_tmp_dir = value + "/"
            if option == "--node":
                self.workflow_node = value
            if option =="--bigmat-dir":
                self.bigmat_dir = value
            if option =="--datamat":
                self.datamat_pickle = value
            if option =="--seedsmat":
                self.seedsmat_pickle = value
        
        self.result_dumpfile = "{0}.pickle".format(self.workflow_node)
        self.output_dir = os.path.abspath(self.output_dir)

        if self.bigmat_dir is None:
            self.bigmat_dir = self.output_dir+"/bigmat"
        self.bigmat_dir = os.path.abspath(self.bigmat_dir)

        if self.datamat_pickle is None:
            raise Usage(use_message)
       
        if len(args) < 1:
            raise Usage(use_message)
        self.design_file = args[0]
        return args

def prepare_output_dir():
    shutil.copy(gParams.design_file,
                os.path.abspath("{0}/bigclustKMeansPPDesign.py".format(gRunner.script_dir)))

def attachBigMatrix(bigmat,fcdcPrx):
    gRunner.log("attach bigmat: {0} x {1} from {2}".format(bigmat.RowCnt,bigmat.ColCnt,bigmat.StorePathPrefix))
    (rt, outOIDs)=fcdcPrx.AttachBigMatrix(bigmat.ColCnt,bigmat.RowCnt,bigmat.ColNames,bigmat.StorePathPrefix)
    return outOIDs

def prepareKMeansProject(facetAdminPrx, fcdcPrx, kmeansSAdminPrx, dataOIDs, dataMat, seedOIDs):
    designPath=os.path.abspath(gRunner.script_dir)
    if designPath not in sys.path:
         sys.path.append(designPath)
    import bigclustKMeansPPDesign as design
    domainSize = dataMat.RowCnt
    RowCnt=dataMat.RowCnt
    (rt, rqstProj)=kmeansSAdminPrx.GetBlankProject()
    rqstProj.ProjectName = "KMeans++"
    rqstProj.Seeding = design.Seeding
    if seedOIDs[0]==0:
        rqstProj.Task = iBS.KMeansTaskEnum.KMeansTaskPPSeeds
        #print(str(rqstProj.Seeding))
        rqstProj.K=max(design.Ks)
        rqstProj.BatchKs=design.Ks
        if 1 not in rqstProj.BatchKs:
            rqstProj.BatchKs.append(1)
    else:
        #with seeds
        rqstProj.Task = iBS.KMeansTaskEnum.KMeansTaskRunKMeans
        #print(str(rqstProj.Seeding))
        rqstProj.K=1
        rqstProj.BatchKs=design.Ks
        if 1 in rqstProj.BatchKs:
            rqstProj.BatchKs.remove(1)
    rqstProj.BatchKs=sorted(list(set(rqstProj.BatchKs)))
    rqstProj.BatchTasks=[iBS.KMeansTaskEnum.KMeansTaskRunKMeans]*len(rqstProj.BatchKs)
    gRunner.log("Ks = {0}".format(str(rqstProj.BatchKs)))
    rqstProj.Distance = design.Distance
    rqstProj.MaxIteration = design.MaxIteration
    rqstProj.MinChangeCnt = 1 # design.MinChangeCnt
    rqstProj.MinExplainedChanged = design.MinExplainedChanged
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
    rqstProj.SeedsFeatureIdxFrom=0
    rqstProj.GIDForKClusters=0
    rqstProj.OIDForFeature2ClusterIdx=0
    return rqstProj

def saveKMeansResult(fcdcPrx, computePrx, kmeansServerPrx,proj, dataMat):
    dataRowCnt = proj.FeatureIdxTo - proj.FeatureIdxFrom
    dataColCnt = len(proj.ObserverIDs)
    projectID = proj.ProjectID
    K = max(proj.BatchKs)

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

    gRunner.log("save cluster membership for {0} rows, oid {1} ".format(dataRowCnt, OID_KMembers))
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
    
    #concurrency issue, wait until OID_KMembers has aready been saved
    (rt,values) = fcdcPrx.GetDoublesColumnVector(OID_KMembers,0,1)

    #now prepare output infomation
    (rt,bigvec_store_pathprefix)=fcdcPrx.GetFeatureValuePathPrefix(OID_KMembers)
    bigvec_store_pathprefix = os.path.abspath(bigvec_store_pathprefix)
    gRunner.log("k-members store: {0}".format(bigvec_store_pathprefix))
    bigvec = iBSDefines.BigVecMetaInfo()
    bigvec.Name = "k-members"
    bigvec.StorePathPrefix = bigvec_store_pathprefix
    bigvec.ColID = foi.ObserverID
    bigvec.ColName= foi.ObserverName
    bigvec.RowCnt = foi.DomainSize
    outObj.KMembersVec = bigvec
    gRunner.log("save cluster membership for {0} rows [done]".format(dataRowCnt))

    exportKMembers(fcdcPrx, computePrx, OID_KMembers, outObj.KMembersVec)

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
    bigmat_store_pathprefix = os.path.abspath(bigmat_store_pathprefix)
    gRunner.log("k-clusters store: {0}".format(bigmat_store_pathprefix))
    bigmat = iBSDefines.BigMatrixMetaInfo()
    bigmat.Name = "k-clusters"
    bigmat.StorePathPrefix = bigmat_store_pathprefix
    bigmat.ColIDs = OIDs_KClusters
    bigmat.ColNames= [v.ObserverName for v in dataFois]
    bigmat.RowCnt = K
    bigmat.ColCnt=dataColCnt
    outObj.CentroidsMat = bigmat
    gRunner.log("save cluster centroids, {0} rows {1} cols".format(K,dataColCnt))

    #
    # Save K Seeds
    #
    (rt, dataFois) = fcdcPrx.GetFeatureObservers(proj.ObserverIDs)
    (rt, OIDs_KSeeds) = fcdcPrx.RqstNewFeatureObserversInGroup(dataColCnt,False)
    observerGroupID=OIDs_KSeeds[0];
    (rt,fois)=fcdcPrx.GetFeatureObservers(OIDs_KSeeds)
    for i in range(len(OIDs_KSeeds)):
        observerID = OIDs_KSeeds[i]
        foi = fois[i]
        foi.ObserverName = dataFois[i].ObserverName
        foi.DomainSize= K
        foi.SetPolicy = iBS.FeatureValueSetPolicyEnum.FeatureValueSetPolicyNoRAMImmediatelyToStore
        foi.DomainID=0
    fcdcPrx.SetFeatureObservers(fois);

    (rt, kseeds)=kmeansServerPrx.GetKSeeds(projectID)
    fcdcPrx.SetDoublesRowMatrix(observerGroupID,0,K,kseeds)
    (rt,bigmat_store_pathprefix)=fcdcPrx.GetFeatureValuePathPrefix(observerGroupID)
    bigmat_store_pathprefix = os.path.abspath(bigmat_store_pathprefix)
    gRunner.log("k-seeds store: {0}".format(bigmat_store_pathprefix))
    bigmat = iBSDefines.BigMatrixMetaInfo()
    bigmat.Name = "k-seeds"
    bigmat.StorePathPrefix = bigmat_store_pathprefix
    bigmat.ColIDs = OIDs_KSeeds
    bigmat.ColNames= [v.ObserverName for v in dataFois]
    bigmat.RowCnt = K
    bigmat.ColCnt=dataColCnt
    outObj.SeedsMat = bigmat
    gRunner.log("save seeds, {0} rows {1} cols".format(K,dataColCnt))

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
    bigvec_store_pathprefix = os.path.abspath(bigvec_store_pathprefix)
    gRunner.log("k-cnts store: {0}".format(bigvec_store_pathprefix))
    bigvec = iBSDefines.BigVecMetaInfo()
    bigvec.Name = "k-cnts"
    bigvec.StorePathPrefix = bigvec_store_pathprefix
    bigvec.ColID = foi.ObserverID
    bigvec.ColName= foi.ObserverName
    bigvec.RowCnt = foi.DomainSize
    outObj.KCntsVec = bigvec
    gRunner.log("save cluster member counts")

    #
    # Save Run Statistics
    #
    (rt, krets)=kmeansServerPrx.GetKMeansResults(proj.ProjectID)
    # output matrix
    results_mat_fn=os.path.abspath("{0}/results_mats.txt".format(gParams.output_dir))
    outf = open(results_mat_fn, "w")
    outf.write("{0}\t{1}\t{2}\t{3}\t{4}\t{5}\t{6}\t{7}\t{8}\t{9}\n".format("K","Iteration","Distorsion","ChangedCnt","Explained","WallTime","RowCnt","ColCnt","Seeding","Distance"))
    i=0
    for kr in krets:
        outf.write("{0}\t{1}\t{2}\t{3}\t{4}\t{5}\t{6}\t{7}\t{8}\t{9}\n".format(
            kr.K,
            len(kr.Distorsions),
            kr.Distorsions[len(kr.Distorsions)-1],
            kr.ChangedCnts[len(kr.ChangedCnts)-1],
            kr.Explaineds[len(kr.Explaineds)-1],
            kr.WallTimeSeconds,
            kr.RowCnt,
            kr.ColCnt,
            kr.Seeding,
            kr.Distance))
    outf.close()

    proj.FcdcReader = None
    proj.FcdcForKMeans = None
    outObj.Project =proj
    outObj.DataMat = dataMat
    outObj.Results = krets
    dumpOutput(outObj)

def exportKMembers(fcdcPrx, computePrx, kmembersOID, kmembersVec):
    task=computePrx.GetBlankExportRowMatrixTask()
    task.reader=fcdcPrx
    task.SampleIDs = [kmembersOID]
    task.FeatureIdxFrom=0
    task.FeatureIdxTo=kmembersVec.RowCnt
    task.OutID = 10001
    task.OutPath=os.path.abspath(gParams.output_dir)
    task.OutFile = os.path.abspath("{0}/cluster_assignments.bfv".format(gParams.output_dir))
    task.ConvertToType = iBS.FeatureValueEnum.FeatureValueInt32
    gRunner.log("Export cluster_assignments ...")
    fcdcHelper.exportMatByRange(gRunner.log,fcdcPrx,computePrx,task)
    bfvFile = task.OutFile
    gRunner.logp("cluster_assignments: {0}\n".format(bfvFile))
    return bfvFile

def dumpOutput(kmeansOut):
    fn = os.path.abspath("{0}/{1}".format(gParams.output_dir,gParams.result_dumpfile))
    iBSDefines.dumpPickle(kmeansOut,fn)
    gRunner.log("output: {0}".format(fn))

def main(argv=None):

    # Initialize default parameter values
    global gParams
    global gRunner
    gParams = BDVDParams()
    gRunner = bigKmeansUtil.singleNodeKmeansRunner(iBSConfig.BDT_HomeDir)
    run_argv = sys.argv[:]

    try:
        if argv is None:
            argv = sys.argv
        args = gParams.parse_options(argv)
       
        #print("design file = ",gParams.design_file)

        start_time = datetime.now()

        gRunner.prepare_dirs(gParams.output_dir, gParams.bigmat_dir)
        prepare_output_dir()
        gRunner.init_logger("kmeans++.log")

        gRunner.logp()
        gRunner.log("Beginning bigclust kmeans++ seeds run (v"+bdtUtil.get_version()+")")
        gRunner.logp("-----------------------------------------------")

        # -----------------------------------------------------------
        # launch bigMat
        # -----------------------------------------------------------
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
        dataMat = iBSDefines.loadPickle(gParams.datamat_pickle).BigMat
        dataOIDs = attachBigMatrix(dataMat,fcdcPrx)
        seedOIDs=[0]
        if gParams.seedsmat_pickle is not None:
            seedmatObj=iBSDefines.loadPickle(gParams.seedsmat_pickle)
            if hasattr(seedmatObj,"SeedsMat"):
                seedsMat = iBSDefines.loadPickle(gParams.seedsmat_pickle).SeedsMat
            else:
                seedsMat = iBSDefines.loadPickle(gParams.seedsmat_pickle).BigMat
            seedOIDs = attachBigMatrix(seedsMat,fcdcPrx)

        # -----------------------------------------------------------
        # launch KMeans Server
        # -----------------------------------------------------------
        gParams.kmeanss_tcp_port = bdtUtil.getUsableTcpPort()
        gRunner.prepare_kmeansserver_config(gParams.fcdc_tcp_port, 
                                  gParams.kmeanss_tcp_port)
        gRunner.launchKMeansServer()

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
    
        gRunner.log("KMeans Server activated")

        rqstProj = prepareKMeansProject(facetAdminPrx, fcdcPrx, kmeansSAdminPrx, dataOIDs, dataMat, seedOIDs)
        (rt,retProj) = kmeansSAdminPrx.CreateProjectAndWaitForContractors(rqstProj)

        # -----------------------------------------------------------
        # launch KMeans Contractor
        # -----------------------------------------------------------
        gParams.kmeansc_tcp_port = bdtUtil.getUsableTcpPort()
        gRunner.prepare_kmeansc_config(gParams.fcdc_tcp_port, 
                                  gParams.kmeansc_tcp_port)
        #print("kmeansc_tcp_port = ",gParams.kmeansc_tcp_port)
        gRunner.launchKMeansContractor()

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
        gRunner.log("KMeans Contractor activated")

        # -----------------------------------------------------------
        # Run KMeans
        # -----------------------------------------------------------
        kmeansCAdminPrx.StartNewContract(retProj.ProjectID,kmeansServerPrx)
        time.sleep(4)
        (rt, amdTaskID)=kmeansSAdminPrx.LaunchProjectWithCurrentContractors(retProj.ProjectID)

        preMsg=""
        amdTaskFinished=False
        gRunner.log("")
        gRunner.log("Running KMeans++ seeds with {0} threads".format(gParams.kmeansc_workercnt))
        while (not amdTaskFinished):
            (rt,amdTaskInfo)=kmeansSAdminPrx.GetAMDTaskInfo(amdTaskID)
            thisMsg="task: {0}, batch processed: {1}/{2}".format(amdTaskInfo.TaskName, amdTaskInfo.FinishedCnt, amdTaskInfo.TotalCnt)
            if preMsg!=thisMsg:
                preMsg = thisMsg
                gRunner.log(thisMsg)
            if amdTaskInfo.Status!=iBS.AMDTaskStatusEnum.AMDTaskStatusNormal:
                amdTaskFinished = True;
            else:
                time.sleep(4)
        
        gRunner.log("Running KMeans [done]")

        # -----------------------------------------------------------
        # Output Results
        # -----------------------------------------------------------
        saveKMeansResult(fcdcPrx, computePrx, kmeansServerPrx,retProj, dataMat)

        gRunner.shutdown_kmeansc()
        gRunner.shutdown_kmeanss()
        gRunner.shutdown_bigMat()

        finish_time = datetime.now()
        duration = finish_time - start_time
        gRunner.logp("-----------------------------------------------")
        gRunner.log("Run complete: %s elapsed" %  bdtUtil.formatTD(duration))

    except Usage as err:
        gRunner.shutdown_kmeansc()
        gRunner.shutdown_kmeanss()
        gRunner.shutdown_bigMat()
        gRunner.logp(sys.argv[0].split("/")[-1] + ": " + str(err.msg))
        gRunner.logp("    for detailed help see url ...")
        return 2
    
    except:
        gRunner.logp(traceback.format_exc())
        gRunner.die()


if __name__ == "__main__":
    sys.exit(main())
