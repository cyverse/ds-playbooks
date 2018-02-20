# ansible-tester

This folder contains the source for the `ansible-tester` Docker image. A
container created from this image can be used to test a playbook against a
simple Data Store setup.

## Building the Image

There are two convenience scripts for building the Docker image. `build` will
build the image, and `clean` will delete the image.

## Usage

The container assumes that it will be joining a Docker network created for the
docker compose in `../env`.  The name of this network needs to be provided on
the `docker run` command line. It also assumes that the playbook to be tested
will be in a directory mounted into the container at `/playbooks-under-test`.
Finally, the path to the playbook to test should be provided as the last
argument on the command line. The path should be relative to the mounted
directory.

Here's an example.

```bash
docker run -itv "$(pwd)"/playbooks:/playbooks-under-test:ro --rm --network env_default \
           ansible-tester main.yml
```
