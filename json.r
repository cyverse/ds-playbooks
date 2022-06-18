json {
	*msg = match deserialize(*Serial) with
		| json_deserialize_val(*v, *_) => serialize(*v)
		| json_deserialize_err(*e, *v, *r) => 
			*e ++ ' (remaining="' ++ *r ++ '", deserialized=' ++ serialize(*v) ++ ')';
	writeLine('stdout', *msg);
}

# 1. Only the control characters for horizontal tab, carriage return, and line 
#    feed are supported in strings.
# 2. exponential notation is not supported.
# 3. Numbers may not begin with a . or a +.

substrRem : string * int -> string
substrRem(*String, *NewHdPos) = substr(*String, *NewHdPos, strlen(*String))

strHd : string -> string
strHd(*String) = substr(*String, 0, 1)

strTl : string -> string
strTl(*String) = substrRem(*String, 1)

trimLeadingSpace : string -> string
trimLeadingSpace(*String) = 
	if !(*String like regex '^[[:space:]].*') then *String else trimLeadingSpace(strTl(*String))

append : string * string * list string -> string
append(*Base, *Separator, *Elements) =
	if size(*Elements) == 0 then *Base 
	else
		let *sep = if *Base == '' then '' else *Separator in 
		append(*Base ++ *sep ++ hd(*Elements), *Separator, tl(*Elements))

join : string * list string -> string
join(*Separator, *Elements) = append('', *Separator, *Elements)

rev : list ? -> list ?
rev(*List) = revAccum(list(), *List)

revAccum : list ? * list ? -> list ?
revAccum(*RevList, *List) = 
	if size(*List) == 0 then *RevList else revAccum(cons(hd(*List), *RevList), tl(*List)) 

encode : string -> string
encode(*Unencoded) = encodeAccum('', *Unencoded)

encodeAccum : string * string -> string
encodeAccum(*Encoded, *Unencoded) =
	if strlen(*Unencoded) == 0 then *Encoded
	else
		let *c = strHd(*Unencoded) in
		let *escC = 
			if *c == '"' then '\\"' 
			else if *c == '\n' then '\\n' 
			else if *c == '\r' then '\\r' 
			else if *c == '\t' then '\\t' 
			else if *c == '\\' then '\\\\' 
			else *c in 
		encodeAccum(*Encoded ++ *escC, strTl(*Unencoded))

data json_val =
	| json_array : list json_val -> json_val
	| json_bool : boolean -> json_val
	| json_null : json_val
	| json_num : f double -> json_val
	| json_obj : list (string * json_val) -> json_val
	| json_str : string -> json_val
	| json_empty : json_val

isEmpty : json_val -> boolean
isEmpty(*Val) =
	match *Val with
		| json_array(*a) => false 
		| json_bool(*b) => false
		| json_null => false
		| json_num(*n) => false
		| json_obj(*o) => false
		| json_str(*s) => false
		| json_empty => true

serialize : json_val -> string
serialize(*Val) =
	match *Val with
		| json_array(*a) => '[' ++ join(',', serializeScalars(*a)) ++ ']' 
		| json_bool(*b) => str(*b)
		| json_null => 'null'
		| json_num(*n) => str(*n)
		| json_obj(*o) => '{' ++ join(',', serializeFields(*o)) ++ '}'
		| json_str(*s) => '"' ++ encode(*s) ++ '"'
		| json_empty => ''

serializeFields : list (string * json_val) -> list string
serializeFields(*Unserialized) = serializeFieldsAccum(list(), *Unserialized)

serializeFieldsAccum : list string * list (string * json_val) -> list string
serializeFieldsAccum(*RevSerialized, *Unserialized) = 
	if size(*Unserialized) == 0 then rev(*RevSerialized)
	else
		let (*name, *value) = hd(*Unserialized) in
		let *serializedField = '"' ++ encode(*name) ++ '":' ++ serialize(*value) in
		serializeFieldsAccum(cons(*serializedField, *RevSerialized), tl(*Unserialized))

serializeScalars : list json_val -> list string
serializeScalars(*Unserialized) = serializeScalarsAccum(list(), *Unserialized)

serializeScalarsAccum : list string * list json_val -> list string
serializeScalarsAccum(*RevSerialized, *Unserialized) =
	if size(*Unserialized) == 0 then rev(*RevSerialized)
	else serializeScalarsAccum(cons(serialize(hd(*Unserialized)), *RevSerialized), tl(*Unserialized))

