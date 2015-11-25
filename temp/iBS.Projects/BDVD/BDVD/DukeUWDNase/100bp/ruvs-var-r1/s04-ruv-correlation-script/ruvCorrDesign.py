import iBS

designDir="/dcs01/gcode/fdu1/projects/BDVD/DukeUWDNase/100bp/ruvs-var-r1/s04-ruv-correlation-script"

SignalMate1Design=designDir+"/ruvCorrSignalMate1Design.py"
SignalMate2Design=designDir+"/ruvCorrSignalMate2Design.py"
NoiseMate1Design=designDir+"/ruvCorrNoiseMate1Design.py"
NoiseMate2Design=designDir+"/ruvCorrNoiseMate2Design.py"

OnewayConfig=True
H1H0Ratio = 1
MAX_FDR=0.05

# to draw ROC curves
#KsMate1=   [0,0,1,2,3,4,5,6,7,8,9,10,20,30]
#NsMate1=   [1,0,0,0,0,0,0,0,0,0,0,0,0,0]

KsMate1=   [0,0,1,2,3]
NsMate1=   [0,1,0,0,0]

KsMate2=   [0,0,1,2,3]
NsMate2=   [0,1,0,0,0]

KsMate1=   [0,0,1,2,2,3,3]
NsMate1=   [0,1,0,0,0,0,0]

KsMate2=   [0,0,1,2,3,2,3]
NsMate2=   [0,1,0,0,0,0,0]

