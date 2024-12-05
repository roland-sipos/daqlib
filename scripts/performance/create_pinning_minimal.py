#!/usr/bin/env python
import argparse
import importlib
import json
from socket import gethostname

get_numa_info = getattr(importlib.import_module("auto-discovery"), "get_numa_info")

from rich import print

class cpu_checker:
    def __init__(self, cpu_list : list[int], cpu_list_regions : list[list[list[int]]]) -> None:
        self.cpu_list = cpu_list
        self.cpu_list_regions = cpu_list_regions
        pass


    def __getitem__(self, c : int):
        if c in self.cpu_list:
            self.cpu_list.remove(c)
            for n in self.cpu_list_regions:
                if len(n) == 0:
                    raise Exception("no more free cpus available!")
                if type(n[0]) is list: # there should never be a mix of ints and lists in the cpu list, so this is fine.
                    for h in n:
                        if c in h: h.remove(c)
                else:
                    if c in n: n.remove(c)
            return c
        else:
            raise Exception(f"cpu {c} not found (has it already been allocated?)")


    def range(self, _min : int, _max : int, numa : int, region : int = None):
        if region is None:
            return [self[i] for i in list(self.cpu_list_regions[numa]) if (i >= _min) and (i < _max)]
        else:
            return [self[i] for i in list(self.cpu_list_regions[numa][region]) if (i >= _min) and (i < _max)]


    def alt_range(self, num : int, numa : int, region : int = None):
        if region is None:
            return [self[i] for i in list(self.cpu_list_regions[numa][:num])]
        else:
            return [self[i] for i in list(self.cpu_list_regions[numa][region][:num])]


    def first_available(self, numa : int, region : int = None):
        if region is not None:
            return self.cpu_list_regions[numa][region][0]
        else:
            return self.cpu_list_regions[numa][0]


def cpu_list_to_str(cpus : list[int]) -> list[str]:
    #! for now, just use join, but can try to condense it later on.
    return ",".join(str(c) for c in cpus)


def make_threads(pinning : dict, numa : int, name : int, func : callable, kwargs : dict = None, counter_offset : int = 0):
    counter = 0 + counter_offset
    for n in pinning["daq_application"]:
        if numa == int(n.split(name)[-1][0]):
            counter += 1
            func(pinning, n, counter, **kwargs)
    return


def make_tpproc(pinning : dict, name : str, counter : int, nums : list[int]):
    pinning["daq_application"][name]["threads"] = {f"tpproc-{counter}." : cpu_list_to_str(nums)}
    return


