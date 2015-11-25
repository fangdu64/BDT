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

'''

class Usage(Exception):
    def __init__(self, msg):
        self.msg = msg

output_dir = "/dcs01/gcode/fdu1/data/Kasper2014_WGBS/Quiescent1/bsmooth_align_out_2s_fr/"
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
        

class BsAlignParalleParams:

    def __init__(self):
        self.start_splitIdx = 948844
        self.num_splits = 1
        self.fastq_1_prefix = "/dcs01/gcode/fdu1/data/Kasper2014_WGBS/Quiescent1/SRR{0}_1.fastq"
        self.fastq_2_prefix = "/dcs01/gcode/fdu1/data/Kasper2014_WGBS/Quiescent1/SRR{0}_2.fastq"

    def parse_options(self, argv):
        try:
            opts, args = getopt.getopt(argv[1:], "ho:s:",
                                        ["help",
                                         "output-dir=",
                                         "num-splits=",
                                         "tmp-dir="])
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
            if option in ("-s", "--num-splits"):
                self.num_splits = int(value)
            if option in ("-o", "--output-dir"):
                custom_out_dir = value + "/"
            if option == "--tmp-dir":
                custom_tmp_dir = value + "/"

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


qsub_2strand_fr_text = '''
cd /home/student/fdu1/projects/Imprinting/buildHg19Index

fastq_1=__FASTQ1_FULLPATH__
fastq_2=__FASTQ2_FULLPATH__

#bowtie2Path="~/apps/build/bowtie2-2.2.3/bowtie2"
bowtie2Path="~/apps/build/bowtie2-2.0.5/bowtie2"
ref=/dcs01/gcode/fdu1/data/Common/Homo_sapiens/UCSC/hg19/Sequence/WholeGenomeFasta/genome.fa
index="/dcs01/gcode/fdu1/data/Common/Homo_sapiens/UCSC/hg19/Sequence/BsmoothBowtie2Index/genome"
tempDir="__SPLIT_OUTPUT__/temp"

BSMOOTH_HOME="/home/student/fdu1/apps/build/bsmooth/bsmooth-align"

perl ${BSMOOTH_HOME}/bin/bswc_bowtie2_align.pl \\
		--fastq \\
		--fr \\
		--no-unpaired \\
		--out=__SPLIT_OUTPUT__/ev_bt2_raw \\
		--bscpg \\
		--bowtie2=${bowtie2Path} \\
		--temp-directory=${tempDir} \\
		-- \\
		${index} \\
		-- \\
		${ref} \\
		-- \\
		--ignore-quals \\
		--mm \\
		-t \\
		-p 16 \\
        --ignore-quals \\
		-- \\
		${fastq_1} \\
		-- \\
		${fastq_2}
'''

def prepare_worker_qsub(bathchIdx):
    batchDir="{0}batch{1}".format(output_dir, bathchIdx+1)
    qsubShellFile = "{0}/bsAlign{1}.sh".format(batchDir, bathchIdx+1)

    fileBatchIdx = gParams.start_splitIdx + bathchIdx
    FASTQ1_FULLPATH = gParams.fastq_1_prefix.format(fileBatchIdx)
    FASTQ1_FULLPATH = os.path.abspath(FASTQ1_FULLPATH)
    FASTQ2_FULLPATH = gParams.fastq_2_prefix.format(fileBatchIdx)
    FASTQ2_FULLPATH = os.path.abspath(FASTQ2_FULLPATH)
    SPLIT_OUTPUT =  os.path.abspath(batchDir)
    
    replacements = {"__FASTQ1_FULLPATH__":FASTQ1_FULLPATH, 
                    "__FASTQ2_FULLPATH__":FASTQ2_FULLPATH, 
                    "__SPLIT_OUTPUT__":SPLIT_OUTPUT}

    f = open(qsubShellFile, "w")
    qsub_shell_text = qsub_2strand_fr_text
    lines = qsub_shell_text
    for src, target in replacements.items():
            lines = lines.replace(src, target)
    f.write(lines)
    f.close()

    whichQ=""
    if bathchIdx<30:
        whichQ = "hongkai,"
    #whichQ=""
    #launch qsub
    qsub_cmd = ["qsub",
                "-N bsFR{0}".format(bathchIdx+1),
                "-pe local 36",
                "-l {0}mem_free=40G,h_vmem=40G".format(whichQ),
                #"-q hongkai.q@compute-061 -l mem_free=40G,h_vmem=20G",
                "-o {0}/bsAlign{1}.o".format(batchDir, bathchIdx+1),
                "-e {0}/bsAlign{1}.e".format(batchDir, bathchIdx+1),
                qsubShellFile]
    qsub_shell_cmd=""
    for str in qsub_cmd:
        qsub_shell_cmd=qsub_shell_cmd+str+" "
     
    proc = subprocess.call(qsub_shell_cmd, shell=True)
    ibs_log(qsubShellFile+" submitted!")

def prepare_workers():
    for i in range(gParams.num_splits):
        bathchIdx = i
        batchDir="{0}batch{1}".format(output_dir, bathchIdx+1)
        if not os.path.exists(batchDir):
            os.mkdir(batchDir)
        prepare_worker_qsub(bathchIdx)
        

def main(argv=None):

    # Initialize default parameter values

    global gParams
    global gRunInfo
    global output_dir
    gRunInfo = RunInfo()
    gParams = BsAlignParalleParams()
    run_argv = sys.argv[:]

    try:
        if argv is None:
            argv = sys.argv
        args = gParams.parse_options(argv)
        #params.check()
        output_dir= os.path.abspath(output_dir)+"/"
        start_time = datetime.now()

        prepare_output_dir()
        init_logger(logging_dir + "bsmooth-align-splitfiles.log")

        ibs_logp()
        ibs_log("Beginning bsmooth-align-splitfiles run")
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
