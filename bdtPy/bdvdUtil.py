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
    ruv_scale,
    ruv_rowwise_adjust,
    control_rows_method,
    weak_signal_lb,
    weak_signal_ub,
    lower_quantile_threshold,
    all_in_quantile_fraction,
    featureIdxFrom,
    featureIdxTo,
    workercnt,
    memory_size,
    permutation_cnt
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

    RUVScale = iBS.RUVInputAdjustEnum.RUVInputDoLogE
    if ruv_scale == "mlog2":
        RUVScale = iBS.RUVInputAdjustEnum.RUVInputDoLog2
    elif ruv_scale == "original":
        RUVScale = iBS.RUVInputAdjustEnum.RUVInputDoNothing

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
                   RUVScale,
                   ControlFeaturePolicy,
                   ControlFeatureMaxCntLowBound,
                   ControlFeatureMaxCntUpBound,
                   CtrlQuantile,
                   AllInQuantileFraction,
                   MaxK,
                   featureIdxFrom,
                   featureIdxTo,
                   permutation_cnt)
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

bdvd_export_usage = '''
bdvd-export
'''

def get_ruv_output_mode(
    component,
    artifact_detection):
    RUVOutputMode = None
    if component == 'signal' and artifact_detection == 'aggressive':
        RUVOutputMode = iBS.RUVOutputModeEnum.RUVOutputModeZYthenGroupMean
    elif component == 'signal' and artifact_detection == 'conservative':
        RUVOutputMode = iBS.RUVOutputModeEnum.RUVOutputModeXb
    elif component == 'artifact' and artifact_detection == 'aggressive':
        RUVOutputMode = iBS.RUVOutputModeEnum.RUVOutputModeZY
    elif component == 'artifact' and artifact_detection == 'conservative':
        RUVOutputMode = iBS.RUVOutputModeEnum.RUVOutputModeWa
    elif component == 'random' and artifact_detection == 'aggressive':
        RUVOutputMode = iBS.RUVOutputModeEnum.RUVOutputModeZYGetE
    elif component == 'random' and artifact_detection == 'conservative':
        RUVOutputMode = iBS.RUVOutputModeEnum.RUVOutputModeYminusWaXb
    elif component == 'signal+random' and artifact_detection == 'aggressive':
        RUVOutputMode = iBS.RUVOutputModeEnum.RUVOutputModeYminusZY
    elif component == 'signal+random' and artifact_detection == 'conservative':
        RUVOutputMode = iBS.RUVOutputModeEnum.RUVOutputModeYminusWa
    return RUVOutputMode

def get_ruv_output_scale(scale):
    RUVOutputScale = None
    if scale == 'mlog':
        RUVOutputScale = iBS.RUVOutputScaleEnum.RUVOutputScaleLog
    elif scale == 'original':
        RUVOutputScale = iBS.RUVOutputScaleEnum.RUVOutputScaleRaw
    return RUVOutputScale

