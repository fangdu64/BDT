#!/dcs01/gcode/fdu1/BDT/python/bin/python3.3

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
import pickle
import importlib

import iBSConfig
import iBSUtil
import iBSDefines
import iBSFCDClient as fcdc
import iBS
import Ice

use_message = '''

Usage:
    bdvd-ruv-annotation.py [options] <design-file>

Options:
    -v/--version
    -o/--output-dir                <string>    [ default: ./ruvs_out       ]
    -p/--num-threads               <int>       [ default: 4                ]
    -m/--max-mem                   <int>       [ default: 2000            ]
    --tmp-dir                      <dirname>   [ default: <output_dir>/tmp ]

Advanced Options:
    --place-holder

'''

class Usage(Exception):
    def __init__(self, msg):
        self.msg = msg

output_dir = "./prediction_out/"
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
        self.fcdc_fvworker_size=2
        self.fcdc_tcp_port=16000
        self.fcdc_threadpool_size=4
        self.workflow_node = "ruv-prediction"
        self.result_dumpfile = None
        self.design_file = None
        self.input_pickle = None
        self.bdt_home = None
        self.R_bin = iBSConfig.R_BinDir
        self.resume = False
        self.KsY=None
        self.NsY=None
        self.KsX=None
        self.NsX=None
        self.MatDirs=None
        self.OnewayConfig=True
        
    def parse_options(self, argv):
        try:
            opts, args = getopt.getopt(argv[1:], "hvp:m:o:",
                                        ["version",
                                         "help",
                                         "output-dir=",
                                         "num-threads=",
                                         "max-mem=",
                                         "node=",
                                         "tmp-dir=",
                                         "bdt-home",
                                         "resume"])
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
                self.fcdc_fvworker_size = 2
            if option in ("-m", "--max-mem"):
                self.max_mem = int(value)
            if option in ("-o", "--output-dir"):
                custom_out_dir = value + "/"
                self.resume_dir = value
            if option == "--tmp-dir":
                custom_tmp_dir = value + "/"
            if option == "--node":
                self.workflow_node = value
            if option =="--bdt-home":
                self.bdt_home = value
            if option =="--resume":
                self.resume = True

        self.result_dumpfile = "{0}.pickle".format(self.workflow_node)
        if custom_out_dir:
            output_dir = custom_out_dir
            logging_dir = output_dir + "logs/"
            tmp_dir = output_dir + "tmp/"
            fcdcentral_dir = output_dir + "fcdcentral/"
            script_dir = output_dir + "script/"
        if custom_tmp_dir:
            tmp_dir = custom_tmp_dir

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
    sys.exit(1)

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

    shutil.copy(gParams.design_file,script_dir)

    if not os.path.exists(tmp_dir):
        try:
          os.makedirs(tmp_dir)
        except OSError as o:
          die("\nError creating directory %s (%s)" % (tmp_dir, o))


def exportPairedMats():
    global gParams
    designPath=os.path.abspath(script_dir)
    if designPath not in sys.path:
         sys.path.append(designPath)
    import ruvAnnotateDesign as design
    pairedMatNames=[
            "MatX",
            "MatY"]
    pairedMatDesigns=[
            design.MatXDesign,
            design.MatYDesign]
    matDirs=[]
    for i in range(len(pairedMatDesigns)):
        shutil.copy(pairedMatDesigns[i],"{0}/{1}.py".format(script_dir,pairedMatNames[i]))
        matDir=exportOneMat(pairedMatNames[i])
        matDirs.append(matDir)
   
    gParams.KsX=design.KsX
    gParams.NsX=design.NsX
    gParams.KsY=design.KsY
    gParams.NsY=design.NsY
    gParams.MatDirs=matDirs
    gParams.OnewayConfig=design.OnewayConfig

