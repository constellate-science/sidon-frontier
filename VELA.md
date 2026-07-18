# Additive combinatorics: Sidon sets and N(h,k) bounds — agent charter

This frontier is driven by `vela` and Git. The ordinary producer path is
`next -> work -> reproduce -> land`. Agents stop after the routed landing; only
a registered human or a previously human-signed exact Permit policy changes
accepted state. `vela agents sync` regenerates tool-specific adapters from this
file.

## Agent rules

Agents may:

- inspect `vela status .`, `vela next .`, and `vela check .`;
- claim one ranked target with `vela work <target> --as agent:<name> --json`;
- run the frozen verifier named by the packet;
- land one Receipt through the active work session;
- prepare one exact protected review or policy plan for human inspection; and
- rebuild derived views only through Vela porcelain.

Agents may not:

- approve a protected card or use a human signing key;
- accept, reject, apply, or finalize a truth-bearing proposal;
- treat verifier success, Git publication, or a model answer as acceptance;
- hand-edit accepted events, findings, `frontier.json`, proof views, or
  `vela.lock`; or
- hide the structural debt recorded in `DEBT.md`.

## Fast commands

```bash
vela status . --json
vela next . --limit 1 --json
vela work <target> --as agent:<name> --json
vela reproduce <witness>
vela land --frontier . --work <target> --claim <claim> \
  --type computational --replayability exact \
  --artifact <path>:<kind> --caveat <limit> \
  --as agent:<name> --json
vela review list . --json
vela check . --strict
```
