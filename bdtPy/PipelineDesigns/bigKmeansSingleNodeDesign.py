import os
import pickle
import iBSDefines
import iBS

desing_params=os.path.dirname(os.path.abspath(__file__))+"/design_params.pickle"
desing_params = desing_params.replace('\\', '/')
desing_params=desing_params.replace("/script/design_params.pickle","-script/design_params.pickle")

(Ks,
 Distance,
 MaxIteration,
 MinExplainedChanged,
 FeatureIdxFrom,
 FeatureIdxTo,
 KMeansContractorWorkerCnt,
 Seeding) = iBSDefines.loadPickle(desing_params)

KMeansContractorRAMSize=2000