# iRODS rule language logic for working with JSON documents
#
# 1. Only the control characters for horizontal tab, carriage return, and line
#    feed are supported in strings.
# 2. exponential notation is not supported.
# 3. Numbers may not begin with a . or a +.
#
# © 2022 The Arizona Board of Regents on behalf of The University of Arizona.
# For license information, see https://cyverse.org/license.

_cyverse_json_substrRem : string * int -> string
_cyverse_json_substrRem(*String, *NewHdPos) = substr(*String, *NewHdPos, strlen(*String))

_cyverse_json_strHd : string -> string
_cyverse_json_strHd(*String) = substr(*String, 0, 1)

_cyverse_json_strTl : string -> string
_cyverse_json_strTl(*String) = _cyverse_json_substrRem(*String, 1)

_cyverse_json_trimLeadingSpace : string -> string
_cyverse_json_trimLeadingSpace(*String) =
	if !(*String like regex '^[[:space:]].*') then *String
  else _cyverse_json_trimLeadingSpace(_cyverse_json_strTl(*String))

_cyverse_json_append : string * string * list string -> string
_cyverse_json_append(*Base, *Separator, *Elements) =
	if size(*Elements) == 0 then *Base
	else
		let *sep = if *Base == '' then '' else *Separator in
		_cyverse_json_append(*Base ++ *sep ++ hd(*Elements), *Separator, tl(*Elements))

_cyverse_json_join : string * list string -> string
_cyverse_json_join(*Separator, *Elements) = _cyverse_json_append('', *Separator, *Elements)

_cyverse_json_rev : list ? -> list ?
_cyverse_json_rev(*List) = _cyverse_json_revAccum(list(), *List)

_cyverse_json_revAccum : list ? * list ? -> list ?
_cyverse_json_revAccum(*RevList, *List) =
	if size(*List) == 0 then *RevList
	else _cyverse_json_revAccum(cons(hd(*List), *RevList), tl(*List))

_cyverse_json_encode : string -> string
_cyverse_json_encode(*Unencoded) = _cyverse_json_encodeAccum('', *Unencoded)

_cyverse_json_encodeAccum : string * string -> string
_cyverse_json_encodeAccum(*Encoded, *Unencoded) =
	if strlen(*Unencoded) == 0 then *Encoded
	else
		let *c = _cyverse_json_strHd(*Unencoded) in
		let *escC =
			if *c == '"' then '\\"'
			else if *c == '\n' then '\\n'
			else if *c == '\r' then '\\r'
			else if *c == '\t' then '\\t'
			else if *c == '\\' then '\\\\'
			else *c in
		_cyverse_json_encodeAccum(*Encoded ++ *escC, _cyverse_json_strTl(*Unencoded))

# describes the different types of JSON values
#
data cyverse_json_val =
	# represents an empty value, which is not part of the JSON standard but
	# makes a convenient placeholder for an empty result
	#
	| cyverse_json_empty : cyverse_json_val

	# represents a null
	#
	| cyverse_json_null : cyverse_json_val

	# represents a Boolean
	#
	| cyverse_json_bool : boolean -> cyverse_json_val

	# represents a number
	#
	| cyverse_json_num : f double -> cyverse_json_val

	# represents a string
	#
	| cyverse_json_str : string -> cyverse_json_val

	# represents an array
	#
	| cyverse_json_array : list cyverse_json_val -> cyverse_json_val

	# represents an object as a list of fields, where each field is an ordered
	# pair where the first is the name and the second is the value
	#
	| cyverse_json_obj : list (string * cyverse_json_val) -> cyverse_json_val

_cyverse_json_isEmpty : cyverse_json_val -> boolean
_cyverse_json_isEmpty(*Val) =
	match *Val with
		| cyverse_json_array(*a) => false
		| cyverse_json_bool(*b) => false
		| cyverse_json_null => false
		| cyverse_json_num(*n) => false
		| cyverse_json_obj(*o) => false
		| cyverse_json_str(*s) => false
		| cyverse_json_empty => true

# serializes a JSON document
# Parameters:
#  *Val  the document (value) to serialize
# Returns:
#  the serialized document
#
json_serialize : cyverse_json_val -> string
json_serialize(*Val) =
	match *Val with
		| cyverse_json_array(*a)
			=> '[' ++ _cyverse_json_join(',', _cyverse_json_serializeScalars(*a)) ++ ']'
		| cyverse_json_bool(*b) => str(*b)
		| cyverse_json_null => 'null'
		| cyverse_json_num(*n) => str(*n)
		| cyverse_json_obj(*o)
			=> '{' ++ _cyverse_json_join(',', _cyverse_json_serializeFields(*o)) ++ '}'
		| cyverse_json_str(*s) => '"' ++ _cyverse_json_encode(*s) ++ '"'
		| cyverse_json_empty => ''

