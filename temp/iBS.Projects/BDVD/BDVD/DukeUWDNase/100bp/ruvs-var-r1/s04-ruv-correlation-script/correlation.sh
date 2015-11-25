#qsub -M fdu1@jhmi.edu -m e -cwd -pe local 30 -l mem_free=2G,h_vmem=200G correlation.sh

cd /dcs01/gcode/fdu1/BDT
node="s04-ruv-correlation"
projectDir="/dcs01/gcode/fdu1/projects/BDVD/DukeUWDNase/100bp/ruvs-var-r1"
nodeOutDir="${projectDir}/${node}"
#if [ -d "${nodeOutDir}" ]; then
	#rm ${nodeOutDir} -rf
#fi

designFile="${projectDir}/s04-ruv-correlation-script/ruvCorrDesign.py"

bdvd-ruv-correlation.py --node ${node} \
		--num-threads 36 \
		--max-mem 16000 \
		--output-dir ${nodeOutDir} \
		--resume \
		${designFile}

