# This is a collection of functions and rules for constructing serialized JSON 
# documents.
#
# © 2021 The Arizona Board of Regents on behalf of The University of Arizona. 
# For license information, see https://cyverse.org/license.


_ipcJson_encodeObject(*SerialFields) {
  *res = '{';
  if (size(*SerialFields) > 0) {
	*res = *res ++ hd(*SerialFields);
    foreach(*field in tl(*SerialFields)) {
      *res = *res ++ ',' ++ *field;
    }
  }
  *res ++ '}';
}

_ipcJson_encodeString(*Str) {
  *escStr = '';
  *len = strlen(*Str);
  *pos = 0;
  while (*len > *pos) {
    *c = substr(*Str, *pos, *pos + 1);
    *escC = if *c == '"' then '\\"' else
            if *c == '\t' then '\\t' else
            if *c == '\n' then '\\n' else
            if *c == '\r' then '\\r' else
            if *c == '\\' then '\\\\' else
            *c;
    *escStr = *escStr ++ *escC;
    *pos = *pos + 1;
  }
  '"' ++ *escStr ++ '"';
}

_ipcJson_mkField(*Label, *SerialVal) = '"' ++ *Label ++ '":' ++ *SerialVal

# construct a serialized JSON number field. This version performs no error checking.
#
# Parameters:
#   *Label - the name of the field
#   *Val - the value of the field
#
ipc_jsonNumber(*Label, *Val) = _ipcJson_mkField(*Label, '*Val')

# construct a serialized JSON string field
#
# Parameters:
#   *Label - the name of the field
#   *Val - the value of the field
#
ipc_jsonString(*Label, *Val) = _ipcJson_mkField(*Label, _ipcJson_encodeString(*Val))

# construct a serialized JSON array field from its serialized elements
#
# Parameters:
#   *Label - the name of the field
#   *Val - the serialized array values
#
ipc_jsonStringArray(*Label, *Values) {
  *serialArray = '[';
  if (size(*Values) > 0) {
    *serialArray = *serialArray ++ _ipcJson_encodeString(hd(*Values));
    foreach (*val in tl(*Values)) {
      *serialArray = *serialArray ++ ',' ++ _ipcJson_encodeString(*val);
    }
  }
  *serialArray = *serialArray ++ ']';
  _ipcJson_mkField(*Label, *serialArray);
}

# construct a serialized JSON object field from its serialized fields
#
# Parameters:
#   *Label - the name of the object field
#   *SerialFields - the list of pre-serialized fields ot include in the object
#
ipc_jsonObject(*Label, *SerialFields) = 
  _ipcJson_mkField(*Label, _ipcJson_encodeObject(*SerialFields))

# construct a serialized JSON document from its serialized fields
#
# Parameters:
#   *SerialFields - A list of pre-serialized fields to include in the document
#
ipc_jsonDocument(*SerialFields) = _ipcJson_encodeObject(*SerialFields)

# construct a serialized JSON object field from a Boolean value
#
# Parameters:
#   *Label - the name of the object field
#   *flag - the Boolean value of the field.
#
ipc_jsonBoolean(*Label, *flag) {
  if (*flag) {
    _ipcJson_mkField(*Label, "true");
  } else {
    _ipcJson_mkField(*Label, "false");
  }
}
