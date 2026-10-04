#!/bin/bash
#$ -N gasmerge
#$ -cwd
#$ -hold_jid gasgen             # waits for all slices
#$ -l h_rt=00:30:00,h_data=4G
#$ -j y
#$ -o garfield_code/logs/
#$ -M charlesvictorio@ucla.edu
#$ -m ea

NTASKS=40

# load the job environment:
source /u/local/Modules/default/init/modules.sh
module load apptainer

apptainer exec ~/myimage.sif bash -c "
    cd /opt/root
    source bin/thisroot.sh
    source /opt/garfieldpp/share/Garfield/setupGarfield.sh
    cd ~/MWPC-Simulation/garfield_code/build
    ./gasmerge $NTASKS
"