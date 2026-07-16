# Audit every K1–K9 condition

Status: Planned
Requirements: K1, K2, K3, K4, K5, K6, K7, K8, K9
Depends on: 09/07

## Outcome
K1–K7 and K9 are decided solely from the one immutable primary attempt, K8 solely from the separate immutable outage attempt, and every normative calculation and observation passes without run mixing.

## Current gap
Individual phase analyses need one final acceptance matrix that enforces source-run separation, exact clocks/formulas, listener coverage, and the distinction between K8 teardown and primary K7.

## In scope
- Reperform or independently check K1 duration, K2 terminal index set, K3 exact monotonic-versus-PCM duration, K4 every-boundary listening, K5 pre-Stop live matching ACK, K6 every pending transition/deadline, K7 Stop position/final-partial deadline, and K9 sleep/wake/counts against the same primary bundle.
- Check K8 from the different outage bundle: clean start, active-recording pending zero immediately before 60 continuous seconds offline, continued capture, and same-run pending zero within 60 seconds after reconnect while still active.
- Verify each row references immutable hashes, the same frozen firmware/configuration, and no supplemental second-session, smoke, verification build, or teardown substitution.
- Reconcile docs/design.md for every behavior, interface, implementation status, evidence status, or known gap changed by this task.

## Out of scope
- Combining attempts, relaxing a threshold, using outage teardown as K7, or treating supplemental outage listening as primary K4.

## Likely files
- docs/spec.md
- docs/design.md

## Acceptance
- K1–K7/K9 all pass from one primary run and K8 passes from one separate outage run, with exact calculations and observations independently reproducible from the manifests.
- Every K row identifies its run, attempt, firmware/configuration fingerprints, raw evidence hashes, and applicable listener/observation record.
- Any failure blocks the demo-complete claim and triggers replay or replanning according to the root rules.
- docs/design.md truthfully matches the post-task implementation and evidence; if no edit was needed, the handoff explains why.

## Required handoff
Report requirements addressed, files changed, checks run, unrun physical checks, and remaining gaps.