def make_rte(pinning : dict, name : str, counter : int, cpus : cpu_checker, numa : int, n_threads : int, n_regions : int, n_cpus : int):
    for i in range(n_threads):
        if n_regions == 1:
            c = cpus.first_available(numa)
            pinning["daq_application"][name]["threads"][f"rte-worker-{c}"] = str(cpus[c])
        else:
            if i >= (n_threads//2):
                c = cpus.first_available(numa, 1)
                pinning["daq_application"][name]["threads"][f"rte-worker-{c}"] = str(cpus[c])
            else:
                c = cpus.first_available(numa, 0)
                pinning["daq_application"][name]["threads"][f"rte-worker-{c}"] = str(cpus[c])
    return


def make_parent(pinning : dict, name : str, counter : int, cpus : cpu_checker, numa : int, n_regions : int, n_cpus : int):
    if n_regions == 1:
        parents = [i for i in cpus.cpu_list_regions[numa] if (i >= cpus.first_available(numa)) and (i < (cpus.first_available(numa) + (2 * n_cpus) + 1))]
    else:
        # parents = ""
        # for t, v in pinning["daq_application"][name]["threads"].items():
        #     if ("tpproc" in t) or ("rte" in t) or ("record" in t): continue
        #     parents = ",".join([parents, v])
        parents = [i for i in cpus.cpu_list_regions[numa][0] if (i >= cpus.first_available(numa, 0)) and (i < (cpus.first_available(numa, 0) + n_cpus + 1))] + [i for i in cpus.cpu_list_regions[numa][1] if (i >= cpus.first_available(numa, 1)) and (i < (cpus.first_available(numa, 1) + n_cpus + 1))]
    pinning["daq_application"][name]["parent"] = cpu_list_to_str(parents)

    # pinning["daq_application"][name]["parent"] = parents[1:]
    return


def make_rawprocs(pinning : dict, name : str, counter : int, cpus : cpu_checker, numa : int, n_regions : int, n_cpus : int):
    if n_regions == 1:
        raw_proc = cpus.alt_range(n_cpus, numa)
        # raw_proc = cpus.range(cpus.first_available(numa), cpus.first_available(numa) + 17, numa)
    else:
        raw_proc = cpus.alt_range(n_cpus//2, numa, 0) + cpus.alt_range(n_cpus//2, numa, 1)
        # raw_proc = cpus.range(cpus.first_available(numa, 0), cpus.first_available(numa, 0) + 8, numa, 0) + cpus.range(cpus.first_available(numa, 1), cpus.first_available(numa, 1) + 8, numa, 1)
    pinning["daq_application"][name]["threads"][f"rawproc-0-{counter}.."] = cpu_list_to_str(raw_proc)
    return


def make_ccp(pinning : dict, name : str, counter : int, cpus : cpu_checker, numa : int, n_regions : int, n_cpus : int):
    if n_regions == 1:
        ccp_threads = cpus.alt_range(n_cpus, numa)
        # ccp_threads = cpus.range(cpus.first_available(numa), cpus.first_available(numa) + 7, numa)
    else:
        ccp_threads = cpus.alt_range(n_cpus//2, numa, 0) + cpus.alt_range(n_cpus//2, numa, 1)
        # ccp_threads = cpus.range(cpus.first_available(numa, 0), cpus.first_available(numa, 0) + 3, numa, 0) + cpus.range(cpus.first_available(numa, 1), cpus.first_available(numa, 1) + 3, numa, 1)
    pinning["daq_application"][name]["threads"][f"cleanup-{counter}."] =  cpu_list_to_str(ccp_threads)
    pinning["daq_application"][name]["threads"][f"consumer-{counter}."] = cpu_list_to_str(ccp_threads)
    pinning["daq_application"][name]["threads"][f"periodic-{counter}."] = cpu_list_to_str(ccp_threads)
    return


def make_recording(pinning : dict, name : str, counter : int, nums : list[int]):
    pinning["daq_application"][name]["threads"][f"recording-{counter}.."] = cpu_list_to_str(nums)
    return


def create_threads_numa(pinning : dict, cpus : cpu_checker, numa : int, name : str, thread_nums : dict[int], n_regions : int, numa_apps : list[int], max_cpus : dict[int]):
    """
    RULES:
    
    socket is equal to numa node

    applications are assigned cores from one numa node only e.g. for srv031 an application cannot be assigned both cpus 1 and 33.
    
    numa node divided into numerical regions i.e. numa 0 could be 0-32, 64-95

    lowest cpu number per region should not be pinned to any thread i.e. for srv031, exclude 0,64,32,96 from the pinning.

    threads with more than one cpu assigned are done so symmetrically i.e. tpproc : 1, 65, cleanup-1 : 22-24,86-88.

    parent threads for each readout application are shared.
    rte-workers should be assigned to one cpu each (were the current numbers chosen for a given reason?).
    rte-worker threads should not be assigned as a parent thread.
    rte-worker threads are assigned symmetrically.

    rawproc are assigned 8 cpus per region (16 total).
    cleanup, consumer, recording and periodic threads assigned 3 cpus per region (6 total). They all share the same cpus.
    
    tpproc is assigned one cpu per socket (2 total).
    tpproc is assigned the second smallest cpu in each region i.e. 1 and 65 for numa 0.

    for an APA:
        4 rte-workers
        1 raw_proc
        1 tpproc

        2 cleanup (1 per recording/periodic?)
        2 consumer
        1 recording
        1 periodic
    """
    # tp procs
    if n_regions == 1:
        tp_procs_numa = [cpus[cpus.first_available(numa)] for i in range(max_cpus["tpproc"])]
    else:
        remaining = max_cpus["tpproc"]
        tp_procs_numa = []
        while remaining > 0:
            for i in range(n_regions):
                if remaining == 0: break
                tp_procs_numa.append(cpus[cpus.first_available(numa, i)])
                remaining -= 1

    make_threads(pinning, numa, name, make_tpproc, {"nums" : tp_procs_numa})

    # rtes
    make_threads(pinning, numa, name, make_rte, {"numa" : numa, "cpus" : cpus, "n_threads" : thread_nums["rte"], "n_regions" : n_regions, "n_cpus" : max_cpus["rte"]})

    # parent threads
    make_threads(pinning, numa, name, make_parent, {"numa" : numa, "cpus" : cpus, "n_regions" : n_regions, "n_cpus" : max_cpus["rawproc"] + max_cpus["ccp"]})

    # rawprocs
    make_threads(pinning, numa, name, make_rawprocs, {"numa" : numa, "cpus" : cpus, "n_regions" : n_regions, "n_cpus" : max_cpus["rawproc"]}, numa_apps[numa - 1] if numa > 0 else 0)

    # cleanup, consumer, periodic
    make_threads(pinning, numa, name, make_ccp, {"numa" : numa, "cpus" : cpus, "n_regions" : n_regions, "n_cpus" : max_cpus["ccp"]}, numa_apps[numa - 1] if numa > 0 else 0)

    # recording #! this appears to have higher priority than ccp threads
    if n_regions == 1:
        recording_numa = cpus.range(cpus.cpu_list_regions[numa][-1] - (max_cpus["recording"] - 1), cpus.cpu_list_regions[numa][-1] + 1, numa)
    else:
        n_cpus = max_cpus["recording"] // n_regions
        remainder = max_cpus["recording"] % n_regions
        recording_numa = []
        for i in range(n_regions):
            if i == (n_regions - 1):
                n = n_cpus + remainder
            else:
                n = n_cpus
            recording_numa.extend(cpus.range(cpus.cpu_list_regions[numa][i][-1] - (n - 1), cpus.cpu_list_regions[numa][i][-1] + 1, numa, i))
    make_threads(pinning, numa, name, make_recording, {"nums" : recording_numa}, numa_apps[numa - 1] if numa > 0 else 0)
    return


def main(args = argparse.Namespace):
    pinning = {"daq_application" : {}}

    daq_app_names = f"ru{args.readout_server.replace('-', '')}eth"

    numa_dict = get_numa_info()[0]

    #* this is just to emulate the numactl output for np04-srv-031
    if args.fake is True:
        if args.readout_server == "np04-srv-031":
            fake_cpu_pinning = {
                "0" : list(range(0, 32)) + list(range(64,96)),
                "1" : list(range(32, 64)) + list(range(96, 128))
            }
        elif args.readout_server == "np02-srv-003":
            fake_cpu_pinning = {
                "0" : list(range(0, 112, 2)),
                "1" : [i + 1 for i in range(0, 112, 2)]
            }
        else:
            raise Exception(f"Cannot generate fake CPU info for {args.readout_server}")

        n_numa = len(fake_cpu_pinning)
    else:
        n_numa = len(numa_dict)

    if args.fake is True:
        for k in numa_dict:
            numa_dict[k]["cpus"] = fake_cpu_pinning[k]


    #! this should be read from the oks config
    split = args.num_apps // n_numa
    app_names = []
    numa_apps = []
    for i in range(n_numa):
        numa_apps.append(0)
        for j in range(split):
            if (args.num_apps == n_numa):
                app_name = f"{daq_app_names}{i}"
            else:
                app_name = f"{daq_app_names}{i}{j}"
            app_names.append(app_name)
            numa_apps[i] += 1

    if (args.num_apps % n_numa) > 0:
        for i in range(n_numa):
            if len(app_names) < args.num_apps:
                app_names.append(f"{daq_app_names}{i}{split + i}")
                numa_apps[i] += 1
            else:
                break

    for name in app_names:
        pinning["daq_application"]["--name " + name] = {}

    cpus_all = []
    for i in numa_dict.values():
       cpus_all += i["cpus"]

    n_cpus_total = len(cpus_all)

    cores_per_app = n_cpus_total // len(app_names)

    # how many cores should be assigned to a single thread (sharing rules are omitted here). Taken from np04-srv-031 pinning
    max_cpus = {k : getattr(args, k) for k in max_cpus_default}

    # as done for np04-srv-031
    n_threads_APA = {
        "rte" : 4,
        "tpproc" : 1,
        "rawproc" : 1,
        "cleanup" : 1,
        "consumer" : 1,
        "periodic" : 1,
        "recording" : 1,
    }

    # as done for np02-srv-003eth0 (not for eth1???)
    n_threads_CRP = {
        "rte" : 6,
        "tpproc" : 3,
        "rawproc" : 1,
        "cleanup" : 4,
        "consumer" : 4,
        "periodic" : 3,
        "recording" : 1,
    }

    total_cpus_used = sum(v for v in max_cpus.values())

    for numa in numa_dict:
        cpus = numa_dict[numa]["cpus"]
        min_stride = min([cpus[i] - cpus[i-1] for i in range(1, len(numa_dict[numa]["cpus"]))])
        region_boundaries = []
        for i in range(1, len(numa_dict[numa]["cpus"])):
            if (cpus[i] - cpus[i-1]) > min_stride:
                region_boundaries.append(i)
        if len(region_boundaries) > 1:
            raise Exception("more than two regions has not been supported yet.")
        elif len(region_boundaries) == 0:
            print("only one region was found")
            n_regions = 1
            numa_dict[numa]["regions"] = cpus
        else:
            n_regions = 2
            numa_dict[numa]["regions"] = [cpus[:region_boundaries[0]], cpus[region_boundaries[0]:]]

    print(f"headroom : {cores_per_app - total_cpus_used}")

    cpus_remaining = list(cpus_all)
    remaining_regions = [v["regions"] for v in numa_dict.values()]

    # remove first thread and hypercore on each numa node
    if n_regions == 1:
        cpus_remaining.remove(numa_dict["0"]["regions"][0])
        cpus_remaining.remove(numa_dict["1"]["regions"][0])
        remaining_regions[0].pop(0)
        remaining_regions[1].pop(0)

    else: # must have hypercores
        cpus_remaining.remove(numa_dict["0"]["regions"][0][0])
        cpus_remaining.remove(numa_dict["0"]["regions"][1][0])
        cpus_remaining.remove(numa_dict["1"]["regions"][0][0])
        cpus_remaining.remove(numa_dict["1"]["regions"][1][0])

        remaining_regions[0][0].pop(0)
        remaining_regions[0][1].pop(0)
        remaining_regions[1][0].pop(0)
        remaining_regions[1][1].pop(0)


    if "np02" in args.readout_server:
        thread_nums = n_threads_CRP
    elif "np04" in args.readout_server:
        thread_nums = n_threads_APA
    else:
        raise Exception(f"do not know what readout plane is used for {args.readout_server}")

    cpus = cpu_checker(cpus_remaining, remaining_regions)
    for i in range(2):
        create_threads_numa(pinning, cpus, i, daq_app_names, thread_nums, n_regions, numa_apps, max_cpus)

    print(pinning)
    print("remaining cpus:")
    print(cpus.cpu_list_regions)

    with open("cpupin-all-running.json", "w") as f:
        json.dump(pinning, f, indent = 4)

    print("pinning has been written to cpupin-all-running.json")

    return

if __name__ == "__main__":
    max_cpus_default = {
        "rte" : 1,
        "tpproc" : 2,
        "rawproc" : 16,
        "ccp" : 6,
        "recording" : 6
    }

    parser = argparse.ArgumentParser("generate a pinning file for a readout machine")
    parser.add_argument("-r", "--readout_server", type = str, default = gethostname(), help = "hostname for the machine, if not provided the current machine hostname is used")
    parser.add_argument("-f", "--fake", action="store_true", help = "fake the numactl output for the specified readout machine")
    parser.add_argument("-n", "--num_apps", type = int, default = 1, help = "number of daq_applications to make.")

    for k, v in max_cpus_default.items():
        if k == "ccp":
            name = "consumer, cleanup or periodic"
        else:
            name = k
        parser.add_argument(f"--{k}", dest = k, type = int, default = v, help = f"number of cpus to assign to a {name} thread")

    args = parser.parse_args()

    print(args)
    main(args)