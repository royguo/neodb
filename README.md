# neodb

## Docker Image
You could use the `docker/Dockerfile` to compile & run NeoDB's docker image for development or demo run.

```
cd docker
# build the docker image, name it as `neodb_image`
docker build -t neodb_image .

# Start a container on privileges mode
sudo docker run --cap-add=SYS_PTRACE \
                --security-opt seccomp=unconfined \
                --name=neodb \
                -it -v {HOST_NEODB_PATH}:{CONTAINER_NEODB_PATH} neodb_image

```


## Memory Cost
NeoDB is designed for columar storage, which means all values should be relatively large (e.g. > 64KB).

If we have a 30TB HDD disk, then:
- Total KV pairs should probally be 30TB/64KB = 500 Millions
- Total memory cost is 500M * 30B(average key size) * 2(load factor) = 3000MB
