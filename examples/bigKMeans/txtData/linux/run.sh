scriptPath=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
. ${scriptPath}/../../../config/bdt_path_linux.sh
${bdtInstallDir}/bigKmeans \
	--input-type text-mat \
	--data ${bdtDatasetsDir}/txtMat/dnase_test.txt \
	--nrow 92554 \
	--ncol 45 \
	--k 100 \
	--out ${scriptPath}/out \
	--thread-num 4 \
	--dist-type Euclidean \
	--max-iter 100 \
	--min-expchg 0.0001
