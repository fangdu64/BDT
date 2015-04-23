#!__PYTHON_BIN_PATH__

"""
create data matrix from bam files
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

BDT_HomeDir=os.path.abspath(os.path.dirname(os.path.abspath(__file__))+"../../..")

Platform = None
if Platform == "Windows":
    # this file will be at install\
    bdtInstallDir = BDT_HomeDir
    icePyDir = os.path.abspath(bdtInstallDir+"/dependency/IcePy")
    bdtPyDir = os.path.abspath(bdtInstallDir+"/bdt/bdtPy")
    for dir in [icePyDir, bdtPyDir]:
        if dir not in sys.path:
            sys.path.append(dir)

import iBSConfig
iBSConfig.BDT_HomeDir = BDT_HomeDir

import bdtUtil
import iBSDefines
import iBSFCDClient as fcdc
import bigMatUtil
import iBS
import Ice

use_message = '''
bam2mat creats data matrix from BAM file samples.

Usage:
   bigmat-bam2mat [options] <design_file>

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

gParams=None
gRunner=None

class Usage(Exception):
    def __init__(self, msg):
        self.msg = msg

class BDVDParams:
    def __init__(self):
        self.output_dir = None
        self.bigmat_dir = None
        self.max_mem = 2000
        self.num_threads = 4
        self.fcdc_fvworker_size=self.num_threads
        self.fcdc_tcp_port=16000
        self.fcdc_threadpool_size=2
        self.workflow_node = "bam2mat"
        self.result_dumpfile = None
        self.design_file = None
        self.column_names = None

    def parse_options(self, argv):
        try:
            opts, args = getopt.getopt(argv[1:], "hvp:m:o:",
                                        ["version",
                                         "help",
                                         "out=",
                                         "num-threads=",
                                         "max-mem=",
                                         "node=",
                                         "bigmat-dir=",
                                         "col-names="])
        except getopt.error as msg:
            raise Usage(msg)

        # option processing
        for option, value in opts:
            if option in ("-v", "--version"):
                print("bam2mat v",bdtUtil.get_version())
                sys.exit(0)
            if option in ("-h", "--help"):
                raise Usage(use_message)
            if option in ("-p", "--num-threads"):
                self.num_threads = int(value)
                self.fcdc_fvworker_size = self.num_threads
            if option in ("-m", "--max-mem"):
                self.max_mem = int(value)
            if option in ("-o", "--out"):
                self.output_dir = value
            if option == "--node":
                self.workflow_node = value
            if option =="--bigmat-dir":
                self.bigmat_dir = value
            if option == "--col-names":
                self.column_names=value.split(',')
                if len(self.column_names)<1:
                    raise Usage("--col-names invalid")
        
        self.result_dumpfile = "{0}.pickle".format(self.workflow_node)
        self.output_dir = os.path.abspath(self.output_dir)

        if self.bigmat_dir is None:
            self.bigmat_dir = self.output_dir+"/bigmat"
        self.bigmat_dir = os.path.abspath(self.bigmat_dir)

        if len(args) < 1:
            raise Usage(use_message)
        self.design_file = args[0]
        return args

def prepare_output_dir():
    shutil.copy(gParams.design_file,
                os.path.abspath("{0}/bam2MatDesign.py".format(gRunner.script_dir)))

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
        fullname=os.path.abspath(dir+"/"+bam_file)
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
    designPath=os.path.abspath(gRunner.script_dir)
    if designPath not in sys.path:
         sys.path.append(designPath)
    import bam2MatDesign as design

    if (design.sample_table_lines is not None):
        sampleList=define_samples(design.sample_table_lines)
    else:
        sample_table_lines = design.sample_table_content.split('\n')
        sampleList=define_samples(sample_table_lines)

    binInfo=define_genomicBins(design.chromosomes, design.bin_width)
    #check configs
    return sampleList,binInfo

def exportTxtInfo():
    fn = "{0}/{1}".format(gParams.output_dir,gParams.result_dumpfile)
    bam2mat = iBSDefines.loadPickle(fn)
    binmap=bam2mat.BinMap
    #output binmap
    bin_info_fn="{0}/binmap_info.txt".format(gParams.output_dir);
    outf = open(bin_info_fn, "w")
    outf.write("{0}\t{1}\t{2}\n".format("RefName","BinFrom","BinTo"))
    for i in range(len(binmap.RefNames)):
        outf.write("{0}\t{1}\t{2}\n".format(binmap.RefNames[i],binmap.RefBinFroms[i],binmap.RefBinTos[i]))
    outf.close()
    
    #output matrixs
    obj=bam2mat.BigMat
    matrix_info_fn="{0}/matrix_info.txt".format(gParams.output_dir);
    outf = open(matrix_info_fn, "w")
    outf.write("{0}\t{1}\t{2}\t{3}\n".format("MatrixID","DataFile","RowCnt","ColCnt"))
    outf.write("{0}\t{1}\t{2}\t{3}\n".format(1,obj.StorePathPrefix,obj.RowCnt,obj.ColCnt))
    outf.close()

    #output colNames
    colnames_fn="{0}/colnames.txt".format(gParams.output_dir);
    outf = open(colnames_fn, "w")
    for colname in obj.ColNames:
        outf.write("{0}\n".format(colname))
    outf.close()

def dumpOutput(bam2mat):
    fn = "{0}/{1}".format(gParams.output_dir,gParams.result_dumpfile)
    iBSDefines.dumpPickle(bam2mat,fn)

def main(argv=None):
    global gParams
    global gRunner
    gParams = BDVDParams()
    gRunner = bigMatUtil.bigMatRunner(iBSConfig.BDT_HomeDir)

    run_argv = sys.argv[:]

    try:
        if argv is None:
            argv = sys.argv
        args = gParams.parse_options(argv)
       
        start_time = datetime.now()

        gRunner.prepare_dirs(gParams.output_dir, gParams.bigmat_dir)
        prepare_output_dir()
        gRunner.init_logger("bam2mat.log")

        gRunner.log("Threads used: {0} for processing BAM files, additioanl {1} for supporting".format(gParams.fcdc_fvworker_size, gParams.fcdc_threadpool_size))
        gRunner.logp()
        gRunner.log("Beginning bam2mat run (v"+bdtUtil.get_version()+")")
        gRunner.logp("-----------------------------------------------")


        gParams.fcdc_tcp_port = bdtUtil.getUsableTcpPort()
        gRunner.prepare_bigmat_config(gParams.fcdc_tcp_port, 
                                  gParams.fcdc_fvworker_size, 
                                  gParams.fcdc_threadpool_size)

        sampleList,binInfo = getTaskConfig()

        gRunner.launch_bigMat()
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
    
        gRunner.log("bigMat activated")
        
        intersectRefs = bdtUtil.getIntersectRefsFromBamSamples(sampleList,samplePrx)
        gRunner.log("Sample Num: {0}, Common chromosomes: {1}".format(len(sampleList), str(intersectRefs)))

        if (binInfo.RefNames is None) or (len(binInfo.RefNames)==0):
            binInfo.RefNames=intersectRefs
        else:
            for ref in binInfo.RefNames:
                if ref not in intersectRefs:
                    gRunner.die(ref +" is not present in all samples")
        gRunner.log("chromosomes considered: {0}".format(str(binInfo.RefNames)))
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

        gRunner.log("")
        (rt, sampleIDs, amdTaskID) = samplePrx.CreateBamToBinCountSamples(samples)

        preFinishedCnt=0
        amdTaskFinished=False
        gRunner.log("Extracting bin values ...")
        gRunner.log("Sample IDs: {0}".format(str(list(range(1,len(samples)+1)))))
        gRunner.log("sample processed: {0}/{1}".format(0, len(samples)))
        while (not amdTaskFinished):
            (rt,amdTaskInfo)=fcdcPrx.GetAMDTaskInfo(amdTaskID)
            if preFinishedCnt<amdTaskInfo.FinishedCnt:
                preFinishedCnt = amdTaskInfo.FinishedCnt
                gRunner.log("Sample Processed: {0}/{1}".format(preFinishedCnt, amdTaskInfo.TotalCnt))
            if amdTaskInfo.TotalCnt==amdTaskInfo.FinishedCnt:
                amdTaskFinished = True;
            else:
                time.sleep(4)
        minSampleID = min(sampleIDs)
        (rt, osis)=fcdcPrx.GetObserversStats(sampleIDs)
        binCount = int(osis[0].Cnt)
        gRunner.log("Bin Count: "+str(binCount))
        for osi in osis:
            gRunner.logp("Sample {0}: Max = {1}, Min = {2}, Sum = {3}".format(osi.ObserverID-minSampleID+1, int(osi.Max), int(osi.Min), int(osi.Sum)))
        gRunner.log("Extracting bin values [done]")
        
        #combine into big matrix
        computePrx.GetBlankVectors2MatrixTask()
        (rt, outOIDs,amdTaskID)=fcdc.LaunchVectors2MatrixTask(computePrx,sampleIDs,0,binCount)

        preFinishedCnt=0
        amdTaskFinished=False
        gRunner.log("")
        gRunner.log("Merging into big matrix ...")
        #bdvd_log("final sample IDs: {0}".format(str(outOIDs)))
        while (not amdTaskFinished):
            (rt,amdTaskInfo)=fcdcPrx.GetAMDTaskInfo(amdTaskID)
            if preFinishedCnt<amdTaskInfo.FinishedCnt:
                preFinishedCnt = amdTaskInfo.FinishedCnt
                gRunner.log("batch processed: {0}/{1}".format(preFinishedCnt, amdTaskInfo.TotalCnt))
            if amdTaskInfo.TotalCnt==amdTaskInfo.FinishedCnt:
                amdTaskFinished = True;
            else:
                time.sleep(4)

        gRunner.log("Merging into big matrix [done]")

        #recalculate statistics
        bigMatrixID= outOIDs[0]
        bmPrx=facetAdminPrx.GetBigMatrixFacet(bigMatrixID)
        (rt, amdTaskID)=bmPrx.RecalculateObserverStats(250)
        preFinishedCnt=0
        amdTaskFinished=False
        gRunner.log("")
        gRunner.log("Calculting statistics for big matrix ...")
        while (not amdTaskFinished):
            (rt,amdTaskInfo)=fcdcPrx.GetAMDTaskInfo(amdTaskID)
            if preFinishedCnt<amdTaskInfo.FinishedCnt:
                preFinishedCnt = amdTaskInfo.FinishedCnt
                gRunner.log("batch processed: {0}/{1}".format(preFinishedCnt, amdTaskInfo.TotalCnt))
            if amdTaskInfo.TotalCnt==amdTaskInfo.FinishedCnt:
                amdTaskFinished = True;
            else:
                time.sleep(4)

        gRunner.log("Calculting statistics for big matrix [done]")
        
        (rt, osis)=fcdcPrx.GetObserversStats(outOIDs)

        #now perpare output infomation
        (rt,bigmat_store_pathprefix)=fcdcPrx.GetFeatureValuePathPrefix(outOIDs[0])
        bigmat_store_pathprefix = os.path.abspath(bigmat_store_pathprefix)
        gRunner.log("bigmat store: {0}".format(bigmat_store_pathprefix))
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

        gRunner.shutdown_bigMat()

        finish_time = datetime.now()
        duration = finish_time - start_time
        gRunner.logp("-----------------------------------------------")
        gRunner.log("Run complete: %s elapsed" %  bdtUtil.formatTD(duration))

    except Usage as err:
        gRunner.shutdown_bigMat()
        gRunner.logp(sys.argv[0].split("/")[-1] + ": " + str(err.msg))
        return 2
    
    except:
        gRunner.logp(traceback.format_exc())
        gRunner.die()


if __name__ == "__main__":
    sys.exit(main())
