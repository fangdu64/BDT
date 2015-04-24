thisScriptPath=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
. ${thisScriptPath}/../../config/bdt_path_linux.sh

${bdtInstallDir}/bigMat \
	--input binary-mat@${bdtDatasetsDir}/binaryMat/dnase_test.bfv \
	--nrow 92554 \
	--ncol 45 \
	--out ${thisScriptPath}/02-out
