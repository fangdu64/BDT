#!__PYTHON_BIN_PATH__

"""
pipeline-kmeans++.py
Created by Fang Du on 2014-11-11.
"""
import os
BDT_HomeDir=os.path.dirname(os.path.abspath(__file__))
if os.getcwd()!=BDT_HomeDir:
    os.chdir(BDT_HomeDir)

import sys, traceback
import getopt
import subprocess
import shutil
import time
from datetime import datetime, date
import logging
import random

import iBSConfig
import iBSUtil
import iBSDefines
import iBSFCDClient as fcdc
import iBS
import Ice

use_message = '''
KMeans++ (multihreads, local node)

Usage:
    pipeline-kmeans++.py [options] <--data data_file> <--nrow row_cnt> <--ncol col_cnt> <--k cluster_num> <--out out_dir>

Options:
    -v/--version
    -p/--thread-num                <int>       [ default: 4             ]
    --dist-type                    <string>    [ Euclidean, Correlation ]

Advanced Options:

'''

class Usage(Exception):
    def __init__(self, msg):
        self.msg = msg

output_dir = "~/kmeans++/"
logging_dir = output_dir + "logs/"

bdvd_log_handle = None #main log file handle
bdvd_logger = None # main logging object

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
        self.start_from = '1-input-mat'
        self.dry_run = True
        self.remove_before_run = True

        self.pipeline_rundir = None
        self.data_file = None
        self.kmeansc_workercnt = 4
        self.dist_type = "Euclidean"
        self.kmeans_ks = None
        self.row_cnt = None
        self.col_cnt = None
        self.kmeans_maxiter = 100
        self.kmeans_minexplainedchange = 0.0001

    def parse_options(self, argv):
        try:
            opts, args = getopt.getopt(argv[1:], "hvp:m:o:",
                                        ["version",
                                         "help",
                                         "start-from=",
                                         "data=",
                                         "out=",
                                         "thread-num=",
                                         "dist-type=",
                                         "ncol=",
                                         "nrow=",
                                         "k=",
                                         "max-iter=",
                                         "min-expchg="])
        except getopt.error as msg:
            raise Usage(msg)

        global output_dir
        global logging_dir

        custom_out_dir = None

        # option processing
        for option, value in opts:
            if option in ("-v", "--version"):
                print("KMeans++ v",iBSUtil.get_version())
                sys.exit(0)
            if option in ("-h", "--help"):
                raise Usage(use_message)
            if option in ("-p", "--thread-num"):
                self.kmeansc_workercnt = int(value)
            if option in ("-o", "--out"):
                custom_out_dir = value + "/"
            if option =="--start-from":
                if value not in ('1-input-mat', '2-kmeans++'):
                    raise Usage('invalid --start-from')
                self.start_from = value
            if option == "--data":
                self.data_file = value
            if option in ("--ncol"):
                self.col_cnt = int(value)
            if option in ("--nrow"):
                self.row_cnt = int(value)
            if option in ("--max-iter"):
                self.kmeans_maxiter = int(value)
            if option in ("--min-expchg"):
                self.kmeans_minexplainedchange = float(value)
            if option =="--dist_type":
                self.dist_type = value
            if option =="--k":
                self.kmeans_ks = iBSUtil.parseIntSeq(value)
                if len(self.kmeans_ks)<1:
                    raise Usage(use_message)

        if custom_out_dir:
            output_dir = custom_out_dir
            logging_dir = output_dir + "logs/"
        self.pipeline_rundir=output_dir+"run"
        
        if (self.data_file is None) or (self.kmeans_ks is None) or (custom_out_dir is None):
            raise Usage(use_message)
        if (self.col_cnt is None) or (self.row_cnt is None):
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
    sys.exit(1)


# Ensures that the output, logging, and temp directories are present. If not,
# they are created
def prepare_output_dir():
    bdvd_log("Preparing output location "+output_dir)
    if not os.path.exists(output_dir):
        os.mkdir(output_dir)

    if not os.path.exists(logging_dir):
        os.mkdir(logging_dir)
    
    if not os.path.exists(gParams.pipeline_rundir):
        os.mkdir(gParams.pipeline_rundir)