# describes a response from a deserialization operation, where Value is the type
# of the deserialized value
data json_deserialize_res(Value) =
	# the success result, an ordered-pair containing the deserialized value and
	# the remainder of the serialization string
	| json_deserialize_val : Value * string -> json_deserialize_res(Value)

	# the failure result, an ordered-triple containing an error message, the 
	# portion deserialized so far and the remainder of the serialization string
	| json_deserialize_err : string * Value * string -> json_deserialize_res(Value)

deserialize : string -> json_deserialize_res(json_val)
deserialize(*Serial) =
	let *Serial = trimLeadingSpace(*Serial) in
	if *Serial == '' then json_deserialize_val(json_empty, *Serial)
	else
		let *res = deserializeValue(*Serial) in
		match *res with 
			| json_deserialize_err(*m, *v, *s) => *res
			| json_deserialize_val(*val, *Serial) =>
				let *Serial = trimLeadingSpace(*Serial) in
				if *Serial == '' then json_deserialize_val(*val, *Serial) 
				else json_deserialize_err('unexpected content after document', *val, *Serial)

deserializeValue : string -> json_deserialize_res(json_val)
deserializeValue(*Serial) =
	let *Serial = trimLeadingSpace(*Serial) in
	if *Serial like 'null*' then deserializeNull(*Serial) 
	else if *Serial like '"*' then deserializeString(*Serial)
	else if *Serial like regex '^(false|true).*' then deserializeBoolean(*Serial)
	else if *Serial like regex '^-\{0,1\}[0-9].*' then deserializeNumber(*Serial)
	else if *Serial like '[*' then deserializeArray(*Serial)
	else if *Serial like '{*' then deserializeObject(*Serial)
	else json_deserialize_err('there is no legal next value', json_empty, *Serial)

deserializeArray : string -> json_deserialize_res(json_val)
deserializeArray(*Serial) =
	let *Serial = strTl(*Serial) in                    # remove opening bracket
	match deserializeArrayAccum(list(), *Serial) with
		| json_deserialize_err(*msg, *elmts, *Serial) => 
			json_deserialize_err(*msg, json_array(*elmts), *Serial)
		| json_deserialize_val(*elmts, *Serial) =>
			let *Serial = trimLeadingSpace(*Serial) in
			if *Serial like ']*' then json_deserialize_val(json_array(*elmts), strTl(*Serial)) 
			else json_deserialize_err('missing end of array', json_array(*elmts), *Serial)

deserializeArrayAccum : list json_val * string -> json_deserialize_res(list json_val) 
deserializeArrayAccum(*RevCurElmts, *Serial) =
	let *Serial = trimLeadingSpace(*Serial) in
	if *Serial like regex '^(,|]).*' then json_deserialize_val(rev(*RevCurElmts), *Serial)
	else 
		match deserializeValue(*Serial) with
			| json_deserialize_err(*msg, *val, *Serial) =>
				let *RevCurElmts = if isEmpty(*val) then *RevCurElmts else cons(*val, *RevCurElmts) in 
				json_deserialize_err(*msg, rev(*RevCurElmts), *Serial)
			| json_deserialize_val(*newElmt, *Serial) =>
				let *Serial = trimLeadingSpace(*Serial) in
				let *Serial = 
					if *Serial like regex '^,[[:space:]]*[^\]].*' then strTl(*Serial) else *Serial in
					deserializeArrayAccum(cons(*newElmt, *RevCurElmts), *Serial)

deserializeBoolean : string -> json_deserialize_res(json_val)
deserializeBoolean(*Serial) =
	if *Serial like 'false*' then json_deserialize_val(json_bool(false), substrRem(*Serial, 5))
	else json_deserialize_val(json_bool(true), substrRem(*Serial, 4))

deserializeNull : string -> json_deserialize_res(json_val)
deserializeNull(*Serial) = json_deserialize_val(json_null, substrRem(*Serial, 4)) 

