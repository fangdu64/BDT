import os
import pickle
import iBSDefines
import iBS

desing_params=os.path.dirname(os.path.abspath(__file__))+"/design_params.pickle"
desing_params = desing_params.replace('\\', '/')
desing_params=desing_params.replace("/script/design_params.pickle","-script/design_params.pickle")

(StorePathPrefix,
 CalculateStatistics,
 RowCnt,
 ColCnt,
 ColNames)= iBSDefines.loadPickle(desing_params)