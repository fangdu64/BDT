#!__PYTHON_BIN_PATH__

"""
command line tool for bigmat export
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
gSteps = ['1-data-mat', '2-row-idxs','3-run-export']
gRowSelectors = ['all', 'range', 'specified-rows']

class BDVDParams:
    def __init__(self):
        self.output_dir = None
        self.logging_dir = None
        self.start_from = gSteps[0]
        self.dry_run = True
        self.remove_before_run = True
        self.pipeline_rundir = None
        self.data_params = None
        self.data_indir = None
        self.row_selector = gRowSelectors[0]
        self.rowidx_from = None
        self.rowidx_to = None
        self.rowidxs_params = None
        self.rowidxs_indir = None
        self.column_ids = None
        self.export_name = None

    def parse_options(self, argv):
        dataParams = bigMatUtil.BigMatParams()
        rowIdxsParams = bigMatUtil.BigMatParams()
        args = argv[1:]
        args = dataParams.strip_argv('data-', args)
        args = rowIdxsParams.strip_argv('rowidxs-', args)
        dataArgv = dataParams.argv
        rowIdxsArgv = rowIdxsParams.argv
        bdvdArgv = args
        try:
            opts, args = getopt.getopt(bdvdArgv, "",
                ["version",
                "help",
                "thread-num=",
                "memory-size=",
                "out=",
                "column-ids=",
                "row-selector=",
                "rowidx-from=",
                "rowidx-to=",
                "export-name="])

        except getopt.error as msg:
            raise iBSDefines.BdtUsage(msg)

        for option, value in opts:
            if option == "--version":
                print("bdvd v",bdtUtil.get_version())
                sys.exit(0)
            if option == "--help":
                raise iBSDefines.BdtUsage(use_message)
            if option in ("-o", "--out"):
                self.output_dir = value
            if option == "--row-selector":
                allowedValues = gRowSelectors;
                if value not in allowedValues:
                    raise iBSDefines.BdtUsage('--row-selector should be one of the {0}'.format(allowedValues))
                self.row_selector = value
            if option == "--rowidx-from":
                self.rowidx_from = int(value)
            if option == "--rowidx-to":
                self.rowidx_to = int(value)
            if option =="--column-ids":
                self.column_ids = bdtUtil.parseIntSeq(value)
                if len(self.column_ids)<1:
                    raise iBSDefines.BdtUsage('invalid column-ids: {0}'.format(value))
            if option =="--start-from":
                allowedValues = gSteps;
                if value not in allowedValues:
                    raise iBSDefines.BdtUsage('--start-from should be one of the {0}'.format(allowedValues))
                self.start_from = value
            if option =="--export-name":
                self.export_name=value
        requiredNames = ['--out']
        providedValues = [self.output_dir]
        noneIdx = bdtUtil.getFirstNone(providedValues)
        if noneIdx != -1:
            raise iBSDefines.BdtUsage("{0} is required".format(requiredNames[noneIdx]))

        self.output_dir = os.path.abspath(self.output_dir)
        self.logging_dir = os.path.abspath(self.output_dir + "/logs")
        self.pipeline_rundir=os.path.abspath(self.output_dir + "/run")

        # parse options for data
        self.data_indir =os.path.abspath("{0}/{1}".format(self.pipeline_rundir, gSteps[0]))
        dataArgv.extend(['--data-out',self.data_indir])
        self.data_params = bigMatUtil.BigMatParams()
        self.data_params.parse_options("data-", dataArgv)

        if self.row_selector == "specified-rows" or len(rowIdxsArgv) > 0:
            self.rowidxs_indir=os.path.abspath("{0}/{1}".format(self.pipeline_rundir, gSteps[0]))
            rowIdxsArgv.extend(['--rowidxs-out',self.rowidxs_indir])
            self.rowidxs_params = bigMatUtil.BigMatParams()
            self.rowidxs_params.parse_options("rowidxs-", rowIdxsArgv)
        
        if self.export_name is None:
            self.export_name = 'export'
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

def s02_rowidxs():
    nodeName = gSteps[1]
    return bigMatUtil.run_bigMat(
        gRunner,
        Platform,
        BDT_HomeDir,
        nodeName,
        gParams.pipeline_rundir,
        gParams.dry_run,
        gParams.remove_before_run,
        gParams.rowidxs_params)

def s03_run_export(datamatPickle, rowIdxsPickle):
    nodeName = gSteps[2]
    return bdvdUtil.run_bigmat_export(
        gRunner,
        Platform,
        BDT_HomeDir,
        nodeName,
        gParams.pipeline_rundir,
        gParams.dry_run,
        gParams.remove_before_run,
        datamatPickle,
        rowIdxsPickle,
        gParams.row_selector,
        gParams.rowidx_from,
        gParams.rowidx_to,
        gParams.column_ids,
        gParams.export_name);

def outputR():
    obj = iBSDefines.loadPickle(
        iBSDefines.derivePickleFile(gParams.output_dir))

    infile = open("{0}/bdt/bdtR/outputTemplates/bigMatOutputTemplate.R".format(BDT_HomeDir))
    outfile = open("{0}/logs/output.R".format(gParams.output_dir), "w")

    bigmat = obj.Export

    replacements = {"__NAME__": bigmat.Name,
                    "__STORE_PATH_PREFIX__": bigmat.StorePathPrefix.replace('\\','/'),
                    "__ROW_CNT__":str(bigmat.RowCnt),
                    "__COL_CNT__":str(bigmat.ColCnt),
                    "__COL_NAMES__":str(bigmat.ColNames).replace('[','').replace(']',''),
                    "__COL_IDS__":str(bigmat.ColIDs).replace('[','').replace(']','')
                    }

    for line in infile:
        for src, target in replacements.items():
            line = line.replace(src, target)
        outfile.write(line)
    infile.close()
    outfile.close()

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
        gRunner.init_logger(os.path.abspath(gParams.logging_dir + "/bigmat-export.log"))

        gRunner.logp()
        gRunner.log("Beginning bigmat export run v({0})".format(bdtUtil.get_version()))
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

        rowIdxsPickle = None
        if gParams.rowidxs_params is not None:
            if gParams.dry_run and gParams.start_from == gSteps[1]:
                gParams.dry_run = False
            rowIdxsPickle = s02_rowidxs()
            if gParams.dry_run and not os.path.exists(rowIdxsPickle):
                gRunner.log("retrieve existing result for: {0}".format(gSteps[1]))
                gRunner.log("from: {0}".format(rowIdxsPickle))
                gRunner.die('file not exist')
                gRunner.log("")

        if gParams.dry_run and gParams.start_from == gSteps[2]:
            gParams.dry_run = False

        s03_run_export(datamatPickle, rowIdxsPickle)

        runSummary = iBSDefines.NodeRunSummaryDefine()
        runSummary.NodeDir = gParams.output_dir
        runSummary.NodeType = "bigmat-export"
        runSummaryPicke = "{0}/logs/runSummary.pickle".format(gParams.output_dir)
        iBSDefines.dumpPickle(runSummary, runSummaryPicke)

        # dump pickles generated from each step into one pickle
        allResults = iBSDefines.BigmatExportResultsDefine()
        bigmatExportPickle = os.path.abspath("{0}/run/{1}/{1}.pickle".format(gParams.output_dir, gSteps[2]))
        allResultsPicke = os.path.abspath("{0}/logs/results.pickle".format(gParams.output_dir))
        allResults.Export = iBSDefines.loadPickle(bigmatExportPickle)
        iBSDefines.dumpPickle(allResults, allResultsPicke)

        outputR()

        finish_time = datetime.now()
        duration = finish_time - start_time
        gRunner.logp("-----------------------------------------------")
        gRunner.log("Run complete: %s elapsed" %  bdtUtil.formatTD(duration))

    except iBSDefines.BdtUsage as err:
        gRunner.logp(sys.argv[0].split("/")[-1] + ": " + str(err.msg))
        return 2
    
    except:
        gRunner.logp(traceback.format_exc())
        gRunner.die()

if __name__ == "__main__":
    sys.exit(main())
