import sys, traceback, Ice
import time
import bdtUtil
import iBS

def exportMatByRange(bdt_log,fcdcPrx, computePrx, task):
    (rt,amdTaskID)=computePrx.ExportRowMatrix(task)
    preMsg=""
    amdTaskFinished=False
    while (not amdTaskFinished):
        (rt,amdTaskInfo)=fcdcPrx.GetAMDTaskInfo(amdTaskID)
        thisMsg="task: {0}, batch processed: {1}/{2}".format(amdTaskInfo.TaskName, amdTaskInfo.FinishedCnt, amdTaskInfo.TotalCnt)
        if preMsg!=thisMsg:
            preMsg = thisMsg
            bdt_log(thisMsg)
        if amdTaskInfo.Status!=iBS.AMDTaskStatusEnum.AMDTaskStatusNormal:
            amdTaskFinished = True;
        else:
            time.sleep(1)