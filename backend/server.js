const express = require("express");
const cors = require("cors");
const fs = require("fs");
const path = require("path");
const { execFile } = require("child_process");

const app = express();
const PORT = 5000;

app.use(cors());
app.use(express.json());

const projectRoot = path.join(__dirname, "..");

const compilerPath = path.join(projectRoot, "compiler.exe");
const inputPath = path.join(projectRoot, "input", "source.txt");

const irPath = path.join(projectRoot, "generated", "out.ir");
const cPath = path.join(projectRoot, "generated", "out.c");
const outputPath = path.join(projectRoot, "generated", "output.txt");

function readFileSafe(filePath) {
  try {
    return fs.readFileSync(filePath, "utf8");
  } catch {
    return "";
  }
}

function extractSection(text, startMarker, endMarker) {
  const startIndex = text.indexOf(startMarker);

  if (startIndex === -1) {
    return "";
  }

  const contentStart = startIndex + startMarker.length;
  const endIndex = endMarker ? text.indexOf(endMarker, contentStart) : -1;

  if (endIndex === -1) {
    return text.slice(contentStart).trim();
  }

  return text.slice(contentStart, endIndex).trim();
}

app.post("/api/run", (req, res) => {
  const code = req.body.code;

  if (!code || typeof code !== "string") {
    return res.status(400).json({
      success: false,
      error: "No source code provided.",
    });
  }

  fs.writeFileSync(inputPath, code, "utf8");

  execFile(
    compilerPath,
    [inputPath],
    { cwd: projectRoot },
    (error, stdout, stderr) => {
      const ir = readFileSafe(irPath);
      const cCode = readFileSafe(cPath);
      const output = readFileSafe(outputPath);

      const tokens = extractSection(
        stdout,
        "=== SCANNER ===",
        "=== PARSER ==="
      );

      const ast = extractSection(
        stdout,
        "=== Abstract Syntax Tree ===",
        "=== SEMANTIC ANALYSIS ==="
      );

      const semantic = extractSection(
        stdout,
        "=== SEMANTIC ANALYSIS ===",
        "=== INTERMEDIATE CODE ==="
      );

      const intermediateFromLog = extractSection(
        stdout,
        "=== INTERMEDIATE CODE ===",
        "IR written to generated/out.ir"
      );

      const codeGeneration = extractSection(
        stdout,
        "=== CODE GENERATION ===",
        "=== EXECUTABLE GENERATION ==="
      );

      const executableGeneration = extractSection(
        stdout,
        "=== EXECUTABLE GENERATION ===",
        "=== PROGRAM OUTPUT ==="
      );

      res.json({
        success: !error,

        tokens,
        ast,
        semantic,

        ir: ir || intermediateFromLog,
        cCode,
        output,

        codeGeneration,
        executableGeneration,

        stdout,
        stderr,
        error: error ? error.message : "",
      });
    }
  );
});

app.listen(PORT, () => {
  console.log(`Backend running on http://localhost:${PORT}`);
});