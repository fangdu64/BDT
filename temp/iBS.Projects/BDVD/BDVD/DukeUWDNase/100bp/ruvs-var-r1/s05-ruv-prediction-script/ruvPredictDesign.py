import iBS

designDir="/dcs01/gcode/fdu1/projects/BDVD/DukeUWDNase/100bp/ruvs-var-r1/s05-ruv-prediction-script"

MatXDesign=designDir+"/ruvPredictMatXDesign.py"
MatYDesign=designDir+"/ruvPredictMatYDesign.py"

OnewayConfig=True

KsX=   [0,0,1,2,3,4,5,6,7,8,9,10,20,30]
NsX=   [1,0,0,0,0,0,0,0,0,0,0,0,0,0]

#KsY=   [0,0,0,0,0,0,0,0,0,0,0,0,0,0]
#NsY=   [0,0,0,0,0,0,0,0,0,0,0,0,0,0]
KsY=   [2,2,2,2,2,2,2,2,2,2,2,2,2,2]
NsY=   [0,0,0,0,0,0,0,0,0,0,0,0,0,0]

#DNase UW only Cell Lines, 1-based
ColIDs_Train = [57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,
                76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,
                95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,
                111,112,113,114,115,116,117,118,119,120,121,122,123,124,
                125,126,127,128,129,130,131,132]

#DNase Duke only Cell Lines, 1-based
ColIDs_Test = [1,3,4,5,6,7,8,9,10,11,13,14,15,16,17,18,21,22,24,25,28,30,
               32,33,34,36,37,38,40,42,44,45,46,47,48,49,50,52,53,54,55,56]

MinPredictorCnt=20