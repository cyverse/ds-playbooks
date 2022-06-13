json {
  *t = deserialize(*Serial);
  writeLine('stdout', serialize(*t));
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
  let (*doc, *remains) = deserializeVal(*Serial) in
  let trimLeadingSpace(*remains) == '' then *doc 
  else let (*err, *_) = mkDeserializeErrRes('unexpected content after document', *Serial) in *err

deserializeVal : string -> json_val * string
deserializeVal(*Serial) =
  let *Serial = trimLeadingSpace(*Serial) in
  let *h = strHd(*Serial) in
    if *h == '[' then deserializeArray(*Serial)
    else if *h == 'f' || *h == 't' then deserializeBoolean(*Serial)
    else if *h == 'n' then deserializeNull(*Serial)
    else if *h like regex '[-0-9]' then deserializeNumber(*Serial)
    else if *h == '"' then deserializeString(*Serial)
    else if *h == '{' then deserializeObject(*Serial) 
    else mkDeserializeErrRes('there is no legal next value', *Serial) in

deserializeArray : string -> json_val * string
deserializeArray(*Serial) =
  let *initSerial = *Serial in
  let *Serial = tlStr(*Serial) in
  let (*elmts, *Serial) = deserializeArrayAccum(list(), *Serial) in
  let *Serial = trimLeadingSpace(*Serial) in
  if *Serial like ']*' then (json_array(*elmts), tlStr(*Serial)) 
  else mkDeserializeErrRes('expected next value to be an array', *initSerial)

deserializeArrayAccum : list json_val * string -> list json_val * string 
deserializeArrayAccum(*revCurElmts, *Serial) =
  let (*newElmt, *Serial) = deserializeVal(*Serial) in
  let *revCurElmts = cons(*newElmt, *revCurElmts) in
  let *Serial = trimLeadingSpace(*Serial) in
  if !(*Serial like ',*') then (rev(*revCurElmts), *Serial) 
  else deserializeArrayAccum(*revCurElmts, strTl(*Serial))

deserializeBoolean : string -> json_val * string
deserializeBoolean(*Serial) =
  if *Serial like 'false*' then (json_bool(false), substrRem(*Serial, 5))
  else if *Serial like 'true*' then (json_bool(true), substrRem(*Serial, 4))
  else mkDeserializeErrRes('expected next value to be a Boolean', *Serial)

deserializeNull : string -> json_val * string
deserializeNull(*Serial) =
  if *Serial like 'null*' then (json_null, substrRem(*Serial, 4)) 
  else mkDeserializeErrRes("expected next value to be 'null'", *Serial)

# FORMAT -?[0-9]+(\.[0-9]*)?
deserializeNumber : string -> json_val * string
deserializeNumber(*Serial) =
  if *Serial like regex '^-\{0,1\}[0-9].*' then 
    let (*numStrBuf, *Serial) = 
      if *Serial like '-*' then ('-', strTl(*Serial)) else ('', *Serial) in 
    let (*numStrBuf, *Serial) = extractDigits(*numStrBuf, *Serial) in
    let (*numStrBuf, *Serial) = 
      if *Serial like '.*' then extractDigits(*numStrBuf ++ '.', strTl(*Serial))
      else (*numStrBuf, *Serial) in
    (json_num(double(*numStrBuf)), *Serial)
  else mkDeserializeErrRes("expected next value to be a number", *Serial)

extractDigits : string * string -> string * string
extractDigits(*Buf, *Serial) =
  if *Serial == '' || !(*Serial like regex '^[0-9].*') then (*Buf, *Serial)
  else extractDigits(*Buf ++ strHd(*Serial), strTl(*Serial)) 

deserializeObject : string -> json_val * string
deserializeObject(*Serial) =
  let *initSerial = *Serial in
  let *Serial = tlStr(*Serial) in
  let *fields = deserialObjectAccum(list(), *Serial) in
  let *Serial = trimLeadingSpace(*Serial) in
  if *Serial like '}*' then (json_obj(*fields), tlStr(*Serial))
  else mkDeserializeErrRes('expected next object to be an object', *initSerial)

deserializeObjectAccum : list (string * json_val) * string -> list (string * json_val) * string
# TODO implement

deserializeString(*Serial) =
  let *initSerial = *Serial in
  let *buf = '' in
  let *Serial = triml(*Serial, '"') in
  let (*str, *Serial) = extractString('', *Serial) in
  if *Serial like '"*' then (json_str(*str), strTl(*Serial))
  else mkDeserializeErrRes("expected next value to be a string", *initSerial)

extractString : string * string -> string * string
extractString(*Buf, *Serial) =
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
    extractString(*Buf, *Serial)

INPUT *Serial=''
OUTPUT ruleExecOut