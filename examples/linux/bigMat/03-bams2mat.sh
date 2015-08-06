thisScriptPath=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
. ${thisScriptPath}/../../config/bdt_path_linux.sh

${bdtHome}/bigMat \
	--input bams@${thisScriptPath}/03_bam_files.txt \
	--chromosomes chr1,chr2,chr3,chr4,chr5,chr6,chr7,chr8,chr9,chr10,chr11,chr12,chr13,chr14,chr15,chr16,chr17,chr18,chr19,chrX \
	--bin-width 100 \
	--thread-num 4 \
	--out ${thisScriptPath}/03-out
