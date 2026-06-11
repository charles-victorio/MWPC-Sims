FROM ghcr.io/lobis/root-geant4-garfield:latest

RUN apt-get update && apt-get install -y git cmake make g++ && \
    git clone https://gitlab.cern.ch/garfield/garfieldpp.git /opt/garfieldpp && \
    cd /opt/garfieldpp && \
    git checkout 3e74b857 && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local/garfieldpp/install && \
    make -j$(nproc) && \
    make install

CMD ["/bin/bash"]
