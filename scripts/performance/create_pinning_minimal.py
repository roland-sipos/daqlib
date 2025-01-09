#!/usr/bin/env python
"""
Created on: 05/12/2024 12:17

Author: Shyam Bhuller

Description: Create a cpu pinning file for a readout server.
"""
import argparse
import json
import os
import subprocess

from socket import gethostname

from rich import print

class CPUList:
    """
    A class to represent a CPU list. CPUs can be retireved from the list,
    and if so, that CPU number is removed from the list. Used to keep track
    of CPUs when assigning them to threads.

    Attributes
    ----------
    cpu_list : list[int]
        Flat list of available CPUs.
    cpu_list_regions : list[list[list[int]]]
        List of available CPUs split into numa and if applicable, regions.

    Methods
    -------
    __getitem__:
        Return the desired CPU number and remove this from the availble CPUs lists.
    range:
        Loop over the CPU list and return a list of CPUs, and remove them from the available CPUs lists.
    alt_range:
        Loop over the CPU list and return a list of CPUs using "for each", and remove them from the available CPUs lists.
    first_available:
        Return the first CPU (number at index 0) and remove this from the available CPUs lists.
    """
    def __init__(self, cpu_list : list[int], cpu_list_regions : list[list[list[int]]]) -> None:
        self.cpu_list = cpu_list
        self.cpu_list_regions = cpu_list_regions
        pass


    def __getitem__(self, c : int) -> int:
        """ Return the desired CPU number and remove this from the availble cpus lists.

        Args:
            c (int): CPU number.

        Raises:
            Exception: Available CPU list is empty.
            Exception: CPU number was not found.

        Returns:
            int: CPU number.
        """
        if c in self.cpu_list:
            self.cpu_list.remove(c)
            for n in self.cpu_list_regions: # loop over numa
                if len(n) == 0:
                    raise Exception("no more free cpus available!")
                if type(n[0]) is list: # there should never be a mix of ints and lists in the cpu list, so this is fine.
                    for h in n: # loop over region
                        if c in h: h.remove(c)
                else:
                    if c in n: n.remove(c)
            return c
        else:
            raise Exception(f"cpu {c} not found (has it already been allocated?)")


    def range(self, _min : int, _max : int, numa : int, region : int = None) -> list[int]:
        """ Loop over the CPU list and return a list of CPUs, and remove them from the available CPUs lists.

        Args:
            _min (int): Min index
            _max (int): Max index
            numa (int): Numa to loop over
            region (int, optional): Region to loop over. Defaults to None.

        Returns:
            list[int]: List of selected CPUs.
        """
        if region is None:
            return [self[i] for i in list(self.cpu_list_regions[numa]) if (i >= _min) and (i < _max)]
        else:
            return [self[i] for i in list(self.cpu_list_regions[numa][region]) if (i >= _min) and (i < _max)]


    def alt_range(self, num : int, numa : int, region : int = None) -> list[int]:
        """ Loop over the CPU list and return a list of CPUs using "for each", and remove them from the available CPUs lists.

        Args:
            num (int): Number of CPUs to return
            numa (int): Numa to loop over
            region (int, optional): Region to loop over. Defaults to None.

        Returns:
            list[int]: List of selected CPUs.
        """
        if region is None:
            return [self[i] for i in list(self.cpu_list_regions[numa][:num])]
        else:
            return [self[i] for i in list(self.cpu_list_regions[numa][region][:num])]


    def first_available(self, numa : int, region : int = None) -> int:
        """ Return the first CPU (number at index 0) and remove this from the available CPUs lists.

        Args:
            numa (int): Numa to select from
            region (int, optional): Region to select from. Defaults to None.

        Returns:
            int: first available CPU
        """
        if region is not None:
            return self.cpu_list_regions[numa][region][0]
        else:
            return self.cpu_list_regions[numa][0]


def run_command(host : str, cmd : str) -> subprocess.CompletedProcess:
    """ Run bash command on a given host.

    Args:
        host (str): Host name.
        cmd (str): Command to run.

    Returns:
        subprocess.CompletedProcess: Output of command. 
    """
    return subprocess.run(['ssh', f'{os.environ["USER"]}@{host}', f'{cmd}'], capture_output = True)


