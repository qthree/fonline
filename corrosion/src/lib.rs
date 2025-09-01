use std::path::Path;

pub fn engine_root() -> &'static Path {
    Path::new(env!("CARGO_MANIFEST_DIR")).parent().expect("parent of CARGO_MANIFEST_DIR")
}
