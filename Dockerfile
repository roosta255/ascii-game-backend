FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update &&     apt-get install -y git cmake g++ libjsoncpp-dev uuid-dev openssl libssl-dev zlib1g-dev libsqlite3-dev libboost-all-dev libbrotli-dev libcurl4-openssl-dev pkg-config nlohmann-json3-dev

RUN apt-get install -y ninja-build ccache

RUN git clone https://github.com/drogonframework/drogon.git &&     cd drogon && git submodule update --init && mkdir build && cd build &&     cmake .. && make -j$(nproc) && make install

WORKDIR /app
COPY . .
RUN cmake -B build_ninja -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build_ninja -j$(nproc)

CMD ["./build_ninja/match_backend"]
