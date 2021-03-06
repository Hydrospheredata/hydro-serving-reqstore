FROM lerrox/grpc-aws-sdk-cpp:0.0.1 as build1

FROM lerrox/aws-cpp-sdk-rocksdb-cloud:0.0.2 as build2

ENV Protobuf_DIR "/var/local/git/grpc/third_party/protobuf"
ENV CMAKE_PREFIX_PATH=/usr/local/aws-sdk-cpp

COPY --from=build1 /usr/local /local_tmp
COPY --from=build1 /usr/local/share/grpc /usr/local/share/grpc
COPY --from=build1 /usr/local/include/google  /usr/local/include/google
COPY --from=build1 /usr/local/include/grpc  /usr/local/include/grpc
COPY --from=build1 /usr/local/include/'grpc++'  /usr/local/include/'grpc++'
COPY --from=build1 /usr/local/include/grpcpp  /usr/local/include/grpcpp
COPY --from=build1 /usr/local/bin/protoc  /usr/local/bin
COPY --from=build1 /usr/local/bin/grpc_cpp_plugin  /usr/local/bin

RUN apt-get update \
    && apt-get install -y \
       bzip2 build-essential autoconf git pkg-config zlib1g zlib1g-dev openssl libssl-dev libgtk-3-dev autogen libsnappy-dev \
       automake libtool curl make g++ unzip cmake libgtest-dev libcurl4-openssl-dev golang libiodbc2 libiodbc2-dev \
    && cp /local_tmp/lib/lib* /usr/local/lib && ls /usr/local/lib && ldconfig \
    && mkdir /usr/src/poco && cd /usr/src/poco && wget http://pocoproject.org/releases/poco-1.9.0/poco-1.9.0-all.tar.gz \
    && tar -zxvf poco-1.9.0-all.tar.gz && cd poco-1.9.0-all && ./configure --static --no-tests && make -j$(nproc) && make install


ADD ./src /app/src
ADD ./src/spdlog /app/external/spdlog

WORKDIR /app/build

RUN cmake -E env LDFLAGS="-lcurl -lssl -lcrypto -lz -lrt" cmake ../src \
    && cmake --build . \
    && CTEST_OUTPUT_ON_FAILURE=TRUE cmake --build . --target \
    && groupadd -r reqstore && useradd -r -g reqstore reqstore


# for gdbserver
EXPOSE 2000

ADD ./src /app/src
ADD ./src/spdlog /app/external/spdlog

VOLUME "/app"
WORKDIR "/app"

