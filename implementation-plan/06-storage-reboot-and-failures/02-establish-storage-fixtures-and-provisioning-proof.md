# Establish storage fixtures and provisioning proof

Status: Planned
Requirements: F1, F3, F5
Depends on: 06/01

## Outcome
A disposable-board procedure proves explicit `storage-flash` provisioning on erased clean storage and establishes hash-manifested, fail-closed fixture inputs for later storage tests.

## Current gap
design.md requires an erased/disposable-board provisioning proof before pending-audio preservation tests, but no reproducible fixture manifest, backup location, or guarded preflight is recorded.

## In scope
- Define external synthetic fixture manifests, partition geometry, exact read/write commands, hash checks, inventory preconditions, expected post-boot classifications, and scenario-specific teardown rules.
- After the preflight passes on a disposable board with authoritative clean inventory, run `idf.py storage-flash` once, boot, prove an empty mounted filesystem, and record the generated-image and readback hashes.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Creating pending finalized audio before the clean provisioning proof passes.
- Using a real session recording as disposable fixture data or running `storage-flash` as ordinary firmware deployment.

## Likely files
- main/CMakeLists.txt
- partitions.csv
- docs/design.md

## Acceptance
- The preflight records a clean authoritative inventory, disposable-board identity, partition offset/size, ignored backup directory, and exact SHA-256 manifest before any partition write.
- The sole provisioning command is the documented `storage-flash` target; readback geometry and SHA-256 match the expected generated image.
- Boot mounts the explicitly provisioned partition empty without automatic formatting, and post-boot inventory is authoritatively empty.
- The handoff records that later ordinary `idf.py flash` must not touch storage and that no downstream partition write may bypass its scenario gate.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
