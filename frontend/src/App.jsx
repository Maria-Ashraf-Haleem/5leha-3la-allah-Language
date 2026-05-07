import { useMemo, useState } from "react";
import Editor from "@monaco-editor/react";
import "./App.css";

const starterCode = `7ot elbdya() {
    7ot score = 10 + 5;

    law (score > 10) {
        efda7("score is greater than 10");
        efda7(score);
    }

    efda7("done");
}`;

function App() {
  const [code, setCode] = useState(starterCode);
  const [result, setResult] = useState(null);
  const [activeTab, setActiveTab] = useState("output");
  const [loading, setLoading] = useState(false);
  const [theme, setTheme] = useState("dark");

  function toggleTheme() {
    setTheme((currentTheme) => (currentTheme === "dark" ? "light" : "dark"));
  }

  async function runCode() {
    setLoading(true);
    setResult(null);

    try {
      const response = await fetch("http://localhost:5000/api/run", {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify({ code }),
      });

      const data = await response.json();
      setResult(data);

      if (!data.success || data.stderr || data.error) {
        setActiveTab("errors");
      } else {
        setActiveTab("output");
      }
    } catch (err) {
      setResult({
        success: false,
        error:
          "Cannot connect to backend. Make sure backend is running on port 5000.",
        stderr: String(err),
        stdout: "",
        tokens: "",
        ast: "",
        semantic: "",
        ir: "",
        cCode: "",
        output: "",
      });
      setActiveTab("errors");
    } finally {
      setLoading(false);
    }
  }

  function clearCode() {
    setCode("");
    setResult(null);
    setActiveTab("output");
  }

  function loadExample() {
    setCode(starterCode);
    setResult(null);
    setActiveTab("output");
  }

  const tabs = [
    { id: "output", label: "Program Output", content: result?.output },
    { id: "tokens", label: "Tokens", content: result?.tokens },
    { id: "ast", label: "AST", content: result?.ast },
    { id: "semantic", label: "Semantic Result", content: result?.semantic },
    { id: "ir", label: "Intermediate Code", content: result?.ir },
    { id: "cCode", label: "Generated C", content: result?.cCode },
    {
      id: "errors",
      label: "Errors",
      content: [result?.stderr, result?.error].filter(Boolean).join("\n"),
    },
    { id: "stdout", label: "Compiler Log", content: result?.stdout },
  ];

  const activeContent =
    tabs.find((tab) => tab.id === activeTab)?.content || "No output yet.";

  const status = useMemo(() => {
    if (loading) return { text: "Running", className: "running" };
    if (!result) return { text: "Ready", className: "ready" };
    if (!result.success || result.stderr || result.error) {
      return { text: "Failed", className: "failed" };
    }
    return { text: "Success", className: "success" };
  }, [loading, result]);

  return (
    <div className={`app-shell ${theme}`}>
      <header className="topbar">
        <div className="brand">
          <div className="brand-mark">&lt;/&gt;</div>

          <div>
            <h1>5leha 3la Allah Online Compiler</h1>
            <p>Compiler pipeline workspace</p>
          </div>
        </div>

        <div className="topbar-actions">
          <div className={`status ${status.className}`}>
            <span></span>
            {status.text}
          </div>

          <button className="btn secondary" onClick={toggleTheme}>
            {theme === "dark" ? "☀ Light" : "🌙 Dark"}
          </button>

          <button className="btn secondary" onClick={loadExample}>
            Example
          </button>

          <button className="btn secondary" onClick={clearCode}>
            Clear
          </button>

          <button className="btn primary" onClick={runCode} disabled={loading}>
            {loading ? "Running..." : "Run"}
          </button>
        </div>
      </header>

      <main className="workspace">
        <section className="pane editor-pane">
          <div className="pane-header">
            <div>
              <h2>Source Code</h2>
              <p>Write and execute code using your language syntax.</p>
            </div>

            <span className="meta">{code.length} characters</span>
          </div>

          <div className="editor-area">
            <Editor
              height="100%"
              defaultLanguage="c"
              value={code}
              theme={theme === "dark" ? "vs-dark" : "light"}
              onChange={(value) => setCode(value || "")}
              options={{
                fontSize: 16,
                minimap: { enabled: false },
                automaticLayout: true,
                scrollBeyondLastLine: false,
                padding: { top: 16, bottom: 16 },
                fontLigatures: true,
                lineNumbers: "on",
                wordWrap: "on",
              }}
            />
          </div>
        </section>

        <section className="pane result-pane">
          <div className="pane-header">
            <div>
              <h2>Compiler Output</h2>
              <p>Inspect each compiler stage separately.</p>
            </div>
          </div>

          <div className="tabs">
            {tabs.map((tab) => (
              <button
                key={tab.id}
                className={activeTab === tab.id ? "tab active" : "tab"}
                onClick={() => setActiveTab(tab.id)}
              >
                {tab.label}
              </button>
            ))}
          </div>

          <div className="output-area">
            <pre>{activeContent}</pre>
          </div>
        </section>
      </main>
    </div>
  );
}

export default App;