# -----------------------------------------------------------
# Input Data to BigMat
# -----------------------------------------------------------
def s01_ext2mat():
    nodeName = "s01_ext2mat"
    nodeDir=gParams.pipeline_rundir+"/"+nodeName
    nodeScriptDir=nodeDir+"-script"

    subnode_picke_file = "{0}/{1}.pickle".format(nodeDir,nodeName)
    if gParams.dry_run:
        return subnode_picke_file

    if gParams.remove_before_run and os.path.exists(nodeDir):
        bdvd_logp("remove existing dir: {0}".format(nodeDir))
        shutil.rmtree(nodeDir)

    if not os.path.exists(nodeScriptDir):
        os.mkdir(nodeScriptDir)

    #
    # prepare design file
    #
    ColCnt = gParams.col_cnt
    RowCnt = gParams.row_cnt
    DataFile = gParams.data_file
    FieldSep = " "
    SampleNames=["V{0}".format(v) for v in range(1,ColCnt+1)]
    CalcStatistics = False
    design_params=(SampleNames,ColCnt,RowCnt,DataFile,FieldSep,CalcStatistics)
    params_pickle_fn="{0}/design_params.pickle".format(nodeScriptDir)
    iBSDefines.dumpPickle(design_params, params_pickle_fn)

    design_file=BDT_HomeDir+"/iBS/iBSPy/PipelineDesigns/bdvdTxt2MatDesign.py"
    shutil.copy(design_file,nodeScriptDir)

    #
    # Run node
    #
    design_fn=os.path.abspath(nodeScriptDir)+"/bdvdTxt2MatDesign.py"
    subnode=nodeName
    cmdpath="{0}/bdvd-txt2mat.py".format(BDT_HomeDir)
    node_cmd = [cmdpath,
                "--node",nodeName,
                "--num-threads", "4",
                "--output-dir",nodeDir,
                design_fn]
      
    shell_cmd=""
    for strCmd in node_cmd:
        shell_cmd=shell_cmd+strCmd+" "
    #print(shell_cmd)

    bdvd_logp("run subtask at: {0}".format(nodeDir))
    proc = subprocess.call(node_cmd)
    bdvd_logp("end subtask \n")
    return subnode_picke_file


# -----------------------------------------------------------
# KMeans ++
# -----------------------------------------------------------
def s02_kmeansPP(datamatPickle):
    nodeName = "s02_kmeansPP"
    nodeDir=gParams.pipeline_rundir+"/"+nodeName
    nodeScriptDir=nodeDir+"-script"

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

    design_params=(Ks,Distance,MaxIteration,MinExplainedChanged,FeatureIdxFrom,FeatureIdxTo,KMeansContractorWorkerCnt)
    params_pickle_fn="{0}/design_params.pickle".format(nodeScriptDir)
    iBSDefines.dumpPickle(design_params, params_pickle_fn)

    design_file=BDT_HomeDir+"/iBS/iBSPy/PipelineDesigns/bigclustKMeansPPDesign.py"
    shutil.copy(design_file,nodeScriptDir)

    #
    # Run node
    #
    design_fn=os.path.abspath(nodeScriptDir)+"/bigclustKMeansPPDesign.py"
    subnode=nodeName
    cmdpath="{0}/bigclust-kmeans++.py".format(BDT_HomeDir)
    node_cmd = [cmdpath,
                "--node",nodeName,
                "--datamat", datamatPickle,
                "--output-dir",nodeDir,
                design_fn]
      
    shell_cmd=""
    for str in node_cmd:
        shell_cmd=shell_cmd+str+" "
    #print(shell_cmd)

    bdvd_logp("run subtask at: {0}".format(nodeDir))
    proc = subprocess.call(node_cmd)
    subnode_picke_file="{0}/{1}.pickle".format(nodeDir,nodeName)
    bdvd_logp("end subtask \n")
    return (nodeDir,subnode_picke_file)

def preparePipelineResult(kmeansPPNodeDir):
    outfile="{0}cluster_assignments.bfv".format(output_dir)
    shutil.copy("{0}/gid_10001.bfv".format(kmeansPPNodeDir),
                "{0}cluster_assignments.bfv".format(output_dir))

    bdvd_logp("cluster_assignments: {0}\n".format(outfile))

def main(argv=None):

    # Initialize default parameter values
    global gParams
    gParams = BDVDParams()
    run_argv = sys.argv[:]

    try:
        if argv is None:
            argv = sys.argv
        args = gParams.parse_options(argv)
       
        start_time = datetime.now()

        prepare_output_dir()
        init_logger(logging_dir + "kmeans++.log")

        bdvd_logp()
        bdvd_log("Beginning kmeans++ run (v"+iBSUtil.get_version()+")")
        bdvd_logp("-----------------------------------------------")

        # -----------------------------------------------------------
        # launch FCDCentral
        # -----------------------------------------------------------
       
        if gParams.dry_run and gParams.start_from == '1-input-mat':
            gParams.dry_run = False

        datamatPickle = s01_ext2mat()

        if gParams.dry_run:
            bdvd_log("retrieve existing result for: 1-input-mat")
            bdvd_log("from: {0}".format(datamatPickle))
            if not os.path.exists(datamatPickle):
                die('file not exist')
            bdvd_log("")
        
        if gParams.dry_run and gParams.start_from == '2-kmeans++':
            gParams.dry_run = False

        (nodeDir,subnode_picke)=s02_kmeansPP(datamatPickle)
        preparePipelineResult(nodeDir)
        # -----------------------------------------------------------
        # launch KMeans Server
        # -----------------------------------------------------------
        

        # -----------------------------------------------------------
        # launch KMeans Contractor
        # -----------------------------------------------------------
        
        # -----------------------------------------------------------
        # Run KMeans
        # -----------------------------------------------------------

        finish_time = datetime.now()
        duration = finish_time - start_time
        bdvd_logp("-----------------------------------------------")
        bdvd_log("Run complete: %s elapsed" %  iBSUtil.formatTD(duration))

    except Usage as err:
        bdvd_logp(sys.argv[0].split("/")[-1] + ": " + str(err.msg))
        bdvd_logp("    for detailed help see url ...")
        return 2
    
    except:
        bdvd_logp(traceback.format_exc())
        die()

if __name__ == "__main__":
    sys.exit(main())
