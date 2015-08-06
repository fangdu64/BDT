thisScriptPath=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
. ${thisScriptPath}/../../config/bdt_path_linux.sh
${bdtHome}/bigKmeans \
	--data-input kmeans-data-mat@${thisScriptPath}/01-out \
	--seeds-input kmeans-seeds-mat@${thisScriptPath}/01-out \
	--k 100 \
	--out ${thisScriptPath}/03-out \
	--thread-num 4 \
	--dist-type Euclidean \
	--max-iter 100 \
	--min-expchg 0.0001
