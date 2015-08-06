#qsub -cwd -pe local 36 -l mem_free=100G,h_vmem=200G bam2mat.sh

thisScriptPath=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
. ${thisScriptPath}/../../../config/bdt_path_linux.sh

Rscript \
	--no-restore \
	--no-save \
	${thisScriptPath}/bdvd.R