_cyverse_json_serializeScalarsAccum : list string * list cyverse_json_val -> list string
_cyverse_json_serializeScalarsAccum(*RevSerialized, *Unserialized) =
	if size(*Unserialized) == 0 then _cyverse_json_rev(*RevSerialized)
	else
    _cyverse_json_serializeScalarsAccum(
		cons(json_serialize(hd(*Unserialized)), *RevSerialized), tl(*Unserialized) )

_cyverse_json_serializeScalars : list cyverse_json_val -> list string
_cyverse_json_serializeScalars(*Unserialized) =
	_cyverse_json_serializeScalarsAccum(list(), *Unserialized)

_cyverse_json_serializeFieldsAccum : list string * list (string * cyverse_json_val) -> list string
_cyverse_json_serializeFieldsAccum(*RevSerialized, *Unserialized) =
	if size(*Unserialized) == 0 then _cyverse_json_rev(*RevSerialized)
	else
		let (*name, *value) = hd(*Unserialized) in
		let *serializedField =
			'"' ++ _cyverse_json_encode(*name) ++ '":' ++ cyverse_json_serialize(*value) in
		_cyverse_json_serializeFieldsAccum(cons(*serializedField, *RevSerialized), tl(*Unserialized))

_cyverse_json_serializeFields : list (string * cyverse_json_val) -> list string
_cyverse_json_serializeFields(*Unserialized) =
	_cyverse_json_serializeFieldsAccum(list(), *Unserialized)

# describes a response from a deserialization operation, where Value is the type
# of the deserialized value
data cyverse_json_deserialize_res(Value) =
	# the success result, an ordered-pair containing the deserialized value and
	# the remainder of the serialization string
	| cyverse_json_deserialize_val : Value * string -> cyverse_json_deserialize_res(Value)

	# the failure result, an ordered-triple containing an error message, the
	# portion deserialized so far and the remainder of the serialization string
	| cyverse_json_deserialize_err : string * Value * string -> cyverse_json_deserialize_res(Value)

json_deserialize : string -> cyverse_json_deserialize_res(json_val)
json_deserialize(*Serial) =
	let *Serial = _cyverse_json_trimLeadingSpace(*Serial) in
	if *Serial == '' then cyverse_json_deserialize_val(json_empty, *Serial)
	else
		let *res = _cyverse_json_deserializeValue(*Serial) in
		match *res with
			| cyverse_json_deserialize_err(*m, *v, *s) => *res
			| cyverse_json_deserialize_val(*val, *Serial) =>
				let *Serial = _cyverse_json_trimLeadingSpace(*Serial) in
				if *Serial == '' then cyverse_json_deserialize_val(*val, *Serial)
				else cyverse_json_deserialize_err('unexpected content after document', *val, *Serial)

_cyverse_json_deserializeValue : string -> cyverse_json_deserialize_res(json_val)
_cyverse_json_deserializeValue(*Serial) =
	let *Serial = _cyverse_json_trimLeadingSpace(*Serial) in
	if *Serial like 'null*' then _cyverse_json_deserializeNull(*Serial)
	else if *Serial like '"*' then _cyverse_json_deserializeString(*Serial)
	else if *Serial like regex '^(false|true).*' then _cyverse_json_deserializeBoolean(*Serial)
	else if *Serial like regex '^-\{0,1\}[0-9].*' then _cyverse_json_deserializeNumber(*Serial)
	else if *Serial like '[*' then _cyverse_json_deserializeArray(*Serial)
	else if *Serial like '{*' then _cyverse_json_deserializeObject(*Serial)
	else cyverse_json_deserialize_err('there is no legal next value', cyverse_json_empty, *Serial)

_cyverse_json_deserializeArray : string -> cyverse_json_deserialize_res(json_val)
_cyverse_json_deserializeArray(*Serial) =
	let *Serial = _cyverse_json_strTl(*Serial) in                    # remove opening bracket
	match _cyverse_json_deserializeArrayAccum(list(), *Serial) with
		| cyverse_json_deserialize_err(*msg, *elmts, *Serial) =>
			cyverse_json_deserialize_err(*msg, cyverse_json_array(*elmts), *Serial)
		| cyverse_json_deserialize_val(*elmts, *Serial) =>
			let *Serial = _cyverse_json_trimLeadingSpace(*Serial) in
			if *Serial like ']*' then
				cyverse_json_deserialize_val(json_array(*elmts), _cyverse_json_strTl(*Serial))
			else cyverse_json_deserialize_err(
				'missing end of array', cyverse_json_array(*elmts), *Serial )

