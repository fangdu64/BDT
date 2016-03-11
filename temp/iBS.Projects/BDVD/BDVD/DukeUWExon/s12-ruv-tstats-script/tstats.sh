#qsub -cwd -pe local 32 -l hongkai,mem_free=100G,h_vmem=200G tstats.sh

cd /home/student/fdu1/BDT/install
node="s12-ruvs-tstats"
projectDir="/dcs01/gcode/fdu1/projects/BDVD/DukeUWExon"
nodeOutDir="${projectDir}/${node}"
if [ -d "${nodeOutDir}" ]; then
        rm ${nodeOutDir} -rf
fi

ruvDir="${projectDir}/s02-ruvs"
designFile="${projectDir}/s12-ruv-tstats-script/bdvdRuvTstatsDesign.py"
inputPickle="${projectDir}/s04-ruv-hvfeatures/s04-ruv-hvfeatures.pickle"

bdvd-ruv-tstats.py --node ${node} \
		--num-threads 16 \
		--max-mem 2000 \
		--output-dir ${nodeOutDir} \
		--ruv-dir ${ruvDir} \
		--rowidxs ${inputPickle} \
		${designFile}

