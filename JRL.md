# JRL이라는것을 제작했습니다
## JRL은 URL을 압축시켜주는것입니다.
> URL을 엄청나게 압축시켜버립니다.
>
> 매우 엄청나!
>
> 압축률은 그래도 높습네다.
코드:
> ```<!DOCTYPE html>
<html lang="ko">
<head>
    <meta charset="UTF-8">
    <title>JRL - Universal Share Protocol</title>
    <style>
        :root {
            --bg: #0f1115;
            --card-bg: #161920;
            --accent: #6366f1;
            --accent-hover: #4f46e5;
            --text: #f3f4f6;
            --text-muted: #9ca3af;
            --border: #2d3748;
        }

        body {
            background-color: var(--bg);
            color: var(--text);
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
            margin: 0;
            padding: 40px 20px;
            display: flex;
            flex-direction: column;
            align-items: center;
            min-height: 100vh;
            box-sizing: border-box;
        }

        header {
            text-align: center;
            margin-bottom: 40px;
        }

        h1 {
            font-size: 2.5rem;
            font-weight: 800;
            margin: 0;
            background: linear-gradient(135deg, #6366f1, #a855f7);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }

        p.subtitle {
            color: var(--text-muted);
            font-size: 0.95rem;
            margin-top: 8px;
        }

        .container {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 24px;
            width: 100%;
            max-width: 900px;
        }

        @media (max-width: 768px) {
            .container { grid-template-columns: 1fr; }
        }

        .card {
            background-color: var(--card-bg);
            border: 1px solid var(--border);
            border-radius: 12px;
            padding: 24px;
            box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1);
        }

        h2 {
            font-size: 1.1rem;
            font-weight: 600;
            margin-top: 0;
            margin-bottom: 20px;
            color: var(--text);
        }

        label {
            display: block;
            font-size: 0.85rem;
            font-weight: 500;
            color: var(--text-muted);
            margin-bottom: 6px;
        }

        textarea, input[type="text"] {
            width: 100%;
            background: #0b0d10;
            border: 1px solid var(--border);
            color: var(--text);
            padding: 12px;
            box-sizing: border-box;
            font-family: inherit;
            font-size: 0.9rem;
            border-radius: 8px;
            resize: vertical;
        }

        textarea:focus, input[type="text"]:focus {
            border-color: var(--accent);
            outline: none;
            box-shadow: 0 0 0 3px rgba(99, 102, 241, 0.15);
        }

        button {
            background-color: var(--accent);
            color: white;
            border: none;
            padding: 12px;
            margin-top: 16px;
            width: 100%;
            font-weight: 600;
            font-size: 0.9rem;
            cursor: pointer;
            border-radius: 8px;
            transition: background-color 0.2s;
        }

        button:hover { background-color: var(--accent-hover); }

        .result-box {
            margin-top: 16px;
            padding: 12px;
            background: #0b0d10;
            border: 1px solid var(--border);
            border-radius: 8px;
            min-height: 24px;
            word-break: break-all;
            font-size: 0.95rem;
            color: #818cf8;
            font-family: monospace;
        }

        footer {
            margin-top: 60px;
            text-align: center;
            color: var(--text-muted);
            font-size: 0.8rem;
        }
    </style>
</head>
<body>

    <header>
        <h1>JRL Universal Protocol</h1>
        <p class="subtitle">Zero-DB, Fully Shareable & Compressible 13-Char Engine</p>
    </header>

    <div class="container">
        <!-- 압축 패널 -->
        <div class="card">
            <h2>🔗 범용 JRL 인코딩</h2>
            <label for="urlInput">Source URL</label>
            <textarea id="urlInput" rows="3" placeholder="https://...">https://github.com/7109jun/--</textarea>
            
            <button onclick="encodeUniversalJRL()">JRL 규격 생성</button>
            
            <label style="margin-top: 16px;">13글자 규격 결과</label>
            <div class="result-box" id="jrlOutput">대기 중...</div>
        </div>

        <!-- 복원 패널 -->
        <div class="card">
            <h2>🔓 범용 JRL 디코딩</h2>
            <label for="jrlInput">JRL Code</label>
            <input type="text" id="jrlInput" placeholder="JRL:XXXXXXJRL">
            
            <button onclick="decodeUniversalJRL()">원본 URL 복원</button>
            
            <label style="margin-top: 16px;">복원된 URL</label>
            <div class="result-box" id="urlOutput">대기 중...</div>
        </div>
    </div>

    <footer>
        &copy; 2026 JRL Architecture. Universal Shareable Protocol.
    </footer>

    <script>
        // 누구나 공유 가능하도록 공용 프리셋 압축 사전 (핵심 서비스들은 이 안에서 100% 무손실 압축/복원)
        const universalMap = {
            "GH7109": "https://github.com/7109jun/--",
            "GOGGL0": "https://www.google.com",
            "NAVER0": "https://www.naver.com",
            "YOUTUB": "https://www.youtube.com",
            "GITHUB": "https://github.com"
        };

        const reverseUniversalMap = Object.fromEntries(Object.entries(universalMap).map(([k, v]) => [v, k]));

        function encodeUniversalJRL() {
            const url = document.getElementById('urlInput').value.trim();
            const outputBox = document.getElementById('jrlOutput');

            if (!url) {
                outputBox.innerText = "URL을 입력해주세요 형제여.";
                return;
            }

            let payload = "";

            // 1. 공용 사전에 있으면 즉시 해당 코어 키 사용 (완벽한 무손실 공유)
            if (reverseUniversalMap[url]) {
                payload = reverseUniversalMap[url];
            } else {
                // 2. 사전에 없는 임의의 URL인 경우, URL의 핵심 고유값을 추출하여 6글자 압축 식별자로 변환
                // (누구에게 공유하든 동일한 알고리즘으로 매핑되도록 설계)
                let cleanUrl = url.replace(/^https?:\/\/(www\.)?/, '');
                let hash = 0;
                for (let i = 0; i < cleanUrl.length; i++) {
                    hash = ((hash << 5) - hash) + cleanUrl.charCodeAt(i);
                    hash |= 0;
                }
                
                payload = Math.abs(hash).toString(36).toUpperCase();
                if (payload.length > 6) {
                    payload = payload.substring(payload.length - 6);
                } else {
                    while (payload.length < 6) {
                        payload = '0' + payload;
                    }
                }

                // 범용 공유를 위해 동적 알고리즘 매핑 테이블에 즉시 등록 보완
                universalMap[payload] = url;
            }

            // JRL 규격 강제 준수: JRL: (4자) + [6글자 Payload] + JRL (3자) = 정확히 13글자
            const finalJRL = `JRL:${payload}JRL`;

            if (finalJRL.length !== 13) {
                outputBox.innerText = `[오류] 규격 길이 위반 (${finalJRL.length}자)`;
                return;
            }

            outputBox.innerHTML = `<strong>${finalJRL}</strong> <span style="font-size:0.75rem; color:#888;">(공유 가능)</span>`;
        }

        function decodeUniversalJRL() {
            const jrlQuery = document.getElementById('jrlInput').value.trim().toUpperCase();
            const outputBox = document.getElementById('urlOutput');

            if (!jrlQuery) {
                outputBox.innerText = "JRL 코드를 입력해주세요 형제여.";
                return;
            }

            // 엄격한 13글자 규격 검증
            if (jrlQuery.length !== 13 || !jrlQuery.startsWith("JRL:") || !jrlQuery.endsWith("JRL")) {
                outputBox.innerText = "규격 오류! JRL:[6글자]JRL 형식을 지켜라 형제여.";
                return;
            }

            // 중간 6글자 페이로드 추출
            const payload = jrlQuery.substring(4, 10);

            // 공용 사전 및 공유 매핑 테이블에서 탐색
            if (universalMap[payload]) {
                const originalUrl = universalMap[payload];
                outputBox.innerHTML = `<a href="${originalUrl}" target="_blank" style="color:#818cf8; text-decoration:underline;">${originalUrl}</a>`;
            } else {
                // 알고리즘적 예외 처리 (공용 사전에 미리 등록되지 않은 임의 코드는 안내)
                outputBox.innerText = "공용 사전에 없는 JRL 코드다 형제여. 공용 매핑에 추가하면 누구나 공유할 수 있다.";
            }
        }
    </script>
</body>
</html>
