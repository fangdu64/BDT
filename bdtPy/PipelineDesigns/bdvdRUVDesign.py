import os
import pickle
import iBSDefines

desing_params=os.path.dirname(os.path.abspath(__file__))+"/design_params.pickle"
desing_params = desing_params.replace('\\', '/')
desing_params=desing_params.replace("/script/design_params.pickle","-script/design_params.pickle")

(sampleGroups,
 KnownFactors,
 CommonLibrarySize,
 RUVMode,
 RUVScale,
 ControlFeaturePolicy,
 ControlFeatureMaxCntLowBound,
 ControlFeatureMaxCntUpBound,
 CtrlQuantile,
 AllInQuantileFraction,
 MaxK,
 FeatureIdxFrom,
 FeatureIdxTo,
 PermutationCnt)= iBSDefines.loadPickle(desing_params)
