import os
import pickle
import iBSDefines
import iBS

desing_params=os.path.dirname(os.path.abspath(__file__))+"/design_params.pickle"
desing_params=desing_params.replace("/script/design_params.pickle","-script/design_params.pickle")

(sampleGroups,
 KnownFactors,
 CommonLibrarySize,
 RUVMode,
 ControlFeaturePolicy,
 ControlFeatureMaxCntLowBound,
 ControlFeatureMaxCntUpBound,
 CtrlQuantile,
 AllInQuantileFraction,
 MaxK,
 FeatureIdxFrom,
 FeatureIdxTo)= iBSDefines.loadPickle(desing_params)