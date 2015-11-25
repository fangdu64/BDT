#!/dcs01/gcode/fdu1/BDT/python/bin/python3.3

"""
pipeline-bdvd.py

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
BDVD

Usage:
    pipeline-bdvd.py <input-options> <ruv-options> <output-options> <--out out_dir>

<input-options>:
--input-type   <string>    [ bam-files, text-mat, binary-mat ]

for [bam-files] the following options are required:
    --bam-samples-file  <string>
    --chromosomes       <string> e.g., chr1,chr2
    --bin-width         <int>

for [text-mat] the following options are required:
    --text-mat-file     <string>
    --nrow              <int>
    --ncol              <int>

for [binary-mat] the following options are required:
    --binary-mat-file   <string>
    --nrow              <int>
    --ncol              <int>

<pre-normalization>:
--pre-normalization   <string>    [ none, column-sum]
for [common-column-sum]:
    --common-column-sum [int], if not set, use median of column sums

<ruv-options>:
--sample-groups     <string> e.g., [1,2],[3,4],[5,6]
--ruv-scale  <string>    [ none, mlog (moderated log link: log(x+c))] default mlog
--ruv-mlog-c     <double>    [ default 1.0]
--ruv-type   <string>    [ ruvg, ruvs, ruvr ]
for [ruvs] and [ruvg]:
    required options:
    --control-rows-method   <string>    [all,weak-signal,lower-quantile,specified-rows] default all
    for [weak-signal]
    --weak-signal-lb    <double>
    --weak-signal-ub    <double>
    for [lower-quantile]
    --lower-quantile-threshold   <double> default 0.5
    --all-in-quantile-fraction   <double> default 1
    for [specified-rows]
    --control-rows-file  <string> a text file with each row as control row index (0-based)
    --ruv-row-adjust    <string>    [none, unitary-length]

optional options:
   --known-factors     <string> e.g., [1,1,1,1,0,0], default is none


other options:
--thread-num        <int> default 4
--memory-size      <int> MB default 2000
--start-from    <string> [1-input-mat (default), 2-perform-ruv, 3-perform-vd, 4-output]

'''

class Usage(Exception):
    def __init__(self, msg):
        self.msg = msg

output_dir = "~/bdvd/"
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
        self.pipeline_rundir = None
        self.workercnt = 4
        self.memory_size = 2000
        self.input_type = None
        self.sample_names = None
        self.bam_samples_file = None
        self.chromosomes = None
        self.bin_width = 100
        self.txt_mat_file = None
        self.binary_mat_file = None
        self.row_cnt = None
        self.col_cnt = None
        self.pre_normalization = None
        self.common_column_sum = None
        self.sample_groups = None
        self.ruv_scale = None
        self.ruv_mlog_c = 1.0
        self.ruv_type = None
        self.control_rows_method = 'all'
        self.weak_signal_lb = None
        self.weak_signal_ub = None
        self.lower_quantile_threshold = None
        self.all_in_quantile_fraction = 1.0
        self.control_rows_file = None
        self.ruv_rowwise_adjust = None
        self.known_factors = None
        self.start_from = '1-input-mat'
        self.dry_run = True
        self.remove_before_run = True

    def parse_options(self, argv):
        try:
            opts, args = getopt.getopt(argv[1:], "hvp:m:o:",
                                        ["version",
                                         "help",
                                         "input-type=",
                                         "sample-names=",
                                         "bam-samples-file=",
                                         "chromosomes=",
                                         "bin-width=",
                                         "text-mat-file=",
                                         "binary-mat-file=",
                                         "ncol=",
                                         "nrow=",
                                         "thread-num=",
                                         "memory-size=",
                                         "out=",
                                         "pre-normalization=",
                                         "common-column-sum=",
                                         "sample-groups=",
                                         "ruv-scale=",
                                         "ruv-mlog-c=",
                                         "ruv-type=",
                                         "control-rows-method=",
                                         "weak-signal-lb=",
                                         "weak-signal-ub=",
                                         "lower-quantile-threshold=",
                                         "all-in-quantile-fraction=",
                                         "control-rows-file=",
                                         "ruv-rowwise-adjust=",
                                         "known-factors=",
                                         "start-from="
                                         ])
        except getopt.error as msg:
            raise Usage(msg)

        global output_dir
        global logging_dir

        custom_out_dir = None

        # option processing
        for option, value in opts:
            if option in ("-v", "--version"):
                print("BDVD v",iBSUtil.get_version())
                sys.exit(0)
            if option in ("-h", "--help"):
                raise Usage(use_message)
            if option in ("-p", "--thread-num"):
                self.workercnt = int(value)
            if option == "--memory-size":
                self.memory_size = int(value)
            if option in ("-o", "--out"):
                custom_out_dir = value + "/"
            if option == "--input-type":
                self.input_type = value
            if option == "--sample-names":
                self.sample_names=value.split(',')
                if len(self.sample_names)<1:
                    raise Usage("--sample-names invalid")
            if option == "--bam-samples-file":
                self.bam_samples_file = value
            if option == "--chromosomes":
                self.chromosomes=value.split(',')
                if len(self.chromosomes)<1:
                    raise Usage("--chromosomes invalid")
            if option == "--bin-width":
                self.bin_width = int(value)
            if option == "--text-mat-file":
                self.txt_mat_file = value
            if option == "--binary-mat-file":
                self.binary_mat_file = value
            if option in ("--ncol"):
                self.col_cnt = int(value)
            if option in ("--nrow"):
                self.row_cnt = int(value)
            if option == "--pre-normalization":
                self.pre_normalization = value
            if option == "--common-column-sum":
                if value == "median":
                    self.common_column_sum = None
                else:
                    self.common_column_sum = float(value)
            if option =="--sample-groups":
                print('sample-groups = {0}'.format(value))
                self.sample_groups = iBSUtil.parseIntSeqSeq(value)
                if len(self.sample_groups)<1:
                    raise Usage('invalid sample-groups: {0}'.format(value))
            if option == "--ruv-scale":
                self.ruv_scale = value
            if option == "--ruv-mlog-c":
                self.ruv_mlog_c = float(value)
            if option == "--ruv-type":
                self.ruv_type = value
            if option == "--control-rows-method":
                self.control_rows_method = value
            if option == "--weak-signal-lb":
                self.weak_signal_lb = float(value)
            if option == "--weak-signal-ub":
                self.weak_signal_ub = float(value)
            if option == "--lower-quantile-threshold":
                self.lower_quantile_threshold = float(value)
            if option == "--all-in-quantile-fraction":
                self.all_in_quantile_fraction = float(value)
            if option == "--control-rows-file":
                self.control_rows_file = value
            if option == "--ruv-rowwise-adjust":
                self.ruv_rowwise_adjust = value
            if option =="--known-factors":
                if value == 'none':
                    continue
                self.known_factors = iBSUtil.parseIntSeqSeq(value)
                if len(self.known_factors)<1:
                    raise Usage('invalid known-factors: {0}'.format(value))
            if option =="--start-from":
                if value not in ('1-input-mat', '2-perform-ruv', '3-perform-vd', '4-output'):
                    raise Usage('invalid --start-from')
                self.start_from = value

        if custom_out_dir:
            output_dir = custom_out_dir
            logging_dir = output_dir + "logs/"
        self.pipeline_rundir=output_dir+"run"
        
        if (self.input_type is None) or (custom_out_dir is None):
            raise Usage('invalid --input-type')
        
        ## input-type section
        if (self.input_type == "bam-files"):
            if (self.bam_samples_file is None) or (self.bin_width is None):
                raise Usage(use_message)
            if not os.path.exists(self.bam_samples_file):
                raise Usage("--bam-samples-file {0} not exist".format(self.bam_samples_file))
        elif (self.input_type == "text-mat"):
            if (self.txt_mat_file is None) or (self.col_cnt is None) or (self.row_cnt is None):
                raise Usage(use_message)
        elif (self.input_type == "binary-mat"):
            if (self.binary_mat_file is None):
               raise Usage('binary-mat not exist')
            if (self.col_cnt is None) or (self.row_cnt is None):
               raise Usage('need --ncol, --nrow')
        else:
            raise Usage(use_message)

        ## pre normalization
        if (self.pre_normalization not in(None,"none","column-sum")):
            raise Usage("invalid pre-normalization")
        if (self.pre_normalization == "none"):
            self.pre_normalization = None
        
        ## ruv options
        if (self.ruv_scale not in(None,"none","mlog")):
            raise Usage("invalid ruv-scale")
        if (self.ruv_scale == "none"):
            self.ruv_scale = None

        if (self.ruv_type not in("ruvs","ruvg")):
            raise Usage("invalid ruv-type")

        if (self.control_rows_method not in("all","weak-signal","lower-quantile", "specified-rows")):
            raise Usage("invalid control-rows-method")

        if self.control_rows_file is not None and (not os.path.exists(self.control_rows_file)):
                raise Usage("--control-rows-file {0} not exist".format(self.control_rows_file))

        if (self.ruv_rowwise_adjust not in(None,"none","unitary-length")):
            raise Usage("invalid ruv-rowwise-adjust")
        if (self.ruv_rowwise_adjust == "none"):
            self.ruv_rowwise_adjust = None

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
# bam to BigMat
# -----------------------------------------------------------
def s01_bam2mat():
    nodeName = "s01_bam2mat"
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
    chromosomes = gParams.chromosomes
    bin_width = gParams.bin_width
    with open(gParams.bam_samples_file) as f:
        sample_table_lines = f.read().splitlines()
    
    design_params=(sample_table_lines,chromosomes,bin_width)
    params_pickle_fn="{0}/design_params.pickle".format(nodeScriptDir)
    iBSDefines.dumpPickle(design_params, params_pickle_fn)

    design_file=BDT_HomeDir+"/iBS/iBSPy/PipelineDesigns/bdvdBam2MatDesign.py"
    shutil.copy(design_file,nodeScriptDir)

    #
    # Run node
    #
    design_fn=os.path.abspath(nodeScriptDir)+"/bdvdBam2MatDesign.py"
    subnode=nodeName
    cmdpath="{0}/bdvd-bam2mat.py".format(BDT_HomeDir)
    node_cmd = [cmdpath,
                "--node",nodeName,
                "--num-threads", str(gParams.workercnt),
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
# text-mat to BigMat
# -----------------------------------------------------------
def s01_txt2mat():
    nodeName = "s01_txt2mat"
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
    DataFile = gParams.txt_mat_file
    FieldSep = " "
    if gParams.sample_names is None:
        SampleNames=["V{0}".format(v) for v in range(1,ColCnt+1)]
    else:
        SampleNames = gParams.sample_names
    CalcStatistics = True
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
# binary-mat to BigMat
# -----------------------------------------------------------
def s01_binary2mat():
    nodeName = "s01_binary2mat"
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
    StorePathPrefix = gParams.binary_mat_file
    CalculateStatistics = True
    RowCnt = gParams.row_cnt
    ColCnt = gParams.col_cnt
    if gParams.sample_names is None:
        ColNames=["V{0}".format(v) for v in range(1,ColCnt+1)]
    else:
        ColNames = gParams.sample_names
    design_params=(StorePathPrefix,CalculateStatistics,RowCnt,ColCnt,ColNames)
    params_pickle_fn="{0}/design_params.pickle".format(nodeScriptDir)
    iBSDefines.dumpPickle(design_params, params_pickle_fn)

    design_file=BDT_HomeDir+"/iBS/iBSPy/PipelineDesigns/bdvdBFV2MatDesign.py"
    shutil.copy(design_file,nodeScriptDir)

    #
    # Run node
    #
    design_fn=os.path.abspath(nodeScriptDir)+"/bdvdBFV2MatDesign.py"
    subnode=nodeName
    cmdpath="{0}/bdvd-bfv2mat.py".format(BDT_HomeDir)
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

def s01_1_ctrlRowMat():
    nodeName = "s01_1_ctrlRowsMat"
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
    ColCnt = 1
    RowCnt = None
    DataFile = gParams.control_rows_file
    FieldSep = None
    StartingRowIdx = 0 # no heading
    AddValue = -1 # input is 1-based, convert to 0-based
    SampleNames=["V{0}".format(v) for v in range(1,ColCnt+1)]
    CalcStatistics = True
    design_params=(SampleNames,ColCnt,RowCnt,DataFile,FieldSep, StartingRowIdx, AddValue, CalcStatistics)
    params_pickle_fn="{0}/design_params.pickle".format(nodeScriptDir)
    iBSDefines.dumpPickle(design_params, params_pickle_fn)

    design_file=BDT_HomeDir+"/iBS/iBSPy/PipelineDesigns/bdvdIDs2MatDesign.py"
    shutil.copy(design_file,"{0}/bdvdTxt2MatDesign.py".format(nodeScriptDir))

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
# s02 RUV
# -----------------------------------------------------------
def s02_RUV(datamatPickle, ctrlRowPickle):
    nodeName = "s02_ruv"
    nodeDir=gParams.pipeline_rundir+"/"+nodeName
    nodeScriptDir=nodeDir+"-script"

    subnode_picke_file = "{0}/{1}.pickle".format(nodeDir,nodeName)
    if gParams.dry_run:
        return (nodeDir,subnode_picke_file)

    if gParams.remove_before_run and os.path.exists(nodeDir):
        bdvd_logp("remove existing dir: {0}".format(nodeDir))
        shutil.rmtree(nodeDir)

    if not os.path.exists(nodeScriptDir):
        os.mkdir(nodeScriptDir)

    #
    # prepare design file
    #
    sampleGroups = gParams.sample_groups
    KnownFactors = gParams.known_factors
    if gParams.pre_normalization is None:
        CommonLibrarySize = 0
    elif gParams.pre_normalization == "column-sum" and gParams.common_column_sum is None:
        CommonLibrarySize = -1 # use median column-sum
    else:
        CommonLibrarySize = gParams.common_column_sum

    if gParams.ruv_type == "ruvs" and gParams.ruv_rowwise_adjust is None:
        RUVMode = iBS.RUVModeEnum.RUVModeRUVs
    elif gParams.ruv_type == "ruvs" and gParams.ruv_rowwise_adjust == "unitary-length":
        RUVMode = iBS.RUVModeEnum.RUVModeRUVsForVariation
    elif gParams.ruv_type == "ruvg" and gParams.ruv_rowwise_adjust is None:
        RUVMode = iBS.RUVModeEnum.RUVModeRUVg
    elif gParams.ruv_type == "ruvg" and gParams.ruv_rowwise_adjust == "unitary-length":
        RUVMode = iBS.RUVModeEnum.RUVModeRUVgForVariation

    ControlFeaturePolicy = iBS.RUVControlFeaturePolicyEnum.RUVControlFeaturePolicyNone
    ControlFeatureMaxCntLowBound = None
    ControlFeatureMaxCntUpBound = None
    CtrlQuantile = None
    AllInQuantileFraction = None
    if gParams.control_rows_method == "all":
        ControlFeaturePolicy = iBS.RUVControlFeaturePolicyEnum.RUVControlFeaturePolicyNone
    elif gParams.control_rows_method == "weak-signal":
        ControlFeaturePolicy = iBS.RUVControlFeaturePolicyEnum.RUVControlFeaturePolicyMaxCntLow
        ControlFeatureMaxCntLowBound = gParams.weak_signal_lb
        ControlFeatureMaxCntUpBound = gParams.weak_signal_ub
    elif gParams.control_rows_method == "lower-quantile":
        ControlFeaturePolicy = iBS.RUVControlFeaturePolicyEnum.RUVControlFeaturePolicyAllInLowerQuantile
        CtrlQuantile = gParams.lower_quantile_threshold
        AllInQuantileFraction = gParams.all_in_quantile_fraction
    elif gParams.control_rows_method == "specified-rows":
        ControlFeaturePolicy = iBS.RUVControlFeaturePolicyEnum.RUVControlFeaturePolicyFeatureIdxList

    MaxK = len(sampleGroups)
    FeatureIdxFrom = None
    FeatureIdxTo = None

    design_params=(sampleGroups,
                   KnownFactors,
                   CommonLibrarySize,
                   RUVMode,
                   ControlFeaturePolicy,
                   ControlFeatureMaxCntLowBound,
                   ControlFeatureMaxCntUpBound,
                   CtrlQuantile,
                   AllInQuantileFraction,
                   MaxK,
                   FeatureIdxFrom,
                   FeatureIdxTo)
    params_pickle_fn="{0}/design_params.pickle".format(nodeScriptDir)
    iBSDefines.dumpPickle(design_params, params_pickle_fn)

    design_file=BDT_HomeDir+"/iBS/iBSPy/PipelineDesigns/bdvdRUVDesign.py"
    shutil.copy(design_file,nodeScriptDir)

    #
    # Run node
    #
    design_fn=os.path.abspath(nodeScriptDir)+"/bdvdRUVDesign.py"
    subnode=nodeName
    cmdpath="{0}/bdvd-ruv.py".format(BDT_HomeDir)
    node_cmd = [cmdpath,
                "--node",nodeName,
                "--num-threads", str(gParams.workercnt),
                "--max-mem", str(gParams.memory_size),
                "--output-dir",nodeDir,
                "--bigmat",datamatPickle]
    
    if ctrlRowPickle is not None:
        node_cmd.extend(["--ctrl-rows", ctrlRowPickle])
    
    node_cmd.append(design_fn)

    shell_cmd=""
    for strCmd in node_cmd:
        shell_cmd=shell_cmd+strCmd+" "
    #print(shell_cmd)

    bdvd_logp("run subtask at: {0}".format(nodeDir))
    proc = subprocess.call(node_cmd)
    bdvd_logp("end subtask \n")
    return (nodeDir,subnode_picke_file)

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
        init_logger(logging_dir + "bdvd.log")

        bdvd_logp()
        bdvd_log("Beginning BDVD run (v"+iBSUtil.get_version()+")")
        bdvd_logp("-----------------------------------------------")

        # -----------------------------------------------------------
        # prepare bigmat
        # -----------------------------------------------------------
        if gParams.dry_run and gParams.start_from == '1-input-mat':
            gParams.dry_run = False

        datamatPickle = None
        if (gParams.input_type == "bam-files"):
            datamatPickle = s01_bam2mat()
        elif (gParams.input_type == "text-mat"):
            datamatPickle = s01_ext2mat()
        elif (gParams.input_type == "binary-mat"):
            datamatPickle = s01_binary2mat()

        if gParams.dry_run:
            bdvd_log("retrieve existing result for: 1-input-mat")
            bdvd_log("from: {0}".format(datamatPickle))
            if not os.path.exists(datamatPickle):
                die('file not exist')
            bdvd_log("")
        
        ctrlRowPickle = None
        if (gParams.control_rows_method == "specified-rows"):
            ctrlRowPickle = s01_1_ctrlRowMat()

        # -----------------------------------------------------------
        # run RUV
        # -----------------------------------------------------------
        if gParams.dry_run and gParams.start_from == '2-perform-ruv':
            gParams.dry_run = False

        s02_RUV(datamatPickle, ctrlRowPickle)

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
