#!/bin/bash
#$ -N gainPara
#$ -cwd
#$ -t 1-20
#$ -l h_rt=02:00:00,h_data=12G   # generous per-task; one point is minutes, not hours
#$ -j y
#$ -o garfield_code/logs/
##$ -tc 10                      # uncomment to cap concurrent tasks if your cluster requires
#$ -M charlesvictorio@ucla.edu
#$ -m ea

# NTASKS=5

OTHER_ARGS="$*"

source /u/local/Modules/default/init/modules.sh
module load apptainer
apptainer exec --contain --bind $HOME ~/myimage.sif bash -c "
    cd /opt/root
    source bin/thisroot.sh
    source /opt/geant4/bin/geant4.sh
    source /opt/garfieldpp/share/Garfield/setupGarfield.sh
    cd ~/MWPC-Simulation/garfield_code && mkdir -p build && cd build
    ./mwpcMicro $SGE_TASK_ID $OTHER_ARGS
"
# make && 