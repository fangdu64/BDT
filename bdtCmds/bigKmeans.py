#!__PYTHON_BIN_PATH__

"""
command line tools for big k-means 
"""

import os
import sys, traceback
import getopt
import subprocess
import shutil
import time
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
import bigKmeansUtil
import iBSFCDClient as fcdc
import bigMatUtil
import iBS
import Ice

use_message = '''
bigKmeans

Usage:
    bigKmeans [options] <--data data_file> <--nrow row_cnt> <--ncol col_cnt> <--k cluster_num> <--out out_dir>

Options:
    -v/--version
    -p/--thread-num                <int>       [ default: 4             ]
    --dist-type                    <string>    [ Euclidean, Correlation ]

Advanced Options:

'''

gParams=None
gRunner=None
gSteps = ['1-data-mat', '2-seeds-mat','3-run-kmeans']
gSeedingMethod = ['kmeans++', 'random', 'provided']

class Usage(Exception):
    def __init__(self, msg):
        self.msg = msg

class BigKmeansParams:
    def __init__(self):
        self.output_dir = None
        self.logging_dir = None
        self.start_from = gSteps[0]
        self.dry_run = True
        self.remove_before_run = True
        self.pipeline_rundir = None
        self.data_params = None
        self.data_indir = None
        self.kmeansc_workercnt = 4
        self.dist_type = "Euclidean"
        self.kmeans_ks = None
        self.kmeans_maxiter = 100
        self.kmeans_minexplainedchange = 0.0001
        self.seeding_method = gSeedingMethod[0]
        self.seeds_params = None
        self.seeds_indir = None
        self.slave_num = None

    def parse_options(self, argv):
        dataParams = bigMatUtil.BigMatParams()
        seedsParams = bigMatUtil.BigMatParams()
        args = argv[1:]
        args = dataParams.strip_argv('data-', args)
        args = seedsParams.strip_argv('seeds-', args)
        dataArgv = dataParams.argv
        seedsArgv = seedsParams.argv
        bigKmeansArgv = args
        try:
            opts, args = getopt.getopt(bigKmeansArgv, "",
                ["version",
                "help",
                "start-from=",
                "out=",
                "thread-num=",
                "dist-type=",
                "k=",
                "max-iter=",
                "min-expchg=",
                "seeding-method=",
                "slave-num="])

        except getopt.error as msg:
            raise Usage(msg)

        # option processing
        for option, value in opts:
            if option in ("-v", "--version"):
                print("bigKmeans v",bdtUtil.get_version())
                sys.exit(0)
            if option in ("-h", "--help"):
                raise Usage(use_message)
            if option in ("-p", "--thread-num"):
                self.kmeansc_workercnt = int(value)
            if option =="--slave-num":
                self.slave_num = int(value)
            if option in ("-o", "--out"):
                self.output_dir = value
            if option =="--start-from":
                allowedValues = gSteps;
                if value not in allowedValues:
                    raise Usage('--start-from should be one of the {0}'.format(allowedValues))
                self.start_from = value
            if option in ("--max-iter"):
                self.kmeans_maxiter = int(value)
            if option in ("--min-expchg"):
                self.kmeans_minexplainedchange = float(value)
            if option =="--dist_type":
                allowedValues = ('Euclidean', 'Correlation');
                if value not in allowedValues:
                    raise Usage('--dist_type should be one of the {0}'.format(allowedValues))
                self.dist_type = value
            if option =="--k":
                self.kmeans_ks = bdtUtil.parseIntSeq(value)
                if len(self.kmeans_ks)<1:
                    raise Usage('invalid --k')
            if option == "--seeding-method":
                allowedValues = gSeedingMethod
                if value not in allowedValues:
                    raise Usage('--seeding-method should be one of the {0}'.format(allowedValues))
                self.seeding_method = value

        requiredNames = ['--k', '--out']
        providedValues = [self.kmeans_ks, self.output_dir]
        noneIdx = bdtUtil.getFirstNone(providedValues)
        if noneIdx != -1:
            raise Usage("{0} is required".format(requiredNames[noneIdx]))

        self.output_dir = os.path.abspath(self.output_dir)
        self.logging_dir = os.path.abspath(self.output_dir + "/logs")
        self.pipeline_rundir=os.path.abspath(self.output_dir + "/run")

        # parse options for data
        self.data_indir =os.path.abspath("{0}/{1}".format(self.pipeline_rundir, gSteps[0]))
        dataArgv.extend(['--data-out',self.data_indir])
        self.data_params = bigMatUtil.BigMatParams()
        self.data_params.parse_options("data-", dataArgv)
        
        if self.seeding_method == "provided" or len(seedsArgv) > 0:
            self.seeds_indir=os.path.abspath("{0}/{1}".format(self.pipeline_rundir, gSteps[1]))
            seedsArgv.extend(['--seeds-out',self.seeds_indir])
            self.seeds_params = bigMatUtil.BigMatParams()
            self.seeds_params.parse_options("seeds-", seedsArgv)
        return args

