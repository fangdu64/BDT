#qsub -cwd -pe local 20 -l hongkai,mem_free=100G,h_vmem=200G fstats.sh

cd /home/student/fdu1/BDT/install
node="s06-ruv-fstats"
projectDir="/dcs01/gcode/fdu1/projects/BDVD/DukeUWExon"
nodeOutDir="${projectDir}/${node}"
if [ -d "${nodeOutDir}" ]; then
        rm ${nodeOutDir} -rf
fi

ruvDir="${projectDir}/s02-ruvs"
designFile="${projectDir}/s06-ruv-fstats-script/bdvdRuvFstatsDesign.py"
inputPickle="${projectDir}/s04-ruv-hvfeatures/s04-ruv-hvfeatures.pickle"

bdvd-ruv-fstats.py --node ${node} \
		--num-threads 16 \
		--max-mem 2000 \
		--output-dir ${nodeOutDir} \
		--ruv-dir ${ruvDir} \
		--rowidxs ${inputPickle} \
		${designFile}

