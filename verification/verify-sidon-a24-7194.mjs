#!/usr/bin/env node

import { createHash } from "node:crypto";
import { readFile } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

const EXPECTED_N = 24;
const EXPECTED_SIZE = 7194;
const EXPECTED_WITNESS_ROOT = "878b05e01dbc4a785e5a671f977509f0bb338dfcb58ac53bf03d47bf6465f01e";

const probe = Object.freeze({
  left_index: 4023,
  right_index: 3324,
  partner_index: 2379,
  replace_index: 7193,
  replacement_hex: "61e8d7",
  replaced_hex: "891624",
});

function sha256(bytes) {
  return createHash("sha256").update(bytes).digest("hex");
}

function pointToBinaryCode(point, n) {
  let code = 0;
  for (let coordinate = 0; coordinate < n; coordinate += 1) {
    code |= point[coordinate] << coordinate;
  }
  return code;
}

function pointToTernaryCode(point, powersOfThree) {
  let code = 0;
  for (let coordinate = 0; coordinate < point.length; coordinate += 1) {
    code += point[coordinate] * powersOfThree[coordinate];
  }
  return code;
}

function binaryCodeToPoint(code, n) {
  return Array.from({ length: n }, (_, coordinate) => (code >>> coordinate) & 1);
}

function validateShape(witness) {
  if (witness?.kind !== "sidon") throw new Error("witness kind must be sidon");
  if (witness?.n !== EXPECTED_N) throw new Error(`witness dimension must be ${EXPECTED_N}`);
  if (witness?.claimed_size !== EXPECTED_SIZE) {
    throw new Error(`claimed_size must be ${EXPECTED_SIZE}`);
  }
  if (!Array.isArray(witness?.points) || witness.points.length !== EXPECTED_SIZE) {
    throw new Error(`witness must contain exactly ${EXPECTED_SIZE} points`);
  }

  for (const [pointIndex, point] of witness.points.entries()) {
    if (!Array.isArray(point) || point.length !== EXPECTED_N) {
      throw new Error(`point ${pointIndex} must contain exactly ${EXPECTED_N} coordinates`);
    }
    for (const [coordinate, bit] of point.entries()) {
      if (bit !== 0 && bit !== 1) {
        throw new Error(`point ${pointIndex}, coordinate ${coordinate} is not binary`);
      }
    }
  }
}

function verify(points, n) {
  const powersOfThree = Array.from({ length: n }, (_, coordinate) => 3 ** coordinate);
  const binaryCodes = new Set();
  const ternaryCodes = new Float64Array(points.length);

  for (let index = 0; index < points.length; index += 1) {
    const binaryCode = pointToBinaryCode(points[index], n);
    if (binaryCodes.has(binaryCode)) {
      return { ok: false, reason: "duplicate-point", point_code: binaryCode };
    }
    binaryCodes.add(binaryCode);
    ternaryCodes[index] = pointToTernaryCode(points[index], powersOfThree);
  }

  const sumCount = (points.length * (points.length + 1)) / 2;
  const sums = new Float64Array(sumCount);
  let cursor = 0;
  for (let left = 0; left < ternaryCodes.length; left += 1) {
    for (let right = left; right < ternaryCodes.length; right += 1) {
      sums[cursor] = ternaryCodes[left] + ternaryCodes[right];
      cursor += 1;
    }
  }
  if (cursor !== sumCount) throw new Error("internal unordered-sum count mismatch");

  sums.sort();
  for (let index = 1; index < sums.length; index += 1) {
    if (sums[index] === sums[index - 1]) {
      return {
        ok: false,
        reason: "pairwise-sum-collision",
        checked_sums: sumCount,
        collision_sum_code: sums[index],
      };
    }
  }

  return { ok: true, checked_sums: sumCount };
}

function hexPoint(points, index) {
  return pointToBinaryCode(points[index], EXPECTED_N).toString(16).padStart(6, "0");
}

const witnessPath = process.argv[2] ?? "artifacts/sidon-a24-gpt56-7194.witness.json";
const witnessBytes = await readFile(witnessPath);
const witnessRoot = sha256(witnessBytes);
if (witnessRoot !== EXPECTED_WITNESS_ROOT) {
  throw new Error(`unexpected witness root sha256:${witnessRoot}`);
}

const witness = JSON.parse(witnessBytes.toString("utf8"));
validateShape(witness);

const baseline = verify(witness.points, witness.n);
if (!baseline.ok) throw new Error(`baseline verification failed: ${JSON.stringify(baseline)}`);

if (hexPoint(witness.points, probe.replace_index) !== probe.replaced_hex) {
  throw new Error("adversarial probe replacement binding drifted");
}

const mutatedPoints = witness.points.map((point) => point.slice());
mutatedPoints[probe.replace_index] = binaryCodeToPoint(
  Number.parseInt(probe.replacement_hex, 16),
  witness.n,
);

const powersOfThree = Array.from({ length: witness.n }, (_, coordinate) => 3 ** coordinate);
const originalCollisionCode =
  pointToTernaryCode(mutatedPoints[probe.left_index], powersOfThree)
  + pointToTernaryCode(mutatedPoints[probe.right_index], powersOfThree);
const injectedCollisionCode =
  pointToTernaryCode(mutatedPoints[probe.partner_index], powersOfThree)
  + pointToTernaryCode(mutatedPoints[probe.replace_index], powersOfThree);
if (originalCollisionCode !== injectedCollisionCode) {
  throw new Error("adversarial probe does not preserve its bound collision identity");
}

const adversarial = verify(mutatedPoints, witness.n);
if (adversarial.ok || adversarial.reason !== "pairwise-sum-collision") {
  throw new Error(`adversarial probe was not rejected: ${JSON.stringify(adversarial)}`);
}

const sourcePath = fileURLToPath(import.meta.url);
const sourceRoot = sha256(await readFile(sourcePath));
const report = {
  schema: "sidon.independent-verification.v1",
  claim: "The reconstructed witness contains 7,194 distinct points in {0,1}^24 and has unique unordered componentwise pair sums.",
  subject: {
    path: path.relative(process.cwd(), witnessPath),
    sha256: `sha256:${witnessRoot}`,
    dimension: witness.n,
    point_count: witness.points.length,
  },
  verifier: {
    implementation: "node-base3-float64-sort-v1",
    source_path: path.relative(process.cwd(), sourcePath),
    source_sha256: `sha256:${sourceRoot}`,
    method: "Encode each binary point as an exact base-3 integer, sort every unordered pair-code sum, and reject adjacent duplicates.",
    independence_note: "This JavaScript implementation is separate from the frozen Rust vela-verify capsule used by Canopus.",
  },
  result: {
    outcome: "pass",
    unordered_componentwise_sums_checked: baseline.checked_sums,
  },
  adversarial_probe: {
    kind: "collision_injection",
    mutation: probe,
    collision_identity: {
      first_pair: [probe.left_index, probe.right_index],
      second_pair: [probe.partner_index, probe.replace_index],
      exact_base3_sum_code: originalCollisionCode,
    },
    outcome: "rejected",
    reason: adversarial.reason,
  },
  authority: {
    kind: "non_authoritative_auxiliary_evidence",
    vela_state_changed: false,
    review_attachment_registered: false,
    note: "This report strengthens reproducibility but is not a Vela verifier attachment or a human acceptance decision.",
  },
};

process.stdout.write(`${JSON.stringify(report, null, 2)}\n`);
