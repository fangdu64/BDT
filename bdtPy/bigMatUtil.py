import sys
import os
import getopt
import socket
import logging
import subprocess
import shutil
import time
from datetime import datetime, date
import iBSDefines
import iBS
import bdtUtil

def exportMatByRange(bdt_log,fcdcPrx, computePrx, task):
    (rt,amdTaskID)=computePrx.ExportRowMatrix(task)
    preMsg=""
    amdTaskFinished=False
    while (not amdTaskFinished):
        (rt,amdTaskInfo)=fcdcPrx.GetAMDTaskInfo(amdTaskID)
        thisMsg="task: {0}, batch processed: {1}/{2}".format(amdTaskInfo.TaskName, amdTaskInfo.FinishedCnt, amdTaskInfo.TotalCnt)
        if preMsg!=thisMsg:
            preMsg = thisMsg
            bdt_log(thisMsg)
        if amdTaskInfo.Status!=iBS.AMDTaskStatusEnum.AMDTaskStatusNormal:
            amdTaskFinished = True;
        else:
            time.sleep(1)

def waitForMatrixReadable(fcdcPrx, gid):
    (rt,values) = fcdcPrx.GetDoublesRowMatrix(gid,0,1)
    return rt

def waitForMatricesReadable(fcdcPrx, gids):
    for gid in gids:
        (rt,values) = fcdcPrx.GetDoublesRowMatrix(gid,0,1)
    return rt

def waitForVectorReadable(fcdcPrx, oid):
    (rt,values) = fcdcPrx.GetDoublesColumnVector(oid,0,1)
    return rt

def waitForVectorsReadable(fcdcPrx, oids):
    for oid in oids:
        (rt,values) = fcdcPrx.GetDoublesColumnVector(oid,0,1)
    return rt

# bigMatRunner
class bigMatRunner:
    def __init__(self, bdtHomeDir, runBin = 'bigMat'):
        self.run_bin = runBin
        self.bdt_home_dir = bdtHomeDir
        self.output_dir = None
        self.logging_dir = None
        self.bigmat_popen = None
        self.bigmat_log_file = None
        self.bdvd_logger = None # main logging object
        self.bdvd_log_handle = None #main log file handle
        self.bigmat_dir = None
        self.script_dir = None

    # Ensures that the output, logging, and temp directories are present. If not,
    # they are created
    def prepare_dirs(self, outDir, bigMatDir):
        self.output_dir = outDir
        self.logging_dir = os.path.abspath(self.output_dir + "/logs")
        self.bigmat_dir = bigMatDir
        self.script_dir = os.path.abspath(self.output_dir + "/script")

        if not os.path.exists(self.output_dir):
            os.mkdir(self.output_dir)

        if not os.path.exists(self.logging_dir):
            os.mkdir(self.logging_dir)
    
        if not os.path.exists(self.script_dir):
            os.mkdir(self.script_dir)

        if not os.path.exists(self.bigmat_dir):
            os.mkdir(self.bigmat_dir)

        db_dir=os.path.abspath(self.bigmat_dir+"/FCDCentralDB")
        if not os.path.exists(db_dir):
            os.mkdir(db_dir)

        fvstore_dir=os.path.abspath(self.bigmat_dir+"/FeatureValueStore")
        if not os.path.exists(fvstore_dir):
            os.mkdir(fvstore_dir)

    def init_logger(self, log_name):
        log_fname = os.path.abspath(self.logging_dir + "/" + log_name)
        self.bdvd_logger = logging.getLogger('project')
        formatter = logging.Formatter('%(asctime)s %(message)s', '[%Y-%m-%d %H:%M:%S]')
        self.bdvd_logger.setLevel(logging.DEBUG)

        # output logging information to stderr
        hstream = logging.StreamHandler(sys.stderr)
        hstream.setFormatter(formatter)
        self.bdvd_logger.addHandler(hstream)
    
        #
        # Output logging information to file
        if os.path.isfile(log_fname):
            os.remove(log_fname)
        logfh = logging.FileHandler(log_fname)
        logfh.setFormatter(formatter)
        self.bdvd_logger.addHandler(logfh)
        self.bdvd_log_handle=logfh.stream
    
    def log(self, out_str):
      if self.bdvd_logger:
           self.bdvd_logger.info(out_str)

    # error msg
    def logp(self, out_str=""):
        print(out_str,file=sys.stderr)
        if self.bdvd_log_handle:
            print(out_str, file=self.bdvd_log_handle)

    def prepare_bigmat_config(self, tcpPort,fvWorkerSize, iceThreadPoolSize ):
        infile = open("{0}/bdt/config/FCDCentralServer.config".format(self.bdt_home_dir))
        outfile = open(self.bigmat_dir+"/FCDCentralServer.config", "w")

        replacements = {"__FCDCentral_TCP_PORT__":str(tcpPort), 
                        "__FeatureValueWorker.Size__":str(fvWorkerSize), 
                        "__Ice.ThreadPool.Server.Size__":str(iceThreadPoolSize)}

        for line in infile:
            for src, target in replacements.items():
                line = line.replace(src, target)
            outfile.write(line)
        infile.close()
        outfile.close()
    
    def launch_bigMat(self):
        bigmat_path = os.path.abspath("{0}/bdt/bin/{1}".format(self.bdt_home_dir, self.run_bin))
        bm_cmd = [bigmat_path]
        self.log("Launching bigMat ...")
        self.bigmat_log_file = open(os.path.abspath(self.logging_dir + "/bigmat.log"),"w")
        self.bigmat_popen = subprocess.Popen(bm_cmd, cwd=self.bigmat_dir, stdout=self.bigmat_log_file)
    
    def shutdown_bigMat(self):
        if self.bigmat_popen is not None:
            self.bigmat_popen.terminate()
            self.bigmat_popen.wait()
            self.bigmat_popen = None
            self.log("bigMat shutdown")
        if self.bigmat_log_file is not None:
            self.bigmat_log_file.close()

    def die(self, msg=None):
        if msg is not None:
            self.logp(msg)
        self.shutdown_bigMat()
        sys.exit(1)

