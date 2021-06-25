use std::env;
use std::path::Path;

use libbpf_cargo::SkeletonBuilder;

const SRC: &str = "./src/bpf/exitstat.bpf.c";

fn main() {
    let path = env::var("OUT_DIR").unwrap() + "/exitstat.skel.rs";
    let skel = Path::new(&path);
    SkeletonBuilder::new(SRC).generate(&skel).unwrap();
    println!("cargo:rerun-if-changed={}", SRC);
}
