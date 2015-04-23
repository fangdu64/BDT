#!__PYTHON_BIN_PATH__

"""
command line tool for bdvd
"""
import os
import sys, traceback
import getopt
import subprocess
import shutil
import time
from datetime import datetime, date
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
import iBSDefines
import bdtUtil
import bigKmeansUtil
import iBSFCDClient as fcdc
import bigMatUtil
import bdvdUtil
import iBS
import Ice

gParams=None
gRunner=None
gSteps = ['1-data-mat', '2-ctrl-rows','3-run-ruv','4-run-vd']
gCtrlRowsMethods = ["all","weak-signal","lower-quantile", "specified-rows"]
gPreNormalizationMethods = ["column-sum"]
gRuvTypes = ["ruvs","ruvg"]
gRuvScaleMethods = ["mlog"]

class BDVDParams:
    def __init__(self):
        self.output_dir = None
        self.logging_dir = None
        self.start_from = gSteps[0]
        self.dry_run = True
        self.remove_before_run = True
        self.pipeline_rundir = None
        self.workercnt = 4
        self.memory_size = 2000
        self.data_params = None
        self.data_indir = None
        self.pre_normalization = None
        self.common_column_sum = None
        self.sample_groups = None
        self.ruv_scale = None
        self.ruv_mlog_c = 1.0
        self.ruv_type = None
        self.control_rows_method = gCtrlRowsMethods[0]
        self.weak_signal_lb = None
        self.weak_signal_ub = None
        self.lower_quantile_threshold = None
        self.all_in_quantile_fraction = 1.0
        self.ctrl_rows_params = None
        self.ctrl_rows_indir = None
        self.ruv_rowwise_adjust = None
        self.known_factors = None

    def parse_options(self, argv):
        dataParams = bigMatUtil.BigMatParams()
        ctrRowsParams = bigMatUtil.BigMatParams()
        args = argv[1:]
        args = dataParams.strip_argv('data-', args)
        args = ctrRowsParams.strip_argv('ctrl-rows-', args)
        dataArgv = dataParams.argv
        ctrlRowsArgv = ctrRowsParams.argv
        bdvdArgv = args
        try:
            opts, args = getopt.getopt(bdvdArgv, "",
                ["version",
                "help",
                "thread-num=",
                "memory-size=",
                "out=",
                "pre-normalization=",
                "common-column-sum=",
                "sample-groups=",
                "ruv-scale=",
                "ruv-mlog-c=",
                "ruv-type=",
                "control-rows-method=",
                "weak-signal-lb=",
                "weak-signal-ub=",
                "lower-quantile-threshold=",
                "all-in-quantile-fraction=",
                "control-rows-file=",
                "ruv-rowwise-adjust=",
                "known-factors=",
                "start-from="])

        except getopt.error as msg:
            raise iBSDefines.BdtUsage(msg)

        for option, value in opts:
            if option == "--version":
                print("bdvd v",bdtUtil.get_version())
                sys.exit(0)
            if option == "--help":
                raise iBSDefines.BdtUsage(use_message)
            if option == "--thread-num":
                self.workercnt = int(value)
            if option == "--memory-size":
                self.memory_size = int(value)
            if option in ("-o", "--out"):
                self.output_dir = value
            if option == "--pre-normalization":
                allowedValues = gPreNormalizationMethods;
                if value not in allowedValues:
                    raise iBSDefines.BdtUsage('--pre-normalization should be one of the {0}'.format(allowedValues))
                self.pre_normalization = value
            if option == "--common-column-sum":
                if value == "median":
                    self.common_column_sum = None
                else:
                    self.common_column_sum = float(value)
            if option =="--sample-groups":
                self.sample_groups = bdtUtil.parseIntSeqSeq(value)
                if len(self.sample_groups)<1:
                    raise iBSDefines.BdtUsage('invalid sample-groups: {0}'.format(value))
            if option == "--ruv-scale":
                allowedValues = gRuvScaleMethods;
                if value not in allowedValues:
                    raise iBSDefines.BdtUsage('--ruv-scale should be one of the {0}'.format(allowedValues))
                self.ruv_scale = value
            if option == "--ruv-mlog-c":
                self.ruv_mlog_c = float(value)
            if option == "--ruv-type":
                allowedValues = gRuvTypes;
                if value not in allowedValues:
                    raise iBSDefines.BdtUsage('--ruv-type should be one of the {0}'.format(allowedValues))
                self.ruv_type = value
            if option == "--control-rows-method":
                allowedValues = gCtrlRowsMethods;
                if value not in allowedValues:
                    raise iBSDefines.BdtUsage('--control-rows-method should be one of the {0}'.format(allowedValues))
                self.control_rows_method = value
            if option == "--weak-signal-lb":
                self.weak_signal_lb = float(value)
            if option == "--weak-signal-ub":
                self.weak_signal_ub = float(value)
            if option == "--lower-quantile-threshold":
                self.lower_quantile_threshold = float(value)
            if option == "--all-in-quantile-fraction":
                self.all_in_quantile_fraction = float(value)
            if option == "--ruv-rowwise-adjust":
                self.ruv_rowwise_adjust = value
            if option =="--known-factors":
                self.known_factors = bdtUtil.parseIntSeqSeq(value)
                if len(self.known_factors)<1:
                    raise iBSDefines.BdtUsage('invalid known-factors: {0}'.format(value))
            if option =="--start-from":
                allowedValues = gSteps;
                if value not in allowedValues:
                    raise iBSDefines.BdtUsage('--start-from should be one of the {0}'.format(allowedValues))
                self.start_from = value

        requiredNames = ['--out', '--sample-groups', '--ruv-type']
        providedValues = [self.output_dir, self.sample_groups, self.ruv_type]
        noneIdx = bdtUtil.getFirstNone(providedValues)
        if noneIdx != -1:
            raise Usage("{0} is required".format(requiredNames[noneIdx]))

        self.output_dir = os.path.abspath(self.output_dir)
        self.logging_dir = os.path.abspath(self.output_dir + "/logs")
        self.pipeline_rundir=os.path.abspath(self.output_dir + "/run")

         # parse options for data
        self.data_indir =os.path.abspath("{0}/{1}".format(self.pipeline_rundir, gSteps[0]))
        dataArgv.extend(['--data-out',self.data_indir])
        self.data_params = bigMatUtil.BigMatParams()
        self.data_params.parse_options("data-", dataArgv)

        if self.control_rows_method == "specified-rows" or len(ctrlRowsArgv) > 0:
            self.ctrl_rows_indir=os.path.abspath("{0}/{1}".format(self.pipeline_rundir, gSteps[1]))
            ctrlRowsArgv.extend(['--ctrl-rows-out',self.ctrl_rows_indir])
            self.ctrl_rows_params = bigMatUtil.BigMatParams()
            self.ctrl_rows_params.parse_options("ctrl-rows-", ctrlRowsArgv)

        return args


