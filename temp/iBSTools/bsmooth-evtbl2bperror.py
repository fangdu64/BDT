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

use_message = '''
Usage:
    bsmooth-evtbl2bperror [options] -e <ev_bt2_sorted>

Required arguments:
    <ev_bt2_sorted> Paths to a directory containing sorted read-level measurements

Options:
    --mapq-min             <int>       [ default: 20                             ]
    -r/--refs              <string>    [ default: search from ev_bt2_sorted dir  ]
    -o/--output-dir        <string>    [ default: ./evtbl2bigwig_out             ]
'''

class Usage(Exception):
    def __init__(self, msg):
        self.msg = msg

output_dir = "./evtbl2bperror_out/"
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

class Evtbl2BigwigParams:

    def __init__(self):
        #max mem allowed in Mb
        self.max_mem = 2000
        self.num_threads = 4
        self.bsmooth_mapq_min= 20
        self.bsmooth_qual_min = 0
        self.wig_name="allRefs"
        self.refs=None

    def parse_options(self, argv):
        try:
            opts, args = getopt.getopt(argv[1:], "hvo:e:r:",
                                        ["version",
                                         "help",
                                         "output-dir=",
                                         "ev-dir=",
                                         "mapq-min=",
                                         "wig-prefix=",
                                         "qual-min=",
                                         "tmp-dir=",
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
            if option in ("-v", "--version"):
                print("BDVD v",iBSUtil.get_version())
                sys.exit(0)
            if option in ("-h", "--help"):
                raise Usage(use_message)
            if option in ("-p", "--num-threads"):
                self.num_threads = int(value)
            if option in ("--wig-prefix"):
                self.wig_name = value
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

def processAllRefs(refs):
    
    for ref in refs:
        evtblFilePath = "{0}{1}.tsv".format(ev_dir,ref)
        ibs_log("Processing {0} ...".format(ref))
        if not os.path.exists(logging_dir):
            die(ref+" not exist")

        processOneRef(ref, evtblFilePath)

    ibs_log("MAPQ Min = {0}, Filtered Cnt = {1}".format(gParams.bsmooth_mapq_min, gRunInfo.nfilt_mapq))
    ibs_log("Uniqe Filtered Cnt = {0}".format(gRunInfo.nfilt_uniqe))
    ibs_log("")
    output_baseerror_table()

def output_baseerror_table():
    outFile=output_dir+"base_statistics.txt"
    f = open(outFile, "w")
    f.write("{0}\t{1}\t{2}\t{3}\t{4}\t{5}\t{6}\t{7}\t{8}\t{9}\n".format("cycle","w_cnt","wr_cnt","c_cnt","cr_cnt","w_err","wr_err","c_err","cr_err","error"))
    for i in range(gRunInfo.max_cyc+1):
        f.write("{0}".format(i))
        sumCnt=0
        sumError=0
        for iswatson in [True,False]:
            for isforward in [True, False]:
                f.write("\t{0}".format(gRunInfo.watson_fw_cyc_cnts[iswatson,isforward][i]))
                sumCnt+=gRunInfo.watson_fw_cyc_cnts[iswatson,isforward][i]
        for iswatson in [True,False]:
            for isforward in [True, False]:
                f.write("\t{0}".format(gRunInfo.watson_fw_cyc_errors[iswatson,isforward][i]))
                sumError+=gRunInfo.watson_fw_cyc_errors[iswatson,isforward][i]
        f.write("\t{0:.4f}\n".format(sumError/sumCnt))
    f.close()
    ibs_log("Output file: {0}".format(outFile))

def processOneRef(ref, evtblFilePath):
    global gRunInfo

    f=open(evtblFilePath)
    preRefOffset=-1
    siteWatsonMU=0
    siteWatsonM=0
    siteCrickMU=0
    siteCrickM=0
    siteWatsonE=0
    siteCrickE=0
    lineCnt=0
    validCnt=0
    
    #unique reads covering the current CpG site
    watsonUniqeReads=[]
    crickUniqeReads=[]

    for line in f:
        lineCnt=lineCnt+1
        cols=line.split('\t')
        if len(cols) < 13:
            continue
        RefOffset=int(cols[1])
       
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

        watsonMU=0
        watsonM=0
        crickMU=0
        crickM=0

        if IsWaston==0:
            RefOffset=RefOffset-1

        if preRefOffset>0 and RefOffset != preRefOffset:
            crickUniqeReads=[]
            watsonUniqeReads=[]
            siteWatsonMU=0
            siteWatsonM=0
            siteCrickMU=0
            siteCrickM=0
            siteWatsonE=0
            siteCrickE=0

        preRefOffset=RefOffset

        mismatched=False
        if IsWaston==1 and Allele=="C":
            watsonMU=1
            watsonM=1
        elif IsWaston==1 and Allele=="T":
            watsonMU=1
        elif IsWaston==0 and Allele=="G":
            #G->C Mythylated
            crickMU=1
            crickM=1
        elif IsWaston==0 and Allele=="A":
            #A->T UnMythylated
            crickMU=1
        else:
            mismatched = True
        if SeqCycle>gRunInfo.max_cyc:
            gRunInfo.max_cyc = SeqCycle

        gRunInfo.watson_fw_cyc_cnts[IsWaston==1,IsForward==1][SeqCycle]+=1
        if mismatched:
            gRunInfo.watson_fw_cyc_errors[IsWaston==1,IsForward==1][SeqCycle]+=1
            if IsWaston==1:
                siteWatsonE = siteWatsonE+1
            else:
                siteCrickE = siteCrickE+1
        

        siteWatsonMU=siteWatsonMU+watsonMU
        siteWatsonM=siteWatsonM+watsonM
        siteCrickMU=siteCrickMU+crickMU
        siteCrickM=siteCrickM+crickM
        validCnt=validCnt+1
        if lineCnt%100000==0:
            ibs_log("Processing {0} line {1} valid {2} ...".format(ref,lineCnt,validCnt))
    f.close()

def main(argv=None):

    # Initialize default parameter values

    global gParams
    global gRunInfo
    gRunInfo = RunInfo()
    gParams = Evtbl2BigwigParams()
    run_argv = sys.argv[:]

    try:
        if argv is None:
            argv = sys.argv
        args = gParams.parse_options(argv)
        start_time = datetime.now()

        prepare_output_dir()
        init_logger(logging_dir + "evtbl2bigwig.log")

        ibs_logp()
        ibs_log("Beginning BSmooth Evidence Table to Sequencing Cycle Statistics run")
        ibs_logp("-----------------------------------------------")

        processAllRefs(gParams.refs)

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
