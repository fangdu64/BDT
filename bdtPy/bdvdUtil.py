import sys
import os
import getopt
import socket
import logging
import subprocess
import shutil
import time
from datetime import datetime, date
import iBSDefines
import iBS
import bdtUtil

bdvd_use_message = '''
bdvd
Usage:
    bdvd.py <input-options> <ruv-options> <output-options> <--out out_dir>

<input-options>:
--data-input   <string>    [ bam-files, text-mat, binary-mat ]

<pre-normalization>:
--pre-normalization   <string>    [ none, column-sum]
for [common-column-sum]:
    --common-column-sum [int], if not set, use median of column sums

<ruv-options>:
--sample-groups     <string> e.g., [1,2],[3,4],[5,6]
--ruv-scale  <string>    [ none, mlog (moderated log link: log(x+c))] default mlog
--ruv-mlog-c     <double>    [ default 1.0]
--ruv-type   <string>    [ ruvg, ruvs, ruvr ]
for [ruvs] and [ruvg]:
    required options:
    --control-rows-method   <string>    [all,weak-signal,lower-quantile,specified-rows] default all
    for [weak-signal]
    --weak-signal-lb    <double>
    --weak-signal-ub    <double>
    for [lower-quantile]
    --lower-quantile-threshold   <double> default 0.5
    --all-in-quantile-fraction   <double> default 1
    for [specified-rows]
    --control-rows-file  <string> a text file with each row as control row index (0-based)
    --ruv-row-adjust    <string>    [none, unitary-length]

optional options:
   --known-factors     <string> e.g., [1,1,1,1,0,0], default is none


other options:
--thread-num        <int> default 4
--memory-size      <int> MB default 2000
--start-from    <string> [1-input-mat (default), 2-perform-ruv, 3-perform-vd, 4-output]
'''

def run_bdvd_ruv(
    gRunner,
    platform,
    bdtHomeDir,
    nodeName,
    runDir,
    dryRun,
    removeBeforeRun,
    datamatPickle,
    ctrlRowPickle,
    sampleGroups,
    KnownFactors,
    pre_normalization,
    common_column_sum,
    ruv_type,
    ruv_rowwise_adjust,
    control_rows_method,
    weak_signal_lb,
    weak_signal_ub,
    lower_quantile_threshold,
    all_in_quantile_fraction,
    featureIdxFrom,
    featureIdxTo,
    workercnt,
    memory_size
    ):

    nodeDir = os.path.abspath("{0}/{1}".format(runDir, nodeName))

    out_picke_file = os.path.abspath("{0}/{1}.pickle".format(nodeDir,nodeName))
    if dryRun:
        return out_picke_file

    if removeBeforeRun and os.path.exists(nodeDir):
        gRunner.logp("remove existing dir: {0}".format(nodeDir))
        shutil.rmtree(nodeDir)
    
    nodeScriptDir = nodeDir + "-script"
    if not os.path.exists(nodeScriptDir):
        os.mkdir(nodeScriptDir)

    #
    # prepare design file
    #
    if pre_normalization is None:
        CommonLibrarySize = 0
    elif pre_normalization == "column-sum" and common_column_sum is None:
        CommonLibrarySize = -1 # use median column-sum
    else:
        CommonLibrarySize = common_column_sum

    if ruv_type == "ruvs" and ruv_rowwise_adjust is None:
        RUVMode = iBS.RUVModeEnum.RUVModeRUVs
    elif ruv_type == "ruvs" and ruv_rowwise_adjust == "unitary-length":
        RUVMode = iBS.RUVModeEnum.RUVModeRUVsForVariation
    elif ruv_type == "ruvg" and ruv_rowwise_adjust is None:
        RUVMode = iBS.RUVModeEnum.RUVModeRUVg
    elif ruv_type == "ruvg" and ruv_rowwise_adjust == "unitary-length":
        RUVMode = iBS.RUVModeEnum.RUVModeRUVgForVariation

    ControlFeaturePolicy = iBS.RUVControlFeaturePolicyEnum.RUVControlFeaturePolicyNone
    ControlFeatureMaxCntLowBound = None
    ControlFeatureMaxCntUpBound = None
    CtrlQuantile = None
    AllInQuantileFraction = None
    if control_rows_method == "all":
        ControlFeaturePolicy = iBS.RUVControlFeaturePolicyEnum.RUVControlFeaturePolicyNone
    elif control_rows_method == "weak-signal":
        ControlFeaturePolicy = iBS.RUVControlFeaturePolicyEnum.RUVControlFeaturePolicyMaxCntLow
        ControlFeatureMaxCntLowBound = weak_signal_lb
        ControlFeatureMaxCntUpBound = weak_signal_ub
    elif control_rows_method == "lower-quantile":
        ControlFeaturePolicy = iBS.RUVControlFeaturePolicyEnum.RUVControlFeaturePolicyAllInLowerQuantile
        CtrlQuantile = lower_quantile_threshold
        AllInQuantileFraction = all_in_quantile_fraction
    elif control_rows_method == "specified-rows":
        ControlFeaturePolicy = iBS.RUVControlFeaturePolicyEnum.RUVControlFeaturePolicyFeatureIdxList

    MaxK = len(sampleGroups)

    design_params=(sampleGroups,
                   KnownFactors,
                   CommonLibrarySize,
                   RUVMode,
                   ControlFeaturePolicy,
                   ControlFeatureMaxCntLowBound,
                   ControlFeatureMaxCntUpBound,
                   CtrlQuantile,
                   AllInQuantileFraction,
                   MaxK,
                   featureIdxFrom,
                   featureIdxTo)
    params_pickle_fn=os.path.abspath("{0}/design_params.pickle".format(nodeScriptDir))
    iBSDefines.dumpPickle(design_params, params_pickle_fn)

    design_file=os.path.abspath(bdtHomeDir+"/bdt/bdtPy/PipelineDesigns/bdvdRUVDesign.py")
    shutil.copy(design_file,nodeScriptDir)

    #
    # Run node
    #
    design_fn=os.path.abspath(nodeScriptDir)+"/bdvdRUVDesign.py"
    cmdpath=os.path.abspath("{0}/bdt/bdtCmds/bdvd-ruv".format(bdtHomeDir))
    if platform == "Windows":
        node_cmd = ["py", cmdpath]
    else:
        node_cmd = [cmdpath]
    node_cmd.extend(["--node", nodeName,
                "--num-threads", str(workercnt),
                "--max-mem", str(memory_size),
                "--out", nodeDir,
                "--data-mat", datamatPickle])
    
    if ctrlRowPickle is not None:
        node_cmd.extend(["--ctrl-rows", ctrlRowPickle])
    
    node_cmd.append(design_fn)

    gRunner.logp("run subtask at: {0}".format(nodeDir))
    proc = subprocess.call(node_cmd)
    gRunner.logp("end subtask \n")
    return (nodeDir,out_picke_file)
