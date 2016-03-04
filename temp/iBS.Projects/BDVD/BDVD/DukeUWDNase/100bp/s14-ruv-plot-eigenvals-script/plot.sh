cd /dcs01/gcode/fdu1/BDT
node="s14-ruv-plot-eigenvals"
projectDir="/dcs01/gcode/fdu1/projects/BDVD/DukeUWDNase/100bp"
nodeOutDir="${projectDir}/${node}"
if [ -d "${nodeOutDir}" ]; then
    rm ${nodeOutDir} -rf
fi

ruvDir="${projectDir}/s02-ruvs-var"

bdvd-ruv-plot-eigenvals.py --node ${node} \
		--output-dir ${nodeOutDir} \
		--ruv-dir ${ruvDir} \
		--max-k 10 \
		--min-fraction 0.15