def exportOneMat(matName):
    if gParams.resume:
        return "{0}{1}".format(output_dir,matName)
    
    designPath=os.path.abspath(script_dir)
    if designPath not in sys.path:
         sys.path.append(designPath)
    design = importlib.import_module(matName)
    Ks=design.Ks
    Ns=design.Ns
    RUVOutputMode = design.RUVOutputMode
    sampleIDs=design.Samples
    design_params=(Ks,Ns,RUVOutputMode,sampleIDs)
    export_nd_pickle_fn="{0}/export_design.pickle".format(designPath)
    iBSDefines.dumpPickle(design_params, export_nd_pickle_fn)

    #prepare feature idxs to export
    featureIdxs=[]
    for line in open(design.UniqueFeatureIdxsFile):
        fields=line.rstrip('\n').split('\t')
        featureIdxs.append(int(fields[0]))
    rowIdxs = iBSDefines.FeatureIdxsOutputDefine(featureIdxs)
    rowIdxs_pickle="{0}/export_rowIdxs.pickle".format(designPath)
    iBSDefines.dumpPickle(rowIdxs, rowIdxs_pickle)

    #prepare script to run ruv-exportbyrowidxs node
    export_nd_design_fn="{0}/bdvdRuvExportByRowIdxsDesign.py".format(designPath)
    outfile = open(export_nd_design_fn, "w")
    outfile.write("import pickle\n")
    outfile.write("import iBSDefines\n")
    outfile.write("import iBS\n")
    outfile.write("Ks,Ns,RUVOutputMode,sampleIDs=iBSDefines.loadPickle('{0}')\n".format(export_nd_pickle_fn))
    outfile.close()

    #export
    cmdpath="bdvd-ruv-exportbyrowidxs.py"
    subnode=matName
    if gParams.bdt_home is not None:
        cmdpath="{0}/bdvd-ruv-exportbyrowidxs.py".format(gParams.bdt_home)
    export_cmd = [cmdpath,
                "--node","{0}".format(subnode),
                "--num-threads", "{0}".format(gParams.num_threads),
                "--max-mem","{0}".format(gParams.max_mem),
                "--output-dir","{0}{1}".format(output_dir,subnode),
                "--ruv-dir", "{0}".format(design.RUVDir),
                "--rowidxs", "{0}".format(rowIdxs_pickle),
                export_nd_design_fn]
      
    shell_cmd=""
    for str in export_cmd:
        shell_cmd=shell_cmd+str+" "
    print(shell_cmd)

    bdvd_logp("run subtask at: {0}{1}".format(output_dir,subnode))
    proc = subprocess.call(export_cmd)
    subnode_picke_file="{0}{1}/{2}.pickle".format(output_dir,subnode,subnode)

    #prepare matrix info for R
    #output matrixs
    obj = iBSDefines.loadPickle(subnode_picke_file)
    matrix_info_fn="{0}{1}/matrix_info.txt".format(output_dir,subnode);
    outf = open(matrix_info_fn, "w")
    outf.write("{0}\t{1}\t{2}\t{3}\t{4}\t{5}\t{6}\n".format("MatrixID","DataFile","k","extW","RowCnt","ColCnt","RUVOutMode"))
    Ks=obj.Ks
    for i in range(len(Ks)):
        outf.write("{0}\t{1}\t{2}\t{3}\t{4}\t{5}\t{6}\n".format(i+1,obj.BfvFiles[i],obj.Ks[i],obj.Ns[i],obj.RowCnt,obj.ColCnt,obj.RUVOutputMode))
    outf.close()
    
    #output data rowIDs
    shutil.copy(design.RowIDsFile,"{0}{1}/dataRowIDs.txt".format(output_dir,subnode))
    if design.LocationTblFile is not None:
        shutil.copy(design.LocationTblFile,"{0}{1}/locationTblFile.txt".format(output_dir,subnode))
        shutil.copy(design.LocationRowIDsFile,"{0}{1}/dataLocationTblRowIDs.txt".format(output_dir,subnode))

    #output colNames
    colnames_fn="{0}{1}/colnames.txt".format(output_dir,subnode)
    outf = open(colnames_fn, "w")
    for colname in obj.ColNames:
        outf.write("{0}\n".format(colname))
    outf.close()
    matDir="{0}{1}".format(output_dir,subnode)
    return matDir

def prepare_RScripts():
    RDir="{0}/R".format(output_dir)
    if not os.path.exists(RDir):
        os.mkdir(RDir)
    
    RScriptDir=os.path.abspath(RDir)

    # ==============================
    # Common.R
    infile = open("./iBS/iBS.R/BDVD/Common.R")
    outfile = open(RScriptDir+"/Common.R", "w")

    replacements = {"__NOTHING__":"__SOMETHING__"}

    for line in infile:
        for src, target in replacements.items():
            line = line.replace(src, target)
        outfile.write(line)
    infile.close()
    outfile.close()


    strOnewayConfig="TRUE"
    if not gParams.OnewayConfig:
        strOnewayConfig="FALSE"

    # ==============================
    # RUV_Fstats.R
    infile = open("./iBS/iBS.R/BDVD/RUV_Annotation.R")
    outfile = open(RScriptDir+"/RUV_Annotation.R", "w")
    replacements = {"__RSCRIPT_DIR__":RScriptDir,
                    "__XDataDir__":gParams.MatDirs[0],
                    "__YDataDir__":gParams.MatDirs[1],
                    "__KsY__":str(gParams.KsY).replace('[','').replace(']',''),
                    "__NsY__":str(gParams.NsY).replace('[','').replace(']',''),
                    "__KsX__":str(gParams.KsX).replace('[','').replace(']',''),
                    "__NsX__":str(gParams.NsX).replace('[','').replace(']',''),
                    "__OUT_DIR__":output_dir[:-1],
                    "__NUM_THREADS__":str(gParams.num_threads),
                    "__OnewayConfig__":strOnewayConfig
                    }
    for line in infile:
        for src, target in replacements.items():
            line = line.replace(src, target)
        outfile.write(line)
    infile.close()
    outfile.close()

##
## run RUV_Annotation.R
##
def runAnnotation():
    RDir="{0}/R".format(output_dir)
    RScriptDir=os.path.abspath(RDir)

    cmdpath="{0}/Rscript".format(gParams.R_bin)
    r_cmd = [cmdpath,
                "--no-restore",
                "--no-save",
                "{0}/{1}".format(RScriptDir,"RUV_Annotation.R")]
      
    shell_cmd=""
    for str in r_cmd:
        shell_cmd=shell_cmd+str+" "
    print(shell_cmd)
    subnode = "R"
    bdvd_logp("run subtask at: {0}{1}".format(output_dir,subnode))
    proc = subprocess.call(r_cmd)

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
        bdvd_log("Beginning RUV Annotation run (v"+iBSUtil.get_version()+")")
        bdvd_logp("-----------------------------------------------")

        exportPairedMats()
        prepare_RScripts()
        runAnnotation()
        bdvd_log("RUV Annotation [done]")

        finish_time = datetime.now()
        duration = finish_time - start_time
        bdvd_logp("-----------------------------------------------")
        bdvd_log("Run complete: %s elapsed" %  iBSUtil.formatTD(duration))

    except Usage as err:
        bdvd_logp(sys.argv[0].split("/")[-1] + ": " + str(err.msg))
        bdvd_logp("    for detailed help see url ...")
        return 2
    
    except:
        bdvd_logp(traceback.format_exc())
        die()


if __name__ == "__main__":
    sys.exit(main())
