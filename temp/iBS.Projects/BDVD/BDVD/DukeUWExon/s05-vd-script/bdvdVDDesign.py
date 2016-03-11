import iBS
# ----------------------------------------------------------------------
# RUV
# ----------------------------------------------------------------------
# specify number of unwanted factors to use
Ks=   [0,0,1,2,3,4,5,6,7,8,9,10]

# specify number of known factors to use
Ns=   [0,1,0,0,0,0,0,0,0,0,0,0]


FeatureIdxFrom = None
FeatureIdxTo = None

RUVOutputMode = iBS.RUVOutputModeEnum.RUVOutputModeYminusZY
#RUVOutputMode = iBS.RUVOutputModeEnum.RUVOutputModeYminusWa