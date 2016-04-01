import os
import pickle
import iBSDefines

desing_params=os.path.dirname(os.path.abspath(__file__))+"/design_params.pickle"
desing_params = desing_params.replace('\\', '/')
desing_params=desing_params.replace("/script/design_params.pickle","-script/design_params.pickle")

(Ks,
 Ns,
 RUVOutputMode,
 RUVOutputScale,
 OutputWorkerNum,
 ColIds,
 FeatureIdxFrom,
 FeatureIdxTo,
 RowSelectionMethod,
 WithSignalThreshold,
 WithSignalColCnt,
 WithSignalSamplingRowCnt,
 RowIdxsOutFile) = iBSDefines.loadPickle(desing_params)
