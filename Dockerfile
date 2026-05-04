FROM ghcr.io/lobis/root-geant4-garfield:latest

COPY . /MWPC-Simulation

RUN mkdir -p /MWPC-Simulation/build && \
	cd /MWPC-Simulation/build && \
	cmake /simulation -DCMAKE_CXX_STANDARD=17 && \
	make -j$(nproc)

WORKDIR /MWPC-Simulation/build

CMD ["/bin/bash"]
