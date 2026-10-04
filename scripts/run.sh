#!/bin/bash
##$ -N name
#$ -cwd
#$ -l h_rt=01:00:00,h_data=8G
#$ -j y
#$ -o logs/
#$ -M charlesvictorio@ucla.edu
#$ -m ea

CMD="$*"

source /u/local/Modules/default/init/modules.sh
module load apptainer
apptainer exec --contain --bind $HOME ~/myimage.sif bash -c "
    cd /opt/root
    source bin/thisroot.sh
    source /opt/geant4/bin/geant4.sh
    source /opt/garfieldpp/share/Garfield/setupGarfield.sh
    cd ~/MWPC-Simulation/garfield_code && mkdir -p build && cd build
    $CMD
"


#   cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17 \
#            -DCMAKE_PREFIX_PATH='/opt/geant4;/opt/root;/opt/garfieldpp'