def s01_data_mat():
    nodeName = gSteps[0]
    return bigMatUtil.run_bigMat(
        gRunner,
        Platform,
        BDT_HomeDir,
        nodeName,
        gParams.pipeline_rundir,
        gParams.dry_run,
        gParams.remove_before_run,
        gParams.data_params)

def s02_ctrlrows():
    nodeName = gSteps[1]
    return bigMatUtil.run_bigMat(
        gRunner,
        Platform,
        BDT_HomeDir,
        nodeName,
        gParams.pipeline_rundir,
        gParams.dry_run,
        gParams.remove_before_run,
        gParams.ctrl_rows_params)

def s03_RUV(datamatPickle, ctrlRowPickle):
    nodeName = gSteps[2]
    featureIdxFrom = None
    featureIdxTo = None
    return bdvdUtil.run_bdvd_ruv(
        gRunner,
        platform,
        BDT_HomeDir,
        nodeName,
        gParams.pipeline_rundir,
        gParams.dry_run,
        gParams.remove_before_run,
        datamatPickle,
        ctrlRowPickle,
        gParams.sample_groups,
        gParams.known_factors,
        gParams.pre_normalization,
        gParams.common_column_sum,
        gParams.ruv_type,
        gParams.ruv_rowwise_adjust,
        gParams.control_rows_method,
        gParams.weak_signal_lb,
        gParams.weak_signal_ub,
        gParams.lower_quantile_threshold,
        gParams.all_in_quantile_fraction,
        featureIdxFrom,
        featureIdxTo,
        gParams.workercnt,
        gParams.memory_size)

def main(argv=None):
    global gParams
    global gRunner
    gParams = BDVDParams()
    gRunner = bdtUtil.bdtRunner()
    run_argv = sys.argv[:]

    try:
        if argv is None:
            argv = sys.argv
        args = gParams.parse_options(argv)
       
        start_time = datetime.now()

        gRunner.prepare_dirs(gParams.output_dir, gParams.logging_dir, gParams.pipeline_rundir)
        gRunner.init_logger(os.path.abspath(gParams.logging_dir + "/bdvd.log"))

        gRunner.logp()
        gRunner.log("Beginning bdvd run v({0})".format(bdtUtil.get_version()))
        gRunner.logp("-----------------------------------------------")

        # -----------------------------------------------------------
        # prepare bigmat
        # -----------------------------------------------------------
        if gParams.dry_run and gParams.start_from == gSteps[0]:
            gParams.dry_run = False

        datamatPickle = s01_data_mat()
        if gParams.dry_run and not os.path.exists(datamatPickle):
            gRunner.log("retrieve existing result for: {0}".format(gSteps[0]))
            gRunner.log("from: {0}".format(datamatPickle))
            gRunner.die('file not exist')
            gRunner.log("")

        ctrlRowsPickle = None
        if gParams.ctrl_rows_params is not None:
            if gParams.dry_run and gParams.start_from == gSteps[1]:
                gParams.dry_run = False
            ctrlRowsPickle = s02_ctrlrows()
            if gParams.dry_run and not os.path.exists(ctrlRowsPickle):
                gRunner.log("retrieve existing result for: {0}".format(gSteps[1]))
                gRunner.log("from: {0}".format(ctrlRowsPickle))
                gRunner.die('file not exist')
                gRunner.log("")

        if gParams.dry_run and gParams.start_from == gSteps[2]:
            gParams.dry_run = False

        s03_RUV(datamatPickle, ctrlRowPickle)

        runSummary = iBSDefines.NodeRunSummaryDefine()
        runSummary.NodeDir = gParams.output_dir
        runSummary.NodeType = "bdvd"
        runSummaryPicke = "{0}/logs/runSummary.pickle".format(gParams.output_dir)
        iBSDefines.dumpPickle(runSummary, runSummaryPicke)

        finish_time = datetime.now()
        duration = finish_time - start_time
        gRunner.logp("-----------------------------------------------")
        gRunner.log("Run complete: %s elapsed" %  bdtUtil.formatTD(duration))

    except Usage as err:
        gRunner.logp(sys.argv[0].split("/")[-1] + ": " + str(err.msg))
        return 2
    
    except:
        gRunner.logp(traceback.format_exc())
        gRunner.die()

if __name__ == "__main__":
    sys.exit(main())
