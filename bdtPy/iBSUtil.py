import sys
import os
import socket
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
   return "__VERSION__"

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