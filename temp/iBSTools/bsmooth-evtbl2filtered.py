#!/dcs01/gcode/fdu1/BDT/python/bin/python3.3

"""
bsmooth-evtbl2bperror.py

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
import glob
import re

use_message = '''
Usage:
    bsmooth-evtbl2filtered [options] -e <ev_bt2_sorted>

Required arguments:
    <ev_bt2_sorted> Paths to a directory containing sorted read-level measurements
    <ref>
    <from> one-base 
    <to> one-based


Options:
    --mapq-min      <int>       [ default: 20                                  ]
    -o/--output-dir <string>    [ default: ./evtbl2subset_out                  ]
    --readid        <string>    [ default: ^SRR\d+.(?P<read>\d+).(?P<end>\d).+$]
    -r/--refs       <string>    [ default: search from ev_bt2_sorted dir  ]
'''

class Usage(Exception):
    def __init__(self, msg):
        self.msg = msg

output_dir = "./evtbl2subset_out/"
ev_dir = None
logging_dir = output_dir + "logs/"
script_dir = output_dir + "script/"
tmp_dir = output_dir + "tmp/"
ibs_log_handle = None #main log file handle
ibs_logger = None # main logging object
chormSize_path="/home/student/fdu1/apps/install/ucsc/hg19.chrom.sizes"

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
        self.nfilt_mapq = 0
        self.nfilt_qual = 0
        self.nfilt_uniqe = 0
        self.maxReadLen=200
        self.max_cyc=0
        self.watson_fw_cyc_cnts={(True,True):[0]*self.maxReadLen,
                                 (True,False):[0]*self.maxReadLen,
                                 (False,True):[0]*self.maxReadLen,
                                 (False,False):[0]*self.maxReadLen}

        self.watson_fw_cyc_errors={(True,True):[0]*self.maxReadLen,
                                   (True,False):[0]*self.maxReadLen,
                                   (False,True):[0]*self.maxReadLen,
                                   (False,False):[0]*self.maxReadLen}

class Evtbl2SubsetParams:

    def __init__(self):
        #max mem allowed in Mb
        self.max_mem = 2000
        self.num_threads = 4
        self.bsmooth_mapq_min= 20
        self.bsmooth_qual_min = 0
        self.refs=None
        self.pos_from=None
        self.pos_to=None
        self.read_id=r"^SRR\d+.(?P<read>\d+).(?P<end>\d).+$"

    def parse_options(self, argv):
        try:
            opts, args = getopt.getopt(argv[1:], "hvo:e:r:",
                                        ["version",
                                         "help",
                                         "output-dir=",
                                         "ev-dir=",
                                         "mapq-min=",
                                         "qual-min=",
                                         "tmp-dir=",
                                         "readid=",
                                         "refs="])

        except getopt.error as msg:
            raise Usage(msg)

        global output_dir
        global logging_dir
        global tmp_dir
        global script_dir
        global ev_dir

        custom_tmp_dir = None
        custom_out_dir = None
        # option processing
        for option, value in opts:
            if option in ("-h", "--help"):
                raise Usage(use_message)
            if option in ("--mapq-min"):
                self.bsmooth_mapq_min = int(value)
            if option in ("--qual-min"):
                self.bsmooth_qual_min = int(value)
            if option in ("-o", "--output-dir"):
                custom_out_dir = value + "/"
            if option == "--tmp-dir":
                custom_tmp_dir = value + "/"
            if option in ("-e", "--ev-dir"):
                ev_dir = value + "/"
            if option in ("--readid"):
                self.read_id = value
            if option in ("-r", "--refs"):
                self.refs=value.split(',')
                if len(self.refs)<1:
                    raise Usage(use_message)
            
        if ev_dir is None:
                raise Usage(use_message)
        
        if custom_out_dir:
            output_dir = custom_out_dir
            logging_dir = output_dir + "logs/"
            tmp_dir = output_dir + "tmp/"
            script_dir = output_dir + "script/"
        if custom_tmp_dir:
            tmp_dir = custom_tmp_dir

        if self.refs is None:
            self.refs = getAllRefsFromEvDir(ev_dir)

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
       
def formatTD(td):
    days = td.days
    hours = td.seconds // 3600
    minutes = (td.seconds % 3600) // 60
    seconds = td.seconds % 60

    if days > 0:
        return '%d days %02d:%02d:%02d' % (days, hours, minutes, seconds)
    else:
        return '%02d:%02d:%02d' % (hours, minutes, seconds)

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

def getAllRefsFromEvDir(evDir):
    refFiles=glob.glob("{0}.*.tsv.full_name".format(evDir))
    refs=[]
    for refFile in refFiles:
        refs.append(refFile.split('.')[1])
    
    return getSortedRefNames(refs)

def processAllRefs():
    
    refs=gParams.refs
    for ref in refs:
        subset_path=output_dir+"{0}_filtered.tsv".format(ref)
        subsetFile = open(subset_path, "w")
        evtblFilePath = "{0}{1}.tsv".format(ev_dir,ref)
        ibs_log("Processing {0} ...".format(ref))
        if not os.path.exists(logging_dir):
            die(ref+" not exist")
        processOneRef(ref, evtblFilePath, subsetFile, -1, -1)
        subsetFile.close()

    
    ibs_log("MAPQ Min = {0}, Filtered Cnt = {1}".format(gParams.bsmooth_mapq_min, gRunInfo.nfilt_mapq))
    ibs_log("Uniqe Filtered Cnt = {0}".format(gRunInfo.nfilt_uniqe))
    ibs_log("")

def processOneRef(ref, evtblFilePath, subsetFile, posFrom, posTo):
    global gRunInfo

    reReadID=re.compile(gParams.read_id)

    f=open(evtblFilePath)
    preRefOffset=-1
    lineCnt=0
    validCnt=0
    
    #unique reads covering the current CpG site
    watsonUniqeReads=[]
    crickUniqeReads=[]

    for line in f:
        lineCnt=lineCnt+1
        if lineCnt%100000==0:
            ibs_log("Processing {0} line {1} valid {2} ...".format(ref,lineCnt,validCnt))
        cols=line.rstrip('\n').split('\t')
        if len(cols) < 13:
            continue
        RefOffset=int(cols[1])
        if RefOffset<posFrom:
            continue
        if posTo>0 and RefOffset>= posTo:
            break
       
        Allele=cols[3]
        IsWaston=int(cols[4])
        IsForward=int(cols[5])
        SeqCycle=int(cols[9])
        AlLen=int(cols[10])
        MAPQ=int(cols[12])

        if MAPQ<gParams.bsmooth_mapq_min:
            gRunInfo.nfilt_mapq=gRunInfo.nfilt_mapq+1
            continue
        
        readKey="{0}-{1}".format(SeqCycle,IsForward)
        if IsWaston==1 and readKey in watsonUniqeReads:
            gRunInfo.nfilt_uniqe=gRunInfo.nfilt_uniqe+1
            continue
        elif IsWaston==1 and readKey not in watsonUniqeReads:
            watsonUniqeReads.append(readKey) 
        elif IsWaston==0 and readKey in crickUniqeReads:
            gRunInfo.nfilt_uniqe=gRunInfo.nfilt_uniqe+1
            continue
        elif IsWaston==0 and readKey not in crickUniqeReads:
            crickUniqeReads.append(readKey)


        if IsWaston==0:
            RefOffset=RefOffset-1

        if preRefOffset>0 and RefOffset != preRefOffset:
            crickUniqeReads=[]
            watsonUniqeReads=[]
        
        m=reReadID.search(cols[2])
        cols[2]=m.group('read')
        for col in cols:
            subsetFile.write(col+"\t")
        subsetFile.write(m.group('end')+'\n')
        preRefOffset=RefOffset
        validCnt=validCnt+1
        
    f.close()

def main(argv=None):

    # Initialize default parameter values

    global gParams
    global gRunInfo
    gRunInfo = RunInfo()
    gParams = Evtbl2SubsetParams()
    run_argv = sys.argv[:]

    try:
        if argv is None:
            argv = sys.argv
        args = gParams.parse_options(argv)
        start_time = datetime.now()

        prepare_output_dir()
        init_logger(logging_dir + "evtbl2subset.log")

        ibs_logp()
        ibs_log("Beginning BSmooth Filter Evidence Table run")
        ibs_logp("-----------------------------------------------")

        processAllRefs()

        finish_time = datetime.now()
        duration = finish_time - start_time
        ibs_logp("-----------------------------------------------")
        ibs_log("Run complete: {0} elapsed".format(formatTD(duration)))

    except Usage as err:
        ibs_logp(sys.argv[0].split("/")[-1] + ": " + str(err.msg))
        ibs_logp("    for detailed help see url ...")
        return 2
    
    except:
        ibs_logp(traceback.format_exc())
        die()

if __name__ == "__main__":
    sys.exit(main())
