#!/usr/bin/env bash
#
# URL encodes the path segments of the provided absolute path

set -o errexit -o nounset -o pipefail

main() {
	if (( $# < 1 )); then
		printf 'Requires a path to encode\n' >&2
		return 1
	fi

	local path="$1"

	local encodedPath
	encodedPath="$(urlencode -m "$path")"

	# Some URL decoders can't handle spaces encoded as '+', so change to ASCII
   # form.
	printf '%s' "${encodedPath//+/%20}"
}

main "$@"