bigmat_input_types = [
    'text-mat',
    'binary-mat',
    'text-rowids',
    'bams',
    'kmeans-seeds-mat',
    'kmeans-centroids-mat',
    'kmeans-data-mat']

bigmat_use_message = '''
bigMat

Usage:
    bigMat [options] <--data data_file> <--nrow row_cnt> <--ncol col_cnt> <--k cluster_num> <--out out_dir>

Advanced Options:

'''

bigmat_steps = ['1-input-mat', 'end']

class BigMatParams:
    def __init__(self):
        self.argv = None
        self.output_dir = None
        self.logging_dir = None
        self.start_from = bigmat_steps[0]
        self.dry_run = True
        self.remove_before_run = True
        self.pipeline_rundir = None
        self.input_type = None
        self.input_location = None
        self.row_cnt = None
        self.col_cnt = None
        self.chromosomes = None
        self.chromosomes_original = None
        self.bin_width = None
        self.thread_cnt = 4
        self.memory_size = 2000
        self.calc_statistics = False
        self.column_sep = None
        self.skip_cols = None
        self.skip_rows = None
        self.col_names = None
        self.col_names_original = None
        self.row_index_base = 0

    def strip_argv(self, prefix, argv):
        prefixTag = '--{0}'.format(prefix)
        self.argv = []
        left_overs =[True]*len(argv)
        for i in range(len(argv)):
            arg = argv[i]
            if len(arg) > len(prefixTag) and arg[:len(prefixTag)]== prefixTag:
                if self.is_unary_option(prefix, arg):
                    self.argv.append(arg)
                    left_overs[i] = False
                else:
                    self.argv.extend([arg, argv[i+1]])
                    left_overs[i] = False
                    left_overs[i+1] = False
        args = []
        for i in range(len(argv)):
            if left_overs[i]:
                args.append(argv[i])
        return args

    def is_unary_option(self, prefix, arg):
        unaryOptions = [
            "--{0}version".format(prefix),
            "--{0}help".format(prefix),
            "--{0}calc-statistics".format(prefix)]
        return arg in unaryOptions

    def parse_options(self, prefix, argvs):
        try:
            opts, args = getopt.getopt(argvs, "",
                ["{0}version".format(prefix),
                "{0}help".format(prefix),
                "{0}input=".format(prefix),
                "{0}out=".format(prefix),
                "{0}ncol=".format(prefix),
                "{0}nrow=".format(prefix),
                "{0}chromosomes=".format(prefix),
                "{0}bin-width=".format(prefix),
                "{0}thread-num=".format(prefix),
                "{0}memory-size=".format(prefix),
                "{0}calc-statistics".format(prefix),
                "{0}col-sep=".format(prefix),
                "{0}col-names=".format(prefix),
                "{0}index-base=".format(prefix),
                "{0}skip-cols=".format(prefix),
                "{0}skip-rows=".format(prefix)])

        except getopt.error as msg:
            raise iBSDefines.BdtUsage(msg)

        # option processing
        for option, value in opts:
            if option == "--{0}version".format(prefix):
                print("Big Data Tools - bigMat v",bdtUtil.get_version())
                sys.exit(0)
            if option == "--{0}help".format(prefix):
                raise iBSDefines.BdtUsage(bigmat_use_message)
            if option == "--{0}out".format(prefix):
                self.output_dir = value
            if option == "--{0}start-from".format(prefix):
                allowedValues = bigmat_steps;
                if value not in allowedValues:
                    raise iBSDefines.BdtUsage('--start-from should be one of the {0}'.format(allowedValues))
                self.start_from = value
            if option =="--{0}input".format(prefix):
                allowedValues = bigmat_input_types
                (self.input_type, self.input_location) = bdtUtil.parseMatInput(value)
                if self.input_type is None or self.input_location is None:
                    raise iBSDefines.BdtUsage('--input should be of the format: type@location')
                if self.input_type not in allowedValues:
                    raise iBSDefines.BdtUsage('--input type@location, type should be one of the {0}'.format(allowedValues))
                self.input_location = os.path.abspath(self.input_location)
            if option == "--{0}ncol".format(prefix):
                self.col_cnt = int(value)
            if option == "--{0}nrow".format(prefix):
                self.row_cnt = int(value)
            if option == "--{0}chromosomes".format(prefix):
                self.chromosomes_original = value
                self.chromosomes=value.split(',')
                if len(self.chromosomes)<1:
                    raise iBSDefines.BdtUsage("--{0}chromosomes invalid".format(prefix))
            if option == "--{0}bin-width".format(prefix):
                self.bin_width = int(value)
            if option == "--{0}thread-num".format(prefix):
                self.thread_cnt = int(value)
            if option == "--{0}memory-size".format(prefix):
                self.memory_size = int(value)
            if option == "--{0}calc-statistics".format(prefix):
                self.calc_statistics = True
            if option == "--{0}col-sep".format(prefix):
                if value.startswith('"') and value.endswith('"'):
                    value = value[1:-1]
                self.column_sep = value
            if option == "--{0}col-names".format(prefix):
                self.col_names_original = value
                self.col_names=value.split(',')
                if len(self.col_names)<1:
                    raise iBSDefines.BdtUsage("--{0}col-names invalid".format(prefix))
            if option == "--{0}index-base".format(prefix):
                self.row_index_base = int(value)
            if option == "--{0}skip-cols".format(prefix):
                self.skip_cols = int(value)
            if option == "--{0}skip-rows".format(prefix):
                self.skip_rows = int(value)

        if self.input_type in  ['text-mat', 'binary-mat']:
            requiredNames = [
                '--{0}input'.format(prefix),
                '--{0}out'.format(prefix),
                '--{0}ncol'.format(prefix),
                '--{0}nrow'.format(prefix)]
            providedValues = [self.input_type, self.output_dir, self.col_cnt, self.row_cnt]
            providedFiles = [self.input_location]
        elif self.input_type in ['kmeans-seeds-mat', 'kmeans-centroids-mat', 'kmeans-data-mat', 'text-rowids']:
            requiredNames = [
                '--{0}input',
                '--{0}out']
            providedValues = [self.input_type, self.output_dir]
            providedFiles = [self.input_location]
        elif self.input_type in ['bams']:
            requiredNames = [
                '--{0}input'.format(prefix),
                '--{0}out'.format(prefix),
                '--{0}chromosomes'.format(prefix),
                '--{0}bin-width'.format(prefix)]
            providedValues = [self.input_type, self.output_dir, self.chromosomes, self.bin_width]
            providedFiles = [self.input_location]
        else:
            allowedValues = bigmat_input_types
            raise iBSDefines.BdtUsage('--input type@location, type should be one of the {0}'.format(allowedValues))

        noneIdx = bdtUtil.getFirstNone(providedValues)
        if noneIdx != -1:
            raise iBSDefines.BdtUsage("{0} is required".format(requiredNames[noneIdx]))

        noneIdx = bdtUtil.getFirstNotExistFile(providedFiles)
        if noneIdx != -1:
            raise iBSDefines.BdtUsage("{0} not exist".format(providedFiles[noneIdx]))

        self.output_dir = os.path.abspath(self.output_dir)
        self.logging_dir = os.path.abspath(self.output_dir + "/logs")
        self.pipeline_rundir=os.path.abspath(self.output_dir + "/run")
        return args

    def get_cmds(self):
        cmds = []
        if self.output_dir is not None:
            cmds.extend(['--out', self.output_dir])
        if self.start_from != bigmat_steps[0]:
            cmds.extend(['--start-from', self.start_from])
        if self.input_type is not None:
            cmds.extend(['--input', '{0}@{1}'.format(self.input_type, self.input_location)])
        if self.col_cnt is not None:
            cmds.extend(['--ncol', str(self.col_cnt)])
        if self.row_cnt is not None:
            cmds.extend(['--nrow', str(self.row_cnt)])
        if self.chromosomes is not None:
            cmds.extend(['--chromosomes', self.chromosomes_original])
        if self.bin_width is not None:
            cmds.extend(['--bin-width', str(self.bin_width)])
        if self.thread_cnt != 4:
            cmds.extend(['--thread-num', str(self.thread_cnt)])
        if self.memory_size != 2000:
            cmds.extend(['--memory-size', str(self.memory_size)])
        if self.calc_statistics is True:
            cmds.extend(['--calc-statistics'])
        if self.column_sep is not None:
            cmds.extend(['--col-sep', self.column_sep])
        if self.col_names is not None:
            cmds.extend(['--col-names', self.col_names_original])
        if self.row_index_base != 0:
            cmds.extend(['--index-base', str(self.row_index_base)])
        if self.skip_cols is not None:
            cmds.extend(['--skip-cols', str(self.skip_cols)])
        if self.skip_rows is not None:
            cmds.extend(['--skip-rows', str(self.skip_rows)])
        return cmds

