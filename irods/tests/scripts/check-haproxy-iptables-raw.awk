#!/usr/bin/awk --file
#
# This script verifies the contents of the raw iptables table on the load
# balancer.
#
# PARAMETERS:
#  IES_IP  The IP address of the primary IES.


BEGIN {
  state = "BEFORE BLOCK";
  table = "";
  preIdx = 0;
  outIdx = 0;

  PreRules[1] = "-A PREROUTING -s 0.0.0.0/0 -p tcp --dport 443 -j NOTRACK";
  PreRules[2] = "-A PREROUTING -s 128.196.65.87 -p tcp --sport 443 -j NOTRACK";
  PreRules[3] = "-A PREROUTING -s 0.0.0.0/0 -p tcp --dport 1247 -j NOTRACK";
  PreRules[4] = "-A PREROUTING -s " IES_IP " -p tcp --sport 1247 -j NOTRACK";
  PreRules[5] = "-A PREROUTING -s 0.0.0.0/0 -p tcp --dport 19990:19999 -j NOTRACK";
  PreRules[6] = "-A PREROUTING -s " IES_IP " -p tcp --sport 19990:19999 -j NOTRACK";

  OutRules[1] = "-A OUTPUT -d 0.0.0.0/0 -p tcp --sport 443 -j NOTRACK";
  OutRules[2] = "-A OUTPUT -d 128.196.65.87 -p tcp --dport 443 -j NOTRACK";
  OutRules[3] = "-A OUTPUT -d 0.0.0.0/0 -p tcp --dport 1247 -j NOTRACK";
  OutRules[4] = "-A OUTPUT -d " IES_IP " -p tcp --dport 1247 -j NOTRACK";
  OutRules[5] = "-A OUTPUT -d 0.0.0.0/0 -p tcp --sport 19990:19999 -j NOTRACK";
  OutRules[6] = "-A OUTPUT -d " IES_IP " -p tcp --dport 19990:19999 -j NOTRACK";
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

  if (newTable == "raw" && state != "IN BLOCK") {
    print "raw table definition outside of configuration block" > "/dev/stderr";
    exit 1;
  }

  if (newTable != "raw" && state == "IN BLOCK") {
    printf "%s table definition inside of configuration block\n", newTable > "/dev/stderr";
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

/^# BEGIN DS MANAGED BLOCK \(load_balancer raw\)$/ {
  if (state != "BEFORE BLOCK") {
    print "configuration block injected twice" > "/dev/stderr";
    exit 1;
  }

  if (table != "") {
    printf "configuration block injected inside definition of the %s table\n", table \
      > "/dev/stderr";

    exit 1;
  }

  state = "IN BLOCK";
  next;
}


table == "raw" && $0 ~ /^-A PREROUTING/ {
  print $0;
  preIdx = preIdx + 1;

  if ($0 != PreRules[preIdx]) {
    printf "PREROUTING rule %d is incorrect: %s\n", preIdx, $0 > "/dev/stderr";
    exit 1;
  }

  next;
}


table == "raw" && $0 ~ /^-A OUTPUT/ {
  outIdx = outIdx + 1;

  if ($0 != OutRules[outIdx]) {
    printf "OUTPUT rule %d is incorrect: %s\n", outIdx, $0 > "/dev/stderr";
    exit 1;
  }

  next;
}


/^# END DS MANAGED BLOCK \(load_balancer raw\)$/ {
  if (state != "IN BLOCK") {
    print "configuration block end before begin" > "/dev/stderr";
    exit 1;
  }

  if (table == "raw") {
    print "configuration block end before raw table definition committed" > "/dev/stderr";
    exit 1;
  }

  state = "AFTER BLOCK";
  next;
}


state == "IN BLOCK" {
  printf "Invalid statement in configuration block (%s)\n", $0 > "/dev/stderr";
  exit 1;
}


END {
  if (preIdx < 6) {
    print "There should be 6 rules added to raw table's PREROUTING chain" > "/dev/stderr";
    exit 1;
  }

  if (outIdx < 6) {
    print "There should be 6 rules added to the raw table's OUTPUT chain" > "/dev/stderr";
    exit 1;
  }
}
