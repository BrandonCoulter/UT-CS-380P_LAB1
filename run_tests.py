#!/usr/bin/env python3
import os
import re
import csv as c
import argparse

from time import sleep
from subprocess import check_output

#
#  Feel free (a.k.a. you have to) to modify this to instrument your code
#

parser = argparse.ArgumentParser()
parser.add_argument("-n", nargs="+", type=int, default=[], help="Number of threads (list<int>) EX: 0 1 2 4")
parser.add_argument("-r", type=int, default=1, help="Number of times to run tests (int)")
parser.add_argument("-i", type=str, default="all", help="Absolute path to input file")
parser.add_argument("-o", type=str, default="out/", help="Absolute path to output file")
parser.add_argument("-l", nargs="+", type=int, default=[100000], help="Number of loops that the operator will do (list<int>) EX: 10 100 1000")
parser.add_argument("-s", action='store_true', help="Use custom barrier implementation")

args = parser.parse_args()

RUNS = 1 if args.r == 1 else args.r
THREADS = [0, 1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30 , 32] if args.n == [] else args.n
LOOPS = [100000] if args.l == [100000] else args.l
INPUTS = [args.i] if args.i != "all" else ["1k.txt", "8k.txt", "16k.txt"]

spin = '--spin' if args.s else ''

complete = 0
total = len(INPUTS) * len(LOOPS) * RUNS * len(THREADS)

csvs = []
for inp in INPUTS:
    out_dir = args.o + inp
    for loop in LOOPS:
        for run in range(RUNS):
            csv = ["{}/{}".format(inp, loop)]
            for thr in THREADS:
                cmd = "./bin/prefix_scan -o {} -n {} -i tests/{} -l {} {}".format(
                   out_dir, thr, inp, loop, spin)
                out = check_output(cmd, shell=True).decode("ascii")
                m = re.search("time: (.*)", out)
                if m is not None:
                    time = m.group(1)
                    csv.append(time)

                complete += 1
                print(f"Completed {complete}/{total}")

                try:
                    cmd = f"diff --brief out/master/{inp} {args.o + inp}"
                    out = check_output(cmd, shell=True).decode("ascii")
                    print(f"{inp} passed diff check for test l={loop} n={thr} run-{run}.\n")
                except:
                    print(f"ERROR:\n\t{inp} has a DIFF. Check output for test l={loop} n={thr} run-{run + 1}.\n")

            csvs.append(csv)
            sleep(0.5)

header = ["microseconds"] + [str(x) for x in THREADS]

print("\n")
print(", ".join(header))

for csv in csvs:
    print (", ".join(csv))

stride = len(INPUTS) * RUNS
# print("\n\nSTRIDE: ",stride)
inp = 0

new_csv = []
for i in range(len(LOOPS)*len(INPUTS)*RUNS):
    if inp >= len(csvs):
        break
    row = []
    row.append(csvs[inp][0])
    # print(f"INDEX: {inp} | INPUT: {csvs[inp][0]}")
    for thread in range(len(THREADS)):
        avg = 0
        for run in range(RUNS):
            # print(f"INP:{inp + run} THREAD:{thread} -> {int(csvs[inp + run][thread + 1])}")
            avg += int(csvs[inp + run][thread + 1])
        avg = avg / RUNS
        row.append(str(avg))
    
    new_csv.append(row)

    inp += stride

print("\n")
print(", ".join(header))
for csv in new_csv:
    for i in range(len(csv)):
        if i <= 1:
            continue
        else:
            csv[i] = str(float(csv[1]) / float(csv[i]))
    csv[1] = str(float(csv[1]) / float(csv[1]))
    print (", ".join(csv))

with open("data.csv", "w", newline="") as f:
    writer = c.writer(f)
    writer.writerow(header)
    writer.writerows(new_csv)


# print()
# for inp in INPUTS:
#     try:
#         cmd = f"diff --brief out/master/{inp} {args.o + inp}"
#         out = check_output(cmd, shell=True).decode("ascii")
#         print(f"{inp} passed diff check.\n")
#     except:
#         print(f"ERROR:\n\t{inp} has a DIFF. Check output.\n")