def run_txt2Mat(
    gRunner,
    platform,
    bdtHomeDir,
    nodeName,
    runDir,
    dryRun,
    removeBeforeRun,
    calculateStatistics,
    col_cnt,
    row_cnt,
    data_file,
    col_names,
    field_sep,
    skipCols,
    skipRows):
    
    nodeDir = os.path.abspath("{0}/{1}".format(runDir, nodeName))
    nodeScriptDir = nodeDir + "-script"

    out_picke_file = os.path.abspath("{0}/{1}.pickle".format(nodeDir,nodeName))
    if dryRun:
        return out_picke_file

    if removeBeforeRun and os.path.exists(nodeDir):
        gRunner.logp("remove existing dir: {0}".format(nodeDir))   
        shutil.rmtree(nodeDir)

    if not os.path.exists(nodeScriptDir):
        os.mkdir(nodeScriptDir)

    #
    # prepare design file
    #
    ColCnt = col_cnt
    RowCnt = row_cnt
    DataFile = data_file
    FieldSep = " "
    if field_sep is not None:
        FieldSep = field_sep

    SampleNames=["V{0}".format(v) for v in range(1,ColCnt+1)]
    if col_names is not None:
        SampleNames = col_names

    CalcStatistics = calculateStatistics
    StartingRowIdx = 0
    if skipRows is not None:
        StartingRowIdx = skipRows
    StartingColIdx = 0
    if skipCols is not None:
        StartingColIdx = skipCols

    design_params=(
        SampleNames,
        ColCnt,
        RowCnt,
        DataFile,
        FieldSep,
        CalcStatistics,
        StartingRowIdx,
        StartingColIdx)
    params_pickle_fn=os.path.abspath("{0}/design_params.pickle".format(nodeScriptDir))
    iBSDefines.dumpPickle(design_params, params_pickle_fn)

    design_file=os.path.abspath(bdtHomeDir+"/bdt/bdtPy/PipelineDesigns/txt2MatDesign.py")
    shutil.copy(design_file,nodeScriptDir)

    #
    # Run node
    #
    design_fn=os.path.abspath(nodeScriptDir)+"/txt2MatDesign.py"
    subnode=nodeName
    cmdpath=os.path.abspath("{0}/bdt/bdtCmds/bigmat-txt2mat".format(bdtHomeDir))
    if platform == "Windows":
        node_cmd = ["py", cmdpath]
    else:
        node_cmd = [cmdpath]
    node_cmd.extend(["--node", nodeName,
                "--num-threads", "4",
                "--out",nodeDir,
                design_fn])
      
    shell_cmd=""
    for strCmd in node_cmd:
        shell_cmd=shell_cmd+strCmd+" "

    gRunner.logp("run subtask: {0}\n".format(nodeName))
    proc = subprocess.call(node_cmd)
    gRunner.logp("end subtask: {0}\n".format(nodeName))
    return out_picke_file

