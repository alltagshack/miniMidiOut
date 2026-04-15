## Docker notes

create container base template on local Dokerfile
```
docker build -t cpp-toolchain:trixie .
```

create image cpp-toolchain01
```
UID=$(id -u) GID=$(id -g) PROJPATH=$(pwd) \
  docker run -d --name cpp-toolchain01 \
    -v $PROJPATH:/home/developer/project:rw \
    -p 2222:22 -e CONTAINER_UID=$UID -e CONTAINER_GID=$GID \
    cpp-toolchain:trixie
```

list all used images and start e.g. cpp-toolchain01
```
docker ps -a
docker start cpp-toolchain01
```

start shell into image e.g. to place pub key into authorized_keys
```
docker exec -it -u developer cpp-toolchain01 /bin/bash
```

get a shell via ssh
```
ssh developer@localhost -p 2222
```

stop or restart
```
docker start/stop cpp-toolchain01
```
