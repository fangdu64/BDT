#!/dcs01/gcode/fdu1/BDT/python/bin/python3.3

"""
fastq-split.py

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
    fastq-split [options] <total_read> <num_split> <reads1>

Options:
    -l/--line-per-read             <int>       [ default: 4                  ]
    --output-prefix                <string>    [ default: reads              ]
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
        self.reads1 = None
        self.num_split = None
        self.output_prefix1=None
        self.output_extention = None
        self.line_per_split= None

    def parse_options(self, argv):
        try:
            opts, args = getopt.getopt(argv[1:], "hvl:o:",
                                        ["version",
                                         "help",
                                         "line-per-read=",
                                         "output-prefix="
                                         "extension="])
        except getopt.error as msg:
            raise Usage(msg)

        # option processing
        for option, value in opts:
            if option in ("-v", "--version"):
                print("v 1.0")
                sys.exit(0)
            if option in ("-h", "--help"):
                raise Usage(use_message)
            if option in ("-l","--line-per-read"):
                self.line_per_read = int(value)
            if option in ("-o", "--output-prefix"):
                self.output_prefix1 = value
            if option in ("--extension"):
                self.output_extention = value
            if option in ("--num-reads"):
                self.num_totalreads = int(value)

        if len(args) != 3:
            raise Usage(use_message)
        
        try:
            self.num_totalreads = int(args[0])
            self.num_split = int(args[1])
            self.reads1 = os.path.abspath(args[2])
            self.line_per_split = int((self.num_totalreads+self.num_split)/self.num_split)*4
            if self.output_prefix1 is None:
                fileName, fileExtension = os.path.splitext(self.reads1)
                self.output_prefix1 = "{0}-split".format(fileName)
                self.output_extention = fileExtension

            if self.output_extention is None:
                fileName, fileExtension = os.path.splitext(self.reads1)
                self.output_extention = fileExtension

        except:
            raise Usage(use_message)
        
        return args


def die(msg=None):
    sys.exit(1)

def split_read():
    
    split_cmd = ["split",
                "--numeric-suffixes",
                "-l {0}".format(gParams.line_per_split),
                gParams.reads1,
                gParams.output_prefix1]     
    shell_cmd=""
    for str in split_cmd:
        shell_cmd=shell_cmd+str+" "
    print(shell_cmd)
    proc = subprocess.call(split_cmd)

    for i in range(gParams.num_split):
        split_file="{0}{1:02d}".format(gParams.output_prefix1,i)
        new_file="{0}{1:02d}{2}".format(gParams.output_prefix1,i+1,gParams.output_extention)
        os.rename(split_file,new_file)
        

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

        start_time = datetime.now()
        split_read()
        finish_time = datetime.now()
        duration = finish_time - start_time
        print("-----------------------------------------------")
        print("Run complete")

    except Usage as err:
        print(sys.argv[0].split("/")[-1] + ": " + str(err.msg))
        print("    for detailed help see url ...")
        return 2
    
    except:
        print(traceback.format_exc())
        die()

if __name__ == "__main__":
    sys.exit(main())
