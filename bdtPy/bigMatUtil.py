import sys
import os
import socket
import logging
import subprocess

# bigMatRunner
class bigMatRunner:
    def __init__(self, bdtHomeDir):
        self.bdt_home_dir = bdtHomeDir
        self.output_dir = None
        self.logging_dir = None
        self.bigmat_popen = None
        self.bigmat_log_file = None
        self.bdvd_logger = None # main logging object
        self.bdvd_log_handle = None #main log file handle
        self.bigmat_dir = None
        self.script_dir = None

    # Ensures that the output, logging, and temp directories are present. If not,
    # they are created
    def prepare_dirs(self, outDir, bigMatDir):
        self.output_dir = outDir
        self.logging_dir = self.output_dir + "/logs"
        self.bigmat_dir = bigMatDir
        self.script_dir = self.output_dir + "/script"

        if not os.path.exists(self.output_dir):
            os.mkdir(self.output_dir)

        if not os.path.exists(self.logging_dir):
            os.mkdir(self.logging_dir)
    
        if not os.path.exists(self.script_dir):
            os.mkdir(self.script_dir)

        if not os.path.exists(self.bigmat_dir):
            os.mkdir(self.bigmat_dir)

        db_dir=self.bigmat_dir+"/FCDCentralDB"
        if not os.path.exists(db_dir):
            os.mkdir(db_dir)

        fvstore_dir=self.bigmat_dir+"/FeatureValueStore"
        if not os.path.exists(fvstore_dir):
            os.mkdir(fvstore_dir)

    def init_logger(self, log_name):
        log_fname = self.logging_dir + "/" + log_name
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
        bigmat_path = "{0}/bdt/bin/bigMat".format(self.bdt_home_dir)
        bm_cmd = [bigmat_path]
        self.log("Launching bigMat ...")
        self.bigmat_log_file = open(self.logging_dir + "/bigmat.log","w")
        self.bigmat_popen = subprocess.Popen(bm_cmd, cwd=self.bigmat_dir, stdout=self.bigmat_log_file)
    
    def shutdown_bigMat(self):
        if self.bigmat_popen is not None:
            self.bigmat_popen.terminate()
            self.bigmat_popen.wait()
            self.bigmat_popen = None
            self.log("bigMat shutdown")
        if self.bigmat_log_file is not None:
            self.bigmat_log_file.close()

    def die(self, msg=None):
        if msg is not None:
            self.logp(msg)
        self.shutdown_bigMat()
        sys.exit(1)