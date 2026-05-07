import { useState } from "react";
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

  async function runCode() {
    setLoading(true);
    setResult(null);

    try {
      const response = await fetch("http://localhost:5000/api/run", {
        method: "POST",
        headers: {
          "Content-Type": "application/json"
        },
        body: JSON.stringify({ code })
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
        stdout: "",
        stderr: String(err),
        ir: "",
        cCode: "",
        output: "",
        error: "Cannot connect to backend. Make sure backend is running on port 5000."
      });
      setActiveTab("errors");
    } finally {
      setLoading(false);
    }
  }

  function clearCode() {
    setCode("");
    setResult(null);
  }

  const tabs = [
    { id: "output", label: "Program Output", content: result?.output },
    { id: "ir", label: "Intermediate Code", content: result?.ir },
    { id: "cCode", label: "Generated C", content: result?.cCode },
    { id: "stdout", label: "Compiler Log", content: result?.stdout },
    {
      id: "errors",
      label: "Errors",
      content: [result?.stderr, result?.error].filter(Boolean).join("\n")
    }
  ];

  const activeContent =
    tabs.find((tab) => tab.id === activeTab)?.content || "No output yet.";

  return (
    <div className="app">
      <header className="header">
        <div>
          <h1>5leha 3la Allah Online Compiler</h1>
          <p>React + Node.js Backend + C Compiler Pipeline</p>
        </div>

        <div className="actions">
          <button className="run-btn" onClick={runCode} disabled={loading}>
            {loading ? "Running..." : "Run"}
          </button>
          <button className="clear-btn" onClick={clearCode}>
            Clear
          </button>
        </div>
      </header>

      <main className="layout">
        <section className="panel editor-panel">
          <div className="panel-title">Code Editor</div>

          <textarea
            className="code-editor"
            value={code}
            onChange={(e) => setCode(e.target.value)}
            spellCheck="false"
          />
        </section>

        <section className="panel output-panel">
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

          <pre className="output-box">{activeContent}</pre>
        </section>
      </main>
    </div>
  );
}

export default App;