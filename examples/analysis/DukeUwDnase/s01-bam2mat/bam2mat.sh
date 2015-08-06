#qsub -cwd -pe local 48 -l mem_free=100G,h_vmem=200G bam2mat.sh

thisScriptPath=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
. ${thisScriptPath}/../../../config/bdt_path_linux.sh

${bdtHome}/bigMat \
	--input bams@${thisScriptPath}/bam_files.txt \
	--chromosomes chr1,chr2,chr3,chr4,chr5,chr6,chr7,chr8,chr9,chr10,chr11,chr12,chr13,chr14,chr15,chr16,chr17,chr18,chr19,chr20,chr21,chr22,chrX \
	--bin-width 100 \
	--thread-num 40 \
	--calc-statistics \
	--out ${thisScriptPath}/out
