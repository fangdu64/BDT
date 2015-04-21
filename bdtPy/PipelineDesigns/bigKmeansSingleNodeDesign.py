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

# ----------------------------------------------------------------------
# Sample Information
# ----------------------------------------------------------------------
#Ks=[1, 100,200,1000]
#Ks=list(range(1,100+1))

#Seeding = iBS.KMeansSeedingEnum.KMeansSeedingKMeansRandom
#Seeding = iBS.KMeansSeedingEnum.KMeansSeedingKMeansPlusPlus
KMeansContractorRAMSize=2000