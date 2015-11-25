#!/dcs01/gcode/fdu1/BDT/python/bin/python3.3

"""
bsmooth-align-par.py

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

use_message = '''
Usage:
    tm-knownamrs-qsubs [options] -e <ev_filtered>

Required arguments:
    <ev_filtered> Paths to a directory containing filtered read-level measurements

Options:
    -r/--refs              <string>    [ default: search from ev_bt2_sorted dir  ]
    -o/--output-dir        <string>    [ default: ./tm_knownamrs_out             ]
'''

class Usage(Exception):
    def __init__(self, msg):
        self.msg = msg

output_dir = "/dcs01/gcode/fdu1/data/ENCODE/WGBS/HudsonAlpha_GM12878_MethylWgbs/bsmooth_align_out_2s_fr/merge/tm_knownamrs_runs/tm_knownamrs_model4_out1/"
logging_dir = output_dir + "logs/"
script_dir = output_dir + "script/"
tmp_dir = output_dir + "tmp/"
ibs_log_handle = None #main log file handle
ibs_logger = None # main logging object

gParams=None
gRunInfo=None

def init_logger(log_fname):
    global ibs_logger
    ibs_logger = logging.getLogger('project')
    formatter = logging.Formatter('%(asctime)s %(message)s', '[%Y-%m-%d %H:%M:%S]')
    ibs_logger.setLevel(logging.DEBUG)

    # output logging information to stderr
    hstream = logging.StreamHandler(sys.stderr)
    hstream.setFormatter(formatter)
    ibs_logger.addHandler(hstream)
    
    #
    # Output logging information to file
    if os.path.isfile(log_fname):
        os.remove(log_fname)
    global ibs_log_handle
    logfh = logging.FileHandler(log_fname)
    logfh.setFormatter(formatter)
    ibs_logger.addHandler(logfh)
    ibs_log_handle=logfh.stream

class RunInfo:

    def __init__(self):
        self.processed_read_cnt = 0
        

class TmKnownAMRsParams:

    def __init__(self):
        self.R_bin = "/home/student/fdu1/apps/install/R/R-3.1/bin"
        self.annotation_data_dir = "/dcs01/gcode/fdu1/data/ENCODE/WGBS/Annotation/Imprinting"
        #chr1,chr10,chr11,chr13,chr14,chr15,chr16,chr18,chr19,chr2,chr20,chr4,chr6,chr7,chr8
        self.refs = None
        self.ev_dir = "/home/student/fdu1/mygcode/data/ENCODE/WGBS/HudsonAlpha_GM12878_MethylWgbs/bsmooth_align_out_2s_fr/merge/ev_filtered/"
        self.betabin_outter_winsize = 5000
        self.betabin_inner_winsize = 1000
        self.betabinfit_evtbl_min_rowcnt = 50
        self.tmfit_basestatistics_file = "/dcs01/gcode/fdu1/data/ENCODE/WGBS/HudsonAlpha_GM12878_MethylWgbs/bsmooth_align_out_2s_fr/merge/baseErrorOut/base_statistics.txt"
        self.tmfit_outter_winsize = 500
        self.tmfit_sliding_cg_sitecnt = 1
        self.tmfit_amr_marginfactor=10
        self.tmfit_unique_read_mincnt=10
        self.tmfit_shrinkage_method = 0
        self.tmfit_model = 4

    def parse_options(self, argv):
        try:
            opts, args = getopt.getopt(argv[1:], "ho:e:r:",
                                        ["help",
                                         "output-dir=",
                                         "num-splits=",
                                         "tmp-dir=",
                                         "ev-dir="])
        except getopt.error as msg:
            raise Usage(msg)

        global output_dir
        global logging_dir
        global tmp_dir
        global script_dir

        custom_tmp_dir = None
        custom_out_dir = None

        # option processing
        for option, value in opts:
            if option in ("-h", "--help"):
                raise Usage(use_message)
            if option in ("-o", "--output-dir"):
                custom_out_dir = value + "/"
            if option == "--tmp-dir":
                custom_tmp_dir = value + "/"
            if option in ("-e", "--ev-dir"):
                self.ev_dir = value + "/"
            if option in ("-r", "--refs"):
                self.refs=value.split(',')
                if len(self.refs)<1:
                    raise Usage(use_message)

        if custom_out_dir:
            output_dir = custom_out_dir
            logging_dir = output_dir + "logs/"
            tmp_dir = output_dir + "tmp/"
            script_dir = output_dir + "script/"
        if custom_tmp_dir:
            tmp_dir = custom_tmp_dir

        return args

# The BDVD logging formatter
def ibs_log(out_str):
  if ibs_logger:
       ibs_logger.info(out_str)

# error msg
def ibs_logp(out_str=""):
    print(out_str,file=sys.stderr)
    if ibs_log_handle:
        print(out_str, file=ibs_log_handle)

def die(msg=None):
    global fcdc_popen
    if msg is not None:
        ibs_logp(msg)
    sys.exit(1)

# Ensures that the output, logging, and temp directories are present. If not,
# they are created
def prepare_output_dir():

    ibs_log("Preparing output location "+output_dir)

    if not os.path.exists(output_dir):
        os.mkdir(output_dir)       

    if not os.path.exists(logging_dir):
        os.mkdir(logging_dir)

    if not os.path.exists(tmp_dir):
        try:
          os.makedirs(tmp_dir)
        except OSError as o:
          die("\nError creating directory %s (%s)" % (tmp_dir, o))

def prepare_RScripts(ref):
    batchDir="{0}{1}".format(output_dir, ref)
    batchRDir="{0}{1}/R".format(output_dir, ref)
    if not os.path.exists(batchRDir):
        os.mkdir(batchRDir)
    
    RScriptDir=os.path.abspath(batchRDir)
    # ==============================
    # BetaBinFit.R
    infile = open("./iBS/iBS.R/Imprinting/BetaBinFit.R")
    outfile = open(batchRDir+"/BetaBinFit.R", "w")

    replacements = {"__NOTHING__":"__SOMETHING__"}

    for line in infile:
        for src, target in replacements.items():
            line = line.replace(src, target)
        outfile.write(line)
    infile.close()
    outfile.close()

    # ==============================
    # TMFit.R
    infile = open("./iBS/iBS.R/Imprinting/TMFit.R")
    outfile = open(batchRDir+"/TMFit.R", "w")

    replacements = {"__NOTHING__":"__SOMETHING__"}

    for line in infile:
        for src, target in replacements.items():
            line = line.replace(src, target)
        outfile.write(line)
    infile.close()
    outfile.close()

    # ==============================
    # AMRFinderFit.R
    infile = open("./iBS/iBS.R/Imprinting/AMRFinderFit.R")
    outfile = open(batchRDir+"/AMRFinderFit.R", "w")

    replacements = {"__NOTHING__":"__SOMETHING__"}

    for line in infile:
        for src, target in replacements.items():
            line = line.replace(src, target)
        outfile.write(line)
    infile.close()
    outfile.close()

    # ==============================
    # ReadBetaBinShrinkage.R
    infile = open("./iBS/iBS.R/Imprinting/ReadBetaBinShrinkage.R")
    outfile = open(batchRDir+"/ReadBetaBinShrinkage.R", "w")

    replacements = {"__NOTHING__":"__SOMETHING__"}

    for line in infile:
        for src, target in replacements.items():
            line = line.replace(src, target)
        outfile.write(line)
    infile.close()
    outfile.close()
    
    # ==============================
    # KnownImprinting.R
    infile = open("./iBS/iBS.R/Imprinting/ReadKnownImprinting.R")
    outfile = open(batchRDir+"/ReadKnownImprinting.R", "w")

    replacements = {"__ANNOTATION_DATA_DIR__":gParams.annotation_data_dir}

    for line in infile:
        for src, target in replacements.items():
            line = line.replace(src, target)
        outfile.write(line)
    infile.close()
    outfile.close()

    # ==============================
    # Task_KnownAMRs_BetaBinFit.R
    infile = open("./iBS/iBS.R/Imprinting/Task_KnownAMRs_BetaBinFit.R")
    outfile = open(batchRDir+"/Task_KnownAMRs_BetaBinFit.R", "w")
    batch_ev_file=os.path.abspath("{0}{1}_filtered.tsv".format(gParams.ev_dir,ref))
    replacements = {"__RSCRIPT_DIR__":RScriptDir,
                    "__EV_TABLE_FILE__":batch_ev_file,
                    "__BetaBin_outterWinSize__":str(gParams.betabin_outter_winsize),
                    "__BetaBin_innerWinSize__":str(gParams.betabin_inner_winsize),
                    "__OUT_DIR__":batchDir,
                    "__REF__":ref,
                    "__MIN_ROWCNT_FOR_EVTBL__":str(gParams.betabinfit_evtbl_min_rowcnt)}
    for line in infile:
        for src, target in replacements.items():
            line = line.replace(src, target)
        outfile.write(line)
    infile.close()
    outfile.close()

    # ==============================
    # Task_KnownAMRs_TMFit.R
    infile = open("./iBS/iBS.R/Imprinting/Task_KnownAMRs_TMFit.R")
    outfile = open(batchRDir+"/Task_KnownAMRs_TMFit.R", "w")
    replacements = {"__RSCRIPT_DIR__":RScriptDir,
                    "__EV_TABLE_FILE__":batch_ev_file,
                    "__BaseStatisticFile__":str(gParams.tmfit_basestatistics_file),
                    "__BetaBin_innerWinSize__":str(gParams.betabin_inner_winsize),
                    "__OUT_DIR__":batchDir,
                    "__REF__":ref,
                    "__TMFIT_OutterWinSize__":str(gParams.tmfit_outter_winsize),
                    "__SlidingCGSiteCnt__":str(gParams.tmfit_sliding_cg_sitecnt),
                    "__AMR_MARGIN_FACTOR__":str(gParams.tmfit_amr_marginfactor),
                    "__UNIQUE_READ_CNT_MIN__":str(gParams.tmfit_unique_read_mincnt),
                    "__ShrinkageMethod__":str(gParams.tmfit_shrinkage_method),
                    "__TMFit_Model__":str(gParams.tmfit_model)}

    for line in infile:
        for src, target in replacements.items():
            line = line.replace(src, target)
        outfile.write(line)
    infile.close()
    outfile.close()

    # ==============================
    # Task_KnownAMRs_AMRFinderFit.R
    infile = open("./iBS/iBS.R/Imprinting/Task_KnownAMRs_AMRFinderFit.R")
    outfile = open(batchRDir+"/Task_KnownAMRs_AMRFinderFit.R", "w")
    replacements = {"__RSCRIPT_DIR__":RScriptDir,
                    "__EV_TABLE_FILE__":batch_ev_file,
                    "__BaseStatisticFile__":str(gParams.tmfit_basestatistics_file),
                    "__BetaBin_innerWinSize__":str(gParams.betabin_inner_winsize),
                    "__OUT_DIR__":batchDir,
                    "__REF__":ref,
                    "__TMFIT_OutterWinSize__":str(gParams.tmfit_outter_winsize),
                    "__SlidingCGSiteCnt__":str(gParams.tmfit_sliding_cg_sitecnt),
                    "__AMR_MARGIN_FACTOR__":str(gParams.tmfit_amr_marginfactor),
                    "__UNIQUE_READ_CNT_MIN__":str(gParams.tmfit_unique_read_mincnt),
                    "__ShrinkageMethod__":str(gParams.tmfit_shrinkage_method),
                    "__TMFit_Model__":str(gParams.tmfit_model)}

    for line in infile:
        for src, target in replacements.items():
            line = line.replace(src, target)
        outfile.write(line)
    infile.close()
    outfile.close()



qsub_shell_text = '''
cd __WD__
__R_BIN__/Rscript --no-restore --no-save ./R/Task_KnownAMRs_BetaBinFit.R
__R_BIN__/Rscript --no-restore --no-save ./R/__ARMFit_RFile__
'''

def prepare_worker_qsub(ref):
    batchDir="{0}{1}".format(output_dir, ref)
    qsubShellFile = "{0}/tm{1}.sh".format(batchDir, ref)

    OUTPUT =  os.path.abspath(batchDir)
    
    ARMFit_RFile="Task_KnownAMRs_TMFit.R"
    if gParams.tmfit_model==3:
        ARMFit_RFile = "Task_KnownAMRs_AMRFinderFit.R"

    replacements = {"__WD__":OUTPUT,
                    "__R_BIN__":gParams.R_bin,
                    "__ARMFit_RFile__":ARMFit_RFile}

    f = open(qsubShellFile, "w")
    lines = qsub_shell_text
    for src, target in replacements.items():
            lines = lines.replace(src, target)
    f.write(lines)
    f.close()

    whichQ=""
    if True:
        whichQ = "hongkai,"
    #whichQ=""
    #launch qsub
    qsub_cmd = ["qsub",
                "-N tm{0}".format(ref),
                "-pe local 2",
                "-l {0}mem_free=40G,h_vmem=20G".format(whichQ),
                #"-q hongkai.q@compute-061 -l mem_free=40G,h_vmem=20G",
                "-o {0}/tm{1}.o".format(batchDir, ref),
                "-e {0}/tm{1}.e".format(batchDir, ref),
                qsubShellFile]
    qsub_shell_cmd=""
    for str in qsub_cmd:
        qsub_shell_cmd=qsub_shell_cmd+str+" "
     
    proc = subprocess.call(qsub_shell_cmd, shell=True)
    ibs_log(qsubShellFile+" submitted!")

def getSortedRefNames(refNames):
    regRefNames=[]
    for ref in refNames:
        if len(ref)==4 and ref[3].isdigit():
            regRefNames.append('chr0'+ref[3])
        else:
            regRefNames.append(ref)
    regRefNames.sort()

    for i in range(len(regRefNames)):
        regRefNames[i] = regRefNames[i].replace('chr0', 'chr')

    return regRefNames

def prepare_workers():
    for ref in gParams.refs:
        batchDir="{0}{1}".format(output_dir, ref)
        if not os.path.exists(batchDir):
            os.mkdir(batchDir)
        prepare_RScripts(ref)
        prepare_worker_qsub(ref)
        

def main(argv=None):

    # Initialize default parameter values

    global gParams
    global gRunInfo
    global output_dir
    gRunInfo = RunInfo()
    gParams = TmKnownAMRsParams()
    run_argv = sys.argv[:]

    try:
        if argv is None:
            argv = sys.argv
        args = gParams.parse_options(argv)
        #params.check()
        output_dir= os.path.abspath(output_dir)+"/"
        start_time = datetime.now()

        prepare_output_dir()
        init_logger(logging_dir + "tm-knownarms.log")

        ibs_logp()
        ibs_log("Beginning tm-knownamrs-qsubs run")
        ibs_logp("-----------------------------------------------")

        prepare_workers()
      
        finish_time = datetime.now()
        duration = finish_time - start_time
        ibs_logp("-----------------------------------------------")
        ibs_log("Run complete")

    except Usage as err:
        ibs_logp(sys.argv[0].split("/")[-1] + ": " + str(err.msg))
        ibs_logp("    for detailed help see url ...")
        return 2
    
    except:
        ibs_logp(traceback.format_exc())
        die()

if __name__ == "__main__":
    sys.exit(main())
