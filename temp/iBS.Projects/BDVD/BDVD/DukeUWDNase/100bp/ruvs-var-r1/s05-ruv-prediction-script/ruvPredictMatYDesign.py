import math
import iBS

# ----------------------------------------------------------------------
# RUV Exon
# ----------------------------------------------------------------------

# specify number of unwanted factors to use
Ks=   [0,0,1,2,3]

# specify number of known factors to use
Ns=   [1,0,0,0,0]

RUVOutputMode = iBS.RUVOutputModeEnum.RUVOutputModeZYthenGroupMean

Samples=[78, 63, 55, 54, 80, 99, 43, 121, 124, 127, 72, 5, 22, 24, 40,
         26, 28, 30, 18, 101, 51, 119, 11, 37, 86, 8, 76, 84, 45, 104, 
         48, 82, 92, 90, 14, 115, 117, 109, 1, 111, 67, 65, 39, 130, 98, 
         133, 32, 107, 57, 61, 16, 69, 35, 88, 94, 96, 204, 206, 208, 210, 
         212, 237, 277, 166, 164, 214, 153, 279, 233, 317, 319, 321, 328, 
         297, 217, 295, 280, 271, 323, 324, 219, 243, 231, 245, 221, 239, 
         223, 299, 301, 202, 282, 171, 247, 315, 251, 253, 255, 257, 259, 
         241, 249, 225, 310, 261, 265, 263, 180, 182, 303, 227, 267, 187, 
         305, 189, 273, 326, 269, 229, 235, 311, 193, 313, 290, 195, 173, 
         292, 161, 157, 176, 294, 306, 308]

UniqueFeatureIdxsFile="/dcs01/gcode/fdu1/projects/BDVD/DNaseExonCorrelation/100bp/s01-TSS-PairIdxs/Exon_UniqueFeatureIdxs.txt"
RowIDsFile="/dcs01/gcode/fdu1/projects/BDVD/DNaseExonCorrelation/100bp/s01-TSS-PairIdxs/Exon_RowIDs.txt"
RUVDir="/dcs01/gcode/fdu1/projects/BDVD/DukeUWExon/ruvs-var/s01-ruvs"




