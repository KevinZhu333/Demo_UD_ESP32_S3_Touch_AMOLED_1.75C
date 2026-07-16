# Phase 06 — Storage, Reboot, and Failures

## Entry conditions

- Phase 05 passes.
- The authoritative inventory capability from `01/03` is operational.
- A fresh inventory invocation reports no active session/path, artifacts, ambiguity, finalized pair, or reservation.
- No partition operation is permitted until `06/02` passes.
- A disposable board and ignored backup directory are available.

Within `06/02`, its clean-inventory, geometry, command, backup-location, and manifest preflight must pass before its sole provisioning operation; no later task may perform a partition operation until all of `06/02` passes.

Task `06/01` is a software-only implementation and static control-flow gate. It
does not induce a live storage failure, write the board partition, or leave an
F4 orphan on the board; the authoritative inventory must therefore remain
empty for `06/02`.

## Task order

1. [06/01 — Preserve unfinished audio on storage failure](01-preserve-unfinished-audio-on-storage-failure.md)
2. [06/02 — Establish storage fixtures and provisioning proof](02-establish-storage-fixtures-and-provisioning-proof.md)
3. [06/03 — Prove failed-upload retention and ACK cleanup](03-prove-failed-upload-retention-and-ack-cleanup.md)
4. [06/04 — Revalidate and recover promoted metadata](04-revalidate-and-recover-promoted-metadata.md)
5. [06/05 — Prove active-WAV power-cut exclusion](05-prove-active-wav-power-cut-exclusion.md)
6. [06/06 — Add bounded in-filesystem fixture repair](06-add-bounded-in-filesystem-fixture-repair.md)
7. [06/07 — Preserve, repair, and drain ambiguous artifacts](07-preserve-repair-and-drain-ambiguous-artifacts.md)
8. [06/08 — Preserve pending audio through ordinary flash](08-preserve-pending-audio-through-ordinary-flash.md)
9. [06/09 — Preserve storage on mount failure](09-preserve-storage-on-mount-failure.md)
10. [06/10 — Stop safely, preserve, and close near-full storage](10-stop-safely-preserve-and-close-near-full-storage.md)

Run tasks in order. A task may start only after its declared predecessor passes. Any pre-phase-07 corrective follows the root [corrective-brick rules](../README.md#correctives-before-phase-07).

## Storage restrictions

- Normal partition writes require a current clean inventory report.
- Ambiguous fixtures are repaired only through bounded in-filesystem operations while index 2 remains pending.
- The repair operation may touch only manifest/hash-matched index-1 files and must prove index-2 WAV/metadata bytes unchanged.
- No repair-image partition write is permitted.

The sole exception to inventory-before-write is mount-failure restoration:

1. Clean inventory is proven before injection.
2. Exact known-good and invalid-image hashes are recorded.
3. Invalid-image readback matches before boot.
4. No session or intervening write occurs.
5. Raw hash remains the invalid-image hash after failed boot.
6. Only the recorded known-good backup may then be restored.
7. Restoration readback must match the original known-good hash.
8. Any mismatch stops without another write.

Near-full teardown permits `storage-flash` only when authoritative inventory exactly matches:

- No active session or active reserved path.
- No finalized pair or upload/cleanup reservation.
- No canonical or temporary sidecar.
- No WAV except the single manifest/hash-identified F4 orphan.
- No unclassified ambiguity.
- Only the manifest/hash-identified synthetic filler and known F4 orphan are present.
- The operator explicitly records F4 cleanup authorization.

## Fixture protocol

- Each scenario starts by recording authoritative inventory, firmware/configuration fingerprints, partition geometry, receiver-directory identity, and a manifest of every expected synthetic file and SHA-256 in an ignored or external directory.
- Synthetic fixtures use unmistakably non-user audio and contain no credentials. Raw partition backups and PCM are treated as sensitive evidence and never enter tracked source.
- A fixture write is refused if the live inventory differs from its declared clean precondition. After injection, only exact manifest/hash matches may be classified as disposable.
- Finalized pairs are never manually deleted: they drain only after a type/run/index/SHA-matching ACK and acknowledgment-driven cleanup. Manual bounded cleanup is limited to explicitly authorized synthetic ambiguity material or an identified F4 orphan.
- Every scenario records post-test inventory. Any unexpected path, hash, reservation, ambiguity, or write result stops the chain without erase or another partition write.

## Exit conditions

- Every storage test ends with verification modes disabled.
- All finalized pairs receive full matching ACK cleanup.
- Only explicitly authorized F4/synthetic cleanup uses `storage-flash`.
- Device inventory is empty after task `06/10`.
- Every storage bundle and raw hash manifest is preserved externally.
- The accepted flag-off build is restored.
