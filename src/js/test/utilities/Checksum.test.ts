/**
 * Copyright 2020 Vectorized, Inc.
 *
 * Licensed as a Redpanda Enterprise file under the Redpanda Community
 * License (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 *
 * https://github.com/vectorizedio/redpanda/blob/master/licenses/rcl.md
 */

import { getChecksumFromFile } from "../../modules/utilities/Checksum";
import * as assert from "assert";
import { join } from "path";

describe("Checksum utilities", () => {
  describe("When we call getChecksumFromFile", () => {
    const expectChecksum =
      "948451abc0b5b04e542fa38ac6a5e1838b8755a48de78d69cd0000cce6affce1";
    describe("Given a existing file", () => {
      it("Should get file content's checksum", (done) => {
        const checksumPromise = getChecksumFromFile(
          join(__dirname, "/CoprocessorTest.ts")
        );
        checksumPromise
          .then((checksum) => {
            assert.equal(checksum, expectChecksum, "checksum unexpected");
            done();
          })
          .catch(done);
      });
    });
    describe("Given a unexiting file", () => {
      it("should it return a Promise reject", () => {
        const checksumPromise = getChecksumFromFile("NoFile.t");
        assert.rejects(checksumPromise, {
          code: "ENOENT",
        });
      });
    });
  });
});
