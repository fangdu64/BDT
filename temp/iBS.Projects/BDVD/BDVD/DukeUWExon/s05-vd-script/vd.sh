#qsub -M fdu1@jhmi.edu -m e -cwd -pe local 48 -l hongkai,mem_free=2G,h_vmem=200G vd.sh

cd /dcs01/gcode/fdu1/BDT
node="s05-vd"
projectDir="/dcs01/gcode/fdu1/projects/BDVD/DukeUWExon"
nodeOutDir="${projectDir}/${node}"
if [ -d "${nodeOutDir}" ]; then
    rm ${nodeOutDir} -rf
fi
ruvDir="/dcs01/gcode/fdu1/projects/BDVD/DukeUWExon/s02-ruvs"
designFile="${projectDir}/${node}-script/bdvdVDDesign.py"

bdvd-vd.py --node ${node} \
		--num-threads 32 \
		--max-mem 2000 \
		--output-dir ${nodeOutDir} \
		--ruv-dir ${ruvDir} \
		${designFile}

