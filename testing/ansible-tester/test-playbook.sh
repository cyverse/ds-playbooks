#!/bin/bash
#
# Usage:
#  test-playbook INSPECT PRETTY VERBOSE HOSTS SETUP PLAYBOOK
#
# Parameters:
#  HOSTS     the inventory hosts to test against
#  INSPECT   if this is set to any value, a shell will be opened that allows 
#            access to the volumes in the env containers.
#  PLAYBOOK  the name of the playbook being tested.
#  PRETTY    if this is set to any value, more info is dumped and newlines in 
#            ouput are expanded.
#  SETUP     the name of a playbook that prepares the environment for testing
#            PLAYBOOK 
#  VERBOSE   if this is set to any value, ansible will be passed the verbose
#            flag -vvv
#
# This program executes and ansible playbook on the test environment.

set -o errexit -o nounset -o pipefail

readonly PLAYBOOK_DIR=/playbooks-under-test
readonly LIBRARY_DIR="$PLAYBOOK_DIR"/library
readonly TEST_DIR="$PLAYBOOK_DIR"/tests


main() {
	local inspect="$1"
	local pretty="$2"
	local verbose="$3"
	local hosts="$4"
	local setup="$5"
	local playbook="$6"

	local inventory=/inventory/"$hosts"

	if [[ -n "$pretty" ]]; then
		export ANSIBLE_STDOUT_CALLBACK=minimal
	fi

	# add the option for module-path only if a library directory exists
	local modPath=
	if [[ -d "$LIBRARY_DIR" ]]; then
		modPath="$LIBRARY_DIR"
	fi

	wait_for_env "$inventory"

	local rc=0

	if (( rc == 0 )) && [[ -n "$setup" ]]; then
		if ! setup_env "$verbose" "$inventory" "$modPath" "$PLAYBOOK_DIR"/"$setup"; then
			rc=1
		fi
	fi

	if (( rc == 0 )) && [[ -n "$playbook" ]]; then
		local playbookPath="$PLAYBOOK_DIR"/"$playbook"
      local testPath="$TEST_DIR"/"$playbook"

		if ! do_test "$verbose" "$inventory" "$modPath" "$playbookPath" "$testPath" ; then
			rc=1
		fi
	fi

	if [[ -n "$inspect" ]]; then
		printf 'opening shell for inspection of volumes\n'
		bash
	fi || true

	return $rc
}


do_test() {
	local verbose="$1"
	local inventory="$2"
	local modPath="$3"
	local playbook="$4"
	local test="$5"

	local verboseOpt
	verboseOpt="$(verbosity "$verbose")"

	printf 'checking playbook syntax\n'
	if \
		! \
			ansible-playbook --syntax-check --inventory-file "$inventory" --module-path="$modPath" \
				"$playbook"
	then
		return 1
	fi

	printf 'running playbook\n'
	# shellcheck disable=SC2086
	if \
		! \
			ansible-playbook $verboseOpt \
				--inventory-file="$inventory" --module-path="$modPath" --skip-tags=no_testing \
				"$playbook"
	then
		return 1
	fi

	if [[ -e "$test" ]]; then
		printf 'testing configuration\n'
		# shellcheck disable=SC2086
		if \
			! \
				ansible-playbook $verboseOpt --module-path="$modPath" --inventory-file="$inventory" \
					"$test"
		then
			return 1
		fi
	fi

	printf 'checking idempotency\n'

	local idempotencyRes
	idempotencyRes="$(run_idempotency "$inventory" "$modPath" "$playbook")"

	if grep --quiet --regexp '^\(changed\|fatal\):' <<< "$idempotencyRes"; then
		echo "$idempotencyRes"
		return 1
	fi
}


run_idempotency() {
	local inventory="$1"
	local modPath="$2"
	local playbook="$3"

   ansible-playbook \
			--inventory-file="$inventory" \
			--module-path="$modPath" \
			--skip-tags='no_testing, non_idempotent' \
			"$playbook" \
		2>&1
}


setup_env() {
	local verbose="$1"
	local inventory="$2"
	local modPath="$3"
	local setup="$4"

	local verboseOpt
	verboseOpt="$(verbosity "$verbose")"

	printf 'preparing environment for testing playbook\n'
	# shellcheck disable=SC2086
	ansible-playbook $verboseOpt \
		--inventory-file="$inventory" --module-path="$modPath" --skip-tags=no_testing \
		"$setup" 
}


wait_for_env() {
	local inventory="$1"
	local output
	
	printf 'waiting for environment to be ready\n'
	if ! output=$(ansible-playbook --inventory-file="$inventory" /wait-for-ready.yml); then
		printf 'failed to bring up the environment\n%s', "$output"
		return 1
	fi
}


verbosity() {
	local verbose="$1"

	if [[ -n "$verbose" ]]; then
		printf -- '-vvv'
	fi
}


main "$@"
