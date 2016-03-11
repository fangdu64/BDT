#qsub -cwd -pe local 32 -l hongkai,mem_free=100G,h_vmem=200G fstats.sh

cd /dcs01/gcode/fdu1/BDT
node="s10-ruv-plotW"
projectDir="/dcs01/gcode/fdu1/projects/BDVD/DukeUWExon"
nodeOutDir="${projectDir}/${node}"
if [ -d "${nodeOutDir}" ]; then
    rm ${nodeOutDir} -rf
fi

ruvDir="${projectDir}/s02-ruvs"

bdvd-ruv-plotW.py --node ${node} \
		--output-dir ${nodeOutDir} \
		--ruv-dir ${ruvDir} \
		-k 10 \
		-n 0

