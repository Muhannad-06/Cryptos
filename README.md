# Cryptos

<p align="center">
  <strong>A bare-metal, transaction-aware hierarchical binary file container and archive engine written in ISO C99.</strong>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Language-C99-00599C?style=for-the-badge&logo=c" alt="C99" />
  <img src="https://img.shields.io/badge/Security-AES--256--GCM-critical?style=for-the-badge&logo=openssl" alt="AES-256-GCM" />
  <img src="https://img.shields.io/badge/Architecture-Append--Only%20Snapshots-blueviolet?style=for-the-badge" alt="Append Only" />
  <img src="https://img.shields.io/badge/Platform-Linux%20%7C%20Windows%20(MinGW)-blue?style=for-the-badge" alt="Platform" />
</p>

---

### Authors & Core Developers
<p align="center">
  <strong>Mohannad El-Shahiedy</strong> &nbsp;&bull;&nbsp; <strong>Mohamed Ali</strong>
  <br>
  <em>Equal Co-creators & System Architects</em>
</p>

---

## Table of Contents
- [Overview](#overview)
- [Key Features](#key-features)
- [Logical Data Hierarchy](#logical-data-hierarchy)
- [On-Disk Binary Specification](#on-disk-binary-specification)
- [Cryptographic Engine](#cryptographic-engine)
- [Modal Interactive CLI](#modal-interactive-cli)
- [Project Structure](#project-structure)
- [Building & Installation](#building--installation)
- [CLI Quickstart](#cli-quickstart)
- [Roadmap & Enhancements](#roadmap--enhancements)

---

## Overview

**Cryptos** is a low-level binary container format and management utility engineered from scratch without reliance on third-party archiving libraries. Designed to mirror the security and organizational demands of modern credential vaults and structured binary packages, Cryptos pairs an **append-only trailing directory architecture** with **Galois/Counter Mode (AES-256-GCM)** authenticated encryption and an in-memory lexical search index.

Whether navigating deeply nested organizational accounts or inspecting historical file revisions via pointer-based time travel, Cryptos delivers $O(1)$ directory lookups, crash-resilient persistence, and strict cross-platform byte reproducibility.

---

## Key Features

- **Strict Hierarchical Model**: Multi-level domain organization: `Archive` $\rightarrow$ `Group` $\rightarrow$ `Entry` $\rightarrow$ `Field`.
- **Append-Only Snapshot Storage**: Changes append payloads and directories to the tail of the file. Historical revisions remain reachable via chained backward directory offsets.
- **Atomic-Style Crash Resilience**: The active archive pointer sits in the final 8 bytes of the file. Incomplete writes leave the previous snapshot intact.
- **Portability & Endian Safety**: Custom Little-Endian serialization primitives (`Bin_IO`) guarantee cross-architecture parity across x86, ARM, and RISC-V systems.
- **Authenticated AEAD Encryption**: AES-256-GCM with 96-bit initialization vectors and 128-bit integrity tags via OpenSSL `EVP`, alongside constant-time memory comparisons.
- **In-Memory Search Index**: Binary Search Tree (BST) indexing for instant $O(\log n)$ entity and full-text keyword searches across loaded containers.
- **Compaction Engine**: Integrated garbage collector (`write_archive_clean`) that prunes orphaned historical snapshots and compacts live payloads.
- **Modal Context-Driven REPL**: State-machine CLI inspired by `diskpart` and Cisco IOS, featuring context transitions, scoped operations, and safety confirmation gates.

---

## Logical Data Hierarchy

```
[ Archive: Global Container ]
  │
  ├── [ Group: "Infrastructure" (ID: 0) ]
  │     ├── [ Entry: "Production-DB" (ID: 0) ]
  │     │     ├── Field: "Host"      (Type: TEXT)
  │     │     ├── Field: "Root-Pass" (Type: PASSWORD)
  │     │     └── Field: "SSL-Cert"  (Type: BINARY)
  │     │
  │     └── [ Entry: "Bastion-Host" (ID: 1) ]
  │           └── Field: "SSH-Key"   (Type: PASSWORD)
  │
  └── [ Group: "Personal" (ID: 1) ]
        └── ...
```

1. **Archive**: The root container holding global metadata, modification timestamps, change counters, and directory pointers.
2. **Group**: High-level logical categories grouping related entries.
3. **Entry**: Individual asset records or account profiles.
4. **Field**: The atomic storage unit holding raw data payloads, MIME types (`TEXT`, `PASSWORD`, `BINARY`), compression flags, and CRC32 checksums.

---

## On-Disk Binary Specification

Cryptos separates physical payload storage from metadata directories, utilizing a **Trailing Central Directory**:

```
+-------------------------------------------------------------------------+
| OFFSET 0x00: Magic Signature (4 Bytes: 0x4D4D3333 -> "MM33")            |
+-------------------------------------------------------------------------+
| CONTINUOUS PAYLOAD STREAM:                                              |
|   ┌──────────────────────────────────────────────────────────────────┐  |
|   │ [Local Field Header 1] [Field 1 Raw Payload Bytes]               │  |
|   │ [Local Field Header 2] [Field 2 Raw Payload Bytes]               │  |
|   │ ...                                                              │  |
|   └──────────────────────────────────────────────────────────────────┘  |
+-------------------------------------------------------------------------+
| ACTIVE DIRECTORY (Snapshot N):                                          |
|   - Directory Header (Magic, Version, Timestamp, Change Counter)        |
|   - Backward Pointer: Absolute File Offset to Snapshot N-1 (8 Bytes)   |
|   - Group Header Records Table                                          |
|   - Entry Header Records Table                                          |
|   - Directory Field Headers Table (Contains Absolute File Offsets)      |
|   - Archive Label & UTF-8 Description                                   |
+-------------------------------------------------------------------------+
| TRAILER (Last 8 Bytes of File):                                         |
|   Absolute Byte Offset pointing to Active Directory (uint64_t)          |
+-------------------------------------------------------------------------+
```

### Binary Read & Write Flow
- **Fast Mounting ($O(1)$)**: The engine jumps directly to `EOF - 8`, reads the 64-bit directory offset, seeks directly to the directory table, and instantiates the object graph without scanning payload streams.
- **Rollback / Time Travel**: Following `prev_dir_offset` enables historical snapshot recovery without external version control systems.
- **Safe Appending**: Updates append new field payloads followed by a fresh directory table; the trailing 8-byte pointer is updated only once data has successfully landed on disk.

---

## Cryptographic Engine

Security is powered by **AES-256-GCM** (Galois/Counter Mode) via the OpenSSL `EVP` API:

- **AEAD Guarantees**: Confirms both confidentiality (payload masking) and integrity (data authenticity). Any bit tampering within the container invalidates the 128-bit authentication tag upon execution of `EVP_DecryptFinal_ex()`.
- **Timing Side-Channel Immunity**: Secret buffers and tags are verified using `CRYPTO_memcmp()` (constant-time execution), neutralizing cache and timing leakage attacks.
- **Cryptographic Randomness**: Ephemeral Initialization Vectors (IVs, 96-bit) and 256-bit symmetric keys are generated via cryptographically secure pseudo-random generators (`RAND_bytes`).

---

## Modal Interactive CLI

The CLI operates as a stateful REPL, providing dedicated command contexts depending on the current layer:

```
[ ARCHIVE CONTEXT ] ──(select group)──> [ GROUP CONTEXT ]
       │                                       │
       │                                  (select entry)
       │                                       │
       ▼                                       ▼
[ FIELD CONTEXT ]   <──(select field)──  [ ENTRY CONTEXT ]
```

- **Archive Context (`<Archive> >`)**: Global operations (`list groups`, `search <term>`, `add group`, `save`, `info`).
- **Group Context (`GROUP <Name> >`)**: Scoped operations (`list`, `add entry`, `set-name`, `delete`, `back`).
- **Entry Context (`ENTRY <Name> >`)**: Account operations (`list`, `add field`, `set-group`, `set-name`, `back`).
- **Field Context (`FIELD <Name> >`)**: Atomic field operations (`show`, `set-content`, `set-type`, `back`).

---

## Project Structure

```
cryptos/
├── src/
│   ├── archive.c          # Archive lifecycle, rollback, and traversal logic
│   ├── archive.h          # Header definitions for Archive, Group, Entry, Field
│   ├── archive-cli.c      # Modal interactive CLI REPL and command handlers
│   ├── Bin_IO.c           # Explicit Little-Endian binary serialization primitives
│   ├── Bin_IO.h           # I/O function prototypes and status codes
│   ├── bst.c              # Binary Search Tree indexing implementation
│   ├── bst.h              # BST search structures and query APIs
│   ├── vector.c           # Generic dynamic resizing array implementation
│   ├── vector.h           # Dynamic array macros and functions
│   ├── error.c            # Platform-safe error handling and formatted logging
│   └── error.h            # Error types and cross-platform defines
├── crypto/
│   ├── aes_gcm.c          # AES-256-GCM authenticated encryption implementation
│   └── aes_gcm.h          # Cryptographic prototypes and OpenSSL wrappers
├── CMakeLists.txt         # CMake build configuration
└── README.md              # Project documentation
```

---

## Building & Installation

### Prerequisites
- **Compiler**: GCC or Clang supporting **ISO C99**.
- **Libraries**: OpenSSL (`libssl` and `libcrypto` headers and binaries).
- **Build System**: CMake (v3.15+) or standard GNU Make.

### Linux (Ubuntu / Debian)
```bash
# Install dependencies
sudo apt-get update
sudo apt-get install build-essential cmake libssl-dev

# Clone repository
git clone https://github.com/your-username/cryptos.git
cd cryptos

# Configure and compile
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Windows (MinGW-w64 / MSYS2)
```bash
# From an MSYS2 MinGW 64-bit shell:
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-openssl

mkdir build && cd build
cmake -G "MinGW Makefiles" ..
mingw32-make
```

---

## CLI Quickstart

Launch the interactive shell:
```bash
./cryptos my_vault.db
```

### Example Session
```text
=== Cryptos Archive Engine ===
Archive 'my_vault.db' opened.

my_vault > add group Infrastructure
Group 'Infrastructure' created.

my_vault > select 1
Switched to context: GROUP Infrastructure

GROUP Infrastructure > add entry ProductionServer
Entry 'ProductionServer' created.

GROUP Infrastructure > select 1
Switched to context: ENTRY ProductionServer

ENTRY ProductionServer > add field RootPassword password
Field 'RootPassword' created.

ENTRY ProductionServer > select 1
Switched to context: FIELD RootPassword

FIELD RootPassword > set-content s3cur3P@ssw0rd!
Content updated.

FIELD RootPassword > show
[Field: RootPassword] Content: s3cur3P@ssw0rd!

FIELD RootPassword > back
ENTRY ProductionServer > back
GROUP Infrastructure > back

my_vault > save
Archive successfully committed to disk.

my_vault > quit
Goodbye!
```

---

## Roadmap & Enhancements

- [ ] **Stream Compression**: Pluggable DEFLATE / zlib compression algorithms for non-text binary streams.
- [ ] **Hardware Acceleration**: Explicit AES-NI acceleration via platform assembly intrinsics.
- [ ] **Key Derivation Function (KDF)**: Argon2id / PBKDF2 passphrase key derivation for password-protected master vaults.
- [ ] **Export / Import Bridges**: JSON and CSV export/import tooling for migration to and from standard credential formats.

---

## License

This project is licensed under the [MIT License](LICENSE) &mdash; feel free to adapt, modify, and build upon it.