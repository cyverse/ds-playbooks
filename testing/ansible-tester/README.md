# ansible-tester

This folder contains the source for the `ansible-tester` Docker image. A
container created from this image can be used to test a playbook against a
simple Data Store setup.

## Sets of Tests

`ansible-tester` performs three sets tests in the order they are presented here.

1. It performs a syntax check of the playbook under test.
1. It runs the playbook under test and then runs any user provided tests on the same environment.
1. It runs the playbook under test again, checking for idempotency.

### User Provided Tests

If any tests are to be performed on the environment after the first run of the
playbook, they should be placed in a playbook with the same name as the playbook
under test. This test playbook should be in a directory named `tests` that is in
the same directory as the playbook under test. Here's an example.

```
parent-dir/
  playbook.yml
  tests/
    playbook.yml
```

### Skipping Idempotency Checks

If any task in the playbook is guaranteed to be non-idempotent, .e.g., a forced
restart, `ansible-tester` would normally falsely fail the test. To let
`ansible-tester` know that a task is non-idempotent, add the tag
`non_idempotent` to this task, and it will be skipped when performing the
idempotency check.

### Skipping Certain Tasks

Some tasks can't be run inside a Docker container, e.g., /etc/hosts can't be
modified. Tag each of these tasks with `no_testing`, and those tasks will be
skipped.

## Testing environment

_TODO document_

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
