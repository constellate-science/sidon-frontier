# Historical debt and current classification

As of Git commit `08aa8aba053f3aba8277cd298469d658382307c3`, canonical
replay is exact:

- event log: `sha256:5f85d0896688fec7b5a6517928f9e954c0136c8f345ade51011b95b42b9d6cb6`
- snapshot: `sha256:a09958d8bdb66ddcbbd274c345c6beab2fbf409b60e2e940ae4902487d952ce4`
- proposals: `sha256:378798850a42f954df2572c64df196de16043dbaf765207d423ff90e8d1d2f7f`

Two accepted artifact records target findings that are not accepted state:

- `va_49e6efae2300df2b` targets `vf_7ffdf3818f716551`;
- `va_904716d35fec606e` targets `vf_0c5bfa8a18a5714c`.

Each finding ID belongs to exactly one retained pending `finding.add` proposal.
Vela now reports these links as provisional evidence targets rather than as
accepted findings. This classification does not authenticate the proposals,
admit either finding, or alter their pending status.

The proof projection has been regenerated from the exact event log and retained
for clean-clone replay. `vela check . --strict` is green while the two links
remain explicit in the artifact audit. If either pending proposal disappears,
is rejected or withdrawn, becomes ambiguous, or its target changes, the audit
fails closed again. The eventual scientific decisions remain human authority.
