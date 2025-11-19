FROM rootproject/root:6.28.04-ubuntu22.04

ENV ROOTSYS=/opt/root/
ENV GARFIELD_HOME=/opt/garfieldpp
ENV LD_LIBRARY_PATH=/lib:/lib64:/usr/lib:/opt/root/lib:$LD_LIBRARY_PATH
ENV GARFIELD_IONDATA=${GARFIELD_HOME}/Data

WORKDIR /opt/

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y vim git && \
    rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/keithwm422/KeithsGarfield.git garfieldpp && \
    cd garfieldpp && mkdir build && cd build && \
    cmake -DCMAKE_INSTALL_PREFIX=${GARFIELD_HOME}/install .. && \
    make -j "$(nproc)" && make install && \
    mkdir -p ${GARFIELD_HOME}/install/lib64/ && \
    ln -s ${GARFIELD_HOME}/install/lib/* ${GARFIELD_HOME}/install/lib64/

ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${GARFIELD_HOME}/install/lib

CMD [ "bash" ]

