#!/usr/bin/awk --file
#
# This script verifies the contents of the iptables configuration on the WebDAV
# host.


BEGIN {
  state = "BEFORE BLOCK";
  table = "";
  idx = 0;

  Rules[1] = "-A INPUT -m state --state NEW -p tcp -s 0.0.0.0/0 --dport 80 -j ACCEPT";
  Rules[2] = "-A INPUT -m state --state NEW -p tcp -s 0.0.0.0/0 --dport 443 -j ACCEPT";
}


/^$/ {
  next;
}


/^\*/ {
  newTable = substr($0, 2);

  if (table != "") {
    printf "table %s being defined inside definition of table %s\n", newTable, table \
      > "/dev/stderr";

    exit 1;
  }

  if (newTable == "filter" && state != "BEFORE BLOCK") {
    print "configuration block outside of filter tabke definition" > "/dev/stderr";
    exit 1;
  }

  table = newTable;
  next;
}


/^COMMIT/ {
  if (table == "") {
    print "commit outside of table definition" > "/dev/stderr";
    exit 1;
  }

  table = "";
  next;
}


/^# BEGIN DS MANAGED BLOCK \(webdav\)$/ {
  if (state != "BEFORE BLOCK") {
    print "configuration block injected twice" > "/dev/stderr";
    exit 1;
  }

  if (table != "filter") {
    printf "configuration block injected outside of filter table definition\n" > "/dev/stderr";
    exit 1;
  }

  state = "IN BLOCK";
  next;
}


state == "IN BLOCK" && $0 ~ /^-A INPUT/ {
  print $0;
  idx = idx + 1;

  if ($0 != Rules[idx]) {
    printf "FILTER rule %d is incorrect: %s\n", idx, $0 > "/dev/stderr";
    exit 1;
  }

  next;
}


/^# END DS MANAGED BLOCK \(webdav\)$/ {
  if (state != "IN BLOCK") {
    print "configuration block end before begin" > "/dev/stderr";
    exit 1;
  }

  state = "AFTER BLOCK";
  next;
}


state == "IN BLOCK" {
  printf "Invalid statement in configuration block (%s)\n", $0 > "/dev/stderr";
  exit 1;
}


state != "AFTER BLOCK" && $0 ~ /^-A INPUT -j REJECT/ {
  printf "The configuration block not completed before final REJECT\n" > "/dev/stderr";
  exit 1;
}


END {
  if (idx < 2) {
    print "There should be 2 rules added to filter table's INPUT chain" > "/dev/stderr";
    exit 1;
  }
}
