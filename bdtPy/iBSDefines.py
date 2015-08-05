import os
import pickle

class BamSampleInfo:
    def __init__(self,
          name=None, bamfile=None):
        self.name = name
        self.bamfile = bamfile
        if self.name is None:
            self.name=os.path.basename(self.bamfile)


class GenomicBinInfo:
    def __init__(self,
          RefNames=None, BinWidth=100):
        self.RefNames = RefNames
        self.BinWidth = BinWidth

class BigMatrixMetaInfo:
    def __init__(self):
        self.Name = None
        self.StorePathPrefix=None
        self.ColCnt = None
        self.RowCnt = None
        self.ColNames= None
        self.ColIDs = None
        self.ColStats=None

class BigVecMetaInfo:
    def __init__(self):
        self.Name = None
        self.StorePathPrefix=None
        self.RowCnt = None
        self.ColName= None
        self.ColID = None
        self.ColStats=None

class RefNoneoverlapBinMap:
    def __init__(self):
        self.RefNames = None
        self.BinWidth = None
        self.RefBinFroms = None
        self.RefBinTos = None

    def get_genomepos(binIdx):
        ref="chr1"
        posFrom=0
        posTo=0
        return (ref, posFrom, posTo)
    
    def get_binIdx(ref,posFrom,posTo):
        binIdxFrom=0
        binIdxTo=0
        return (binIdxFrom, binIdxTo)

class Bam2MatOutputDefine:
    def __init__(self,
          bigmat=None, binmap=None):
        self.BigMat = bigmat
        self.BinMap = binmap

class Txt2MatOutputDefine:
    def __init__(self,
          bigmat=None):
        self.BigMat = bigmat

class Bfv2MatOutputDefine:
     def __init__(self,
          bigmat=None):
        self.BigMat = bigmat

class FeatureIdxsOutputDefine:
    def __init__(self, featureIdxs=None):
        self.FeatureIdxs = featureIdxs

class HighVariabilityFeaturesOutputDefine:
    def __init__(self, featureIdxs=None, variabilities=None, task=None):
        self.FeatureIdxs = featureIdxs
        self.Variabilities = variabilities
        self.Task =task

class RUVMatrixExportOutputDefine:
    def __init__(self):
        self.OutMatNames = None
        self.BfvFiles = None
        self.ColCnt = None
        self.RowCnt = None
        self.ColNames= None
        self.ColIDs = None
        self.Ks = None
        self.Ns = None
        self.RUVOutputMode = None
        self.RUVOutputScale = None

class QuantileOutputDefine:
    def __init__(self, quantiles=None, qfeatuerIdxs=None, qvalues=None, task=None):
        self.Quantiles = quantiles
        self.qFeatureIdxs = qfeatuerIdxs
        self.qValues = qvalues
        self.Task =task

def dumpPickle(obj, filename):
    with open(filename, 'wb') as f:
        obj = pickle.dump(obj,f)
    return obj

def loadPickle(filename):
    with open(filename, 'rb') as f:
        obj = pickle.load(f)
    return obj

class JABSCpGSitesOutputDefine:
    def __init__(self, fasta=None, mapInfo=None, bigvec=None):
        self.GenomeFasta = fasta
        self.CpGMapInfo =mapInfo
        self.CpGIdx2BpIdxBigVec =  bigvec

class JABSAMRFinderOutputDefine:
    def __init__(self):
        self.H0LLVec =  None
        self.H1LLVec =  None
        self.StatsMat = None
        self.Task =None

class JABSJointSamplesOutputDefine:
    def __init__(self):
        self.SampleNames =  None
        self.SingleResults =  None
        self.JointMat = None
        self.AttachedSingleOIDs = None
        self.TotalWinCnt= None

class BigClustKMeansOutputDefine:
    def __init__(self):
        self.DataMat = None
        self.SeedsMat = None
        self.CentroidsMat = None
        self.KMembersVec = None
        self.KCntsVec = None
        self.Project = None

class BigClustKMeansPPSeedsOutputDefine:
    def __init__(self):
        self.BigMat = None
        self.SeedFeatureIdxs=None
        self.Project = None

class BigClustKMeansPPOutputDefine:
    def __init__(self):
        self.Project = None
        self.Results = None

class NodeRunSummaryDefine:
    def __init__(self):
        self.NodeType = None
        self.NodeName = None
        self.NodeStatus = None
        self.NodeDir = None

class BdvdRuvOutDefine:
    def __init__(self):
        self.BigMatDir = None
        self.RuvFaceInfo = None
        self.EigenValues = None
        self.EigenVectors = None
        self.PermutatedEigenValues = None
        self.Wt = None

class BdvdResultsDefine:
    def __init__(self):
        self.RuvOut = None

class BdvdExportResultsDefine:
    def __init__(self):
        self.Export = None

def getResultPickleFromNodeDir(nodeDir):
    # runSummary.pickle should be in nodeDir/log
    runSummaryPickleFile = "{0}/logs/runSummary.pickle".format(os.path.abspath(nodeDir))
    if not os.path.exists(runSummaryPickleFile):
        return None
    resultSummary = loadPickle(runSummaryPickleFile)
    if resultSummary.NodeType == "bigMat":
        outPickle = "{0}/run/1-input-mat/1-input-mat.pickle".format(os.path.abspath(nodeDir))
        return os.path.abspath(outPickle)
    elif resultSummary.NodeType == "bigKmeans":
        outPickle = "{0}/run/3-run-kmeans/3-run-kmeans.pickle".format(os.path.abspath(nodeDir))
        return os.path.abspath(outPickle)
    elif resultSummary.NodeType == "bdvd":
        outPickle = "{0}/logs/results.pickle".format(os.path.abspath(nodeDir))
        return os.path.abspath(outPickle)
    elif resultSummary.NodeType == "bdvd-export":
        outPickle = "{0}/logs/results.pickle".format(os.path.abspath(nodeDir))
        return os.path.abspath(outPickle)
    else:
        return None

def derivePickleFile(filenameOrDir):
    if filenameOrDir is None:
        return filenameOrDir

    if os.path.isdir(filenameOrDir):
        pickleFile = getResultPickleFromNodeDir(filenameOrDir)
    else:
        pickleFile = appendPickleExtention(filenameOrDir)
    return pickleFile

def appendPickleExtention(filename):
    fn = filename.strip()
    if cmdFile[-7:] != '.pickle':
        return "{0}.pickle".format(fn)

class BdtUsage(Exception):
    def __init__(self, msg):
        self.msg = msg