# FORMAT -?[0-9]+(\.[0-9]*)?
deserializeNumber : string -> json_deserialize_res(json_val)
deserializeNumber(*Serial) =
	let (*numStrBuf, *Serial) = if *Serial like '-*' then ('-', strTl(*Serial)) else ('', *Serial) in 
	let (*numStrBuf, *Serial) = extractDigits(*numStrBuf, *Serial) in
	let (*numStrBuf, *Serial) = 
		if *Serial like '.*' then extractDigits(*numStrBuf ++ '.', strTl(*Serial))
		else (*numStrBuf, *Serial) in
	json_deserialize_val(json_num(double(*numStrBuf)), *Serial)

extractDigits : string * string -> string * string
extractDigits(*Buf, *Serial) =
	if *Serial == '' || !(*Serial like regex '^[0-9].*') then (*Buf, *Serial)
	else extractDigits(*Buf ++ strHd(*Serial), strTl(*Serial)) 

deserializeObject : string -> json_deserialize_res(json_val)
deserializeObject(*Serial) =
	let *Serial = strTl(*Serial) in                     # remove opening brace
	match deserializeObjectAccum(list(), *Serial) with
		| json_deserialize_err(*msg, *fields, *Serial) => 
			json_deserialize_err(*msg, json_obj(*fields), *Serial)
		| json_deserialize_val(*fields, *Serial) =>
			let *Serial = trimLeadingSpace(*Serial) in
			if *Serial like '}*' then json_deserialize_val(json_obj(*fields), strTl(*Serial))
			else json_deserialize_err('missing end of object', json_obj(*fields), *Serial)

deserializeObjectAccum : 
	list (string * json_val) * string -> json_deserialize_res(list (string * json_val))
deserializeObjectAccum(*RevCurFields, *Serial) =
	let *Serial = trimLeadingSpace(*Serial) in
	if *Serial like regex '^[,}].*' then json_deserialize_val(rev(*RevCurFields), *Serial)
	else 
		match deserializeField(*Serial) with
			| json_deserialize_err(*msg, (*name, *val), *Serial) =>
				let *RevCurFields = 
					if *name == '' && isEmpty(*val) then *RevCurFields 
					else cons((*name, *val), *RevCurFields) in
				json_deserialize_err(*msg, rev(*RevCurFields), *Serial)
			| json_deserialize_val(*newField, *Serial) =>
				let *Serial = trimLeadingSpace(*Serial) in
				let *Serial = 
					if *Serial like regex '^,[[:space:]]*[^}].*' then strTl(*Serial) else *Serial in
				deserializeObjectAccum(cons(*newField, *RevCurFields), *Serial)

deserializeField : string -> json_deserialize_res((string * json_val))
deserializeField(*Serial) =
	let *initSerial = *Serial in
	match extractString(*Serial) with
		| json_deserialize_err(*m, *n, *Serial) => 
			json_deserialize_err('invalid field name', ('', json_empty), *initSerial)
		| json_deserialize_val(*name, *Serial) =>
			let *Serial = trimLeadingSpace(*Serial) in 
			if !(*Serial like ':*') then
				json_deserialize_err('object field is missing value', (*name, json_empty), *Serial) 
      	else
				match deserializeValue(strTl(*Serial)) with
					| json_deserialize_err(*msg, *val, *Serial) => 
						json_deserialize_err(*msg, (*name, *val), *Serial)
					| json_deserialize_val(*val, *Serial) => json_deserialize_val((*name, *val), *Serial)

deserializeString : string -> json_deserialize_res(json_val)
deserializeString(*Serial) =
	match extractString(*Serial) with
		| json_deserialize_err(*msg, *val, *Serial) => json_deserialize_err(*msg, json_empty, *Serial)
		| json_deserialize_val(*val, *Serial) => json_deserialize_val(json_str(*val), *Serial)

extractString : string -> json_deserialize_res(string)
extractString(*Serial) =
	let *initSerial = *Serial in
	let *Serial = triml(*Serial, '"') in                      # Remove opening mark
	let (*str, *Serial) = extractStringAccum('', *Serial) in
	if *Serial like '"*' then json_deserialize_val(*str, strTl(*Serial))
	else json_deserialize_err('missing end of string', '', *initSerial)

extractStringAccum : string * string -> string * string
extractStringAccum(*Buf, *Serial) =
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
				(*Buf ++ *char, substrRem(*Serial, 2))
			else (*Buf ++ strHd(*Serial), strTl(*Serial)) in
		extractStringAccum(*Buf, *Serial)

INPUT *Serial=''
OUTPUT ruleExecOut
