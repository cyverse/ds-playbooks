#!/bin/bash

SEARCHPATH="$HOME"

if [ "$1" != "" ]
then
  SEARCHPATH="$1"
  SEARCHPATH=$(echo $(cd "$SEARCHPATH"; pwd) )
fi

find /usr -name 'psqlodbc*.so' 2> /dev/null | grep --invert-match 'w\.so' | head --lines 1
