FROM alpine:3.17 AS build-env

#RUN apk add --update autoconf automake libtool git make gcc zlib g++
RUN apk add --update make gcc cmake g++ linux-headers

COPY . src/

RUN cd src \
  && mkdir BUILD \
  && cd BUILD \
  && cmake .. \
  && make \
  && make install

FROM alpine:3.17

COPY --from=build-env /usr/local/bin /usr/local/bin

WORKDIR /

ENTRYPOINT ["/usr/local/bin/friidump"]