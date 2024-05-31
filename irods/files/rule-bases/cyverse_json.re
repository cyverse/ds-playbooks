# This is a collection of functions and rules for constructing serialized JSON
# documents. It does not support arrays yet.
#
# © 2021 The Arizona Board of Regents on behalf of The University of Arizona.
# For license information, see https://cyverse.org/license.

_cyverse_json_accumEncodedList(*Base, *SerialElmts) =
	if size(*SerialElmts) == 0 then *Base
	else
		let *prefix = if *Base == '' then '' else *Base ++ ',' in
		_cyverse_json_accumEncodedList(*prefix ++ hd(*SerialElmts), tl(*SerialElmts))

_cyverse_json_encodeObject(*SerialFields) =
	'{' ++ _cyverse_json_accumEncodedList('', *SerialFields) ++ '}'

_cyverse_json_encodeString(*Str) =
	let *str = str(*Str) in
	let *escStr = '' in
	let *len = strlen(*str) in
	let *pos = 0 in
	let *_ = while (*len > *pos) {
		let *c = substr(*str, *pos, *pos + 1) in
		let *escC =
			if *c == '"' then '\\"'
			else if *c == '\t' then '\\t'
			else if *c == '\n' then '\\n'
			else if *c == '\r' then '\\r'
			else if *c == '\\' then '\\\\'
			else *c in
		*escStr = *escStr ++ *escC;
		*pos = *pos + 1; }
	in '"' ++ *escStr ++ '"'

_cyverse_json_mkField(*Label, *SerialVal) = '"' ++ *Label ++ '":' ++ *SerialVal

# construct a serialized JSON object field from a Boolean value
#
# Parameters:
#   *Label - the name of the object field
#   *Val - the Boolean value of the field.
#
cyverse_json_boolean: string * boolean -> string
cyverse_json_boolean(*Label, *Val) = _cyverse_json_mkField(*Label, if *Val then 'true' else 'false')

# construct a serialized JSON document from its serialized fields
#
# Parameters:
#   *SerialFields - A list of pre-serialized fields to include in the document
#
cyverse_json_document: list string -> string
cyverse_json_document(*SerialFields) = _cyverse_json_encodeObject(*SerialFields)

# construct a serialized JSON number field. This version performs no error checking.
#
# Parameters:
#   *Label - the name of the field
#   *Val - the value of the field
#
cyverse_json_number: forall V in {double string}, string * f V -> string
cyverse_json_number(*Label, *Val) = _cyverse_json_mkField(*Label, '*Val')

# construct a serialized JSON object field from its serialized fields
#
# Parameters:
#   *Label - the name of the object field
#   *SerialFields - the list of pre-serialized fields ot include in the object
#
cyverse_json_object: string * list string -> string
cyverse_json_object(*Label, *SerialFields) =
	_cyverse_json_mkField(*Label, _cyverse_json_encodeObject(*SerialFields))

# construct a serialized JSON string field
#
# Parameters:
#   *Label - the name of the field
#   *Val - the value of the field
#
cyverse_json_string: forall V in {path string time}, string * V -> string
cyverse_json_string(*Label, *Val) = _cyverse_json_mkField(*Label, _cyverse_json_encodeString(*Val))
