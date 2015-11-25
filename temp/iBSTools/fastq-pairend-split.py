#!/dcs01/gcode/fdu1/BDT/python/bin/python3.3

"""
fastq-pairend-split.py

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
    fastq-split [options] <total_read> <num_split> <reads1> <reads2>

Options:
    -l/--line-per-read             <int>       [ default: 2                   ]
    --output-prefix1               <string>    [ default: reads1              ]
    --output-prefix2               <string>    [ default: reads2              ]
    --extension                    <string>    [ default: input's extension   ]
'''

class Usage(Exception):
    def __init__(self, msg):
        self.msg = msg

gParams=None
gRunInfo=None

class RunInfo:
    def __init__(self):
        self.processed_read_cnt = 0
        
class FastqSplitParams:

    def __init__(self):
        self.line_per_read = 4
        self.num_totalreads = None
        self.fastq_1 = None
        self.fastq_2 = None
        self.num_split = None
        self.output_prefix1=None
        self.output_prefix2=None
        self.output_extention = None

    def parse_options(self, argv):
        try:
            opts, args = getopt.getopt(argv[1:], "hvl:",
                                        ["version",
                                         "help",
                                         "line-per-read=",
                                         "output-prefix1=",
                                         "output-prefix2=",
                                         "extension="])
        except getopt.error as msg:
            raise Usage(msg)

        # option processing
        for option, value in opts:
            if option in ("-v", "--version"):
                print("BDVD v",iBSUtil.get_version())
                sys.exit(0)
            if option in ("-h", "--help"):
                raise Usage(use_message)
            if option in ("-l","--line-per-read"):
                self.line_per_read = int(value)
            if option in ("--output-prefix1"):
                self.output_prefix1 = value
            if option in ("--output-prefix2"):
                self.output_prefix2 = value
            if option in ("--extension"):
                self.output_extention = value
            if option in ("--num-reads"):
                self.num_totalreads = int(value)

        if len(args) != 4:
            raise Usage(use_message)
        
        try:
            self.num_totalreads = int(args[0])
            self.num_split = int(args[1])
            self.reads1 = os.path.abspath(args[2])
            self.reads2 = os.path.abspath(args[3])

            if self.output_prefix1 is None:
                fileName, fileExtension = os.path.splitext(self.reads1)
                self.output_prefix1 = "{0}-split".format(fileName)
                self.output_extention = fileExtension

            if self.output_prefix2 is None:
                fileName, fileExtension = os.path.splitext(self.reads2)
                self.output_prefix2 = "{0}-split".format(fileName)

            if self.output_extention is None:
                fileName, fileExtension = os.path.splitext(self.reads1)
                self.output_extention = fileExtension

        except:
            raise Usage(use_message)
        
        return args


def die(msg=None):
    sys.exit(1)

qsub_shell_text = '''
split --numeric-suffixes \\
		-l __LINES__ \\
		__INPUT__ \\
		__PREFIX__
'''
def qsub_read(readNum, line_per_split, inputFile, outPrefix):
    replacements = {"__LINES__":str(line_per_split), 
                    "__INPUT__":inputFile, 
                    "__PREFIX__":outPrefix}

    f = open(qsubShellFile, "w")
    lines = qsub_shell_text
    for src, target in replacements.items():
            lines = lines.replace(src, target)
    f.write(lines)
    f.close()

    #launch qsub
    qsub_cmd = ["qsub",
                "-N fastqSplit{0}".format(readNum),
                "-l mem_free=10,h_vmem=20G",
                "-o {0}/fastqSplit{1}.o".format(readNum),
                "-e {0}/fastqSplit{2}.e".format(readNum),
                qsubShellFile]
    qsub_shell_cmd=""
    for str in qsub_cmd:
        qsub_shell_cmd=qsub_shell_cmd+str+" "
     
    proc = subprocess.call(qsub_shell_cmd, shell=True)
    ibs_log(qsubShellFile+" submitted!")

def prepare_workers():
    bqsub_read
        

def main(argv=None):

    # Initialize default parameter values

    global gParams
    global gRunInfo
    gRunInfo = RunInfo()
    gParams = FastqSplitParams()
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
