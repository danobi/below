// Copyright (c) Facebook, Inc. and its affiliates.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

namespace cpp2 resctl.below
namespace py3 resctl.below

include "cgroupfs.thrift"
include "procfs.thrift"

struct DataFrame {
  1: Sample sample,
}

struct Sample {
  1: CgroupSample cgroup,
  2: procfs.PidMap processes,
  3: SystemSample system,
  4: procfs.NetStat netstats,
}

struct CgroupSample {
  1: optional cgroupfs.CpuStat cpu_stat,
  2: optional map<string, cgroupfs.IoStat> io_stat,
  3: optional i64 memory_current,
  4: optional cgroupfs.MemoryStat memory_stat,
  5: optional cgroupfs.Pressure pressure,
  6: optional map<string, CgroupSample> children,
  7: optional i64 memory_swap_current,
  8: optional i64 memory_high,
  9: optional cgroupfs.MemoryEvents memory_events,
  10: optional i64 inode_number,
}

struct SystemSample {
  1: procfs.Stat stat,
  2: procfs.MemInfo meminfo,
  3: procfs.VmStat vmstat
  4: string hostname,
  5: procfs.DiskMap disks,
  6: optional string kernel_version,
  7: optional string os_release,
}
