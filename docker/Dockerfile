FROM debian:buster

RUN apt-get update && apt-get install apt-transport-https ca-certificates -y

# mirrors
RUN echo "deb https://mirrors.tuna.tsinghua.edu.cn/debian/ buster main contrib non-free" > /etc/apt/sources.list && \
    echo "deb https://mirrors.tuna.tsinghua.edu.cn/debian/ buster-updates main contrib non-free" >> /etc/apt/sources.list && \
    echo "deb https://mirrors.tuna.tsinghua.edu.cn/debian/ buster-backports main contrib non-free" >> /etc/apt/sources.list && \
    echo "deb https://mirrors.tuna.tsinghua.edu.cn/debian-security buster/updates main contrib non-free" >> /etc/apt/sources.list

# apt update & install system tools
RUN apt-get update \
  && apt-get install wget xz-utils git vim curl zsh build-essential python -y

# install third-party dependencies
RUN apt-get install libgtest-dev libzbd-dev -y

# on-my-zsh
RUN yes | sh -c "$(curl -fsSL https://raw.github.com/ohmyzsh/ohmyzsh/master/tools/install.sh)"

# cmake
RUN wget https://github.com/Kitware/CMake/releases/download/v3.20.4/cmake-3.20.4-linux-x86_64.tar.gz -O /tmp/cmake-3.20.4-linux-x86_64.tar.gz \
  && tar -zxf /tmp/cmake-3.20.4-linux-x86_64.tar.gz -C /usr/local \
  && ln -s /usr/local/cmake-3.20.4-linux-x86_64/bin/cmake /usr/local/bin/cmake \
  && ln -s /usr/local/cmake-3.20.4-linux-x86_64/bin/ctest /usr/local/bin/ctest \
  && rm -rf /tmp/*

# clang
RUN wget https://github.com/llvm/llvm-project/releases/download/llvmorg-12.0.1/clang+llvm-12.0.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz -O /tmp/clang.tar.xz \
  && mkdir -p /opt/clang_12_0_1 \
  && tar -xvf /tmp/clang.tar.xz -C /opt/clang_12_0_1 --strip-components=1 \
  && cd /opt/clang_12_0_1/lib \
  && rm -rf libunwind* \
  && rm -rf /tmp/*

ENV CLANG_HOME /opt/clang_12_0_1
ENV CC ${CLANG_HOME}/bin/clang
ENV CXX ${CLANG_HOME}/bin/clang++
ENV PATH ${CLANG_HOME}/bin:${PATH}

ENV ASAN_OPTIONS detect_container_overflow=0:detect_odr_violation=0
