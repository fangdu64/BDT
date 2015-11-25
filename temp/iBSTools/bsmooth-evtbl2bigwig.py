#!/dcs01/gcode/fdu1/BDT/python/bin/python3.3

"""
bsmooth-evtbl2bigwig.py

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
from copy import deepcopy
import glob

import iBSUtil
import iBSFCDClient as fcdc
import iBS
import Ice

use_message = '''
Usage:
    bsmooth-evtbl2bigwig [options] -e <ev_bt2_sorted>

Required arguments:
    <ev_bt2_sorted> Paths to a directory containing sorted read-level measurements

Options:
    --mapq-min             <int>       [ default: 20                             ]
    --wig-prefix           <string>    [ default: allRefs                        ]
    -r/--refs              <string>    [ default: search from ev_bt2_sorted dir  ]
    -o/--output-dir        <string>    [ default: ./evtbl2bigwig_out             ]
'''

class Usage(Exception):
    def __init__(self, msg):
        self.msg = msg

output_dir = "./evtbl2bigwig_out/"
ev_dir = None
logging_dir = output_dir + "logs/"
fcdcentral_dir = output_dir + "fcdcentral/"
script_dir = output_dir + "script/"
tmp_dir = output_dir + "tmp/"
ibs_log_handle = None #main log file handle
ibs_logger = None # main logging object
wigToBigWig_path="/home/student/fdu1/apps/install/ucsc/wigToBigWig"
chormSize_path="/home/student/fdu1/apps/install/ucsc/hg19.chrom.sizes"

fcdc_popen = None
fcdc_log_file=None

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
        self.fcdc_fvworker_size=self.num_threads
        self.fcdc_tcp_port= 16000
        self.fcdc_threadpool_size= 2
        self.bsmooth_mapq_min= 20
        self.bsmooth_qual_min = 0
        self.wig_name="allRefs"
        self.fr_separated=False
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
                                         "fr-separated",
                                         "tmp-dir=",
                                         "refs="])
        except getopt.error as msg:
            raise Usage(msg)

        global output_dir
        global logging_dir
        global tmp_dir
        global fcdcentral_dir
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
                self.fcdc_fvworker_size = self.num_threads
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
            if option == "--fr-separated":
                self.fr_separated = True
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
            fcdcentral_dir = output_dir + "fcdcentral/"
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
    global fcdc_popen
    if msg is not None:
        ibs_logp(msg)
    shutdownFCDCentral()
    sys.exit(1)

def shutdownFCDCentral():
    global fcdc_popen
    if fcdc_popen is not None:
        fcdc_popen.terminate()
        fcdc_popen.wait()
        fcdc_popen = None
        ibs_log("FCDCentral shutdown")
    if fcdc_log_file is not None:
        fcdc_log_file.close()

# Ensures that the output, logging, and temp directories are present. If not,
# they are created
def prepare_output_dir():

    ibs_log("Preparing output location "+output_dir)

    if not os.path.exists(output_dir):
        os.mkdir(output_dir)       

    if not os.path.exists(logging_dir):
        os.mkdir(logging_dir)
         
    if not os.path.exists(fcdcentral_dir):
        os.mkdir(fcdcentral_dir)

    fcdc_db_dir=fcdcentral_dir+"FCDCentralDB"
    if not os.path.exists(fcdc_db_dir):
        os.mkdir(fcdc_db_dir)

    fcdc_fvstore_dir=fcdcentral_dir+"FeatureValueStore"
    if not os.path.exists(fcdc_fvstore_dir):
        os.mkdir(fcdc_fvstore_dir)

    if not os.path.exists(tmp_dir):
        try:
          os.makedirs(tmp_dir)
        except OSError as o:
          die("\nError creating directory %s (%s)" % (tmp_dir, o))
       

def prepare_fcdcentral_config(tcpPort,fvWorkerSize, iceThreadPoolSize ):
    infile = open("./iBS/config/FCDCentralServer.config")
    outfile = open(fcdcentral_dir+"FCDCentralServer.config", "w")

    replacements = {"__FCDCentral_TCP_PORT__":str(tcpPort), 
                    "__FeatureValueWorker.Size__":str(fvWorkerSize), 
                    "__Ice.ThreadPool.Server.Size__":str(iceThreadPoolSize)}

    for line in infile:
        for src, target in replacements.items():
            line = line.replace(src, target)
        outfile.write(line)
    infile.close()
    outfile.close()

def launchFCDCentral():
    global fcdc_popen
    global fcdc_log_fhandle
    fcdcentral_path=os.getcwd()+"/iBS/bin/FCDCentral"
    fcdc_cmd = [fcdcentral_path]
    ibs_log("Launching FCDCentral ...")
    fcdc_log_file = open(logging_dir + "fcdc.log","w")
    fcdc_popen = subprocess.Popen(fcdc_cmd, cwd=fcdcentral_dir, stdout=fcdc_log_file)

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
    
    wigfileM_path=output_dir+"{0}M.wig".format(gParams.wig_name)
    wigfileU_path=output_dir+"{0}U.wig".format(gParams.wig_name)
    wigfileError_path=output_dir+"{0}Error.wig".format(gParams.wig_name)
    wigfileMP_path=output_dir+"{0}MP.wig".format(gParams.wig_name)

    wigfileM = open(wigfileM_path, "w")
    wigfileU = open(wigfileU_path, "w")
    wigfileError = open(wigfileError_path, "w")
    wigfileMP = open(wigfileMP_path, "w")

    for ref in refs:
        evtblFilePath = "{0}{1}.tsv".format(ev_dir,ref)
        ibs_log("Processing {0} ...".format(ref))
        if not os.path.exists(logging_dir):
            die(ref+" not exist")

        processOneRef(ref, evtblFilePath, wigfileM, wigfileU, wigfileError, wigfileMP)
    
    wigfileMP.close()
    wigfileError.close()
    wigfileM.close()
    wigfileU.close()

    ibs_log("MAPQ Min = {0}, Filtered Cnt = {1}".format(gParams.bsmooth_mapq_min, gRunInfo.nfilt_mapq))
    ibs_log("Uniqe Filtered Cnt = {0}".format(gRunInfo.nfilt_uniqe))
    ibs_log("")
    ibs_log("wigToBigWig ...")

    bigWigfileM_path=output_dir+"{0}M.bw".format(gParams.wig_name)
    bigWigfileU_path=output_dir+"{0}U.bw".format(gParams.wig_name)
    bigWigfileError_path=output_dir+"{0}Error.bw".format(gParams.wig_name)
    bigWigfileMP_path=output_dir+"{0}MP.bw".format(gParams.wig_name)

    bigWig_cmd = [wigToBigWig_path, wigfileM_path, chormSize_path, bigWigfileM_path]
    proc = subprocess.call(bigWig_cmd)
    ibs_log(bigWigfileM_path+" done!")

    bigWig_cmd = [wigToBigWig_path, wigfileU_path, chormSize_path, bigWigfileU_path]
    proc = subprocess.call(bigWig_cmd)
    ibs_log(bigWigfileU_path+" done!")

    bigWig_cmd = [wigToBigWig_path, wigfileError_path, chormSize_path, bigWigfileError_path]
    proc = subprocess.call(bigWig_cmd)
    ibs_log(bigWigfileError_path+" done!")

    bigWig_cmd = [wigToBigWig_path, wigfileMP_path, chormSize_path, bigWigfileMP_path]
    proc = subprocess.call(bigWig_cmd)
    ibs_log(bigWigfileMP_path+" done!")


def OutputOneSite(wigfileM, wigfileU, wigfileError, wigfileMP, sitePos, siteWatsonMU, siteWatsonM, siteCrickMU, siteCrickM , siteWatsonE, siteCrickE):
    if gParams.fr_separated:
        OutputOneSite_FRSeparated(wigfileM, wigfileU, wigfileError, wigfileMP, sitePos, siteWatsonMU, siteWatsonM, siteCrickMU, siteCrickM , siteWatsonE, siteCrickE)
    else:
        OutputOneSite_FRCombined(wigfileM, wigfileU, wigfileError, wigfileMP, sitePos, siteWatsonMU, siteWatsonM, siteCrickMU, siteCrickM , siteWatsonE, siteCrickE)

def OutputOneSite_FRCombined(wigfileM, wigfileU, wigfileError, wigfileMP, sitePos, siteWatsonMU, siteWatsonM, siteCrickMU, siteCrickM , siteWatsonE, siteCrickE):    
    wigfileM.write("{0}\t{1}\n".format(sitePos, siteWatsonM+siteCrickM))
    wigfileU.write("{0}\t{1}\n".format(sitePos, siteWatsonMU+siteCrickMU-siteWatsonM-siteCrickM))
    wigfileError.write("{0}\t{1}\n".format(sitePos, siteWatsonE+siteCrickE))
    mp=0
    if (siteWatsonMU+siteCrickMU)>0:
        mp=(siteWatsonM+siteCrickM)/(siteWatsonMU+siteCrickMU)
   
    if siteWatsonMU+siteCrickMU>4:
        wigfileMP.write("{0}\t{1}\n".format(sitePos, mp))


def OutputOneSite_FRSeparated(wigfileM, wigfileU, wigfileError, wigfileMP, sitePos, siteWatsonMU, siteWatsonM, siteCrickMU, siteCrickM , siteWatsonE, siteCrickE):
    wigfileM.write("{0}\t{1}\n{2}\t{3}\n".format(sitePos, siteWatsonM, sitePos+1,-siteCrickM))
    wigfileU.write("{0}\t{1}\n{2}\t{3}\n".format(sitePos, siteWatsonMU-siteWatsonM, sitePos+1,-(siteCrickMU-siteCrickM)))
    wigfileError.write("{0}\t{1}\n{2}\t{3}\n".format(sitePos, siteWatsonE, sitePos+1,-siteCrickE))
    w_mp=0
    if siteWatsonMU>0:
        w_mp=siteWatsonM/siteWatsonMU
    c_mp=0
    if siteCrickMU>0:
        c_mp=siteCrickM/siteCrickMU
    if siteWatsonMU>1:
        wigfileMP.write("{0}\t{1}\n".format(sitePos, w_mp))
    if siteCrickMU>1:
        wigfileMP.write("{0}\t{1}\n".format(sitePos+1, -c_mp))
    

def processOneRef(ref, evtblFilePath, wigfileM, wigfileU, wigfileError, wigfileMP):
    global gRunInfo

    wigfileM.write("variableStep chrom={0} span=1\n".format(ref))
    wigfileU.write("variableStep chrom={0} span=1\n".format(ref))
    wigfileError.write("variableStep chrom={0} span=1\n".format(ref))
    wigfileMP.write("variableStep chrom={0} span=1\n".format(ref))

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
        if lineCnt%100000==0:
            ibs_log("Processing {0} line {1} valid {2} ...".format(ref,lineCnt,validCnt))
        cols=line.split('\t')
        if len(cols) < 13:
            ibs_log("Processing {0} line {1} valid {2}, error cols<13 ...".format(ref,lineCnt,validCnt))
            continue
        RefOffset=int(cols[1])
       
        Allele=cols[3]
        IsWaston=int(cols[4])
        IsForward=int(cols[5])
        SeqCycle=int(cols[9])
        AlLen=int(cols[10])
        MAPQ=int(cols[12])

        if IsWaston==0:
            RefOffset=RefOffset-1

        if preRefOffset>0 and RefOffset != preRefOffset:
            OutputOneSite(wigfileM, wigfileU, wigfileError, wigfileMP, preRefOffset, siteWatsonMU, siteWatsonM, siteCrickMU, siteCrickM, siteWatsonE, siteCrickE)
            crickUniqeReads=[]
            watsonUniqeReads=[]
            siteWatsonMU=0
            siteWatsonM=0
            siteCrickMU=0
            siteCrickM=0
            siteWatsonE=0
            siteCrickE=0

        preRefOffset=RefOffset

        if MAPQ<gParams.bsmooth_mapq_min:
            gRunInfo.nfilt_mapq=gRunInfo.nfilt_mapq+1
            continue
        
        readKey="{0}-{1}".format(SeqCycle,IsForward)
        if IsWaston==1:
            if readKey in watsonUniqeReads:
                gRunInfo.nfilt_uniqe=gRunInfo.nfilt_uniqe+1
                continue
            else:
                watsonUniqeReads.append(readKey)
        else:
            if readKey in crickUniqeReads:
                gRunInfo.nfilt_uniqe=gRunInfo.nfilt_uniqe+1
                continue
            else:
                crickUniqeReads.append(readKey)

        watsonMU=0
        watsonM=0
        crickMU=0
        crickM=0

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

        if mismatched:
            if IsWaston==1:
                siteWatsonE = siteWatsonE+1
            else:
                siteCrickE = siteCrickE+1
        
        siteWatsonMU=siteWatsonMU+watsonMU
        siteWatsonM=siteWatsonM+watsonM
        siteCrickMU=siteCrickMU+crickMU
        siteCrickM=siteCrickM+crickM
        validCnt=validCnt+1

    if preRefOffset>0:
        OutputOneSite(wigfileM, wigfileU,wigfileError, wigfileMP, preRefOffset, siteWatsonMU, siteWatsonM, siteCrickMU, siteCrickM, siteWatsonE, siteCrickE)
    f.close()

def main(argv=None):

    # Initialize default parameter values

    global gParams
    global gRunInfo
    gRunInfo = RunInfo()
    gParams = Evtbl2BigwigParams()
    run_argv = sys.argv[:]

    global fcdc_popen
    fcdc_popen = None

    try:
        if argv is None:
            argv = sys.argv
        args = gParams.parse_options(argv)
        start_time = datetime.now()

        prepare_output_dir()
        init_logger(logging_dir + "evtbl2bigwig.log")

        ibs_logp()
        ibs_log("Beginning BSmooth Evidence Table to BigWig run (v"+iBSUtil.get_version()+")")
        ibs_logp("-----------------------------------------------")

        processAllRefs(gParams.refs)

        finish_time = datetime.now()
        duration = finish_time - start_time
        ibs_logp("-----------------------------------------------")
        ibs_log("Run complete: %s elapsed" %  iBSUtil.formatTD(duration))

    except Usage as err:
        shutdownFCDCentral()
        ibs_logp(sys.argv[0].split("/")[-1] + ": " + str(err.msg))
        ibs_logp("    for detailed help see url ...")
        return 2
    
    except:
        ibs_logp(traceback.format_exc())
        die()

if __name__ == "__main__":
    sys.exit(main())
