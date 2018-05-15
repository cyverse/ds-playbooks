# VERSION: 1
#
# The custom policies for the DE

@include 'de-env'

# Sets the policies assigned the the DE job user when it is created.
#
de_acCreateUser {
  msiCreateUser ::: msiRollback;
  msiCommit;
}
