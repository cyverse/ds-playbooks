#!/bin/bash
#
# Usage:
#  test-playbook INSPECT PRETTY VERBOSE HOSTS SETUP [PLAYBOOK] 
#
# Parameters:
#  HOSTS     the inventory hosts to test against
#  INSPECT   if this is `true`, a shell will be opened that allows access to the
#            volumes in the env containers.
#  PLAYBOOK  the name of the playbook being tested.
#  PRETTY    if this is `true`, more info is dumped and newlines in ouput are
#            expanded.
#  SETUP     the name of a playbook that prepares the environment for testing
#            PLAYBOOK 
#  VERBOSE   if this is set to any value, ansible will be passed the verbose
#            flag -vvv
#
# This program executes and ansible playbook on the test environment.

set -o errexit -o nounset -o pipefail


main() {
	local inspect="$1"
	local pretty="$2"
	local verbose="$3"
	local hosts="$4"
	local setup="$5"

	local playbook 
	if (( $# >= 6 ))
	then
		playbook="$6"
	fi

	if [[ "$pretty" == true ]]
	then
		export ANSIBLE_STDOUT_CALLBACK=minimal
	fi

	if [[ -n "$setup" || -n "${playbook-}" ]]
	then
		do_test "$verbose" "$hosts" "$setup" "${playbook-}"
	fi || true

	if [ "$inspect" = true ]
	then
		printf 'Opening shell for inspection of volumes\n'
		bash
	fi || true

	return 0
}


do_test() {
	local verbose="$1"
	local hosts="$2"
	local setup="$3"
	local playbook="$4"

	local inventory=/inventory/"$hosts"

	local verbosity
	if [ -n "$verbose" ]
	then
		verbosity=-vvv
	fi

	printf 'Waiting for environment to be ready\n'
	if ! ansible-playbook --inventory-file "$inventory" /wait-for-ready.yml > /dev/null
	then
		return 1
	fi

	if [[ -n "$setup" ]]
	then
		local setupPath=/playbooks-under-test/"$setup"

		printf 'Preparing environment for testing playbook\n'
		if ! \
			ansible-playbook ${verbosity=} --inventory-file "$inventory" --skip-tags no_testing \
				"$setupPath" 
		then
			return 1
		fi
	fi

	if [[ -n "$playbook" ]]
	then
		local playbookPath=/playbooks-under-test/"$playbook"
		local testPath=/playbooks-under-test/tests/"$playbook"

		printf 'Checking playbook syntax\n'
		if ! ansible-playbook --syntax-check --inventory-file "$inventory" "$playbookPath"
		then
			return 1
		fi

		printf 'Running playbook\n'
		if ! \
			ansible-playbook ${verbosity=} --inventory-file "$inventory" --skip-tags no_testing \
				"$playbookPath"
		then
			return 1
		fi

		local libPathOption
		# add the option for module-path only if a library directory exists
		if [ -d /playbooks-under-test/library ]
		then
			libPathOption="--module-path /playbooks-under-test/library"
		fi

		if [ -e "$testPath" ]
		then
			printf 'Checking configuration\n'
			if ! \
				ansible-playbook ${verbsosity=} --inventory-file "$inventory" ${libPathOption=} \
					"$testPath"
			then
				return 1
			fi
		fi

		printf 'Checking idempotency\n'

		local idempotencyRes
		idempotencyRes="$( \
      	ansible-playbook --inventory-file "$inventory" --skip-tags 'no_testing, non_idempotent' \
					"$playbookPath" \
				2>&1 )"

		if grep --quiet --regexp '^\(changed\|failed\):' <<< "$idempotencyRes"
		then
			printf '%s\n' "$idempotencyRes"
			return 1
		fi
	fi
}


main "$@"
