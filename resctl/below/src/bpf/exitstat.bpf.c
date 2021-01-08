#ifdef FBCODE_BUILD
#include <bpf/vmlinux/vmlinux.h>
#else
#include "../open_source/vmlinux.h"
#endif // FBCODE_BUILD

#include <bpf/bpf_core_read.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

#define TASK_COMM_LEN 16

struct {
  __uint(type, BPF_MAP_TYPE_PERF_EVENT_ARRAY);
  __uint(key_size, sizeof(u32));
  __uint(value_size, sizeof(u32));
} events SEC(".maps");

struct metadata {
  pid_t tid; // thread (task) ID
  pid_t ppid; // parent process ID
  pid_t pgrp; // process group ID
  uint32_t sid; // session ID
  uint32_t cpu; // CPU task is running on
  char comm[TASK_COMM_LEN]; // process name
};

struct exitstats {
  uint64_t min_flt; /* Minor Page Fault Count - copy on write */
  uint64_t maj_flt; /* Major Page Fault Count - virtual memory */
  uint64_t utime_us; /* user cpu time in us */
  uint64_t stime_us; /* system cpu time in us */
  uint64_t etime_us; /* elapsed time in us */
  uint64_t nr_threads; /* Number of threads */
  uint64_t io_read_bytes; /* bytes of read i/o */
  uint64_t io_write_bytes; /* bytes of write i/o */
  uint64_t active_rss_pages; /* Active RSS usage, pages */
};

struct event {
  struct metadata meta;
  struct exitstats stats;
};

// sched:sched_process_exit is triggered right before process/thread exits. At
// this point we capture last taskstats to account resource usage of short-lived
// processes. We also check tas->signal.live counter to determine if this thread
// is the last thread in a process and thus is also a process exit.
SEC("tracepoint/sched/sched_process_exit")
int tracepoint__sched__sched_process_exit(
    struct trace_event_raw_sched_process_template* args
) {
  struct task_struct* task = (struct task_struct*)bpf_get_current_task();
  u64 pid_tgid = bpf_get_current_pid_tgid();
  u64 now = bpf_ktime_get_ns();

  struct event data = {};
  data.meta.tid = pid_tgid & 0xFFFFFFFF;
  data.meta.ppid = BPF_CORE_READ(task, real_parent, tgid);
  data.meta.pgrp = BPF_CORE_READ(task, group_leader, tgid);
  data.meta.sid = BPF_CORE_READ(task, sessionid);
  data.meta.cpu = BPF_CORE_READ(task, cpu);
  bpf_get_current_comm(&data.meta.comm, sizeof(data.meta.comm));

  /* read/calculate exitstats */
  data.stats.min_flt = BPF_CORE_READ(task, min_flt);
  data.stats.maj_flt = BPF_CORE_READ(task, maj_flt);
  data.stats.utime_us = BPF_CORE_READ(task, utime) / 1000;
  data.stats.stime_us = BPF_CORE_READ(task, stime) / 1000;
  data.stats.nr_threads = BPF_CORE_READ(task, signal, nr_threads);

  /* CONFIG_TASK_IO_ACCOUNTING is always enabled in fbk kernels */
  data.stats.io_read_bytes = BPF_CORE_READ(task, ioac.read_bytes);
  data.stats.io_write_bytes = BPF_CORE_READ(task, ioac.write_bytes);

  data.stats.etime_us = (now - BPF_CORE_READ(task, start_time)) / 1000;
  const struct mm_struct* mm = BPF_CORE_READ(task, mm);
  if (mm) {
    u64 file_pages = BPF_CORE_READ(mm, rss_stat.count[MM_FILEPAGES].counter);
    u64 anon_pages = BPF_CORE_READ(mm, rss_stat.count[MM_ANONPAGES].counter);
    u64 shmem_pages = BPF_CORE_READ(mm, rss_stat.count[MM_SHMEMPAGES].counter);
    u64 active_rss = file_pages + anon_pages + shmem_pages;

    data.stats.active_rss_pages = active_rss;
  } else {
    data.stats.active_rss_pages = 0;
  }

  bpf_perf_event_output(
      args, &events, BPF_F_CURRENT_CPU, &data, sizeof(struct event));
  return 0;
}

char _license[] SEC("license") = "GPL";
