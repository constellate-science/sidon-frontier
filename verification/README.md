# Independent Build Week verification

`verify-sidon-a24-7194.mjs` is a second implementation for the pending GPT-5.6
Sidon result. It does not reuse the frozen Rust `vela-verify` executable used by
Canopus. Instead, it encodes every binary vector as an exact base-3 integer,
sorts all 25,880,415 unordered pair-code sums, and rejects any duplicate.

It also applies one deterministic, root-bound mutation to the public witness.
The mutation keeps 7,194 distinct binary points but injects a known pair-sum
collision. The same implementation must reject that altered input.

Run from the repository root:

```bash
node verification/verify-sidon-a24-7194.mjs \
  artifacts/sidon-a24-gpt56-7194.witness.json
```

The checked-in JSON report is the exact stdout of that command. This auxiliary
verification changes no Vela state, is not registered as a durable Vela
verifier attachment, and is not a human acceptance decision.
