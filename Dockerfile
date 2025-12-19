FROM alpine:latest AS build
RUN apk add --no-cache g++ cmake make musl-dev zlib-dev zlib-static
WORKDIR /src
COPY . .
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXE_LINKER_FLAGS="-static -Wl,-Bstatic -lz -Wl,-Bdynamic"

RUN cmake --build build --config Release --parallel

FROM scratch
COPY --from=build /src/build/src/calculator /app
ENTRYPOINT ["/app"]
CMD []