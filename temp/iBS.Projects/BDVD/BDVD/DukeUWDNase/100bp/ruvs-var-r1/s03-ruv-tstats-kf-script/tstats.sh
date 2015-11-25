#qsub -m e -M fdu1@jhmi.edu -cwd -pe local 20 -l hongkai,mem_free=2G,h_vmem=200G tstats.sh

cd /home/student/fdu1/BDT/install
node="s03-ruv-tstats-kf"
projectDir="/dcs01/gcode/fdu1/projects/BDVD/DukeUWDNase/100bp/ruvs-var-r1"
nodeOutDir="${projectDir}/${node}"
if [ -d "${nodeOutDir}" ]; then
        rm ${nodeOutDir} -rf
fi

ruvDir="/dcs01/gcode/fdu1/projects/BDVD/DukeUWDNase/100bp/s02-ruvs-var"
designFile="${projectDir}/s03-ruv-tstats-kf-script/bdvdRuvTstatsDesign.py"
inputPickle="/dcs01/gcode/fdu1/projects/BDVD/DNaseExonCorrelation/100bp/s01-TSS-PairIdxs/DNase_UniqueFeatureIdxs.txt"

bdvd-ruv-tstats.py --node ${node} \
		--num-threads 20 \
		--max-mem 16000 \
		--output-dir ${nodeOutDir} \
		--ruv-dir ${ruvDir} \
		--rowidxs ${inputPickle} \
		--rowidxs-txt \
		${designFile}

