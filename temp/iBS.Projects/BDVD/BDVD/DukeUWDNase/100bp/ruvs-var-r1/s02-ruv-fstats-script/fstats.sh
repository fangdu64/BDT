#qsub -M fdu1@jhmi.edu -m e -cwd -pe local 28 -l mem_free=2G,h_vmem=200G fstats.sh

cd /home/student/fdu1/BDT/install
node="s02-ruv-fstats"
projectDir="/dcs01/gcode/fdu1/projects/BDVD/DukeUWDNase/100bp/ruvs-var-r1"
nodeOutDir="${projectDir}/${node}"
if [ -d "${nodeOutDir}" ]; then
        rm ${nodeOutDir} -rf
fi

ruvDir="/dcs01/gcode/fdu1/projects/BDVD/DukeUWDNase/100bp/s02-ruvs-var"
designFile="${projectDir}/s02-ruv-fstats-script/bdvdRuvFstatsDesign.py"
inputPickle="/dcs01/gcode/fdu1/projects/BDVD/DNaseExonCorrelation/100bp/s01-TSS-PairIdxs/DNase_UniqueFeatureIdxs.txt"

bdvd-ruv-fstats.py --node ${node} \
		--num-threads 24 \
		--max-mem 16000 \
		--output-dir ${nodeOutDir} \
		--ruv-dir ${ruvDir} \
		--rowidxs ${inputPickle} \
		--rowidxs-txt \
		${designFile}

