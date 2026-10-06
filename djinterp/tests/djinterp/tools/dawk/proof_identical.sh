#!/bin/bash
# byte-identical proof: old (pre-refactor) vs new dcheck, every sheet x root x variant
cd /home/claude/w2
OLD=/tmp/old/build/dcheck; NEW=./build/dcheck
pass=0; fail=0
variants=("" "--source.tree=true --source.files=all" "--cascade.resolve=all --source.tree=true --source.files=all" "--cascade.resolve=layers --source.tree=true" "--severity.mode=ignore --source.files=all")
printf '%-10s %-5s %-4s %9s %9s\n' sheet root var old_ms new_ms > /tmp/proof_times.txt
for sheet in banner guard include section c_style text; do
  for root in inc/djinterp src/djinterp; do
    v=0
    for extra in "${variants[@]}"; do
      t0=$(date +%s%N); $OLD $extra --report.findings=10000000 sheets/$sheet.dss $root > /tmp/o.txt 2>/tmp/o.err; t1=$(date +%s%N)
      $NEW $extra --report.findings=10000000 sheets/$sheet.dss $root > /tmp/n.txt 2>/tmp/n.err; t2=$(date +%s%N)
      a=$(( (t1 - t0) / 1000000 )); b=$(( (t2 - t1) / 1000000 ))
      if [ -s /tmp/o.txt ] && cmp -s /tmp/o.txt /tmp/n.txt && cmp -s /tmp/o.err /tmp/n.err; then pass=$((pass+1)); else fail=$((fail+1)); echo "DIFF: $sheet $root [$extra]"; diff /tmp/o.txt /tmp/n.txt | head -4; fi
      printf '%-10s %-5s %-4s %9s %9s\n' $sheet ${root%%/*} v$v $a $b >> /tmp/proof_times.txt
      v=$((v+1))
    done
  done
done
echo "report runs: $pass identical, $fail differing"
# fix passes: the same repair sequence on two copies, old and new
for who in old new; do rm -rf /tmp/fix_$who; mkdir -p /tmp/fix_$who; cp -r inc/djinterp /tmp/fix_$who/inc; done
fixpass=0; fixfail=0
for sheet in banner include guard section; do
  for round in 1 2 3; do
    $OLD --fix=true sheets/$sheet.dss /tmp/fix_old/inc > /tmp/fo.txt 2>&1
    $NEW --fix=true sheets/$sheet.dss /tmp/fix_new/inc > /tmp/fn.txt 2>&1
    if cmp -s /tmp/fo.txt /tmp/fn.txt; then fixpass=$((fixpass+1)); else fixfail=$((fixfail+1)); echo "FIX DIFF: $sheet round $round"; diff /tmp/fo.txt /tmp/fn.txt | head -4; fi
  done
done
repaired=$(diff -rq inc/djinterp /tmp/fix_new/inc | wc -l)
if diff -rq /tmp/fix_old/inc /tmp/fix_new/inc > /dev/null; then trees="identical"; else trees="DIFFER"; fi
echo "fix passes: $fixpass identical outputs, $fixfail differing; repaired trees $trees ($repaired files changed by the repairs)"
