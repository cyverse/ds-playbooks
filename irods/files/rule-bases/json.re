# iRODS rule language logic for working with JSON documents
#
# 1. Only the control characters for horizontal tab, carriage return, and line 
#    feed are supported in strings.
# 2. exponential notation is not supported.
# 3. Numbers may not begin with a . or a +.
#
# © 2022 The Arizona Board of Regents on behalf of The University of Arizona.
# For license information, see https://cyverse.org/license.

_json_substrRem : string * int -> string
_json_substrRem(*String, *NewHdPos) = substr(*String, *NewHdPos, strlen(*String))

_json_strHd : string -> string
_json_strHd(*String) = substr(*String, 0, 1)

_json_strTl : string -> string
_json_strTl(*String) = _json_substrRem(*String, 1)

_json_trimLeadingSpace : string -> string
_json_trimLeadingSpace(*String) = 
	if !(*String like regex '^[[:space:]].*') then *String 
  else _json_trimLeadingSpace(_json_strTl(*String))

_json_append : string * string * list string -> string
_json_append(*Base, *Separator, *Elements) =
	if size(*Elements) == 0 then *Base 
	else
		let *sep = if *Base == '' then '' else *Separator in 
		_json_append(*Base ++ *sep ++ hd(*Elements), *Separator, tl(*Elements))

_json_join : string * list string -> string
_json_join(*Separator, *Elements) = _json_append('', *Separator, *Elements)

_json_rev : list ? -> list ?
_json_rev(*List) = _json_revAccum(list(), *List)

_json_revAccum : list ? * list ? -> list ?
_json_revAccum(*RevList, *List) = 
	if size(*List) == 0 then *RevList else _json_revAccum(cons(hd(*List), *RevList), tl(*List)) 

_json_encode : string -> string
_json_encode(*Unencoded) = _json_encodeAccum('', *Unencoded)

_json_encodeAccum : string * string -> string
_json_encodeAccum(*Encoded, *Unencoded) =
	if strlen(*Unencoded) == 0 then *Encoded
	else
		let *c = _json_strHd(*Unencoded) in
		let *escC = 
			if *c == '"' then '\\"' 
			else if *c == '\n' then '\\n' 
			else if *c == '\r' then '\\r' 
			else if *c == '\t' then '\\t' 
			else if *c == '\\' then '\\\\' 
			else *c in 
		_json_encodeAccum(*Encoded ++ *escC, _json_strTl(*Unencoded))

# describes the different types of JSON values
#
data json_val =
	# represents an empty value, which is not part of the JSON standard but
	# makes a convient placeholder for an empty result
	#
	| json_empty : json_val

	# represents a null
	#
	| json_null : json_val

	# represents a Boolean
	#
	| json_bool : boolean -> json_val

	# represents a number
	#
	| json_num : f double -> json_val

	# represents a string
	#
	| json_str : string -> json_val

	# represents an array
	#
	| json_array : list json_val -> json_val

	# represents an object as a list of fields, where each field is an ordered
	# pair where the first is the name and the second is the value
	#
	| json_obj : list (string * json_val) -> json_val

_json_isEmpty : json_val -> boolean
_json_isEmpty(*Val) =
	match *Val with
		| json_array(*a) => false 
		| json_bool(*b) => false
		| json_null => false
		| json_num(*n) => false
		| json_obj(*o) => false
		| json_str(*s) => false
		| json_empty => true

# serializes a JSON document
# Parameters:
#  *Val  the document (value) to serialize
# Returns:
#  the serialized document
#
json_serialize : json_val -> string
json_serialize(*Val) =
	match *Val with
		| json_array(*a) => '[' ++ _json_join(',', _json_serializeScalars(*a)) ++ ']' 
		| json_bool(*b) => str(*b)
		| json_null => 'null'
		| json_num(*n) => str(*n)
		| json_obj(*o) => '{' ++ _json_join(',', _json_serializeFields(*o)) ++ '}'
		| json_str(*s) => '"' ++ _json_encode(*s) ++ '"'
		| json_empty => ''

_json_serializeScalarsAccum : list string * list json_val -> list string
_json_serializeScalarsAccum(*RevSerialized, *Unserialized) =
	if size(*Unserialized) == 0 then _json_rev(*RevSerialized)
	else 
    _json_serializeScalarsAccum(
		cons(json_serialize(hd(*Unserialized)), *RevSerialized), tl(*Unserialized) )

_json_serializeScalars : list json_val -> list string
_json_serializeScalars(*Unserialized) = _json_serializeScalarsAccum(list(), *Unserialized)

_json_serializeFieldsAccum : list string * list (string * json_val) -> list string
_json_serializeFieldsAccum(*RevSerialized, *Unserialized) = 
	if size(*Unserialized) == 0 then _json_rev(*RevSerialized)
	else
		let (*name, *value) = hd(*Unserialized) in
		let *serializedField = '"' ++ _json_encode(*name) ++ '":' ++ json_serialize(*value) in
		_json_serializeFieldsAccum(cons(*serializedField, *RevSerialized), tl(*Unserialized))

