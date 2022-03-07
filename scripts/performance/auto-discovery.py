#! python3

import argparse
import os
import sys
import psutil
import json

import subprocess
from subprocess import check_output

parser = argparse.ArgumentParser(description='Discover hardware setup and available resources.')
parser.add_argument('--device', '-d', action='append', required=False, help='device to try auto-discover')
parser.set_defaults(device=['CERN', 'Ethernet', 'Non-Volatile'])

try:
  args = parser.parse_args()
except:
  parser.print_help()
  sys.exit(0)

dev_dict={}
for dev in args.device:
  dev_dict[dev] = {}

numa_dict={}
numactl_out_raw = check_output(['numactl', '-H']).decode("utf-8")
numactl_out_lines = numactl_out_raw.splitlines()
for numal in numactl_out_lines:
  if (numal.find('cpus') != -1):
    cpu_line = numal.split()
    if cpu_line[1] not in numa_dict:
      numa_dict[cpu_line[1]] = {}
    numa_dict[cpu_line[1]]['cpus'] = cpu_line[3:]
  if (numal.find('size') != -1):
    size_line = numal.split()
    numa_dict[size_line[1]]['size'] = int(size_line[3])*1024
  if (numal.find('free') != -1):
    free_line = numal.split()
    numa_dict[free_line[1]]['free'] = int(free_line[3])*1024

numa_nodes = int(numactl_out_raw.splitlines()[0].split()[1])

lspci_out_raw = check_output(["lspci"])
lspci_out = lspci_out_raw.decode("utf-8").splitlines()

for item in lspci_out:
  devl = item.split(' ')
  devl[1:len(devl)] = [' '.join(devl[1:len(devl)])]
  for dev in args.device:
    if (devl[1].find(dev) != -1):
      dev_dict[dev][devl[0]] = devl
      dev_lspci_out = check_output(["lspci", "-s", devl[0], "-vvvvv"])
      verbose_info = dev_lspci_out.decode("utf-8").splitlines()
      for vline in verbose_info:
        if (vline.find('NUMA') != -1):
          zone = vline.split()[2]
          dev_dict[dev][devl[0]].append(int(zone))

for nodeid in range(numa_nodes):
  numa_dict[str(nodeid)]['devices'] = []

for cat in dev_dict:
  for dev in dev_dict[cat]:
    zone = dev_dict[cat][dev][2]
    numa_dict[str(zone)]['devices'].append((dev, dev_dict[cat][dev][1]))

print('#### Full NUMA map')
print(json.dumps(numa_dict, sort_keys=False, indent=4))
print('\n')

dev_cat = dev_dict.keys()
print('#### PCIe devices')
print('Looked up device names in lspci:', dev_cat)
print('Found devices:')
for cat in dev_cat:
  dev_ids = dev_dict[cat].keys()
  print('Category', cat, ':', len(dev_ids))
  print('  -> lspci ids:', str(dev_ids))
print('\n')

print('#### Hardware info...')
lcpu_count = len(os.sched_getaffinity(0))
pcpu_count = psutil.cpu_count(logical=False)
vmem = psutil.virtual_memory()
print('  -> Logical CPU count:', lcpu_count)
print('  -> Physical CPU count:', pcpu_count)
print('  -> NUMA nodes:', numa_nodes)
print('\n')
for numa in numa_dict:
  print('   * CPUs node', numa, numa_dict[numa]['cpus'])
  print('   * size node', numa, numa_dict[numa]['size'])
  print('   * free node', numa, numa_dict[numa]['free'])
  print('   * devs node', numa, json.dumps(numa_dict[numa]['devices'], indent=4))
  print('\n')
print('\n')

print('#### Memory status:\n', vmem)
print('\n')

exit(0)

