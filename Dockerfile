FROM ghcr.io/lobis/root-geant4-garfield:latest

COPY . /MWPC-Simulation

RUN apt-get update && apt-get install -y curl && \
	curl -fsSL https://cmake.org/files/v4.3/cmake-4.3.0-linux-x86_64.sh -o /tmp/cmake.sh && \
	sh /tmp/cmake.sh --prefix=/usr/local --skip-license && \
	rm /tmp/cmake.sh

RUN mkdir -p /MWPC-Simulation/build && \
	cd /MWPC-Simulation/build && \
	cmake /MWPC-Simulation -DCMAKE_CXX_STANDARD=17 && \
	make -j$(nproc)

WORKDIR /MWPC-Simulation/build

CMD ["/bin/bash"]
