FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update &&     apt-get install -y git cmake g++ libjsoncpp-dev uuid-dev openssl libssl-dev zlib1g-dev libsqlite3-dev libboost-all-dev libbrotli-dev libcurl4-openssl-dev pkg-config nlohmann-json3-dev

RUN git clone https://github.com/drogonframework/drogon.git &&     cd drogon && git submodule update --init && mkdir build && cd build &&     cmake .. && make -j$(nproc) && make install

WORKDIR /app
COPY . .
RUN mkdir build && cd build && cmake .. && make -j$(nproc)

# Add after building drogon_backend
# RUN mkdir test_build \
# && cd test_build \
# && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .. \
# && make -j$(nproc) \
# && ctest || (echo "===== TEST LOG =====" && cat Testing/Temporary/LastTest.log && false)


CMD ["./build/match_backend"]