def s01_data_mat():
    nodeName = gSteps[0]
    return bigMatUtil.run_bigMat(
        gRunner,
        Platform,
        BDT_HomeDir,
        nodeName,
        gParams.pipeline_rundir,
        gParams.dry_run,
        gParams.remove_before_run,
        gParams.data_params)

def s02_seeds_mat():
    nodeName = gSteps[1]
    return bigMatUtil.run_bigMat(
        gRunner,
        Platform,
        BDT_HomeDir,
        nodeName,
        gParams.pipeline_rundir,
        gParams.dry_run,
        gParams.remove_before_run,
        gParams.seeds_params)
# -----------------------------------------------------------
# KMeans ++
# -----------------------------------------------------------
def s03_kmeansPP(datamatPickle, seedsmatPickle):
    nodeName = gSteps[2]
    nodeDir=os.path.abspath(gParams.pipeline_rundir+"/"+nodeName)
    nodeScriptDir=os.path.abspath(nodeDir+"-script")

    if not os.path.exists(nodeScriptDir):
        os.mkdir(nodeScriptDir)

    #
    # prepare design file
    #
    Ks = gParams.kmeans_ks
    Distance = iBS.KMeansDistEnum.KMeansDistEuclidean
    if gParams.dist_type=="Correlation":
        Distance = iBS.KMeansDistEnum.KMeansDistCorrelation
    MaxIteration = gParams.kmeans_maxiter
    MinExplainedChanged = gParams.kmeans_minexplainedchange
    FeatureIdxFrom = None
    FeatureIdxTo = None
    KMeansContractorWorkerCnt = gParams.kmeansc_workercnt
    Seeding = iBS.KMeansSeedingEnum.KMeansSeedingKMeansPlusPlus
    if gParams.seeding_method == gSeedingMethod[1]:
        Seeding = iBS.KMeansSeedingEnum.KMeansSeedingKMeansRandom

    design_params=(Ks,Distance,MaxIteration,MinExplainedChanged,FeatureIdxFrom,FeatureIdxTo,KMeansContractorWorkerCnt,Seeding)
    params_pickle_fn=os.path.abspath("{0}/design_params.pickle".format(nodeScriptDir))
    iBSDefines.dumpPickle(design_params, params_pickle_fn)

    design_file=os.path.abspath(BDT_HomeDir+"/bdt/bdtPy/PipelineDesigns/bigKmeansSingleNodeDesign.py")
    shutil.copy(design_file,nodeScriptDir)

    #
    # Run node
    #
    design_fn=os.path.abspath(nodeScriptDir)+"/bigKmeansSingleNodeDesign.py"
    subnode=nodeName
    cmdpath=os.path.abspath("{0}/bdt/bdtCmds/bigclust-kmeans++".format(BDT_HomeDir))

    if Platform == "Windows":
        node_cmd = ["py", cmdpath]
    else:
        node_cmd = [cmdpath]
    node_cmd.extend(["--node",nodeName,
                "--datamat", datamatPickle,
                "--output-dir",nodeDir])
    if seedsmatPickle is not None:
       node_cmd.extend(["--seeds-mat", seedsmatPickle])
    if gParams.slave_num is not None:
       node_cmd.extend(["--slave-num", str(gParams.slave_num)])
    node_cmd.append(design_fn)
    shell_cmd=""
    for strCmd in node_cmd:
        shell_cmd=shell_cmd+strCmd+" "
    #print(shell_cmd)

    gRunner.logp("run subtask at: {0}".format(nodeDir))
    proc = subprocess.call(node_cmd)
    subnode_picke_file=os.path.abspath("{0}/{1}.pickle".format(nodeDir,nodeName))
    gRunner.logp("end subtask \n")
    return (nodeDir,subnode_picke_file)

