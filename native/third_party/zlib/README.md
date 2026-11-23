# zlib decompression subset

Unmodified files from upstream zlib1.3.2, tag `v1.3.2`, commit
`da607da739fa6047df13e66a2af6b8bec7c2a498`. Source identities are in
[`SOURCE.json`](SOURCE.json); original notices and [`LICENSE`](LICENSE) remain
intact. [Official release](https://zlib.net/),
[upstream repository](https://github.com/madler/zlib/tree/v1.3.2).

Only memory decompression/checksum support is built statically. Native container
validation, file ownership, input/output bounds and publication policy are the
project's responsibility. Compression, gzip file APIs and graphics API handling
are outside this selection. Upstream C compiles separately from project C++.
