FROM ubuntu:24.04 AS build

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
        clang-format \
        cppcheck \
        lcov \
        ca-certificates && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /src

COPY Makefile .clang-format ./
COPY src/ src/
COPY include/ include/
COPY tests/ tests/

RUN make && make test

FROM ubuntu:24.04

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        ca-certificates && \
    rm -rf /var/lib/apt/lists/*

COPY --from=build /src/binlens /usr/local/bin/binlens

ENTRYPOINT ["binlens"]
CMD ["--help"]