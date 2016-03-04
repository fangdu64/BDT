#qsub -M fdu1@jhmi.edu -m e -cwd -pe local 28 -l mem_free=2G,h_vmem=200G fstats.sh

cd /dcs01/gcode/fdu1/BDT
node="s15-ruv-plot-model"
projectDir="/dcs01/gcode/fdu1/projects/BDVD/DukeUWDNase/100bp"
nodeOutDir="${projectDir}/${node}"
if [ -d "${nodeOutDir}" ]; then
        rm ${nodeOutDir} -rf
fi

ruvDir="/dcs01/gcode/fdu1/projects/BDVD/DukeUWDNase/100bp/s02-ruvs-var"
designFile="${projectDir}/${node}-script/bdvdRuvPlotModelDesign.py"

bdvd-ruv-plot-model.py --node ${node} \
		--num-threads 16 \
		--max-mem 16000 \
		--output-dir ${nodeOutDir} \
		--ruv-dir ${ruvDir} \
		${designFile}

