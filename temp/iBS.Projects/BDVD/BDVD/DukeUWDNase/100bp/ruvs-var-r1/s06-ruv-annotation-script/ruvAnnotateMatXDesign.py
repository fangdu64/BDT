import math
import iBS

# ----------------------------------------------------------------------
# RUV DNase
# ----------------------------------------------------------------------

# specify number of unwanted factors to use
Ks=   [0,0,1,2,3,4,5,6,7,8,9,10]

# specify number of known factors to use
Ns=   [1,0,0,0,0,0,0,0,0,0,0,0]

RUVOutputMode = iBS.RUVOutputModeEnum.RUVOutputModeZYthenGroupMean

Samples=[1, 3, 9, 17, 19, 22, 28, 36, 39, 42, 49, 55, 60, 62, 68, 71, 
         73, 75, 79, 81, 84, 88, 90, 93, 95, 97, 100, 102, 104, 109, 
         112, 115, 117, 119, 121, 125, 127, 130, 137, 146, 154, 157, 
         159, 161, 164, 168, 171, 175, 177, 181, 185, 190, 201, 209, 
         215, 217, 221, 223, 225, 227, 229, 231, 233, 235, 237, 244, 
         249, 251, 252, 259, 261, 262, 264, 265, 267, 269, 271, 273, 
         275, 276, 278, 280, 282, 284, 286, 288, 290, 296, 298, 300, 
         302, 304, 308, 310, 312, 314, 316, 318, 320, 322, 324, 326, 
         328, 329, 331, 333, 335, 337, 339, 341, 351, 353, 371, 374, 
         376, 378, 380, 382, 386, 388, 390, 392, 396, 398, 400, 402, 
         404, 409, 414, 420, 422, 424]

UniqueFeatureIdxsFile="/dcs01/gcode/fdu1/projects/BDVD/DNaseExonCorrelation/100bp/s04-NearbyTSS/DNase_UniqueFeatureIdxs.txt"
RowIDsFile="/dcs01/gcode/fdu1/projects/BDVD/DNaseExonCorrelation/100bp/s04-NearbyTSS/DNase_RowIDs.txt"
RUVDir="/dcs01/gcode/fdu1/projects/BDVD/DukeUWDNase/100bp/s02-ruvs-var"
LocationTblFile=None
LocationRowIDsFile=None





