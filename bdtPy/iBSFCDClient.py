import sys, traceback, Ice
import iBSConfig
import bdtUtil

iBS_SliceDir= iBSConfig.BDT_HomeDir+"/bdt/slice/bdt"
iBS_LoadSliceFlag="--all -I"+iBS_SliceDir

Ice.loadSlice(iBS_LoadSliceFlag,[iBS_SliceDir+"/FCDCentralService.ice"])
Ice.loadSlice(iBS_LoadSliceFlag,[iBS_SliceDir+"/SampleService.ice"])
Ice.loadSlice(iBS_LoadSliceFlag,[iBS_SliceDir+"/ComputeService.ice"])
Ice.loadSlice(iBS_LoadSliceFlag,[iBS_SliceDir+"/KMeanService.ice"])
Ice.loadSlice(iBS_LoadSliceFlag,[iBS_SliceDir+"/JointAMRService.ice"])

import iBS

__ic =None

def Init():
    global __ic
    try:
        initData= Ice.InitializationData()
        props= Ice.createProperties()
        # Configure MessageSizeMax (278528 262144-256M, 131072-128M 65536-64M, 32768 - 32M, 16384 -16M)
        props.setProperty("Ice.MessageSizeMax", "278528")
        initData.properties = props;
        __ic = Ice.initialize(initData)
    except:
        traceback.print_exc()
        status = 1

def UnInit():
    global __ic
    if __ic:
        try:
            __ic.destroy()
        except:
            traceback.print_exc()
            status = 1

def GetFCDCProxy(serverhostname = "localhost -p 16000"):
    global __ic
    base = __ic.stringToProxy("FCDCentralService:default -h "+serverhostname)
    fcdcPrx = iBS.FcdcAdminServicePrx.checkedCast(base)
    return fcdcPrx

def GetPCProxy(serverhostname = "localhost -p 16000"): 
    global __ic
    try:
        base = __ic.stringToProxy("ProxyCentralService:default -h "+serverhostname)

        pcPrx = iBS.ProxyCentralServicePrx.checkedCast(base)
        if not pcPrx:
            raise RuntimeError("Invalid proxy")
        else:
            return pcPrx
    except:
        traceback.print_exc()
        status = 1

def GetFacetAdminProxy(serverhostname = "localhost -p 16000"): 
    global __ic
    try:
        base = __ic.stringToProxy("FcdcFacetAdminService:default -h "+serverhostname)

        facetAdminPrx = iBS.FcdcFacetAdminServicePrx.checkedCast(base)
        if not facetAdminPrx:
            raise RuntimeError("Invalid proxy")
        else:
            return facetAdminPrx
    except:
        traceback.print_exc()
        status = 1

def GetBdvdFacetAdminProxy(serverhostname = "localhost -p 16000"): 
    global __ic
    try:
        base = __ic.stringToProxy("BdvdFacetAdminService:default -h "+serverhostname)

        facetAdminPrx = iBS.FcdcFacetAdminServicePrx.checkedCast(base)
        if not facetAdminPrx:
            raise RuntimeError("Invalid proxy")
        else:
            return facetAdminPrx
    except:
        traceback.print_exc()
        status = 1

def GetSeqSampleProxy(serverhostname = "localhost -p 16000"): 
    global __ic
    try:
        base = __ic.stringToProxy("SampleService:default -h "+serverhostname)

        seqSamplePrx = iBS.SeqSampleServicePrx.checkedCast(base)
        if not seqSamplePrx:
            raise RuntimeError("Invalid proxy")
        else:
            return seqSamplePrx
    except:
        traceback.print_exc()
        status = 1

def GetComputeProxy(serverhostname = "localhost -p 16000"): 
    global __ic
    try:
        base = __ic.stringToProxy("ComputeService:default -h "+serverhostname)

        computePrx = iBS.ComputeServicePrx.checkedCast(base)
        if not computePrx:
            raise RuntimeError("Invalid proxy")
        else:
            return computePrx
    except:
        traceback.print_exc()
        status = 1

def GetJointAMRProxy(serverhostname = "localhost -p 16000"): 
    global __ic
    try:
        base = __ic.stringToProxy("JointAMRService:default -h "+serverhostname)

        amrPrx = iBS.JointAMRServicePrx.checkedCast(base)
        if not amrPrx:
            raise RuntimeError("Invalid proxy")
        else:
            return amrPrx
    except:
        traceback.print_exc()
        status = 1

def GetKMeanSAdminProxy(proxyStr = "KMeanServerAdminService:default -h localhost -p 16100"): 
    global __ic
    base = __ic.stringToProxy(proxyStr)
    sadminPrx = iBS.KMeanServerAdminServicePrx.checkedCast(base)
    return sadminPrx

def GetKMeanServerProxy(proxyStr = "KMeanServerService:default -h localhost -p 16100"): 
    global __ic
    try:
        base = __ic.stringToProxy(proxyStr)
        ssPrx = iBS.KMeanServerServicePrx.checkedCast(base)
        if not ssPrx:
            raise RuntimeError("Invalid proxy")
        else:
            return ssPrx
    except:
        traceback.print_exc()
        status = 1

def GetKMeanCAdminProxy(proxyStr = "KMeanContractorAdminService:default -h localhost -p 16150"): 
    global __ic
    base = __ic.stringToProxy(proxyStr)
    caPrx = iBS.KMeanContractorAdminServicePrx.checkedCast(base)
    return caPrx

def GetFeatureDomain(fcdcPrx, domainID):
    (rt, domains)= fcdcPrx.GetFeatureDomains([domainID])
    if len(domains) ==1:
        return domains[0]

def GetFeatureObserver(fcdcPrx, observerID):
    (rt, observers)= fcdcPrx.GetFeatureObservers([observerID])
    if len(observers) ==1:
        return observers[0]

def LaunchVectors2MatrixTask(computePrx, inOIDs, featureIdxFrom, featureIdxTo):
    task = computePrx.GetBlankVectors2MatrixTask()
    task.InOIDs = inOIDs
    task.FeatureIdxFrom = featureIdxFrom
    task.FeatureIdxTo = featureIdxTo
    return computePrx.Vectors2Matrix(task)
  