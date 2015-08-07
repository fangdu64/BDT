
thisScriptPath=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
. ${thisScriptPath}/../../../config/bdt_path_linux.sh

Rscript \
	--no-restore \
	--no-save \
	${thisScriptPath}/bigKmeans.R
