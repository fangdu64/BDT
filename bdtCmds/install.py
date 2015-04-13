import sys, traceback
import getopt
import os
import shutil
import time
from datetime import datetime, date

import iBSConfig

def install_python_cmdline(srcDir, srcFileName, destDir, pythonBinPath):
    infile = open("{0}/{1}".format(srcDir, srcFileName))
    outfile = open("{0}/{1}".format(destDir, srcFileName), "w")

    replacements = {"__PYTHON_BIN_PATH__":pythonBinPath}

    for line in infile:
        for src, target in replacements.items():
            line = line.replace(src, target)
        outfile.write(line)
    infile.close()
    outfile.close()

def main(argv=None):
	bdtCmdsDir = os.path.dirname(os.path.abspath(__file__))
	cmdFiles = next(os.walk(bdtCmdsDir))[2]
	pythonBinPath = iBSConfig.BDT_HomeDir + "/ThirdParty/python/bin/python3.3"
	print(bdtCmdsDir)
	print(cmdFiles)
	for cmdFile in cmdFiles:
		if cmdFile == "install.py":
			continue
		install_python_cmdline(bdtCmdsDir,srcFileName


if __name__ == "__main__":
    sys.exit(main())
