import sys
import os
import socket
import logging
import subprocess
import shutil
import time
from datetime import datetime, date

import iBSDefines

# singleNodeKmeansRunner
class singleNodeKmeansRunner:
    def __init__(self, bdtHomeDir):
        self.bdt_home_dir = bdtHomeDir
        self.output_dir = None
        self.logging_dir = None
        self.bdvd_logger = None # main logging object
        self.bdvd_log_handle = None #main log file handle
        self.script_dir = None

        self.bigmat_dir = None
        self.bigmat_popen = None
        self.bigmat_log_file = None

        self.kmeanss_dir = None
        self.kmeanss_popen = None
        self.kmeanss_log_file = None

        self.kmeansc_dir = None
        self.kmeansc_popen = None
        self.kmeansc_log_file = None

        self.kmeansc_workercnt = None
        self.kmeansc_ramsize = None


    # Ensures that the output, logging, and temp directories are present. If not,
    # they are created
    def prepare_dirs(self, outDir, bigMatDir):
        self.output_dir = outDir
        self.logging_dir = os.path.abspath(self.output_dir + "/logs")
        self.script_dir = os.path.abspath(self.output_dir + "/script")
        self.bigmat_dir = bigMatDir
        self.kmeanss_dir = os.path.abspath(self.output_dir + "/kmeanss")
        self.kmeansc_dir = os.path.abspath(self.output_dir + "/kmeansc")

        if not os.path.exists(self.output_dir):
            os.mkdir(self.output_dir)

        if not os.path.exists(self.logging_dir):
            os.mkdir(self.logging_dir)
    
        if not os.path.exists(self.script_dir):
            os.mkdir(self.script_dir)

        if not os.path.exists(self.bigmat_dir):
            os.mkdir(self.bigmat_dir)

        db_dir=os.path.abspath(self.bigmat_dir+"/FCDCentralDB")
        if not os.path.exists(db_dir):
            os.mkdir(db_dir)

        fvstore_dir=os.path.abspath(self.bigmat_dir+"/FeatureValueStore")
        if not os.path.exists(fvstore_dir):
            os.mkdir(fvstore_dir)

        if not os.path.exists(self.kmeanss_dir):
            os.mkdir(self.kmeanss_dir)

        if not os.path.exists(self.kmeansc_dir):
            os.mkdir(self.kmeansc_dir)

    def init_logger(self, log_name):
        log_fname = os.path.abspath(self.logging_dir + "/" + log_name)
        self.bdvd_logger = logging.getLogger('project')
        formatter = logging.Formatter('%(asctime)s %(message)s', '[%Y-%m-%d %H:%M:%S]')
        self.bdvd_logger.setLevel(logging.DEBUG)

        # output logging information to stderr
        hstream = logging.StreamHandler(sys.stderr)
        hstream.setFormatter(formatter)
        self.bdvd_logger.addHandler(hstream)
    
        #
        # Output logging information to file
        if os.path.isfile(log_fname):
            os.remove(log_fname)
        logfh = logging.FileHandler(log_fname)
        logfh.setFormatter(formatter)
        self.bdvd_logger.addHandler(logfh)
        self.bdvd_log_handle=logfh.stream
    
    def log(self, out_str):
      if self.bdvd_logger:
           self.bdvd_logger.info(out_str)

    # error msg
    def logp(self, out_str=""):
        print(out_str,file=sys.stderr)
        if self.bdvd_log_handle:
            print(out_str, file=self.bdvd_log_handle)

    def prepare_bigmat_config(self, tcpPort,fvWorkerSize, iceThreadPoolSize ):
        infile = open("{0}/bdt/config/FCDCentralServer.config".format(self.bdt_home_dir))
        outfile = open(self.bigmat_dir+"/FCDCentralServer.config", "w")

        replacements = {"__FCDCentral_TCP_PORT__":str(tcpPort), 
                        "__FeatureValueWorker.Size__":str(fvWorkerSize), 
                        "__Ice.ThreadPool.Server.Size__":str(iceThreadPoolSize)}

        for line in infile:
            for src, target in replacements.items():
                line = line.replace(src, target)
            outfile.write(line)
        infile.close()
        outfile.close()
    
    def launch_bigMat(self):
        bigmat_path = os.path.abspath("{0}/bdt/bin/bigMat".format(self.bdt_home_dir))
        bm_cmd = [bigmat_path]
        self.log("Launching bigMat ...")
        self.bigmat_log_file = open(os.path.abspath(self.logging_dir + "/bigmat.log"),"w")
        self.bigmat_popen = subprocess.Popen(bm_cmd, cwd=self.bigmat_dir, stdout=self.bigmat_log_file)
    
    def shutdown_bigMat(self):
        if self.bigmat_popen is not None:
            self.bigmat_popen.terminate()
            self.bigmat_popen.wait()
            self.bigmat_popen = None
            self.log("bigMat shutdown")
        if self.bigmat_log_file is not None:
            self.bigmat_log_file.close()

    def prepare_kmeansserver_config(self, fcdc_tcp_port,kmeanss_tcp_port):
        infile = open("{0}/bdt/config/KMeansServer.config".format(self.bdt_home_dir))
        outfile = open(self.kmeanss_dir+"/KMeansServer.config", "w")
        replacements = {"__FCDCentral_TCP_PORT__":str(fcdc_tcp_port),
                        "__KMeansServer_TCP_PORT__":str(kmeanss_tcp_port)}
        for line in infile:
            for src, target in replacements.items():
                line = line.replace(src, target)
            outfile.write(line)
        infile.close()
        outfile.close()

    def launchKMeansServer(self):
        kmeansserver_path = os.path.abspath("{0}/bdt/bin/bigKmeansServer".format(self.bdt_home_dir))
        kmeanss_cmd = [kmeansserver_path]
        self.log("Launching bigMat ...")
        self.kmeanss_log_file = open(os.path.abspath(self.logging_dir + "/kmeanss.log"),"w")
        self.kmeanss_popen = subprocess.Popen(kmeanss_cmd, cwd=self.kmeanss_dir, stdout=self.kmeanss_log_file)
    
    def shutdown_kmeanss(self):
        if self.kmeanss_popen is not None:
            self.kmeanss_popen.terminate()
            self.kmeanss_popen.wait()
            self.kmeanss_popen = None
            self.log("bigKmeansServer shutdown")
        if self.kmeanss_log_file is not None:
            self.kmeanss_log_file.close()

    def prepare_kmeansc_config(self, fcdc_tcp_port, kmeansc_tcp_port):
        designPath=os.path.abspath(self.script_dir)
        if designPath not in sys.path:
             sys.path.append(designPath)
        import bigclustKMeansPPDesign as design
        self.kmeansc_workercnt =design.KMeansContractorWorkerCnt
        self.kmeansc_ramsize =design.KMeansContractorRAMSize

        infile = open("{0}/bdt/config/KMeansContractor.config".format(self.bdt_home_dir))
        outfile = open(self.kmeansc_dir+"/KMeansContractor.config", "w")
        replacements = {"__FCDCentral_TCP_PORT__":str(fcdc_tcp_port),
                        "__FCDCentral_HOST__":"localhost",
                        "__KMeansContractor_TCP_PORT__":str(kmeansc_tcp_port),
                        "__KMeansContractor_WorkerCnt__":str(design.KMeansContractorWorkerCnt),
                        "__KMeansContractor_RAMSIZE__":str(design.KMeansContractorRAMSize)}
        for line in infile:
            for src, target in replacements.items():
                line = line.replace(src, target)
            outfile.write(line)
        infile.close()
        outfile.close()

    def launchKMeansContractor(self):
        kmeanscontractor_path = os.path.abspath("{0}/bdt/bin/bigKmeansContractor".format(self.bdt_home_dir))
        kmeansc_cmd = [kmeanscontractor_path]
        self.log("Launching bigKmeansContractor ...")
        self.kmeansc_log_file = open(os.path.abspath(self.logging_dir + "/kmeansc.log"),"w")
        self.kmeansc_popen = subprocess.Popen(kmeansc_cmd, cwd=self.kmeansc_dir, stdout=self.kmeansc_log_file)

    def shutdown_kmeansc(self):
        if self.kmeansc_popen is not None:
            self.kmeansc_popen.terminate()
            self.kmeansc_popen.wait()
            self.kmeansc_popen = None
            self.log("bigKmeansContractor shutdown")
        if self.kmeansc_log_file is not None:
            self.kmeansc_log_file.close()

    def die(self, msg=None):
        if msg is not None:
            self.logp(msg)
        self.shutdown_kmeansc()
        self.shutdown_kmeanss()
        self.shutdown_bigMat()   
        sys.exit(1)