def run_txtRowIds2Mat(
    gRunner,
    platform,
    bdtHomeDir,
    nodeName,
    runDir,
    dryRun,
    removeBeforeRun,
    calculateStatistics,
    data_file,
    index_base,
    col_names,
    field_sep,
    skipCols,
    skipRows):
    
    nodeDir = os.path.abspath("{0}/{1}".format(runDir, nodeName))
    nodeScriptDir = nodeDir + "-script"

    out_picke_file = os.path.abspath("{0}/{1}.pickle".format(nodeDir,nodeName))
    if dryRun:
        return out_picke_file

    if removeBeforeRun and os.path.exists(nodeDir):
        gRunner.logp("remove existing dir: {0}".format(nodeDir))   
        shutil.rmtree(nodeDir)

    if not os.path.exists(nodeScriptDir):
        os.mkdir(nodeScriptDir)

    #
    # prepare design file
    #
    ColCnt = 1
    RowCnt = None
    DataFile = data_file
    FieldSep = " "
    if field_sep is not None:
        FieldSep = field_sep
    StartingRowIdx = 0
    if skipRows is not None:
        StartingRowIdx = skipRows
    StartingColIdx = 0
    if skipCols is not None:
        StartingColIdx = skipCols
    AddValue = -index_base
    SampleNames=["V{0}".format(v) for v in range(1,ColCnt+1)]
    if col_names is not None:
        SampleNames = col_names
    CalcStatistics = calculateStatistics
    design_params=(
        SampleNames,
        ColCnt,
        RowCnt,
        DataFile,
        FieldSep,
        StartingRowIdx,
        StartingColIdx,
        AddValue,
        CalcStatistics)
    params_pickle_fn=os.path.abspath("{0}/design_params.pickle".format(nodeScriptDir))
    iBSDefines.dumpPickle(design_params, params_pickle_fn)

    design_file=os.path.abspath(bdtHomeDir+"/bdt/bdtPy/PipelineDesigns/ids2MatDesign.py")
    shutil.copy(design_file,"{0}/txt2MatDesign.py".format(nodeScriptDir))

    #
    # Run node
    #
    design_fn=os.path.abspath(nodeScriptDir)+"/txt2MatDesign.py"
    subnode=nodeName
    cmdpath=os.path.abspath("{0}/bdt/bdtCmds/bigmat-txt2mat".format(bdtHomeDir))
    if platform == "Windows":
        node_cmd = ["py", cmdpath]
    else:
        node_cmd = [cmdpath]
    node_cmd.extend(["--node", nodeName,
                "--num-threads", "4",
                "--out",nodeDir,
                design_fn])

    gRunner.logp("run subtask: {0}\n".format(nodeName))
    proc = subprocess.call(node_cmd)
    gRunner.logp("end subtask: {0}\n".format(nodeName))
    return out_picke_file