def outputR():
    obj = iBSDefines.loadPickle(
        iBSDefines.derivePickleFile(gParams.output_dir))

    infile = open("{0}/bdt/bdtR/outputTemplates/bigKmeansOutputTemplate.R".format(BDT_HomeDir))
    outfile = open("{0}/logs/output.R".format(gParams.output_dir), "w")

    replacements = {"__DATA_MAT_NAME__": obj.DataMat.Name, 
                    "__DATA_MAT_STORE_PATH_PREFIX__": obj.DataMat.StorePathPrefix.replace('\\','/'), 
                    "__DATA_MAT_ROW_CNT__":str(obj.DataMat.RowCnt),
                    "__DATA_MAT_COL_CNT__":str(obj.DataMat.ColCnt),
                    "__DATA_MAT_COL_NAMES__":str(obj.DataMat.ColNames).replace('[','').replace(']',''),
                    "__DATA_MAT_COL_IDS__":str(obj.DataMat.ColIDs).replace('[','').replace(']',''),

                    "__SEEDS_MAT_NAME__": obj.SeedsMat.Name, 
                    "__SEEDS_MAT_STORE_PATH_PREFIX__": obj.SeedsMat.StorePathPrefix.replace('\\','/'), 
                    "__SEEDS_MAT_ROW_CNT__":str(obj.SeedsMat.RowCnt),
                    "__SEEDS_MAT_COL_CNT__":str(obj.SeedsMat.ColCnt),
                    "__SEEDS_MAT_COL_NAMES__":str(obj.SeedsMat.ColNames).replace('[','').replace(']',''),
                    "__SEEDS_MAT_COL_IDS__":str(obj.SeedsMat.ColIDs).replace('[','').replace(']',''),

                    "__CENTROIDS_MAT_NAME__": obj.CentroidsMat.Name, 
                    "__CENTROIDS_MAT_STORE_PATH_PREFIX__": obj.CentroidsMat.StorePathPrefix.replace('\\','/'), 
                    "__CENTROIDS_MAT_ROW_CNT__":str(obj.CentroidsMat.RowCnt),
                    "__CENTROIDS_MAT_COL_CNT__":str(obj.CentroidsMat.ColCnt),
                    "__CENTROIDS_MAT_COL_NAMES__":str(obj.CentroidsMat.ColNames).replace('[','').replace(']',''),
                    "__CENTROIDS_MAT_COL_IDS__":str(obj.CentroidsMat.ColIDs).replace('[','').replace(']',''),

                    "__CLUST_ASSIGNMENT_VEC_NAME__": obj.KMembersVec.Name, 
                    "__CLUST_ASSIGNMENT_VEC_STORE_PATH_PREFIX__": obj.KMembersVec.StorePathPrefix.replace('\\','/'), 
                    "__CLUST_ASSIGNMENT_VEC_ROW_CNT__":str(obj.KMembersVec.RowCnt),
                    "__CLUST_ASSIGNMENT_VEC_COL_NAME__":obj.KMembersVec.ColName,
                    "__CLUST_ASSIGNMENT_VEC_COL_ID__":str(obj.KMembersVec.ColID)
                    }

    for line in infile:
        for src, target in replacements.items():
            line = line.replace(src, target)
        outfile.write(line)
    infile.close()
    outfile.close()

def main(argv=None):
    global gParams
    global gRunner
    gParams = BigKmeansParams()
    gRunner = bdtUtil.bdtRunner()
    run_argv = sys.argv[:]

    try:
        if argv is None:
            argv = sys.argv
        args = gParams.parse_options(argv)
       
        start_time = datetime.now()

        gRunner.prepare_dirs(gParams.output_dir, gParams.logging_dir, gParams.pipeline_rundir)
        gRunner.init_logger(os.path.abspath(gParams.logging_dir + "/bigKmeans.log"))

        gRunner.logp()
        gRunner.log("Beginning bigKmeans run v({0})".format(bdtUtil.get_version()))
        gRunner.logp("-----------------------------------------------")

        # -----------------------------------------------------------
        # launch bigMat
        # -----------------------------------------------------------     
        if gParams.dry_run and gParams.start_from == gSteps[0]:
            gParams.dry_run = False

        datamatPickle = s01_data_mat()
        if gParams.dry_run and not os.path.exists(datamatPickle):
            gRunner.log("retrieve existing result for: {0}".format(gSteps[0]))
            gRunner.log("from: {0}".format(datamatPickle))
            gRunner.die('file not exist')
            gRunner.log("")
        
        seedsmatPickle = None
        if gParams.seeds_params is not None:
            if gParams.dry_run and gParams.start_from == gSteps[1]:
                gParams.dry_run = False
            seedsmatPickle = s02_seeds_mat()
            if gParams.dry_run and not os.path.exists(seedsmatPickle):
                gRunner.log("retrieve existing result for: {0}".format(gSteps[1]))
                gRunner.log("from: {0}".format(seedsmatPickle))
                gRunner.die('file not exist')
                gRunner.log("")

        (nodeDir,subnode_picke)=s03_kmeansPP(datamatPickle, seedsmatPickle)

        runSummary = iBSDefines.NodeRunSummaryDefine()
        runSummary.NodeDir = gParams.output_dir
        runSummary.NodeType = "bigKmeans"
        runSummaryPicke = "{0}/logs/runSummary.pickle".format(gParams.output_dir)
        iBSDefines.dumpPickle(runSummary, runSummaryPicke)

        outputR()

        finish_time = datetime.now()
        duration = finish_time - start_time
        gRunner.logp("-----------------------------------------------")
        gRunner.log("Run complete: %s elapsed" %  bdtUtil.formatTD(duration))

    except Usage as err:
        gRunner.logp(sys.argv[0].split("/")[-1] + ": " + str(err.msg))
        gRunner.logp(" for detailed help see url ...")
        return 2
    
    except:
        gRunner.logp(traceback.format_exc())
        gRunner.die()

if __name__ == "__main__":
    sys.exit(main())
