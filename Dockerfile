From debian:jessie


RUN echo "==> install haka  dependencies" \ 
 && apt-get update \
 && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    dialog apt-utils\
    git make build-essential \
    cmake swig check rsync gawk libpcap-dev libedit-dev libpcre3-dev tshark \
 && rm -rf /var/lib/apt/lists/*


 RUN echo "==> install haka"  \ 
  && cd tmp \
  && git config --global http.sslVerify false \
  && git clone https://github.com/haka-security/haka.git \
  && cd haka \
  && git submodule init \
  && git submodule update \
  && mkdir -p make \
  && cd make \
  && cmake .. \
  && make \
  && make install
