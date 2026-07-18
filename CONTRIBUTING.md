# Contributing

Contributions use Vela's ordinary producer contract. There is no repository
script, auto-merge authority, or maintainer key in the producer path.

1. Start from a clean clone and the Vela release pinned by `vela.lock`.
2. Run `vela next . --limit 1 --json`; do not skip the first ranked target.
3. Claim it with `vela work <target> --as agent:<name> --json`.
4. Follow the returned hash-pinned packet and run the declared frozen verifier.
5. Land one scoped Receipt with `vela land`; include the exact artifact and an
   honest caveat.
6. Push the unsigned Git commits and open a pull request.

CI replays the event log, checks signatures and roots, and re-runs retained
verifiers. A successful verifier is evidence, not acceptance. A landing may be
admitted only by an already signed exact Permit policy; otherwise it remains a
pending proposal for one protected human decision.

Negative work must name its finite search space, algorithm, counts, and replay
command. It must not be described as universal nonexistence unless the search
is exhaustive over the full mathematical space.

Do not hand-edit `.vela/events`, accepted findings, `frontier.json`, proof
views, or `vela.lock`. Do not use a human signing key. See [DEBT.md](DEBT.md)
for the explicit provisional classification of two historical evidence links.
