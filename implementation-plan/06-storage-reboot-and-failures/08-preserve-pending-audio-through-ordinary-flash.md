# Preserve pending audio through ordinary flash

Status: Planned
Requirements: F1, F3, F5, J4
Depends on: 06/07

## Outcome
An ordinary firmware `idf.py flash` leaves a finalized unacknowledged segment and its metadata byte-for-byte intact, and the rebooted device later uploads and full-ACK-cleans it.

## Current gap
design.md separates ordinary flash from `storage-flash`, but pending-audio preservation across an actual firmware flash lacks board evidence.

## In scope
- With clean inventory, finalize one segment while the receiver is unavailable and record its WAV/metadata hashes, pending state, partition geometry, and firmware/configuration fingerprints.
- Run only ordinary `idf.py flash`, prove unchanged pending hashes after reboot, restore the receiver, then require validation, full matching ACK, and acknowledgment-driven cleanup.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Invoking `storage-flash`, erasing the board, or using a partition-image write.
- Accepting a boot that reformats, mutates, renames, or discards the pending pair.

## Likely files
- main/CMakeLists.txt
- partitions.csv
- docs/design.md

## Acceptance
- The pre-flash pair is finalized, unacknowledged, valid, and hash-recorded while the receiver has no accepted copy.
- The command transcript contains ordinary `idf.py flash` and no storage target; post-boot WAV/metadata bytes and hashes are unchanged.
- Recovery posts the preserved pair once in order, a type/run/index/SHA-matching ACK drives WAV-first cleanup, and inventory ends empty.
- The sanitized bundle records both firmware hashes and the unchanged storage artifacts without exposing configuration secrets.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
