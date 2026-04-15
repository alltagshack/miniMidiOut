FROM debian:trixie-slim
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
  build-essential \
  cmake \
  make \
  vim-tiny \
  git \
  wget \
  curl \
  pkg-config \
  gdb \
  openssh-server \
  sudo \
  bash \
  ca-certificates \
  clang \
  clang-tools && rm -rf /var/lib/apt/lists/*
RUN useradd -m developer && echo "developer ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/developer
RUN usermod -s /bin/bash developer
EXPOSE 22
COPY docker-entrypoint.sh /usr/local/bin/docker-entrypoint.sh
RUN chmod +x /usr/local/bin/docker-entrypoint.sh
CMD ["/usr/local/bin/docker-entrypoint.sh"]