_cyverse_json_deserializeArrayAccum :
	list cyverse_json_val * string -> cyverse_json_deserialize_res(list cyverse_json_val)
_cyverse_json_deserializeArrayAccum(*RevCurElmts, *Serial) =
	let *Serial = _cyverse_json_trimLeadingSpace(*Serial) in
	if *Serial like regex '^(,|]).*' then
		cyverse_json_deserialize_val(_cyverse_json_rev(*RevCurElmts), *Serial)
	else
		match _cyverse_json_deserializeValue(*Serial) with
			| cyverse_json_deserialize_err(*msg, *val, *Serial) =>
				let *RevCurElmts =
					if _cyverse_json_isEmpty(*val) then *RevCurElmts else cons(*val, *RevCurElmts) in
				cyverse_json_deserialize_err(*msg, _cyverse_json_rev(*RevCurElmts), *Serial)
			| cyverse_json_deserialize_val(*newElmt, *Serial) =>
				let *Serial = _cyverse_json_trimLeadingSpace(*Serial) in
				let *Serial =
					if *Serial like regex '^,[[:space:]]*[^\]].*' then _cyverse_json_strTl(*Serial)
					else *Serial in
					_cyverse_json_deserializeArrayAccum(cons(*newElmt, *RevCurElmts), *Serial)

_cyverse_json_deserializeBoolean : string -> cyverse_json_deserialize_res(json_val)
_cyverse_json_deserializeBoolean(*Serial) =
	if *Serial like 'false*' then
		cyverse_json_deserialize_val(json_bool(false), _cyverse_json_substrRem(*Serial, 5))
	else cyverse_json_deserialize_val(json_bool(true), _cyverse_json_substrRem(*Serial, 4))

_cyverse_json_deserializeNull : string -> cyverse_json_deserialize_res(json_val)
_cyverse_json_deserializeNull(*Serial) =
	cyverse_json_deserialize_val(json_null, _cyverse_json_substrRem(*Serial, 4))

# FORMAT -?[0-9]+(\.[0-9]*)?
_cyverse_json_deserializeNumber : string -> cyverse_json_deserialize_res(json_val)
_cyverse_json_deserializeNumber(*Serial) =
	let (*numStrBuf, *Serial) =
		if *Serial like '-*' then ('-', _cyverse_json_strTl(*Serial)) else ('', *Serial) in
	let (*numStrBuf, *Serial) = _cyverse_json_extractDigits(*numStrBuf, *Serial) in
	let (*numStrBuf, *Serial) =
		if *Serial like '.*' then
			_cyverse_json_extractDigits(*numStrBuf ++ '.', _cyverse_json_strTl(*Serial))
		else (*numStrBuf, *Serial) in
	cyverse_json_deserialize_val(json_num(double(*numStrBuf)), *Serial)

_cyverse_json_extractDigits : string * string -> string * string
_cyverse_json_extractDigits(*Buf, *Serial) =
	if *Serial == '' || !(*Serial like regex '^[0-9].*') then (*Buf, *Serial)
	else _cyverse_json_extractDigits(
		*Buf ++ _cyverse_json_strHd(*Serial), _cyverse_json_strTl(*Serial))

_cyverse_json_deserializeObject : string -> cyverse_json_deserialize_res(json_val)
_cyverse_json_deserializeObject(*Serial) =
	let *Serial = _cyverse_json_strTl(*Serial) in                     # remove opening brace
	match _cyverse_json_deserializeObjectAccum(list(), *Serial) with
		| cyverse_json_deserialize_err(*msg, *fields, *Serial) =>
			cyverse_json_deserialize_err(*msg, cyverse_json_obj(*fields), *Serial)
		| cyverse_json_deserialize_val(*fields, *Serial) =>
			let *Serial = _cyverse_json_trimLeadingSpace(*Serial) in
			if *Serial like '}*' then
				cyverse_json_deserialize_val(json_obj(*fields), _cyverse_json_strTl(*Serial))
			else cyverse_json_deserialize_err(
				'missing end of object', cyverse_json_obj(*fields), *Serial)

_cyverse_json_deserializeObjectAccum
	: list (string * cyverse_json_val) * string
	-> cyverse_json_deserialize_res(list (string * cyverse_json_val))