_json_serializeFields : list (string * json_val) -> list string
_json_serializeFields(*Unserialized) = _json_serializeFieldsAccum(list(), *Unserialized)

# describes a response from a deserialization operation, where Value is the type
# of the deserialized value
data json_deserialize_res(Value) =
	# the success result, an ordered-pair containing the deserialized value and
	# the remainder of the serialization string
	| json_deserialize_val : Value * string -> json_deserialize_res(Value)

	# the failure result, an ordered-triple containing an error message, the 
	# portion deserialized so far and the remainder of the serialization string
	| json_deserialize_err : string * Value * string -> json_deserialize_res(Value)

json_deserialize : string -> json_deserialize_res(json_val)
json_deserialize(*Serial) =
	let *Serial = _json_trimLeadingSpace(*Serial) in
	if *Serial == '' then json_deserialize_val(json_empty, *Serial)
	else
		let *res = _json_deserializeValue(*Serial) in
		match *res with 
			| json_deserialize_err(*m, *v, *s) => *res
			| json_deserialize_val(*val, *Serial) =>
				let *Serial = _json_trimLeadingSpace(*Serial) in
				if *Serial == '' then json_deserialize_val(*val, *Serial) 
				else json_deserialize_err('unexpected content after document', *val, *Serial)

_json_deserializeValue : string -> json_deserialize_res(json_val)
_json_deserializeValue(*Serial) =
	let *Serial = _json_trimLeadingSpace(*Serial) in
	if *Serial like 'null*' then _json_deserializeNull(*Serial) 
	else if *Serial like '"*' then _json_deserializeString(*Serial)
	else if *Serial like regex '^(false|true).*' then _json_deserializeBoolean(*Serial)
	else if *Serial like regex '^-\{0,1\}[0-9].*' then _json_deserializeNumber(*Serial)
	else if *Serial like '[*' then _json_deserializeArray(*Serial)
	else if *Serial like '{*' then _json_deserializeObject(*Serial)
	else json_deserialize_err('there is no legal next value', json_empty, *Serial)

_json_deserializeArray : string -> json_deserialize_res(json_val)
_json_deserializeArray(*Serial) =
	let *Serial = _json_strTl(*Serial) in                    # remove opening bracket
	match _json_deserializeArrayAccum(list(), *Serial) with
		| json_deserialize_err(*msg, *elmts, *Serial) => 
			json_deserialize_err(*msg, json_array(*elmts), *Serial)
		| json_deserialize_val(*elmts, *Serial) =>
			let *Serial = _json_trimLeadingSpace(*Serial) in
			if *Serial like ']*' then json_deserialize_val(json_array(*elmts), _json_strTl(*Serial)) 
			else json_deserialize_err('missing end of array', json_array(*elmts), *Serial)

_json_deserializeArrayAccum : list json_val * string -> json_deserialize_res(list json_val) 
_json_deserializeArrayAccum(*RevCurElmts, *Serial) =
	let *Serial = _json_trimLeadingSpace(*Serial) in
	if *Serial like regex '^(,|]).*' then json_deserialize_val(_json_rev(*RevCurElmts), *Serial)
	else 
		match _json_deserializeValue(*Serial) with
			| json_deserialize_err(*msg, *val, *Serial) =>
				let *RevCurElmts = 
					if _json_isEmpty(*val) then *RevCurElmts else cons(*val, *RevCurElmts) in 
				json_deserialize_err(*msg, _json_rev(*RevCurElmts), *Serial)
			| json_deserialize_val(*newElmt, *Serial) =>
				let *Serial = _json_trimLeadingSpace(*Serial) in
				let *Serial = 
					if *Serial like regex '^,[[:space:]]*[^\]].*' then _json_strTl(*Serial) 
					else *Serial in
					_json_deserializeArrayAccum(cons(*newElmt, *RevCurElmts), *Serial)

_json_deserializeBoolean : string -> json_deserialize_res(json_val)
_json_deserializeBoolean(*Serial) =
	if *Serial like 'false*' then json_deserialize_val(json_bool(false), _json_substrRem(*Serial, 5))
	else json_deserialize_val(json_bool(true), _json_substrRem(*Serial, 4))

_json_deserializeNull : string -> json_deserialize_res(json_val)
_json_deserializeNull(*Serial) = json_deserialize_val(json_null, _json_substrRem(*Serial, 4)) 

# FORMAT -?[0-9]+(\.[0-9]*)?
_json_deserializeNumber : string -> json_deserialize_res(json_val)
_json_deserializeNumber(*Serial) =
	let (*numStrBuf, *Serial) = 
    if *Serial like '-*' then ('-', _json_strTl(*Serial)) else ('', *Serial) in 
	let (*numStrBuf, *Serial) = _json_extractDigits(*numStrBuf, *Serial) in
	let (*numStrBuf, *Serial) = 
		if *Serial like '.*' then _json_extractDigits(*numStrBuf ++ '.', _json_strTl(*Serial))
		else (*numStrBuf, *Serial) in
	json_deserialize_val(json_num(double(*numStrBuf)), *Serial)

