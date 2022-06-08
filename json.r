json {
  *t = deserialize(*Serial);
  writeLine('stdout', serialize(*t));
}

# 1. Backspaces and form feeds cannot be handled in Strings by this library.
# 2. exponential notation is not supported.
# 3. Numbers may not begin with a . or a +.

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

trimlBlanks : string -> string
trimlBlanks(*Str) = triml(triml(triml(triml(*Str, ' '), '\t'), '\n'), '\r')

encode : string -> string
encode(*Unencoded) = encodeAccum('', *Unencoded)

encodeAccum : string * string -> string
encodeAccum(*Encoded, *Unencoded) =
  if strlen(*Unencoded) == 0 then *Encoded
  else
    let *c = substr(*Unencoded, 0, 1) in
    let *escC = 
      if *c == '"' then '\\"' 
      else if *c == '\t' then '\\t' 
      else if *c == '\n' then '\\n' 
      else if *c == '\r' then '\\r' 
      else if *c == '\\' then '\\\\' 
      else *c in 
    encodeAccum(*Encoded ++ *escC, substr(*Unencoded, 1, strlen(*Unencoded)))

data json_val =
  | json_array : list json_val -> json_val
  | json_bool : boolean -> json_val
  | json_null : json_val
  | json_num : f double -> json_val
  | json_obj : list (string * json_val) -> json_val
  | json_str : string -> json_val
  | json_err : string * json_val -> json_val

mkErrReport : string * json_val -> json_val
mkErrReport(*Msg, *State) = 
  let *errVal = json_obj(list(
    ('msg', json_str(*Msg)), ('state', *State) )) in
  json_obj(list(('ERROR', *errVal)))

mkDeserializeErrRes : string * string -> json_val * string
mkDeserializeErrRes(*Msg, *Serial) = (
  json_err(
    *Msg, 
    json_obj(list(
      ('serialized', json_str(*Serial)) ))),
  *Serial )

serialize : json_val -> string
serialize(*Val) =
  match *Val with
    | json_array(*a) => '[' ++ join(',', serializeScalars(*a)) ++ ']' 
    | json_bool(*b) => str(*b)
    | json_null => 'null'
    | json_num(*n) => str(*n)
    | json_obj(*o) => '{' ++ join(',', serializeFields(*o)) ++ '}'
    | json_str(*s) => '"' ++ encode(*s) ++ '"'
    | json_err(*msg, *state) => serialize(mkErrReport(*msg, *state))

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

deserialize : string -> json_val
deserialize(*Serial) =
  let *stripped = trimlBlanks(*Serial) in
  let *h = substr(*stripped, 0, 1) in
  let (*val, *remnants) =
    if *h == '[' then deserializeArray(*stripped)
    else if *h == 'f' || *h == 't' then deserializeBoolean(*stripped)
    else if *h == 'n' then deserializeNull(*stripped)
    else if *h like regex '^[-0-9]' then deserializeNumber(*stripped)
    else if *h == '"' then deserializeString(*stripped)
    else if *h == '{' then deserializeObject(*stripped) 
    else mkDeserializeErrRes('there is no legal next value', *stripped) in
  *val

deserializeBoolean : string -> json_val * string
deserializeBoolean(*Serial) =
  if *Serial like 'false*' then (json_bool(false), substr(*Serial, 5, strlen(*Serial)))
  else if *Serial like 'true*' then (json_bool(true), substr(*Serial, 4, strlen(*Serial)))
  else mkDeserializeErrRes('expected next value to be a Boolean', *Serial)

deserializeNull : string -> json_val * string
deserializeNull(*Serial) =
  if *Serial like 'null*' then (json_null, substr(*Serial, 4, strlen(*Serial))) 
  else mkDeserializeErrRes("expected next value to be 'null'", *Serial)

deserializeNumber : string -> json_val * string
deserializeNumber(*Serial) = 
TODO figure out why regex isn't matching
  if *Serial like regex '^-?[0-9]' then deserializeNumberAccum('', *Serial)
  else mkDeserializeErrRes("expected next value to be a number", *Serial)

# FORMAT -?[0-9]+(\.[0-9]*)?
deserializeNumberAccum : string * string -> json_val * string
deserializeNumberAccum(*NumStrBuf, *Serial) =
  let *curPos = 0 in
  let *curChar = substr(*Serial, *curPos, *curPos + 1) in
  let *_ = if (*curChar == '-') {
    *NumStrBuf = *NumStrBuf ++ *curChar;
    *curPos = *curPos + 1;
    *curChar = substr(*Serial, *curPos, *curPos + 1) } in
  let *_ = while (*curChar like regex '[0-9]') {
    *NumStrBuf = *NumStrBuf ++ *curChar;
    *curPos = *curPos + 1;
    *curChar = substr(*Serial, *curPos, *curPos + 1) } in
  let *_ = if (*curChar == '.') {
    *NumStrBuf = *NumStrBuf ++ *curChar;
    *curPos = *curPos + 1;
    *curChar = substr(*Serial, *curPos, *curPos + 1) } in     
  let *_ = while (*curChar like regex '[0-9]') {
    *NumStrBuf = *NumStrBuf ++ *curChar;
    *curPos = *curPos + 1;
    *curChar = substr(*Serial, *curPos, *curPos + 1) } in
  (json_num(double(*NumStrBuf)), substr(*Serial, *curPos, strlen(*Serial)))

INPUT *Serial=''
OUTPUT ruleExecOut