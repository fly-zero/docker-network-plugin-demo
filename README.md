# Docker Network Plugin Demo

This project demonstrates how to create and use a custom Docker network plugin.

## Prerequisites

- Docker
- GCC
- BOOST

## Build

1. Clone the repository:
    ```sh
    git clone https://github.com/fly-zero/docker-network-plugin-demo.git
    cd docker-network-plugin-demo
    ```

2. Build the plugin:
    ```sh
    mkdir build
    cd build
    cmake ..
    make
    ```

## Usage

1. Launch the plugin server
   - listen on unix domain socket
        ```sh
        ./test-net /run/docker/plugins/test-net.sock
        ```

    - listen on tcp port
        ```sh
        mkdir -pv /etc/docker/plugins
        echo 'tcp://localhost:5678' > /etc/docker/plugins/test-net.spec
        ./test-net 5678
        ```

2. Create a Docker network using the `test-net` plugin:
    ```sh
    docker network create -d test-net -o "driverDefinedParam=foo" my-net
    ```

3. Run a container using the custom network:
    ```sh
    docker run -it --rm --net my-net alpine ip a
    ```