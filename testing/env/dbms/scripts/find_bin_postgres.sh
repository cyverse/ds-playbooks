#! /bin/bash


main()
{
  # find whatever psql is in the current user's path
  if ! psqlPath=$(which psql)
  then
    printf 'No postgres [psql] found. Aborting.\n' >&2
    return 1
  fi

  # return the bin directory holding psql
  dirname "$psqlPath"
}


set -e

main
