import argparse
import importlib

get_numa_info = getattr(importlib.import_module("auto-discovery"), "get_numa_info")

from rich import print

def main(args = argparse.Namespace):
    pinning = {"daq_application" : {}}

    #* i is the numa number, j is the daq application number
    app_names = [f"runp04srv031eth{i}{j}" for i in range(2) for j in range(2)] #! this will be read from the oks config

    for name in app_names:
        pinning["daq_application"]["--name " + name] = {}

    numa_dict = get_numa_info()[0]
    cpus_all = []
    for i in numa_dict.values():
       cpus_all += i["cpus"]

    n_cpus_total = len(cpus_all)

    cores_per_app = n_cpus_total // len(app_names)

    max_cpu_APA = 4 + 16 + 2 + 6 # 4 rte workers, 16 rawprocs, 2 tpprocs and 6 for cleamup, consumer, recording, periodic.

    for numa in numa_dict:
        cpus = numa_dict[numa]["cpus"]
        region_boundaries = []
        for i in range(1, len(numa_dict[numa]["cpus"])):
            if (cpus[i] - cpus[i-1]) > 1:
                region_boundaries.append(i)
        if len(region_boundaries) > 1:
            raise Exception("more than two regions has not been supported yet.")
        elif len(region_boundaries) == 0:
            print("only one region was found")
            numa_dict[numa]["regions"] = cpus
        else:
            numa_dict[numa]["regions"] = [cpus[:region_boundaries[0]], cpus[region_boundaries[0]:]]

    print(f"headroom : {cores_per_app - max_cpu_APA}")

    {
        "parent" : "",
        "threads" : {
            "rte-worker-{}" : 1,
            "rte-worker-{}" : 1,
            "rte-worker-{}" : 1,
            "rte-worker-{}" : 1,

            "rawproc-0-2.." : 14,
            "tpproc-2." : 2,

            "cleanup-2.." : 4,
            "consumer-2.." : 4,
            "recording-2.." : 4,

            "cleanup-2." : 4,
            "consumer-2." : 4,
            "periodic-2." : 4
        }
    }



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

    return

if __name__ == "__main__":
    parser = argparse.ArgumentParser("generate a template pinning file np04-srv-031")
    args = parser.parse_args()
    main(args)