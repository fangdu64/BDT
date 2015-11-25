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

output_dir = "./bsmooth_align_out/"
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
        self.num_workder = 20
        self.num_totalreads = 1267219548
        self.fastq_1 = None
        self.fastq_2 = None

    def parse_options(self, argv):
        try:
            opts, args = getopt.getopt(argv[1:], "hvo:w:",
                                        ["version",
                                         "help",
                                         "output-dir=",
                                         "num-workers=",
                                         "num-reads=",
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
            if option in ("-v", "--version"):
                print("BDVD v",iBSUtil.get_version())
                sys.exit(0)
            if option in ("-h", "--help"):
                raise Usage(use_message)
            if option in ("-w", "--num-workers"):
                self.num_workder = int(value)
            if option in ("--num-reads"):
                self.num_totalreads = int(value)
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

        if len(args) < 1:
            raise Usage(use_message)

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


qsub_shell_text = '''
cd /home/student/fdu1/projects/Imprinting/buildHg19Index

bowtie2Path="~/apps/build/bowtie2-2.2.3/bowtie2"
ref="/dcs01/gcode/fdu1/data/Common/Homo_sapiens/UCSC/hg19/Sequence/WholeGenomeFasta/genome.fa"
index="/dcs01/gcode/fdu1/data/Common/Homo_sapiens/UCSC/hg19/Sequence/BsmoothBowtie2Index/genome"
outDir="__outDir__"
if [ ! -d "${outDir}" ]; then
        mkdir ${outDir}
fi

BSMOOTH_HOME="/home/student/fdu1/apps/build/bsmooth/bsmooth-align"

perl ${BSMOOTH_HOME}/bin/bswc_bowtie2_align.pl \\
		--fastq \\
		--fr \\
		--no-unpaired \\
		--out=${outDir}/ev_bt2_raw \\
		--bscpg \\
		--bowtie2=${bowtie2Path} \\
		__skip-reads__ \\
		__stop-after__ \\
		-- \\
		${index} \\
		-- \\
		${ref} \\
		-- \\
		--sensitive \\
		-- \\
		/dcs01/gcode/fdu1/data/ENCODE/WGBS/HudsonAlpha_GM12878_MethylWgbs/SRR568016_1.fastq \\
		-- \\
		/dcs01/gcode/fdu1/data/ENCODE/WGBS/HudsonAlpha_GM12878_MethylWgbs/SRR568016_2.fastq
'''
def prepare_worker_qsub(bathchIdx, batch_read_cnt):
    batchDir="{0}batch{1}".format(output_dir, bathchIdx+1)
    qsubShellFile = "{0}/bsAlign{1}.sh".format(batchDir, bathchIdx+1)

    skip_reads = ""
    if bathchIdx>0:
        skip_reads="--skip-reads={0}".format(bathchIdx*batch_read_cnt)
    
    stop_after = ""
    if bathchIdx<(gParams.num_workder-1):
        stop_after="--stop-after={0}".format(batch_read_cnt)

    replacements = {"__outDir__":batchDir, 
                    "__skip-reads__":skip_reads, 
                    "__stop-after__":stop_after}

    f = open(qsubShellFile, "w")
    lines = qsub_shell_text
    for src, target in replacements.items():
            lines = lines.replace(src, target)
    f.write(lines)
    f.close()

    #launch qsub
    qsub_cmd = ["qsub",
                #"-N bsAligner{0}".format(bathchIdx+1),
                "-pe local 2",
                "-l hongkai,mem_free=10,h_vmem=20G",
                "-o {0}/bsAlign{1}.o".format(batchDir, bathchIdx+1),
                "-e {0}/bsAlign{1}.e".format(batchDir, bathchIdx+1),
                qsubShellFile]
    qsub_shell_cmd=""
    for str in qsub_cmd:
        qsub_shell_cmd=qsub_shell_cmd+str+" "
     
    proc = subprocess.call(qsub_shell_cmd, shell=True)
    ibs_log(qsubShellFile+" submitted!")

def prepare_workers():
    batch_read_cnt=int(gParams.num_totalreads/gParams.num_workder)
    for i in range(gParams.num_workder):
        bathchIdx = i
        batchDir="{0}batch{1}".format(output_dir, bathchIdx+1)
        if not os.path.exists(batchDir):
            os.mkdir(batchDir)
        prepare_worker_qsub(bathchIdx,batch_read_cnt)
        

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
        init_logger(logging_dir + "bsmooth-align-par.log")

        ibs_logp()
        ibs_log("Beginning bsmooth-align-par run")
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