def parse_output(output: subprocess.CompletedProcess, separator : str = None) -> list | dict:
    """ Get output from run_command and apply some simple formatting.

    Args:
        output (subprocess.CompletedProcess): Subprocess output.
        separator (str, optional): String separator to split key-value pairs. Defaults to None.

    Returns:
        list | dict: _description_
    """
    output_lines = str(output.stdout)[2:].split("\\n")

    if separator:
        parsed = {}
        for i in output_lines:
            info = i.split(separator)
            if len(info) > 1:
                parsed[info[0]] = info[1].replace("  ", "")

        return parsed
    else:
        return output_lines


def get_numa_info(host : str) -> tuple[dict, int]:
    """ Get CPU information needed to make the pinning file.

    Args:
        host (str): Server host name.

    Returns:
        tuple[dict, int]: Dictionary of values about cpu cores and cache size.
    """
    numa_dict = {}
    numa_nodes = None

    numactl_out = parse_output(run_command(host, "numactl -H"))
    if numactl_out:
        for numal in numactl_out:
            if numal.find('cpus') != -1:
                cpu_line = numal.split()
                if cpu_line[1] not in numa_dict:
                    numa_dict[cpu_line[1]] = {}
                numa_dict[cpu_line[1]]['cpus'] = [int(cpu) for cpu in cpu_line[3:]]

            for mem in ["size", "free"]:
                if numal.find(mem) != -1:
                    value = numal.split()
                    numa_dict[value[1]][mem] = int(value[3])*1024 # convert to KB

    numa_nodes = int(numactl_out[0].split()[1])

    for nodeid in range(numa_nodes):
        numa_dict[str(nodeid)]['devices'] = []

    return numa_dict, numa_nodes


def cpu_list_to_str(cpus : list[int]) -> str:
    """ Convert a list of CPUs to a string format for the json file.

    Args:
        cpus (list[int]): List of CPUs

    Returns:
        str: CPU list string
    """
    #! for now, just use join, but can try to condense it later on.
    return ",".join(str(c) for c in cpus)


def make_threads(pinning : dict, numa : int, name : int, func : callable, kwargs : dict = None, counter_offset : int = 0):
    """ make entries for a specified thread type into the pinning configuration.

    Args:
        pinning (dict): pinning configuration.
        numa (int): Numa to make entry for.
        name (int): Name of the daq application.
        func (callable): Function to call that makes and adds the cpu list to the configuration.
        kwargs (dict, optional): Arguments to pass to func. Defaults to None.
        counter_offset (int, optional): Offset to the application counter. Defaults to 0.
    """
    counter = 0 + counter_offset # application counter, useful when assigning names to certain threads
    for n in pinning["daq_application"]:
        if numa == int(n.split(name)[-1][0]):
            counter += 1
            func(pinning, n, counter, **kwargs)
    return


def make_tpproc(pinning : dict, name : str, counter : int, nums : list[int]):
    """ Assign the tpproc threads in the pinning configuration.

    Args:
        pinning (dict): Pinning configuration.
        name (str): Name of the daq application.
        counter (int): Current application number.
        nums (list[int]): List of cpus to assign.
    """
    pinning["daq_application"][name]["threads"] = {f"tpproc-{counter}." : cpu_list_to_str(nums)}
    return


def make_rte(pinning : dict, name : str, counter : int, cpus : CPUList, numa : int, n_threads : int, n_regions : int, n_cpus : int):
    """ Assign the rte threads in the pinning configuration.

    Args:
        pinning (dict): Pinning configuration.
        name (str): Name of the daq application.
        counter (int): Current application number.
        cpus (CPUList): CPU list to make assignments from.
        numa (int): Numa to make entry for.
        n_threads (int): Number of threads to make.
        n_regions (int): Number of cpu regions in a numa.
        n_cpus (int): number of cpus to assign to a single thread.
    """
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


def make_parent(pinning : dict, name : str, counter : int, cpus : CPUList, numa : int, n_regions : int, n_cpus : int):
    """ Assign the parent thread in the pinning configuration.

    Args:
        pinning (dict): Pinning configuration.
        name (str): Name of the daq application.
        counter (int): Current application number.
        cpus (CPUList): CPU list to make assignments from.
        numa (int): Numa to make entry for.
        n_regions (int): Number of cpu regions in a numa.
        n_cpus (int): number of cpus to assign to a single thread.
    """
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


