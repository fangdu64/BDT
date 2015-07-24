#!__PYTHON_BIN_PATH__

"""
bigclust-kmeans++ contractor
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
import random

BDT_HomeDir=os.path.dirname(os.path.abspath(__file__))

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
import bigKmeansUtil
import iBSDefines
import iBSFCDClient as fcdc
import bigMatUtil
import iBS
import Ice

use_message = '''
KMeans++ (multihreads, contractor)

'''

gParams=None
gRunner=None

class Usage(Exception):
    def __init__(self, msg):
        self.msg = msg

class BDVDParams:

    def __init__(self):
        self.output_dir = None
        self.max_mem = 2000
        self.kmeansc_workercnt = 4
        self.workflow_node = "kmeans-contractor"
        self.design_file = None
        self.kmeansc_tcp_port=16011
        self.kmeansc_ramsize =4000
        self.kmeanss_port=16010
        self.kmeanss_hosts=None

    def parse_options(self, argv):
        try:
            opts, args = getopt.getopt(argv[1:], "hv",
                                        ["version",
                                         "help",
                                         "out=",
                                         "thread-num=",
                                         "max-mem=",
                                         "node=",
                                         "master-host=",
                                         "master-port="])
        except getopt.error as msg:
            raise Usage(msg)

        # option processing
        for option, value in opts:
            if option in ("-v", "--version"):
                print("bigKmeans v",bdtUtil.get_version())
                sys.exit(0)
            if option in ("-h", "--help"):
                raise Usage(use_message)
            if option == "--thread-num":
                self.kmeansc_workercnt = int(value)
            if option == "--max-mem":
                self.max_mem = int(value)
            if option == "--out":
                self.output_dir = value
            if option == "--node":
                self.workflow_node = value
            if option == "--master-host":
                self.kmeanss_hosts = value.split(',')
            if option == "--master-port":
                self.kmeanss_port = int(value)

        self.result_dumpfile = "{0}.pickle".format(self.workflow_node)
        self.output_dir = os.path.abspath(self.output_dir)
        return args


def main(argv=None):

    # Initialize default parameter values
    global gParams
    global gRunner
    gParams = BDVDParams()
    gRunner = bigKmeansUtil.kmeansContractorRunner(iBSConfig.BDT_HomeDir)
    run_argv = sys.argv[:]

    try:
        if argv is None:
            argv = sys.argv
        args = gParams.parse_options(argv)
       
        start_time = datetime.now()

        gRunner.prepare_dirs(gParams.output_dir)
        gRunner.init_logger("kmeans++.log")

        fcdc.Init();

        kmeanssHost="KMeanServerAdminService -t -e 1.1"
        for host in gParams.kmeanss_hosts:
            kmeanssHost = "{0}:tcp -h {1} -p {2}".format(kmeanssHost, host, gParams.kmeanss_port)
        kmeansSAdminPrx = None
        tryCnt=0
        while (tryCnt<10) and (kmeansSAdminPrx is None):
            try:
                kmeansSAdminPrx=fcdc.GetKMeanSAdminProxy(kmeanssHost)
            except Ice.ConnectionRefusedException as ex:
                tryCnt=tryCnt+1
                time.sleep(1)

        if kmeansSAdminPrx is None:
            raise Usage("connection timeout")

        (rt,retProj) = kmeansSAdminPrx.GetActiveProject()

        kmeansServerPrx=kmeansSAdminPrx.GetKMeansSeverProxy()

        # -----------------------------------------------------------
        # launch KMeans Contractor
        # -----------------------------------------------------------
        gParams.kmeansc_tcp_port = bdtUtil.getUsableTcpPort()
        gRunner.prepare_kmeansc_config(gParams.kmeansc_tcp_port, gParams.kmeansc_workercnt)

        gRunner.launchKMeansContractor()

        kmeanscHost="KMeanContractorAdminService:default -h localhost -p {0}".format(gParams.kmeansc_tcp_port)
        kmeansCAdminPrx = None
        tryCnt=0
        while (tryCnt<20) and (kmeansCAdminPrx is None):
            try:
                kmeansCAdminPrx=fcdc.GetKMeanCAdminProxy(kmeanscHost)
            except Ice.ConnectionRefusedException as ex:
                tryCnt=tryCnt+1
                time.sleep(1)

        if kmeansCAdminPrx is None:
            raise Usage("connection timeout")  
        gRunner.log("KMeans Contractor activated")

        # -----------------------------------------------------------
        # Run KMeans
        # -----------------------------------------------------------
        kmeansCAdminPrx.StartNewContract(retProj.ProjectID,kmeansServerPrx)
        amdTaskID = retProj.RunKmeansTaskId
        preMsg=""
        amdTaskFinished=False
        gRunner.log("")
        gRunner.log("Running KMeans++ with {0} threads".format(gParams.kmeansc_workercnt))
        while (not amdTaskFinished):
            try:
                (rt,amdTaskInfo)=kmeansSAdminPrx.GetAMDTaskInfo(amdTaskID)
                thisMsg="task: {0}, batch processed: {1}/{2}".format(amdTaskInfo.TaskName, amdTaskInfo.FinishedCnt, amdTaskInfo.TotalCnt)
                if preMsg!=thisMsg:
                    preMsg = thisMsg
                    gRunner.log(thisMsg)
                if amdTaskInfo.Status!=iBS.AMDTaskStatusEnum.AMDTaskStatusNormal:
                    amdTaskFinished = True;
                else:
                    time.sleep(4)
            except Ice.ConnectionRefusedException as ex:
                    amdTaskFinished= True

        gRunner.log("Running KMeans [done]")

        gRunner.shutdown_kmeansc()

        finish_time = datetime.now()
        duration = finish_time - start_time
        gRunner.logp("-----------------------------------------------")
        gRunner.log("Run complete: %s elapsed" %  bdtUtil.formatTD(duration))

    except Usage as err:
        gRunner.shutdown_kmeansc()
        gRunner.logp(sys.argv[0].split("/")[-1] + ": " + str(err.msg))
        gRunner.logp("    for detailed help see url ...")
        return 2
    
    except:
        gRunner.logp(traceback.format_exc())
        gRunner.die()


if __name__ == "__main__":
    sys.exit(main())
