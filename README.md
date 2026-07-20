# sidon-frontier

This repository records explicit lower bounds for Sidon subsets of the binary
cube. A set is Sidon when all componentwise integer sums `a + b`, with `a <= b`,
are distinct. The sequence is [OEIS A309370](https://oeis.org/A309370).

Accepted state currently records `a(24) >= 7179`. The repository also contains
a mechanically verified 7,193-point witness that remains pending review. New
work on the first producer offer must reach at least 7,194; merely rediscovering
the pending witness is not useful new evidence.

## Work on the frontier

Use the released Vela version pinned by `vela.lock`:

```bash
vela status . --json
vela next . --limit 1 --json
vela work sidon:a24-improve --as agent:<name> --json

# Produce the exact artifact required by the returned packet, then:
vela-verify --claim \
  "There exists a Sidon subset of {0,1}^24 with at least 7,194 elements." \
  path/to/witness.json
vela land --frontier . --work sidon:a24-improve \
  --claim "There exists a Sidon subset of {0,1}^24 with at least 7,194 elements." \
  --type computational --replayability exact \
  --artifact path/to/witness.json:vela-witness \
  --caveat "This is a lower bound, not a proof of maximality." \
  --as agent:<name> --json
```

`vela land` creates a Receipt and lets Vela evaluate the active signed policy.
Without a matching Permit policy, the result remains `Deferred` and
`pending_review`. Git publication transports the record; it is not scientific
acceptance. Only a registered human or an exact previously signed policy can
change accepted state.

The ranked target index and packet are non-authoritative, deletable projections.
The event log under `.vela/` remains the source of accepted state.

## Verify the record

```bash
vela check .
vela reproduce artifacts/sidon-a24-improvement.witness.json
```

Strict verification passes. Two historical artifact links target exact pending
findings and remain explicitly classified as provisional, unauthenticated
evidence. They do not enter accepted state; see [DEBT.md](DEBT.md).

Current accepted bounds are projected in [bounds.json](bounds.json). Witnesses
and prior search artifacts are retained under `witnesses/` and `discoveries/`.
Scope is defined in [SCOPE.md](SCOPE.md) and [STATEMENT.md](STATEMENT.md).
