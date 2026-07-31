# j++
j++은 g++ 못 써서 만들었습니다.
.exe 실행파일로 되긴 하는데..
### 경고가!!!!!!!!!!!!!!!!!!! 안도ㅒ!!!!!!!!!!!!!
# 코드 드릴게요 j++ 짱짱.html이라고 저장하면 됨. 
```<!DOCTYPE html>
<html lang="ko">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>C++ Hex Viewer & Binary Exporter</title>
    <style>
        *, *::before, *::after {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
        }

        :root {
            --bg-primary: #1e1e1e;
            --bg-secondary: #252526;
            --bg-tertiary: #2d2d30;
            --bg-input: #1a1a1a;
            --border: #3f3f46;
            --border-light: #555;
            --text-primary: #d4d4d4;
            --text-secondary: #9cdcfe;
            --text-muted: #808080;
            --accent-blue: #007acc;
            --accent-blue-hover: #1177bb;
            --accent-purple: #605cd8;
            --accent-purple-hover: #7b77e8;
            --accent-green: #4ec9b0;
            --accent-red: #f48771;
            --accent-yellow: #dcdcaa;
            --radius: 6px;
            --shadow: 0 2px 8px rgba(0,0,0,0.4);
            --transition: all 0.2s ease;
        }

        body {
            background: var(--bg-primary);
            color: var(--text-primary);
            font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, Tahoma, Geneva, Verdana, sans-serif;
            padding: 30px 20px;
            max-width: 860px;
            margin: auto;
            line-height: 1.6;
            min-height: 100vh;
        }

        .header {
            text-align: center;
            margin-bottom: 30px;
            padding-bottom: 20px;
            border-bottom: 2px solid var(--border);
            position: relative;
        }

        .header::after {
            content: '';
            position: absolute;
            bottom: -2px;
            left: 50%;
            transform: translateX(-50%);
            width: 120px;
            height: 2px;
            background: linear-gradient(90deg, transparent, var(--accent-blue), transparent);
        }

        h1 {
            color: var(--accent-blue);
            font-size: 22px;
            font-weight: 700;
            letter-spacing: -0.5px;
            margin-bottom: 8px;
        }

        .header .subtitle {
            color: var(--text-muted);
            font-size: 13px;
        }

        .panel {
            background: var(--bg-secondary);
            border: 1px solid var(--border);
            padding: 20px;
            border-radius: var(--radius);
            margin-top: 18px;
            box-shadow: var(--shadow);
            transition: var(--transition);
            position: relative;
            overflow: hidden;
        }

        .panel::before {
            content: '';
            position: absolute;
            top: 0;
            left: 0;
            width: 3px;
            height: 100%;
            background: var(--accent-blue);
            opacity: 0;
            transition: var(--transition);
        }

        .panel:hover {
            border-color: var(--border-light);
            transform: translateY(-1px);
            box-shadow: 0 4px 16px rgba(0,0,0,0.5);
        }

        .panel:hover::before {
            opacity: 1;
        }

        .panel h3 {
            color: var(--text-secondary);
            font-size: 14px;
            font-weight: 600;
            margin-bottom: 12px;
            display: flex;
            align-items: center;
            gap: 8px;
        }

        textarea {
            width: 100%;
            height: 150px;
            background: var(--bg-input);
            color: var(--text-secondary);
            border: 1px solid var(--border);
            padding: 14px;
            font-size: 13px;
            font-family: 'Cascadia Code', 'Fira Code', Consolas, 'Courier New', monospace;
            border-radius: 4px;
            resize: vertical;
            transition: var(--transition);
            line-height: 1.5;
            tab-size: 4;
        }

        textarea:focus {
            outline: none;
            border-color: var(--accent-blue);
            box-shadow: 0 0 0 2px rgba(0, 122, 204, 0.2);
        }

        textarea::placeholder {
            color: var(--text-muted);
            font-style: italic;
        }

        .file-input-wrapper input[type="file"] {
            width: 100%;
            padding: 12px 14px;
            background: var(--bg-input);
            border: 1px dashed var(--border);
            border-radius: 4px;
            color: var(--text-secondary);
            font-size: 13px;
            cursor: pointer;
            transition: var(--transition);
        }

        .file-input-wrapper input[type="file"]:hover {
            border-color: var(--accent-blue);
            background: rgba(0, 122, 204, 0.05);
        }

        .file-input-wrapper input[type="file"]::file-selector-button {
            background: var(--bg-tertiary);
            color: var(--text-primary);
            border: 1px solid var(--border);
            padding: 6px 14px;
            border-radius: 3px;
            cursor: pointer;
            font-size: 12px;
            margin-right: 12px;
            transition: var(--transition);
        }

        .file-input-wrapper input[type="file"]::file-selector-button:hover {
            background: var(--accent-blue);
            color: white;
            border-color: var(--accent-blue);
        }

        .btn-group {
            margin-top: 22px;
            display: flex;
            gap: 12px;
            flex-wrap: wrap;
            justify-content: center;
        }

        button {
            background: var(--accent-blue);
            color: white;
            border: none;
            padding: 12px 22px;
            font-size: 13px;
            font-weight: 600;
            cursor: pointer;
            border-radius: 4px;
            transition: var(--transition);
            position: relative;
            overflow: hidden;
        }

        button::after {
            content: '';
            position: absolute;
            top: 50%;
            left: 50%;
            width: 0;
            height: 0;
            background: rgba(255,255,255,0.15);
            border-radius: 50%;
            transform: translate(-50%, -50%);
            transition: width 0.4s ease, height 0.4s ease;
        }

        button:active::after {
            width: 300px;
            height: 300px;
        }

        button:hover {
            background: var(--accent-blue-hover);
            transform: translateY(-2px);
            box-shadow: 0 4px 12px rgba(0, 122, 204, 0.3);
        }

        button:active {
            transform: translateY(0);
        }

        .hex-btn {
            background: var(--accent-purple);
        }

        .hex-btn:hover {
            background: var(--accent-purple-hover);
            box-shadow: 0 4px 12px rgba(96, 92, 216, 0.3);
        }

        .reset-btn {
            background: var(--bg-tertiary);
            border: 1px solid var(--border);
        }

        .reset-btn:hover {
            background: #444;
            box-shadow: 0 4px 12px rgba(0,0,0,0.3);
        }

        .log-section {
            margin-top: 25px;
        }

        .log-section h3 {
            color: var(--text-muted);
            font-size: 12px;
            text-transform: uppercase;
            letter-spacing: 1px;
            margin-bottom: 10px;
            display: flex;
            align-items: center;
            gap: 6px;
        }

        .log-section h3::before {
            content: '●';
            color: var(--accent-green);
            font-size: 8px;
            animation: pulse 2s infinite;
        }

        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.3; }
        }

        pre {
            background: #0d0d0d;
            padding: 16px;
            border: 1px solid var(--border);
            border-radius: 4px;
            min-height: 80px;
            max-height: 220px;
            overflow-y: auto;
            color: var(--accent-green);
            white-space: pre-wrap;
            word-break: break-all;
            font-family: 'Cascadia Code', 'Fira Code', Consolas, monospace;
            font-size: 12px;
            line-height: 1.6;
            box-shadow: inset 0 2px 6px rgba(0,0,0,0.5);
        }

        pre::-webkit-scrollbar { width: 8px; }
        pre::-webkit-scrollbar-track { background: #1a1a1a; }
        pre::-webkit-scrollbar-thumb { background: #444; border-radius: 4px; }
        pre::-webkit-scrollbar-thumb:hover { background: #555; }

        .success { color: var(--accent-green) !important; }
        .error { color: var(--accent-red) !important; }
        .info { color: var(--accent-yellow) !important; }

        .status-bar {
            margin-top: 20px;
            padding: 10px 14px;
            background: var(--bg-tertiary);
            border-radius: 4px;
            font-size: 11px;
            color: var(--text-muted);
            display: flex;
            justify-content: space-between;
            align-items: center;
            border: 1px solid var(--border);
        }

        .status-dot {
            display: inline-block;
            width: 8px;
            height: 8px;
            border-radius: 50%;
            background: var(--accent-green);
            margin-right: 6px;
            animation: pulse 2s infinite;
        }

        .status-dot.idle {
            background: var(--text-muted);
            animation: none;
        }

        @media (max-width: 600px) {
            body { padding: 15px 12px; }
            h1 { font-size: 18px; }
            .btn-group { flex-direction: column; }
            button { width: 100%; text-align: center; }
            .panel { padding: 14px; }
        }

        .panel, .btn-group, .log-section {
            animation: fadeIn 0.4s ease forwards;
        }

        @keyframes fadeIn {
            from { opacity: 0; transform: translateY(10px); }
            to { opacity: 1; transform: translateY(0); }
        }
    </style>
</head>
<body>

    <div class="header">
        <h1>C++ Hex Viewer & Binary Exporter</h1>
        <p class="subtitle">C++ 소스 코드를 로드해서 Hex 덤프를 확인하거나, raw 바이너리(.bin)로 내보내는 도구.</p>
    </div>

    <div class="panel">
        <h3>📂 파일 선택</h3>
        <div class="file-input-wrapper">
            <input type="file" id="cppFile" accept=".cpp,.cc,.cxx,.c,.h,.hpp,.txt" onchange="handleFile(event)">
        </div>
    </div>

    <div class="panel">
        <h3>📋 코드 직접 붙여넣기</h3>
        <textarea id="pasteCode" placeholder="// 여기에 C++ 코드를 붙여넣기...&#10;// #include &lt;iostream&gt;&#10;// int main() { std::cout &lt;&lt; &quot;hello&quot;; }" spellcheck="false"></textarea>
    </div>

    <div class="btn-group">
        <button class="hex-btn" onclick="processHex()">Hex 덤프 보기</button>
        <button onclick="exportBin()">.bin 으로 내보내기</button>
        <button class="reset-btn" onclick="resetAll()">초기화</button>
    </div>

    <div class="log-section">
        <h3>출력 로그</h3>
        <pre id="output">대기 중... 코드를 입력하거나 파일을 선택하세요.</pre>
    </div>

    <div class="status-bar">
        <span><span class="status-dot idle" id="statusDot"></span><span id="statusText">IDLE</span></span>
        <span id="bufferInfo">Buffer: 0 bytes</span>
    </div>

    <script>
        let activeBuffer = null;
        let activeName = "output";
        const encoder = new TextEncoder();

        function resolveActiveData() {
            const pasteText = document.getElementById('pasteCode').value.trim();
            if (pasteText.length > 0) {
                activeBuffer = encoder.encode(pasteText);
                activeName = "pasted_code";
                updateStatusBar();
                return true;
            }
            if (activeBuffer !== null && activeBuffer.length > 0) {
                updateStatusBar();
                return true;
            }
            return false;
        }

        function handleFile(event) {
            const file = event.target.files[0];
            if (!file) return;

            const dotIndex = file.name.lastIndexOf('.');
            activeName = dotIndex > 0 ? file.name.substring(0, dotIndex) : 'output';

            const reader = new FileReader();
            reader.onload = function(e) {
                try {
                    activeBuffer = new Uint8Array(e.target.result);
                    document.getElementById('pasteCode').value = "";
                    updateStatusBar();
                    showOutput(`[로드 완료] ${file.name} (${formatBytes(activeBuffer.length)})`, 'success');
                } catch (err) {
                    showOutput("파일 읽기 오류: " + err.message, 'error');
                }
            };
            reader.onerror = function() {
                showOutput("파일 읽기 실패: I/O 오류", 'error');
            };
            reader.readAsArrayBuffer(file);
        }

        function processHex() {
            if (!resolveActiveData()) {
                showOutput("처리할 데이터가 없습니다.", 'error');
                return;
            }

            try {
                const maxLen = Math.min(activeBuffer.length, 1024);
                let lines = [];

                for (let offset = 0; offset < maxLen; offset += 16) {
                    const addr = offset.toString(16).padStart(8, '0').toUpperCase();
                    let hexPart = [];
                    let asciiPart = [];

                    for (let i = 0; i < 16; i++) {
                        if (offset + i < maxLen) {
                            const b = activeBuffer[offset + i];
                            hexPart.push(b.toString(16).padStart(2, '0').toUpperCase());
                            asciiPart.push(b >= 0x20 && b <= 0x7E ? String.fromCharCode(b) : '.');
                        } else {
                            hexPart.push('  ');
                            asciiPart.push(' ');
                        }
                    }

                    lines.push(`${addr}  ${hexPart.slice(0, 8).join(' ')}  ${hexPart.slice(8).join(' ')}  |${asciiPart.join('')}|`);
                }

                let msg = `[Hex 덤프: ${maxLen}/${activeBuffer.length} bytes]\n\n` + lines.join('\n');
                if (activeBuffer.length > 1024) {
                    msg += `\n\n... (이하 ${activeBuffer.length - 1024} bytes 생략)`;
                }
                showOutput(msg, 'success');
            } catch (err) {
                showOutput("Hex 처리 오류: " + err.message, 'error');
            }
        }

        function exportBin() {
            if (!resolveActiveData()) {
                showOutput("내보낼 데이터가 없습니다.", 'error');
                return;
            }

            try {
                const blob = new Blob([activeBuffer], { type: 'application/octet-stream' });
                const url = URL.createObjectURL(blob);

                const a = document.createElement('a');
                a.href = url;
                a.download = `${sanitizeFilename(activeName)}.bin`;
                document.body.appendChild(a);
                a.click();
                document.body.removeChild(a);

                setTimeout(() => URL.revokeObjectURL(url), 2000);

                showOutput(
                    `내보내기 완료: ${a.download}\n` +
                    `크기: ${formatBytes(activeBuffer.length)}`,
                    'success'
                );
            } catch (err) {
                showOutput("내보내기 오류: " + err.message, 'error');
            }
        }

        function resetAll() {
            activeBuffer = null;
            activeName = "output";
            document.getElementById('cppFile').value = "";
            document.getElementById('pasteCode').value = "";
            updateStatusBar();
            showOutput("초기화 완료.", 'info');
        }

        function showOutput(msg, type) {
            const out = document.getElementById('output');
            out.className = type || '';
            out.textContent = msg;
        }

        function formatBytes(bytes) {
            if (bytes === 0) return '0 B';
            const units = ['B', 'KB', 'MB'];
            const i = Math.floor(Math.log(bytes) / Math.log(1024));
            return (bytes / Math.pow(1024, i)).toFixed(i > 0 ? 1 : 0) + ' ' + units[i];
        }

        function sanitizeFilename(name) {
            return name.replace(/[<>:"/\\|?*\x00-\x1F]/g, '_').substring(0, 80);
        }

        function updateStatusBar() {
            const dot = document.getElementById('statusDot');
            const text = document.getElementById('statusText');
            const info = document.getElementById('bufferInfo');

            if (activeBuffer && activeBuffer.length > 0) {
                dot.classList.remove('idle');
                text.textContent = 'READY';
                info.textContent = `Buffer: ${formatBytes(activeBuffer.length)}`;
            } else {
                dot.classList.add('idle');
                text.textContent = 'IDLE';
                info.textContent = 'Buffer: 0 bytes';
            }
        }
    </script>
</body>
</html>
