#qsub -M fdu1@jhmi.edu -m e -cwd -pe local 28 -l mem_free=2G,h_vmem=200G hclust.sh

cd /home/student/fdu1/BDT/install
node="s08-ruv-hclust"
projectDir="/dcs01/gcode/fdu1/projects/BDVD/DukeUWExon"
nodeOutDir="${projectDir}/${node}"
if [ -d "${nodeOutDir}" ]; then
        rm ${nodeOutDir} -rf
fi

ruvDir="${projectDir}/s02-ruvs"
designFile="${projectDir}/${node}-script/bdvdRuvHclustDesign.py"
inputPickle="${projectDir}/s04-ruv-hvfeatures/s04-ruv-hvfeatures.pickle"

bdvd-ruv-hclust.py --node ${node} \
		--num-threads 16 \
		--max-mem 2000 \
		--output-dir ${nodeOutDir} \
		--ruv-dir ${ruvDir} \
		--rowidxs ${inputPickle} \
		${designFile}