_json_extractDigits : string * string -> string * string
_json_extractDigits(*Buf, *Serial) =
	if *Serial == '' || !(*Serial like regex '^[0-9].*') then (*Buf, *Serial)
	else _json_extractDigits(*Buf ++ _json_strHd(*Serial), _json_strTl(*Serial)) 

_json_deserializeObject : string -> json_deserialize_res(json_val)
_json_deserializeObject(*Serial) =
	let *Serial = _json_strTl(*Serial) in                     # remove opening brace
	match _json_deserializeObjectAccum(list(), *Serial) with
		| json_deserialize_err(*msg, *fields, *Serial) => 
			json_deserialize_err(*msg, json_obj(*fields), *Serial)
		| json_deserialize_val(*fields, *Serial) =>
			let *Serial = _json_trimLeadingSpace(*Serial) in
			if *Serial like '}*' then json_deserialize_val(json_obj(*fields), _json_strTl(*Serial))
			else json_deserialize_err('missing end of object', json_obj(*fields), *Serial)

_json_deserializeObjectAccum : 
	list (string * json_val) * string -> json_deserialize_res(list (string * json_val))
_json_deserializeObjectAccum(*RevCurFields, *Serial) =
	let *Serial = _json_trimLeadingSpace(*Serial) in
	if *Serial like regex '^[,}].*' then json_deserialize_val(_json_rev(*RevCurFields), *Serial)
	else 
		match _json_deserializeField(*Serial) with
			| json_deserialize_err(*msg, (*name, *val), *Serial) =>
				let *RevCurFields = 
					if *name == '' && _json_isEmpty(*val) then *RevCurFields 
					else cons((*name, *val), *RevCurFields) in
				json_deserialize_err(*msg, _json_rev(*RevCurFields), *Serial)
			| json_deserialize_val(*newField, *Serial) =>
				let *Serial = _json_trimLeadingSpace(*Serial) in
				let *Serial = 
					if *Serial like regex '^,[[:space:]]*[^}].*' then _json_strTl(*Serial) 
					else *Serial in
				_json_deserializeObjectAccum(cons(*newField, *RevCurFields), *Serial)

_json_deserializeField : string -> json_deserialize_res((string * json_val))
_json_deserializeField(*Serial) =
	let *initSerial = *Serial in
	match _json_extractString(*Serial) with
		| json_deserialize_err(*m, *n, *Serial) => 
			json_deserialize_err('invalid field name', ('', json_empty), *initSerial)
		| json_deserialize_val(*name, *Serial) =>
			let *Serial = _json_trimLeadingSpace(*Serial) in 
			if !(*Serial like ':*') then
				json_deserialize_err('object field is missing value', (*name, json_empty), *Serial) 
      	else
				match _json_deserializeValue(_json_strTl(*Serial)) with
					| json_deserialize_err(*msg, *val, *Serial) => 
						json_deserialize_err(*msg, (*name, *val), *Serial)
					| json_deserialize_val(*val, *Serial) => json_deserialize_val((*name, *val), *Serial)

_json_deserializeString : string -> json_deserialize_res(json_val)
_json_deserializeString(*Serial) =
	match _json_extractString(*Serial) with
		| json_deserialize_err(*msg, *val, *Serial) => json_deserialize_err(*msg, json_empty, *Serial)
		| json_deserialize_val(*val, *Serial) => json_deserialize_val(json_str(*val), *Serial)

_json_extractString : string -> json_deserialize_res(string)
_json_extractString(*Serial) =
	let *initSerial = *Serial in
	let *Serial = triml(*Serial, '"') in                            # Remove opening mark
	let (*str, *Serial) = _json_extractStringAccum('', *Serial) in
	if *Serial like '"*' then json_deserialize_val(*str, _json_strTl(*Serial))
	else json_deserialize_err('missing end of string', '', *initSerial)

_json_extractStringAccum : string * string -> string * string
_json_extractStringAccum(*Buf, *Serial) =
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
				(*Buf ++ *char, _json_substrRem(*Serial, 2))
			else (*Buf ++ _json_strHd(*Serial), _json_strTl(*Serial)) in
		_json_extractStringAccum(*Buf, *Serial)

json_getValue : json_val * string -> json_val
json_getValue(*Doc, *FieldName) =
	match *Doc with
		| json_empty => json_empty
		| json_null => json_empty
		| json_bool(*b) => json_empty
		| json_num(*n) => json_empty
		| json_str(*s) => json_empty
		| json_array(*l) => json_empty
		| json_obj(*fields) =>
			let *ans = json_empty in
			let *_ = foreach (*field in *fields) {
            (*name, *val) = *field;
				if (*FieldName == *name) {
					*ans = *val; 
				} } in
			*ans