def run_bdvd_ruv_export(
    gRunner,
    platform,
    bdtHomeDir,
    nodeName,
    runDir,
    dryRun,
    removeBeforeRun,
    rowIdxsPickle,
    row_selector,
    rowidx_from,
    rowidx_to,
    column_ids,
    workercnt,
    memory_size,
    bdvd_dir,
    component,
    scale,
    artifact_detection,
    unwanted_factors,
    known_factors,
    export_names
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

    bdvd_obj = iBSDefines.loadPickle(
        iBSDefines.derivePickleFile(bdvd_dir))
    bigmat_dir = None
    if bdvd_obj.RuvOut is not None:
        bigmat_dir = bdvd_obj.RuvOut.BigMatDir

    RUVOutputMode = get_ruv_output_mode(component, artifact_detection)

    RUVOutputScale = get_ruv_output_scale(scale)

    design_params=(export_names,
                   unwanted_factors,
                   known_factors,
                   RUVOutputMode,
                   RUVOutputScale,
                   workercnt,
                   column_ids,
                   rowidx_from,
                   rowidx_to)
    params_pickle_fn=os.path.abspath("{0}/design_params.pickle".format(nodeScriptDir))
    iBSDefines.dumpPickle(design_params, params_pickle_fn)

    design_file=os.path.abspath(bdtHomeDir+"/bdt/bdtPy/PipelineDesigns/bdvdRuvExportDesign.py")
    shutil.copy(design_file,nodeScriptDir)

    #
    # Run node
    #
    design_fn=os.path.abspath(nodeScriptDir)+"/bdvdRuvExportDesign.py"
    cmdpath=os.path.abspath("{0}/bdt/bdtCmds/bdvd-ruv-export".format(bdtHomeDir))
    if platform == "Windows":
        node_cmd = ["py", cmdpath]
    else:
        node_cmd = [cmdpath]
    node_cmd.extend(["--node", nodeName,
                "--num-threads", str(workercnt),
                "--max-mem", str(memory_size),
                "--out", nodeDir,
                "--bigmat-dir", bigmat_dir])
    
    if rowIdxsPickle is not None:
        node_cmd.extend(["--export-rows", rowIdxsPickle])
    
    node_cmd.append(design_fn)

    gRunner.logp("run subtask at: {0}".format(nodeDir))
    proc = subprocess.call(node_cmd)
    gRunner.logp("end subtask \n")
    return (nodeDir,out_picke_file)

def run_bdvd_ruv_rowselection(
    gRunner,
    platform,
    bdtHomeDir,
    nodeName,
    runDir,
    dryRun,
    removeBeforeRun,
    row_selector,
    rowidx_from,
    rowidx_to,
    column_ids,
    workercnt,
    memory_size,
    bdvd_dir,
    component,
    scale,
    artifact_detection,
    unwanted_factors,
    known_factors,
    with_signal_threshold,
    with_signal_col_cnt,
    with_signal_sampling_row_cnt,
    output_file):

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

    bdvd_obj = iBSDefines.loadPickle(
        iBSDefines.derivePickleFile(bdvd_dir))
    bigmat_dir = None
    if bdvd_obj.RuvOut is not None:
        bigmat_dir = bdvd_obj.RuvOut.BigMatDir

    RUVOutputMode = get_ruv_output_mode(component, artifact_detection)

    RUVOutputScale = get_ruv_output_scale(scale)

    design_params=(unwanted_factors,
                   known_factors,
                   RUVOutputMode,
                   RUVOutputScale,
                   workercnt,
                   column_ids,
                   rowidx_from,
                   rowidx_to,
                   row_selector,
                   with_signal_threshold,
                   with_signal_col_cnt,
                   with_signal_sampling_row_cnt,
                   output_file)
    params_pickle_fn=os.path.abspath("{0}/design_params.pickle".format(nodeScriptDir))
    iBSDefines.dumpPickle(design_params, params_pickle_fn)

    design_file=os.path.abspath(bdtHomeDir+"/bdt/bdtPy/PipelineDesigns/bdvdRuvRowSelectionDesign.py")
    shutil.copy(design_file,nodeScriptDir)

    #
    # Run node
    #
    design_fn=os.path.abspath(nodeScriptDir)+"/bdvdRuvRowSelectionDesign.py"
    cmdpath=os.path.abspath("{0}/bdt/bdtCmds/bdvd-ruv-rowselection".format(bdtHomeDir))
    if platform == "Windows":
        node_cmd = ["py", cmdpath]
    else:
        node_cmd = [cmdpath]
    node_cmd.extend(["--node", nodeName,
                "--num-threads", str(workercnt),
                "--max-mem", str(memory_size),
                "--out", nodeDir,
                "--bigmat-dir", bigmat_dir])

    node_cmd.append(design_fn)

    gRunner.logp("run subtask at: {0}".format(nodeDir))
    proc = subprocess.call(node_cmd)
    gRunner.logp("end subtask \n")
    return (nodeDir,out_picke_file)

def run_bdvd_ruv_vd(
    gRunner,
    platform,
    bdtHomeDir,
    nodeName,
    runDir,
    dryRun,
    removeBeforeRun,
    rowidx_from,
    rowidx_to,
    workercnt,
    memory_size,
    bdvd_dir,
    artifact_detection,
    unwanted_factors,
    known_factors,
    output_file
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

    bdvd_obj = iBSDefines.loadPickle(
        iBSDefines.derivePickleFile(bdvd_dir))
    bigmat_dir = None
    if bdvd_obj.RuvOut is not None:
        bigmat_dir = bdvd_obj.RuvOut.BigMatDir

    RUVOutputMode = get_ruv_output_mode('signal+random', artifact_detection)

    design_params=(output_file,
                   unwanted_factors,
                   known_factors,
                   RUVOutputMode,
                   rowidx_from,
                   rowidx_to)
    params_pickle_fn=os.path.abspath("{0}/design_params.pickle".format(nodeScriptDir))
    iBSDefines.dumpPickle(design_params, params_pickle_fn)

    design_file=os.path.abspath(bdtHomeDir+"/bdt/bdtPy/PipelineDesigns/bdvdRuvVdDesign.py")
    shutil.copy(design_file,nodeScriptDir)

    #
    # Run node
    #
    design_fn=os.path.abspath(nodeScriptDir)+"/bdvdRuvVdDesign.py"
    cmdpath=os.path.abspath("{0}/bdt/bdtCmds/bdvd-ruv-vd".format(bdtHomeDir))
    if platform == "Windows":
        node_cmd = ["py", cmdpath]
    else:
        node_cmd = [cmdpath]
    node_cmd.extend(["--node", nodeName,
                "--num-threads", str(workercnt),
                "--max-mem", str(memory_size),
                "--out", nodeDir,
                "--bigmat-dir", bigmat_dir])

    node_cmd.append(design_fn)

    gRunner.logp("run subtask at: {0}".format(nodeDir))
    proc = subprocess.call(node_cmd)
    gRunner.logp("end subtask \n")
    return (nodeDir,out_picke_file)
