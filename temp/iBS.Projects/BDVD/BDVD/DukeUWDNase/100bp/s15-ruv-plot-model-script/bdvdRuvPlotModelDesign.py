import math
import iBS

# ----------------------------------------------------------------------
# RUV
# ----------------------------------------------------------------------
commonSamples=[
    [3, 4, 219, 220],
	[55, 56, 57, 58, 59, 254, 255],
	[79, 80, 256],
	[81, 82, 83, 257, 258],
	[90, 91, 92, 292, 293],
	[97, 98, 99, 294, 295],
	[100, 101, 306, 307],
	[104, 105, 106, 345, 346],
	[112, 113, 114, 347, 348],
	[121, 122, 349, 350],
	[137, 138, 139, 355, 356],
	[154, 155, 156, 361, 362],
	[159, 160, 365, 366],
	[185, 186, 384, 385],
	[211, 212, 406, 407]]

sampleIDs=[]
for sg in commonSamples:
    sampleIDs.extend(sg)

# Subset of SampleIDs to plot (export all though)
ColIDs=sorted(sampleIDs)
ColIDs=None
# specify number of unwanted factor
K=   3
# specify number of known factor
N=   0
FeatureIdxFrom = 10010
FeatureIdxTo = 10120

Components=[]

##
## Y
##
RUVOutputMode=iBS.RUVOutputModeEnum.RUVOutputModeYminusZY
RowAdjust=iBS.RowAdjustEnum.RowAdjustNone
ComponentName="Y"
Components.append((ComponentName,0,0,RUVOutputMode,RowAdjust))

##
## Xb
##
RUVOutputMode=iBS.RUVOutputModeEnum.RUVOutputModeZYthenGroupMean
RowAdjust=iBS.RowAdjustEnum.RowAdjustNone
ComponentName="Xb"
Components.append((ComponentName,K,N,RUVOutputMode,RowAdjust))

##
## Xb+e
##
RUVOutputMode=iBS.RUVOutputModeEnum.RUVOutputModeYminusZY
RowAdjust=iBS.RowAdjustEnum.RowAdjustNone
ComponentName="Xb+e"
Components.append((ComponentName,K,N,RUVOutputMode,RowAdjust))

##
## Wa
##
RUVOutputMode=iBS.RUVOutputModeEnum.RUVOutputModeZY
RowAdjust=iBS.RowAdjustEnum.RowAdjustNone
ComponentName="Wa"
Components.append((ComponentName,K,N,RUVOutputMode,RowAdjust))

##
## e
##
RUVOutputMode=iBS.RUVOutputModeEnum.RUVOutputModeZYGetE
RowAdjust=iBS.RowAdjustEnum.RowAdjustNone
ComponentName="e"
Components.append((ComponentName,K,N,RUVOutputMode,RowAdjust))