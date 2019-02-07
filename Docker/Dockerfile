FROM ubuntu

# Install.
RUN \
    sed -i 's/# \(.*multiverse$\)/\1/g' /etc/apt/sources.list && \
    apt-get update && \
    apt install -y build-essential && \
    apt install -y cmake && \
    apt install -y libboost-program-options-dev && \
    apt install -y libevent-dev && \
    apt install -y liblog4cplus-dev && \
    apt install -y libgtest-dev && \
    apt install -y byobu git mc vim socat

# Build GTest
RUN \
    cd /usr/src/gtest && \
    cmake . && make && \
    cp *.a /usr/lib

# Build projects
RUN \
    mkdir project && cd project && \
    git clone --branch without_corrections https://github.com/f0rtung/arrival_test_task.git && \
    cd /project/arrival_test_task && \
    cd common && cmake . && make && cd .. && \
    cd proto && cmake . && make && ./lib/ProtoTests && cd .. && \
    cd logger && cmake . && make && cd .. && \
    cd client && cmake . && make && cd .. && \
    cd balancer && cmake . && make

WORKDIR /project/arrival_test_task

# Define default command.
CMD ["mc"]
