#!/usr/bin/awk --file
#
# This script verifies the contents of the raw iptables table on the load
# balancer.
#
# PARAMETERS:
#  IES_IP  The IP address of the primary IES.


BEGIN {
  state = "OUTSIDE BLOCKS";
  table = "";

  inIdx = 0;
  FilterInRules[1] = "-A INPUT -m state --state UNTRACKED -j ACCEPT";
  FilterInRules[2] = "-A INPUT -m state --state NEW -s 0.0.0.0/0 -p tcp --dport 80 -j ACCEPT";
  FilterInRules[3] = "-A INPUT -m state --state NEW -s 0.0.0.0/0 -p tcp --dport 81 -j ACCEPT";

  preIdx = 0;
  RawPreRules[1] = "-A PREROUTING -s 0.0.0.0/0 -p tcp --dport 443 -j NOTRACK";
  RawPreRules[2] = "-A PREROUTING -s 128.196.65.87 -p tcp --sport 443 -j NOTRACK";
  RawPreRules[3] = "-A PREROUTING -s 0.0.0.0/0 -p tcp --dport 1247 -j NOTRACK";
  RawPreRules[4] = "-A PREROUTING -s " IES_IP " -p tcp --sport 1247 -j NOTRACK";
  RawPreRules[5] = "-A PREROUTING -s 0.0.0.0/0 -p tcp --dport 19990:19999 -j NOTRACK";
  RawPreRules[6] = "-A PREROUTING -s " IES_IP " -p tcp --sport 19990:19999 -j NOTRACK";

  outIdx = 0;
  RawOutRules[1] = "-A OUTPUT -d 0.0.0.0/0 -p tcp --sport 443 -j NOTRACK";
  RawOutRules[2] = "-A OUTPUT -d 128.196.65.87 -p tcp --dport 443 -j NOTRACK";
  RawOutRules[3] = "-A OUTPUT -d 0.0.0.0/0 -p tcp --dport 1247 -j NOTRACK";
  RawOutRules[4] = "-A OUTPUT -d " IES_IP " -p tcp --dport 1247 -j NOTRACK";
  RawOutRules[5] = "-A OUTPUT -d 0.0.0.0/0 -p tcp --sport 19990:19999 -j NOTRACK";
  RawOutRules[6] = "-A OUTPUT -d " IES_IP " -p tcp --dport 19990:19999 -j NOTRACK";
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

  if (newTable == "raw" && state != "IN RAW BLOCK") {
    print "raw table definition outside of configuration block" > "/dev/stderr";
    exit 1;
  }

  if (newTable != "raw" && state == "IN RAW BLOCK") {
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


/^# BEGIN DS MANAGED BLOCK \(load_balancer filter\)$/ {
  if (table != "filter") {
    printf "filter configuration block not inside filter table definition\n" > "/dev/stderr";
    exit 1;
  }

  state = "IN FILTER BLOCK";
  next;
}


/^# END DS MANAGED BLOCK \(load_balancer filter\)$/ {
  if (state != "IN FILTER BLOCK") {
    print "filter configuration block end before begin" > "/dev/stderr";
    exit 1;
  }

  if (table != "filter") {
    print "filter configuration block end after filter table definition committed" > "/dev/stderr";
    exit 1;
  }

  state = "OUTSIDE BLOCKS";
  next;
}


state == "IN FILTER BLOCK" {
  print $0;
  inIdx = inIdx + 1;

  if ($0 != FilterInRules[inIdx]) {
    printf "filter rule %d is incorrect: %s\n", inIdx, $0 > "/dev/stderr";
    exit 1;
  }

  next;
}


/^# BEGIN DS MANAGED BLOCK \(load_balancer raw\)$/ {
  if (table != "") {
    printf "raw configuration block injected inside definition of the %s table\n", table \
      > "/dev/stderr";

    exit 1;
  }

  state = "IN RAW BLOCK";
  next;
}


/^# END DS MANAGED BLOCK \(load_balancer raw\)$/ {
  if (state != "IN RAW BLOCK") {
    print "raw configuration block end before begin" > "/dev/stderr";
    exit 1;
  }

  if (table == "raw") {
    print "raw configuration block end before raw table definition committed" > "/dev/stderr";
    exit 1;
  }

  state = "OUTSIDE BLOCKS";
  next;
}


table == "raw" && $0 ~ /^-A PREROUTING/ {
  print $0;
  preIdx = preIdx + 1;

  if ($0 != RawPreRules[preIdx]) {
    printf "raw PREROUTING rule %d is incorrect: %s\n", preIdx, $0 > "/dev/stderr";
    exit 1;
  }

  next;
}


table == "raw" && $0 ~ /^-A OUTPUT/ {
  outIdx = outIdx + 1;

  if ($0 != RawOutRules[outIdx]) {
    printf "raw OUTPUT rule %d is incorrect: %s\n", outIdx, $0 > "/dev/stderr";
    exit 1;
  }

  next;
}


state == "IN RAW BLOCK" {
  printf "Invalid statement in raw configuration block (%s)\n", $0 > "/dev/stderr";
  exit 1;
}


END {
  if (state != "OUTSIDE BLOCKS") {
    printf "configuration block not ended\n" > "/dev/stderr";
    exit 1;
  }

  if (inIdx < 3) {
    print "There should be 3 rules added to the filter table's INPUT chain" > "/dev/stderr";
    exit 1;
  }

  if (preIdx < 6) {
    print "There should be 6 rules added to raw table's PREROUTING chain" > "/dev/stderr";
    exit 1;
  }

  if (outIdx < 6) {
    print "There should be 6 rules added to the raw table's OUTPUT chain" > "/dev/stderr";
    exit 1;
  }
}
