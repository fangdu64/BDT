import os
import pickle
import iBSDefines

desing_params=os.path.dirname(os.path.abspath(__file__))+"/design_params.pickle"
desing_params = desing_params.replace('\\', '/')
desing_params=desing_params.replace("/script/design_params.pickle","-script/design_params.pickle")

sample_table_content = None
(sample_table_lines,
 chromosomes,
 bin_width) = iBSDefines.loadPickle(desing_params)
