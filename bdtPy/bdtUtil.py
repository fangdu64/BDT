import sys
import os
import socket
import logging

# check if current working directory is at BDT install dir
def cwdAtBDT():
    if os.path.exists("./bdt/slice/bdt/BasicSliceDefine.ice"):
        return True
    else:
        return False

# get a usable TCP port at localhost
def getUsableTcpPort():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind(("",0))
    port = s.getsockname()[1]
    s.close()
    return port

def get_version():
   return "1.0"

# Format a DateTime as a pretty string.
# FIXME: Currently doesn't support days!
def formatTD(td):
    days = td.days
    hours = td.seconds // 3600
    minutes = (td.seconds % 3600) // 60
    seconds = td.seconds % 60

    if days > 0:
        return '%d days %02d:%02d:%02d' % (days, hours, minutes, seconds)
    else:
        return '%02d:%02d:%02d' % (hours, minutes, seconds)

def getSortedRefNames(refNames):
    regRefNames=[]
    for ref in refNames:
        if len(ref)==4 and ref[3].isdigit():
            regRefNames.append('chr0'+ref[3])
        else:
            regRefNames.append(ref)
    regRefNames.sort()

    for i in range(len(regRefNames)):
        regRefNames[i] = regRefNames[i].replace('chr0', 'chr')

    return regRefNames

def getIntersectRefsFromBamSamples(sampleList, samplePrx):
    ref2SampleCnt={}
    for si in sampleList:
        (rt,refs,refSizes)=samplePrx.GetRefDataFromBamFile(si.bamfile)
        for ref in refs:
            if ref not in ref2SampleCnt:
                ref2SampleCnt[ref]=1
            else:
                ref2SampleCnt[ref]= ref2SampleCnt[ref]+1

    intersectRefs=[]
    for ref, cnt in ref2SampleCnt.items():
        if cnt==len(sampleList):
            intersectRefs.append(ref)
    intersectRefs = getSortedRefNames(intersectRefs)
    return intersectRefs

def parseIntSeq(value):
    spans=value.split('-')
    ks=[]
    if len(spans)>1:
        begins=[]
        ends=[]
        for i in range(len(spans)):
            vs=[int(v.strip()) for v in spans[i].split(',')]
            begins.append(vs[0])
            ends.append(vs[len(vs)-1])
        for i in range(len(spans)):
            vs=[int(v.strip()) for v in spans[i].split(',')]
            ks.extend(vs)
            if i<(len(spans)-1) and ends[i]<begins[i+1]:
                ks.extend(range(ends[i]+1,begins[i+1]))
    else:
        ks = [int(v.strip()) for v in value.split(',')]
    return ks

def parseIntSeqSeq(value):
    groups=value.split('[')
    ks=[]
    for g in groups:
        g=g.strip()
        if len(g)==0:
            continue
        g=g.replace('],','')
        g=g.replace(']','')
        if len(g)==0:
            continue
        ks.append(parseIntSeq(g))
    return ks

def getFirstNone(valueList):
    for i in range(len(valueList)):
        if valueList is None:
            return i
    return -1

# bdtRunner
class bdtRunner:
    def __init__(self):
        self.output_dir = None
        self.logging_dir = None
        self.pipeline_rundir = None
        self.bdvd_logger = None # main logging object
        self.bdvd_log_handle = None #main log file handle

    # Ensures that the output, logging, and temp directories are present. If not,
    # they are created
    def prepare_dirs(self, outDir, logDir, runDir):
        self.output_dir = outDir
        self.logging_dir = logDir
        self.pipeline_rundir =runDir

        if not os.path.exists(self.output_dir):
            os.mkdir(self.output_dir)

        if not os.path.exists(self.logging_dir):
            os.mkdir(self.logging_dir)
    
        if not os.path.exists(self.pipeline_rundir):
            os.mkdir(self.pipeline_rundir)

    def init_logger(self, log_fname):
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
        if bdvd_log_handle:
            print(out_str, file=self.bdvd_log_handle)

    def die(self, msg=None):
        if msg is not None:
            self.logp(msg)
        sys.exit(1)