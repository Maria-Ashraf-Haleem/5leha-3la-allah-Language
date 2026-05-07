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

app.post("/api/run", (req, res) => {
  const code = req.body.code;

  if (!code || typeof code !== "string") {
    return res.status(400).json({
      success: false,
      error: "No source code provided.",
    });
  }

  fs.writeFileSync(inputPath, code, "utf8");

  execFile(compilerPath, [inputPath], { cwd: projectRoot }, (error, stdout, stderr) => {
    const ir = readFileSafe(irPath);
    const cCode = readFileSafe(cPath);
    const output = readFileSafe(outputPath);

    res.json({
      success: !error,
      stdout,
      stderr,
      ir,
      cCode,
      output,
      error: error ? error.message : "",
    });
  });
});

app.listen(PORT, () => {
  console.log(`Backend running on http://localhost:${PORT}`);
});