FROM ubuntu:24.04

ARG GEANT4_VERSION=v11.4.1
ARG CMAKE_VERSION=4.3.3
ARG CMAKE_CXX_STANDARD=17

ENV DEBIAN_FRONTEND=noninteractive
ENV G4INSTALL=/opt/geant4
ENV GARFIELD_HOME=/opt/garfieldpp
ENV PATH="${G4INSTALL}/bin:/opt/cmake/bin:${PATH}"
ENV LD_LIBRARY_PATH="${G4INSTALL}/lib:${GARFIELD_HOME}/lib:${LD_LIBRARY_PATH}"

# System dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    wget \
    libxerces-c-dev \
    libexpat1-dev \
    libx11-dev \
    libxext-dev \
    libgsl-dev \
    libboost-dev \
    # ROOT dependencies
    dpkg-dev \
    cmake \
    binutils \
    libx11-dev \
    libxpm-dev \
    libxft-dev \
    libxext-dev \
    libssl-dev \
    libpcre3-dev \
    libglu1-mesa-dev \
    libglew-dev \
    libftgl-dev \
    libfftw3-dev \
    libcfitsio-dev \
    python3-dev \
    python3-numpy \
    libxml2-dev \
    liblz4-dev \
    liblzma-dev \
    libzstd-dev \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Install a recent CMake from binary
RUN wget -q https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz \
    && tar -xzf cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz -C /opt \
    && mv /opt/cmake-${CMAKE_VERSION}-linux-x86_64 /opt/cmake \
    && rm cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz

# Install ROOT from CERN binaries (saves a very long compile)
RUN wget -q https://root.cern/download/root_v6.34.00.Linux-ubuntu24.04-x86_64-gcc13.3.tar.gz \
    && tar -xzf root_v6.34.00.Linux-ubuntu24.04-x86_64-gcc13.3.tar.gz -C /opt \
    && rm root_v6.34.00.Linux-ubuntu24.04-x86_64-gcc13.3.tar.gz
ENV ROOTSYS=/opt/root
ENV PATH="${ROOTSYS}/bin:${PATH}"
ENV LD_LIBRARY_PATH="${ROOTSYS}/lib:${LD_LIBRARY_PATH}"
ENV PYTHONPATH="${ROOTSYS}/lib"

# Build Geant4 from source
RUN wget -q https://github.com/Geant4/geant4/archive/refs/tags/${GEANT4_VERSION}.tar.gz \
    && tar -xzf ${GEANT4_VERSION}.tar.gz \
    && mkdir -p geant4-build \
    && cd geant4-build \
    && /opt/cmake/bin/cmake ../geant4-${GEANT4_VERSION#v} \
        -DCMAKE_INSTALL_PREFIX=${G4INSTALL} \
        -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD} \
        -DGEANT4_BUILD_MULTITHREADED=ON \
        -DGEANT4_USE_GDML=ON \
        -DGEANT4_USE_SYSTEM_EXPAT=ON \
        -DGEANT4_INSTALL_DATA=ON \
    && make -j$(nproc) && make install \
    && cd / && rm -rf geant4-${GEANT4_VERSION#v} geant4-build ${GEANT4_VERSION}.tar.gz

# Build Garfield++ from master (no versioned releases; master is canonical)
RUN git clone https://gitlab.cern.ch/garfield/garfieldpp.git ${GARFIELD_HOME} \
    && mkdir -p ${GARFIELD_HOME}/build \
    && cd ${GARFIELD_HOME}/build \
    && source ${G4INSTALL}/bin/geant4.sh \
    && source ${ROOTSYS}/bin/thisroot.sh \
    && /opt/cmake/bin/cmake ${GARFIELD_HOME} \
        -DCMAKE_INSTALL_PREFIX=${GARFIELD_HOME} \
        -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD} \
        -DWITH_GEANT4_UIVIS=ON \
    && make -j$(nproc) && make install

# Entrypoint that sources all environments
RUN echo 'source /opt/root/bin/thisroot.sh' >> /etc/bash.bashrc \
    && echo 'source /opt/geant4/bin/geant4.sh' >> /etc/bash.bashrc \
    && echo 'source /opt/garfieldpp/share/Garfield/cmake/garfield.sh 2>/dev/null || true' >> /etc/bash.bashrc

# Now build your simulation
COPY . /MWPC-Simulation

RUN source /opt/root/bin/thisroot.sh \
    && source /opt/geant4/bin/geant4.sh \
    && mkdir -p /MWPC-Simulation/build \
    && cd /MWPC-Simulation/build \
    && /opt/cmake/bin/cmake /MWPC-Simulation \
        -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD} \
    && make -j$(nproc)

WORKDIR /MWPC-Simulation/build
CMD ["/bin/bash"]
