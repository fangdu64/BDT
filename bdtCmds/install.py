import sys, traceback
import getopt
import os
import shutil
import time
from datetime import datetime, date

import iBSConfig

def install_python_cmdline(srcDir, srcFileName, destDir, pythonBinPath):
    infile = open("{0}/{1}".format(srcDir, srcFileName))
    outFN = "{0}/{1}".format(destDir, srcFileName)
	outfile = open(outFN, "w")

    replacements = {"__PYTHON_BIN_PATH__":pythonBinPath}

    for line in infile:
        for src, target in replacements.items():
            line = line.replace(src, target)
        outfile.write(line)
    infile.close()
    outfile.close()
	os.chmod(outFN, 0o755)

def main(argv=None):
	bdtCmdsDir = os.path.dirname(os.path.abspath(__file__))
	cmdFiles = next(os.walk(bdtCmdsDir))[2]
	pythonBinPath = iBSConfig.BDT_HomeDir + "/ThirdParty/python/bin/python3.3"
	destDir = iBSConfig.BDT_HomeDir + "/bdt"
	print(bdtCmdsDir)
	print(cmdFiles)
	for cmdFile in cmdFiles:
		if cmdFile == "install.py":
			continue
		install_python_cmdline(bdtCmdsDir, cmdFile, destDir, pythonBinPath)

if __name__ == "__main__":
	sys.exit(main())