_cyverse_json_deserializeObjectAccum(*RevCurFields, *Serial) =
	let *Serial = _cyverse_json_trimLeadingSpace(*Serial) in
	if *Serial like regex '^[,}].*' then
		cyverse_json_deserialize_val(_cyverse_json_rev(*RevCurFields), *Serial)
	else
		match _cyverse_json_deserializeField(*Serial) with
			| cyverse_json_deserialize_err(*msg, (*name, *val), *Serial) =>
				let *RevCurFields =
					if *name == '' && _cyverse_json_isEmpty(*val) then *RevCurFields
					else cons((*name, *val), *RevCurFields) in
				cyverse_json_deserialize_err(*msg, _cyverse_json_rev(*RevCurFields), *Serial)
			| cyverse_json_deserialize_val(*newField, *Serial) =>
				let *Serial = _cyverse_json_trimLeadingSpace(*Serial) in
				let *Serial =
					if *Serial like regex '^,[[:space:]]*[^}].*' then _cyverse_json_strTl(*Serial)
					else *Serial in
				_cyverse_json_deserializeObjectAccum(cons(*newField, *RevCurFields), *Serial)

_cyverse_json_deserializeField : string -> cyverse_json_deserialize_res((string * cyverse_json_val))
_cyverse_json_deserializeField(*Serial) =
	let *initSerial = *Serial in
	match _cyverse_json_extractString(*Serial) with
		| cyverse_json_deserialize_err(*m, *n, *Serial) =>
			cyverse_json_deserialize_err('invalid field name', ('', cyverse_json_empty), *initSerial)
		| cyverse_json_deserialize_val(*name, *Serial) =>
			let *Serial = _cyverse_json_trimLeadingSpace(*Serial) in
			if !(*Serial like ':*') then
				cyverse_json_deserialize_err(
					'object field is missing value', (*name, cyverse_json_empty), *Serial)
			else
				match _cyverse_json_deserializeValue(_cyverse_json_strTl(*Serial)) with
					| cyverse_json_deserialize_err(*msg, *val, *Serial) =>
						cyverse_json_deserialize_err(*msg, (*name, *val), *Serial)
					| cyverse_json_deserialize_val(*val, *Serial) =>
						cyverse_json_deserialize_val((*name, *val), *Serial)

_cyverse_json_deserializeString : string -> cyverse_json_deserialize_res(json_val)
_cyverse_json_deserializeString(*Serial) =
	match _cyverse_json_extractString(*Serial) with
		| cyverse_json_deserialize_err(*msg, *val, *Serial) =>
			cyverse_json_deserialize_err(*msg, cyverse_json_empty, *Serial)
		| cyverse_json_deserialize_val(*val, *Serial) =>
			cyverse_json_deserialize_val(json_str(*val), *Serial)

_cyverse_json_extractString : string -> cyverse_json_deserialize_res(string)
_cyverse_json_extractString(*Serial) =
	let *initSerial = *Serial in
	let *Serial = triml(*Serial, '"') in                            # Remove opening mark
	let (*str, *Serial) = _cyverse_json_extractStringAccum('', *Serial) in
	if *Serial like '"*' then cyverse_json_deserialize_val(*str, _cyverse_json_strTl(*Serial))
	else cyverse_json_deserialize_err('missing end of string', '', *initSerial)

_cyverse_json_extractStringAccum : string * string -> string * string
_cyverse_json_extractStringAccum(*Buf, *Serial) =
	if *Serial == '' || *Serial == '\\' || *Serial like '"*' then (*Buf, *Serial)
	else
		let (*Buf, *Serial) =
			if *Serial like '\\*' then
				let *escChar = substr(*Serial, 0, 2) in
				let *char =
					if *escChar == 'n' then '\n'
					else if *escChar == 'r' then '\r'
					else if *escChar == 't' then '\t'
					else *escChar in
				(*Buf ++ *char, _cyverse_json_substrRem(*Serial, 2))
			else (*Buf ++ _cyverse_json_strHd(*Serial), _cyverse_json_strTl(*Serial)) in
		_cyverse_json_extractStringAccum(*Buf, *Serial)

json_getValue : cyverse_json_val * string -> cyverse_json_val
json_getValue(*Doc, *FieldName) =
	match *Doc with
		| cyverse_json_empty => cyverse_json_empty
		| cyverse_json_null => cyverse_json_empty
		| cyverse_json_bool(*b) => cyverse_json_empty
		| cyverse_json_num(*n) => cyverse_json_empty
		| cyverse_json_str(*s) => cyverse_json_empty
		| cyverse_json_array(*l) => cyverse_json_empty
		| cyverse_json_obj(*fields) =>
			let *ans = cyverse_json_empty in
			let *_ = foreach (*field in *fields) {
				(*name, *val) = *field;
				if (*FieldName == *name) {
					*ans = *val;
				} } in
			*ans