def run_bfv2Mat(
    gRunner,
    platform,
    bdtHomeDir,
    nodeName,
    runDir,
    dryRun,
    removeBeforeRun,
    calculateStatistics,
    col_cnt,
    row_cnt,
    data_file,
    col_names):
    
    nodeDir = os.path.abspath("{0}/{1}".format(runDir, nodeName))
    nodeScriptDir = nodeDir + "-script"

    out_picke_file = os.path.abspath("{0}/{1}.pickle".format(nodeDir,nodeName))
    if dryRun:
        return out_picke_file

    if removeBeforeRun and os.path.exists(nodeDir):
        gRunner.logp("remove existing dir: {0}".format(nodeDir))   
        shutil.rmtree(nodeDir)

    if not os.path.exists(nodeScriptDir):
        os.mkdir(nodeScriptDir)

    #
    # prepare design file
    #
    ColCnt = col_cnt
    RowCnt = row_cnt
    StorePathPrefix = data_file

    ColNames=["V{0}".format(v) for v in range(1,ColCnt+1)]
    if col_names is not None:
        ColNames = col_names

    CalculateStatistics = calculateStatistics
    design_params=(StorePathPrefix,CalculateStatistics,RowCnt,ColCnt,ColNames)
    params_pickle_fn=os.path.abspath("{0}/design_params.pickle".format(nodeScriptDir))
    iBSDefines.dumpPickle(design_params, params_pickle_fn)

    design_file=os.path.abspath(bdtHomeDir+"/bdt/bdtPy/PipelineDesigns/bfv2MatDesign.py")
    shutil.copy(design_file,nodeScriptDir)

    #
    # Run node
    #
    design_fn=os.path.abspath(nodeScriptDir)+"/bfv2MatDesign.py"
    cmdpath=os.path.abspath("{0}/bdt/bdtCmds/bigmat-bfv2mat".format(bdtHomeDir))
    if platform == "Windows":
        node_cmd = ["py", cmdpath]
    else:
        node_cmd = [cmdpath]
    node_cmd.extend(["--node", nodeName,
                "--num-threads", "4",
                "--out",nodeDir,
                design_fn])
      
    shell_cmd=""
    for strCmd in node_cmd:
        shell_cmd=shell_cmd+strCmd+" "

    gRunner.logp("run subtask: {0}\n".format(nodeName))
    proc = subprocess.call(node_cmd)
    gRunner.logp("end subtask: {0}\n".format(nodeName))
    return out_picke_file

