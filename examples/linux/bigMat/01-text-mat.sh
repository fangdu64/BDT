thisScriptPath=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
. ${thisScriptPath}/../../config/bdt_path_linux.sh

${bdtHome}/bigMat \
	--input text-mat@${bdtDatasetsDir}/txtMat/dnase_test.txt \
	--nrow 92554 \
	--ncol 45 \
	--out ${thisScriptPath}/01-out
