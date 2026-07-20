import { createHash } from "node:crypto";
import { execFileSync } from "node:child_process";
import { readFile, writeFile } from "node:fs/promises";

const witnessPath = "artifacts/sidon-a24-improvement.witness.json";
const targetPath = "targets/sidon-a24-improve.json";
const indexPath = "targets.json";

const sha256 = (value) => `sha256:${createHash("sha256").update(value).digest("hex")}`;
const encodePoint = (point) => {
  if (!Array.isArray(point) || point.length !== 24 || point.some((bit) => bit !== 0 && bit !== 1)) {
    throw new Error("the source witness contains a non-binary or non-24-dimensional point");
  }
  return Number.parseInt(point.join(""), 2).toString(16).padStart(6, "0");
};

const witnessBytes = await readFile(witnessPath);
const witness = JSON.parse(witnessBytes.toString("utf8"));
if (witness.kind !== "sidon" || witness.n !== 24 || witness.claimed_size !== 7193) {
  throw new Error("the source witness is not the exact tracked 7,193-point Sidon baseline");
}
if (!Array.isArray(witness.points) || witness.points.length !== witness.claimed_size) {
  throw new Error("the source witness cardinality is inconsistent");
}

const encodedPoints = witness.points.map(encodePoint).join(",");
const sourceCommit = execFileSync("git", ["rev-parse", "HEAD"], { encoding: "utf8" }).trim();
const target = JSON.parse(await readFile(targetPath, "utf8"));

target.objective = "Produce an explicit Sidon subset of {0,1}^24 with more than 7,193 points.";
target.current_state.tracked_unaccepted_seed = {
  authority: "repository_evidence_pending_review_not_accepted_state",
  size: 7193,
  path: witnessPath,
  sha256: sha256(witnessBytes),
  source_commit: sourceCommit,
  verification: "Frozen Vela 0.910.0 capsule reproduced 7,193 points and 25,873,221 distinct unordered pair sums on 2026-07-19.",
  encoding: "Comma-separated six-digit lowercase hexadecimal 24-bit vectors; the first source coordinate is the most-significant bit.",
  encoded_points_sha256: sha256(Buffer.from(encodedPoints)),
  encoded_points: encodedPoints,
  provenance: {
    method: "Exact one-removal/two-addition exchange over the tracked 7,192-point seed.",
    benchmark_path: "research/sidon-a24-exchange-benchmark.v1.json",
    removed_point_hex: "e28270",
    added_point_hex: ["22a8cb", "e6811f"]
  }
};
target.output_contract.path = "artifacts/sidon-a24-gpt56-7194.witness.json";
target.output_contract.format.claimed_size = "<integer at least 7194>";
target.output_contract.exact_claim = "There exists a Sidon subset of {0,1}^24 with at least 7,194 elements.";
target.verification.cardinality = "claimed_size equals the number of distinct points and is greater than 7,193.";
target.verification.novelty = "The candidate cardinality exceeds the mechanically verified pending 7,193-point witness, so a verifier pass is non-duplicate lower-bound evidence.";
target.negative_contract.scope = "A failed or bounded search is retained only as non-authoritative Canopus run evidence and does not establish that 7,193 is maximal.";

const targetBytes = Buffer.from(`${JSON.stringify(target, null, 2)}\n`);
await writeFile(targetPath, targetBytes);

const index = JSON.parse(await readFile(indexPath, "utf8"));
const offer = index.targets.find((candidate) => candidate.id === "sidon:a24-improve");
if (offer === undefined) throw new Error("the Sidon a(24) offer is missing from targets.json");
offer.title = "Improve the pending 7,193-point Sidon set in the 24-cube";
offer.why = "Accepted state records 7,179 points, while a mechanically verified 7,193-point witness remains pending review; useful new evidence must reach at least 7,194 without repeating the existing exchange.";
offer.objective = "Produce one explicit Sidon subset of {0,1}^24 with more than 7,193 points. A bounded failure is run evidence only and cannot claim maximality.";
offer.packet.sha256 = sha256(targetBytes);
await writeFile(indexPath, `${JSON.stringify(index, null, 2)}\n`);

process.stdout.write(`${offer.packet.sha256}\n`);