def run_bam2Mat(
    gRunner,
    platform,
    bdtHomeDir,
    nodeName,
    runDir,
    dryRun,
    removeBeforeRun,
    calculateStatistics,
    col_names,
    num_threads,
    chromosomes,
    bin_width,
    bam_samples_file):
    
    nodeDir = os.path.abspath("{0}/{1}".format(runDir, nodeName))
    nodeScriptDir = nodeDir + "-script"

    out_picke_file = os.path.abspath("{0}/{1}.pickle".format(nodeDir,nodeName))
    if dryRun:
        return out_picke_file

    if removeBeforeRun and os.path.exists(nodeDir):
        gRunner.logp("remove existing dir: {0}".format(nodeDir))   
        shutil.rmtree(nodeDir)

    if not os.path.exists(nodeScriptDir):
        os.mkdir(nodeScriptDir)

    #
    # prepare design file
    #
    with open(bam_samples_file) as f:
        sample_table_lines = f.read().splitlines()

    design_params=(sample_table_lines,chromosomes,bin_width)
    params_pickle_fn="{0}/design_params.pickle".format(nodeScriptDir)
    iBSDefines.dumpPickle(design_params, params_pickle_fn)

    design_file=os.path.abspath(bdtHomeDir+"/bdt/bdtPy/PipelineDesigns/bam2MatDesign.py")
    shutil.copy(design_file, nodeScriptDir)

    #
    # Run node
    #
    design_fn=os.path.abspath(nodeScriptDir)+"/bam2MatDesign.py"
    cmdpath=os.path.abspath("{0}/bdt/bdtCmds/bigmat-bam2mat".format(bdtHomeDir))
    if platform == "Windows":
        node_cmd = ["py", cmdpath]
    else:
        node_cmd = [cmdpath]
    node_cmd.extend(["--node", nodeName,
                "--num-threads", str(num_threads),
                "--out",nodeDir,
                design_fn])
      
    shell_cmd=""
    for strCmd in node_cmd:
        shell_cmd=shell_cmd+strCmd+" "

    gRunner.logp("run subtask: {0}\n".format(nodeName))
    proc = subprocess.call(node_cmd)
    gRunner.logp("end subtask: {0}\n".format(nodeName))
    return out_picke_file


def run_bigMat(
    gRunner,
    platform,
    bdtHomeDir,
    nodeName,
    runDir,
    dryRun,
    removeBeforeRun,
    bigMatParams):
    
    nodeDir = os.path.abspath("{0}/{1}".format(runDir, nodeName))

    out_picke_file = os.path.abspath("{0}/run/1-input-mat/1-input-mat.pickle".format(nodeDir))
    if dryRun:
        return out_picke_file

    if removeBeforeRun and os.path.exists(nodeDir):
        gRunner.logp("remove existing dir: {0}".format(nodeDir))
        shutil.rmtree(nodeDir)
    #
    # Run node
    #
    cmdpath=os.path.abspath("{0}/bigMat".format(bdtHomeDir))
    if platform == "Windows":
        node_cmd = ["py", cmdpath]
    else:
        node_cmd = [cmdpath]
    node_cmd.extend(bigMatParams.get_cmds())
      
    shell_cmd=""
    for strCmd in node_cmd:
        shell_cmd=shell_cmd+strCmd+" "

    gRunner.logp("run subtask: {0}\n".format(nodeName))
    proc = subprocess.call(node_cmd)
    gRunner.logp("end subtask: {0}\n".format(nodeName))
    return out_picke_file