def make_rawprocs(pinning : dict, name : str, counter : int, cpus : CPUList, numa : int, n_regions : int, n_cpus : int):
    """ Assign the raw processor threads in the pinning configuration.

    Args:
        pinning (dict): Pinning configuration.
        name (str): Name of the daq application.
        counter (int): Current application number.
        cpus (CPUList): CPU list to make assignments from.
        numa (int): Numa to make entry for.
        n_regions (int): Number of cpu regions in a numa.
        n_cpus (int): number of cpus to assign to a single thread.
    """
    if n_regions == 1:
        raw_proc = cpus.alt_range(n_cpus, numa)
    else:
        raw_proc = cpus.alt_range(n_cpus//2, numa, 0) + cpus.alt_range(n_cpus//2, numa, 1)
    pinning["daq_application"][name]["threads"][f"rawproc-0-{counter}.."] = cpu_list_to_str(raw_proc)
    return


def make_ccp(pinning : dict, name : str, counter : int, cpus : CPUList, numa : int, n_regions : int, n_cpus : int):
    """ Assign the consmer, cleanup and periodic threads in the pinning configuration.

    Args:
        pinning (dict): Pinning configuration.
        name (str): Name of the daq application.
        counter (int): Current application number.
        cpus (CPUList): CPU list to make assignments from.
        numa (int): Numa to make entry for.
        n_regions (int): Number of cpu regions in a numa.
        n_cpus (int): number of cpus to assign to a single thread.
    """
    if n_regions == 1:
        ccp_threads = cpus.alt_range(n_cpus, numa)
    else:
        ccp_threads = cpus.alt_range(n_cpus//2, numa, 0) + cpus.alt_range(n_cpus//2, numa, 1)
    pinning["daq_application"][name]["threads"][f"cleanup-{counter}."] =  cpu_list_to_str(ccp_threads)
    pinning["daq_application"][name]["threads"][f"consumer-{counter}."] = cpu_list_to_str(ccp_threads)
    pinning["daq_application"][name]["threads"][f"periodic-{counter}."] = cpu_list_to_str(ccp_threads)
    return


def make_recording(pinning : dict, name : str, counter : int, nums : list[int]):
    """ Assign the recording thread in the pinning configuration.

    Args:
        pinning (dict): Pinning configuration.
        name (str): Name of the daq application.
        counter (int): Current application number.
        nums (list[int]): List of cpus to assign.
    """
    pinning["daq_application"][name]["threads"][f"recording-{counter}.."] = cpu_list_to_str(nums)
    return


def create_threads_numa(pinning : dict, cpus : CPUList, numa : int, name : str, thread_nums : dict[int], n_regions : int, numa_apps : list[int], max_cpus : dict[int]):
    """ Create threads for the pinning file for a single numa.

    Args:
        pinning (dict): Pinning configuration.
        cpus (CPUList): CPU list to make assignments from.
        numa (int): Numa to make entry for.
        name (str): Name of the daq application.
        thread_nums (dict[int]): Number of threads to make per daq application.
        n_regions (int): Number of regions in the numa (a cpu with hypercores would have n_regions = 1).
        numa_apps (list[int]): Number of each daq_application for the numa.
        max_cpus (dict[int]): Number of cpus to assign to each thread type.

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

    make_threads(pinning, numa, name, make_tpproc, {"nums" : tp_procs_numa}, counter_offset = numa_apps[numa - 1] if numa > 0 else 0)

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

    numa_dict = get_numa_info(args.readout_server)[0]

    #* this is just to emulate the numactl output for np0x machines for testing purposes
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
    # create daq application names
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

    # define the number of threads per APA
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

    # define the cpu regions i.e. if the cpu has hypercores assigned to the different numas
    # e.g. 0,32, 63,95, these will be defined as two distinct regions
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

    print(f"headroom per daq application: {cores_per_app - total_cpus_used}") # printout the available headroom per application after removing the primary core and hpyercore

    # use the correct number of threads depending on the readout plane assembly
    if "np02" in args.readout_server:
        thread_nums = n_threads_CRP
    elif "np04" in args.readout_server:
        thread_nums = n_threads_APA
    else:
        raise Exception(f"do not know what readout plane is used for {args.readout_server}")

    # make the pinning configuration
    cpus = CPUList(cpus_remaining, remaining_regions)
    for i in range(2):
        create_threads_numa(pinning, cpus, i, daq_app_names, thread_nums, n_regions, numa_apps, max_cpus)

    # print created pinning and remaning cpus that were not assigned (excluding the first core and hypercore.)
    print(pinning)
    print("remaining cpus:")
    print(cpus.cpu_list_regions)

    # write to a json file
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