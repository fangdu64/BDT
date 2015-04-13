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

class Csv2MatOutputDefine:
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
        self.BfvFiles = None
        self.ColCnt = None
        self.RowCnt = None
        self.ColNames= None
        self.ColIDs = None
        self.Ks = None
        self.Ns = None
        self.RUVOutputMode = None

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