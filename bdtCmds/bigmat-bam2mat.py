#!__PYTHON_BIN_PATH__

"""
bdvd-bam2mat.py

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

import iBSConfig
import iBSUtil
import iBSDefines
import iBSFCDClient as fcdc
import iBS
import Ice

use_message = '''
BDVD-BAM2Mat creats data matrix from BAM file samples.

Usage:
    bdvd-bam2mat [options] <design_file>

Options:
    -v/--version
    -o/--output-dir                <string>    [ default: ./bam2mat_out       ]
    -p/--num-threads               <int>       [ default: 4                   ]
    -m/--max-mem                   <int>       [ default: 20000               ]
    --tmp-dir                      <dirname>   [ default: <output_dir>/tmp ]

Advanced Options:
    --place-holder

'''

# ----------------------------------------------------------------------
# Sample Information
# ----------------------------------------------------------------------
sample_table_help_message = '''
comma separated fields
Format:
    sample_name, bam_dir, bam_file, biological_group, tags

Columns:
    sample_id         <integer>     [  ]
    sample_name         <string>    [ if left empty, set the same as bam_file ]

    bam_dir             <string>    [ absolute directory containing the bam_file (e.g., /home/user/) 
                                      if left empty, set the same as in the previous row            ]

    bam_file            <string>    [ bam file basename, i.e., stripped out dir (e.g., abc.bam)     ]
    biological_group    <string>    [                                         ]
    tags                <string>    [optional                                 ]
'''

class Usage(Exception):
    def __init__(self, msg):
        self.msg = msg

output_dir = "./bam2mat_out/"
logging_dir = output_dir + "logs/"
fcdcentral_dir = output_dir + "fcdcentral/"
script_dir = output_dir + "script/"
tmp_dir = output_dir + "tmp/"
bdvd_log_handle = None #main log file handle
bdvd_logger = None # main logging object

fcdc_popen = None
fcdc_log_file=None
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

        #max mem allowed in Mb
        self.max_mem = 2000
        self.num_threads = 4
        self.fcdc_fvworker_size=self.num_threads
        self.fcdc_tcp_port=16000
        self.fcdc_threadpool_size=2
        self.workflow_node = "bam2mat"
        self.result_dumpfile = None
        self.datacentral_dir = None
        self.design_file = None

    def parse_options(self, argv):
        try:
            opts, args = getopt.getopt(argv[1:], "hvp:m:o:",
                                        ["version",
                                         "help",
                                         "output-dir=",
                                         "num-threads=",
                                         "max-mem=",
                                         "node=",
                                         "datacentral-dir=",
                                         "tmp-dir="])
        except getopt.error as msg:
            raise Usage(msg)

        global output_dir
        global logging_dir
        global tmp_dir
        global fcdcentral_dir
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
            if option in ("-p", "--num-threads"):
                self.num_threads = int(value)
                self.fcdc_fvworker_size = self.num_threads
            if option in ("-m", "--max-mem"):
                self.max_mem = int(value)
            if option in ("-o", "--output-dir"):
                custom_out_dir = value + "/"
                self.resume_dir = value
            if option == "--tmp-dir":
                custom_tmp_dir = value + "/"
            if option == "--node":
                self.workflow_node = value
            if option =="--datacentral-dir":
                self.datacentral_dir = value
        
        self.result_dumpfile = "{0}.pickle".format(self.workflow_node)
        if custom_out_dir:
            output_dir = custom_out_dir
            logging_dir = output_dir + "logs/"
            tmp_dir = output_dir + "tmp/"
            fcdcentral_dir = output_dir + "fcdcentral/"
            script_dir = output_dir + "script/"
        if custom_tmp_dir:
            tmp_dir = custom_tmp_dir

        if self.datacentral_dir is not None:
            fcdcentral_dir = self.datacentral_dir+"/fcdcentral/"

        if len(args) < 1:
            raise Usage(use_message)
        self.design_file = args[0]
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
    global fcdc_popen
    if msg is not None:
        bdvd_logp(msg)
    shutdownFCDCentral()
    sys.exit(1)

def shutdownFCDCentral():
    global fcdc_popen
    if fcdc_popen is not None:
        fcdc_popen.terminate()
        fcdc_popen.wait()
        fcdc_popen = None
        bdvd_log("FCDCentral shutdown")
    if fcdc_log_file is not None:
        fcdc_log_file.close()

# Ensures that the output, logging, and temp directories are present. If not,
# they are created
def prepare_output_dir():

    bdvd_log("Preparing output location "+output_dir)

    if not os.path.exists(output_dir):
        os.mkdir(output_dir)       

    if not os.path.exists(logging_dir):
        os.mkdir(logging_dir)
         
    if not os.path.exists(script_dir):
        os.mkdir(script_dir)

    #shutil.copy(gParams.design_file,script_dir)
    shutil.copy(gParams.design_file,"{0}/bdvdBam2MatDesign.py".format(script_dir))
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
    bdvd_log("Launching FCDCentral ...")
    fcdc_log_file = open(logging_dir + "fcdc.log","w")
    fcdc_popen = subprocess.Popen(fcdc_cmd, cwd=fcdcentral_dir, stdout=fcdc_log_file)

def define_samples(sample_table_lines):
    sample_list=[]
    #parse sample_table
    dir=""
    for line in sample_table_lines:
        line=line.strip()
        if line in ['',
                    'sample_id,sample_name,bam_dir,bam_file',
                    '[rows begin]',
                    '[rows end]']:
            continue
        if line[0]=="#":
            continue

        cols=line.split(',')
        if len(cols)<4:
            print("ERROR in:",line)
            print("incomplete information")
            return None
        for i in range(len(cols)):
            cols[i]=cols[i].strip()

        sample_name=cols[1]
        bam_dir=cols[2]
        bam_file=cols[3]

        if bam_dir:
            dir=bam_dir
        fullname=dir+bam_file
        if not os.path.exists(fullname):
            print("ERROR in:",line)
            print("BAM file not exist")
            return None

        sample_list.append(iBSDefines.BamSampleInfo(sample_name,fullname))

    return sample_list

def define_genomicBins(chromosomes, bin_width):
    binInfo = iBSDefines.GenomicBinInfo(chromosomes, bin_width)
    return binInfo

def getTaskConfig():
    designPath=os.path.abspath(script_dir)
    if designPath not in sys.path:
         sys.path.append(designPath)
    import bdvdBam2MatDesign as design

    if (design.sample_table_lines is not None):
        sampleList=define_samples(design.sample_table_lines)
    else:
        sample_table_lines = design.sample_table_content.split('\n')
        sampleList=define_samples(sample_table_lines)

    binInfo=define_genomicBins(design.chromosomes, design.bin_width)
    #check configs
    return sampleList,binInfo

def exportTxtInfo():
    fn = "{0}{1}".format(output_dir,gParams.result_dumpfile)
    bam2mat = iBSDefines.loadPickle(fn)
    binmap=bam2mat.BinMap
    #output binmap
    bin_info_fn="{0}binmap_info.txt".format(output_dir);
    outf = open(bin_info_fn, "w")
    outf.write("{0}\t{1}\t{2}\n".format("RefName","BinFrom","BinTo"))
    for i in range(len(binmap.RefNames)):
        outf.write("{0}\t{1}\t{2}\n".format(binmap.RefNames[i],binmap.RefBinFroms[i],binmap.RefBinTos[i]))
    outf.close()
    
    #output matrixs
    obj=bam2mat.BigMat
    matrix_info_fn="{0}matrix_info.txt".format(output_dir);
    outf = open(matrix_info_fn, "w")
    outf.write("{0}\t{1}\t{2}\t{3}\n".format("MatrixID","DataFile","RowCnt","ColCnt"))
    outf.write("{0}\t{1}\t{2}\t{3}\n".format(1,obj.StorePathPrefix,obj.RowCnt,obj.ColCnt))
    outf.close()

    #output colNames
    colnames_fn="{0}colnames.txt".format(output_dir);
    outf = open(colnames_fn, "w")
    for colname in obj.ColNames:
        outf.write("{0}\n".format(colname))
    outf.close()

def dumpOutput(bam2mat):
    fn = "{0}{1}".format(output_dir,gParams.result_dumpfile)
    iBSDefines.dumpPickle(bam2mat,fn)

def main(argv=None):

    # Initialize default parameter values
    global gParams
    gParams = BDVDParams()
    run_argv = sys.argv[:]

    global fcdc_popen
    fcdc_popen = None

    try:
        if argv is None:
            argv = sys.argv
        args = gParams.parse_options(argv)
       
        print("design file = ",gParams.design_file)

        start_time = datetime.now()
        prepare_output_dir()
        init_logger(logging_dir + "bdvd.log")

        bdvd_logp()
        bdvd_log("Beginning BDVD Bam2Mat run (v"+iBSUtil.get_version()+")")
        bdvd_logp("-----------------------------------------------")

        bdvd_log("Threads used: {0} for processing BAM files, additioanl {1} for supporting".format(gParams.fcdc_fvworker_size, gParams.fcdc_threadpool_size))
        gParams.fcdc_tcp_port = iBSUtil.getUsableTcpPort()
        prepare_fcdcentral_config(gParams.fcdc_tcp_port, 
                                  gParams.fcdc_fvworker_size, 
                                  gParams.fcdc_threadpool_size)
        #print("fcdc_tcp_port = ",gParams.fcdc_tcp_port)

        sampleList,binInfo = getTaskConfig()

        launchFCDCentral()

        fcdc.Init();
        fcdcHost="localhost -p "+str(gParams.fcdc_tcp_port)

        fcdcPrx = None
        tryCnt=0
        while (tryCnt<20) and (fcdcPrx is None):
            try:
                fcdcPrx=fcdc.GetFCDCProxy(fcdcHost)
            except Ice.ConnectionRefusedException as ex:
                tryCnt=tryCnt+1
                time.sleep(1)

        if fcdcPrx is None:
            raise Usage("connection timeout")

        facetAdminPrx=fcdc.GetFacetAdminProxy(fcdcHost)
        computePrx=fcdc.GetComputeProxy(fcdcHost)
        samplePrx=fcdc.GetSeqSampleProxy(fcdcHost)
    
        bdvd_log("FCDCentral activated")
        
        intersectRefs = iBSUtil.getIntersectRefsFromBamSamples(sampleList,samplePrx)
        bdvd_log("Sample Num: {0}, Common chromosomes: {1}".format(len(sampleList), str(intersectRefs)))

        if (binInfo.RefNames is None) or (len(binInfo.RefNames)==0):
            binInfo.RefNames=intersectRefs
        else:
            for ref in binInfo.RefNames:
                if ref not in intersectRefs:
                    die(ref +" is not present in all samples")
        bdvd_log("chromosomes considered: {0}".format(str(binInfo.RefNames)))
        blankSample=samplePrx.GetBlankBamToBinCountSample()
        blankSample.RefNames=binInfo.RefNames
        blankSample.BinWidth=binInfo.BinWidth

        samples=[]
        for si in sampleList:
            sample=deepcopy(blankSample)
            sample.SampleName=si.name
            sample.Treatment=""
            sample.BamFile=si.bamfile
            samples.append(sample)
        
        bbci =samplePrx.GetBlankBamToBinCountInfo()
        bbci.BamFile = samples[0].BamFile
        bbci.RefNames = binInfo.RefNames
        bbci.BinWidth = binInfo.BinWidth

        (rt, refBinFroms, refBinTos) = samplePrx.GetRefBinRanges(bbci)
        binmap = iBSDefines.RefNoneoverlapBinMap()
        binmap.BinWidth = binInfo.BinWidth
        binmap.RefBinFroms = refBinFroms
        binmap.RefBinTos = refBinTos
        binmap.RefNames = binInfo.RefNames

        bdvd_log("")
        (rt, sampleIDs, amdTaskID) = samplePrx.CreateBamToBinCountSamples(samples)

        preFinishedCnt=0
        amdTaskFinished=False
        bdvd_log("Extracting bin values ...")
        bdvd_log("Sample IDs: {0}".format(str(list(range(1,len(samples)+1)))))
        bdvd_log("sample processed: {0}/{1}".format(0, len(samples)))
        while (not amdTaskFinished):
            (rt,amdTaskInfo)=fcdcPrx.GetAMDTaskInfo(amdTaskID)
            if preFinishedCnt<amdTaskInfo.FinishedCnt:
                preFinishedCnt = amdTaskInfo.FinishedCnt
                bdvd_log("Sample Processed: {0}/{1}".format(preFinishedCnt, amdTaskInfo.TotalCnt))
            if amdTaskInfo.TotalCnt==amdTaskInfo.FinishedCnt:
                amdTaskFinished = True;
            else:
                time.sleep(4)
        minSampleID = min(sampleIDs)
        (rt, osis)=fcdcPrx.GetObserversStats(sampleIDs)
        binCount = int(osis[0].Cnt)
        bdvd_log("Bin Count: "+str(binCount))
        for osi in osis:
            bdvd_logp("Sample {0}: Max = {1}, Min = {2}, Sum = {3}".format(osi.ObserverID-minSampleID+1, int(osi.Max), int(osi.Min), int(osi.Sum)))
        bdvd_log("Extracting bin values [done]")
        
        #combine into big matrix
        computePrx.GetBlankVectors2MatrixTask()
        (rt, outOIDs,amdTaskID)=fcdc.LaunchVectors2MatrixTask(computePrx,sampleIDs,0,binCount)

        preFinishedCnt=0
        amdTaskFinished=False
        bdvd_log("")
        bdvd_log("Merging into big matrix ...")
        #bdvd_log("final sample IDs: {0}".format(str(outOIDs)))
        while (not amdTaskFinished):
            (rt,amdTaskInfo)=fcdcPrx.GetAMDTaskInfo(amdTaskID)
            if preFinishedCnt<amdTaskInfo.FinishedCnt:
                preFinishedCnt = amdTaskInfo.FinishedCnt
                bdvd_log("batch processed: {0}/{1}".format(preFinishedCnt, amdTaskInfo.TotalCnt))
            if amdTaskInfo.TotalCnt==amdTaskInfo.FinishedCnt:
                amdTaskFinished = True;
            else:
                time.sleep(4)
        
        bdvd_log("Merging into big matrix [done]")

        #recalculate statistics
        bigMatrixID= outOIDs[0]
        bmPrx=facetAdminPrx.GetBigMatrixFacet(bigMatrixID)
        (rt, amdTaskID)=bmPrx.RecalculateObserverStats(250)
        preFinishedCnt=0
        amdTaskFinished=False
        bdvd_log("")
        bdvd_log("Calculting statistics for big matrix ...")
        while (not amdTaskFinished):
            (rt,amdTaskInfo)=fcdcPrx.GetAMDTaskInfo(amdTaskID)
            if preFinishedCnt<amdTaskInfo.FinishedCnt:
                preFinishedCnt = amdTaskInfo.FinishedCnt
                bdvd_log("batch processed: {0}/{1}".format(preFinishedCnt, amdTaskInfo.TotalCnt))
            if amdTaskInfo.TotalCnt==amdTaskInfo.FinishedCnt:
                amdTaskFinished = True;
            else:
                time.sleep(4)
        
        bdvd_log("Calculting statistics for big matrix [done]")
        
        (rt, osis)=fcdcPrx.GetObserversStats(outOIDs)
        #import code
        #code.interact(local=locals())

        #now perpare output infomation
        (rt,bigmat_store_pathprefix)=fcdcPrx.GetFeatureValuePathPrefix(outOIDs[0])
        bdvd_log("bigmat store: {0}".format(bigmat_store_pathprefix))
        bigmat = iBSDefines.BigMatrixMetaInfo()
        bigmat.Name = gParams.workflow_node
        bigmat.ColStats=osis
        bigmat.StorePathPrefix = bigmat_store_pathprefix
        bigmat.ColIDs = outOIDs
        bigmat.ColNames= [s.SampleName for s in samples]
        bigmat.RowCnt = binCount
        bigmat.ColCnt=len(outOIDs)

        bam2mat=iBSDefines.Bam2MatOutputDefine(bigmat,binmap)
        
        dumpOutput(bam2mat)
        exportTxtInfo()

        shutdownFCDCentral()

        finish_time = datetime.now()
        duration = finish_time - start_time
        bdvd_logp("-----------------------------------------------")
        bdvd_log("Run complete: %s elapsed" %  iBSUtil.formatTD(duration))

    except Usage as err:
        shutdownFCDCentral()
        bdvd_logp(sys.argv[0].split("/")[-1] + ": " + str(err.msg))
        bdvd_logp("    for detailed help see url ...")
        return 2
    
    except:
        bdvd_logp(traceback.format_exc())
        die()


if __name__ == "__main__":
    sys.exit(main())
