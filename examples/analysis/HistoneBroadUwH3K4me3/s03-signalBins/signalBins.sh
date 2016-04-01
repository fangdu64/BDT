thisScriptPath=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
. ${thisScriptPath}/../../../config/bdt_path_linux.sh

${bdtHome}/bdvdRowSelection \
    --thread-num 4 \
    --memory-size 8000 \
    --row-selector  with-signal \
    --bdvd-dir ${thisScriptPath}/../s01-bdvd\out \
    --with-signal-threshold 1.8 \
    --with-signal-col-cnt 3 \
    --component signal+random \
    --scale mlog \
    --artifact-detection conservative \
    --unwanted-factors 0 \
    --known-factors 0 \
    --out ${thisScriptPath}/out
