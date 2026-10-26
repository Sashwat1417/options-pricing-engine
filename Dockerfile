# Build stage: compile with g++ and run the tests. Discarded after the build.
FROM ubuntu:24.04 AS build

RUN apt-get update \
    && apt-get install -y --no-install-recommends g++ make cmake ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake --preset release -DOPE_BUILD_CLI=OFF \
    && cmake --build --preset release -j"$(nproc)" \
    && ctest --preset release

# Runtime stage: only the server binary and the web UI, run as a non-root user.
FROM ubuntu:24.04

RUN useradd --system --no-create-home --uid 10001 ope

COPY --from=build /src/build/release/server/ope-server /usr/local/bin/ope-server
COPY server/web /app/web

ENV PORT=8080 \
    WEB_DIR=/app/web

USER ope
EXPOSE 8080

CMD ["ope-server"]
