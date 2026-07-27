# JRL이라는것을 제작했습니다
## JRL은 URL을 압축시켜주는것입니다.
> URL을 엄청나게 압축시켜버립니다.
>
> 매우 엄청나!
>
> 압축률은 그래도 높습네다.
코드:
```<!DOCTYPE html>
<html lang="ko">
<head>
    <meta charset="UTF-8">
    <title>JRL - Platform Tag Protocol</title>
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
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
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
            letter-spacing: -0.025em;
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
            .container {
                grid-template-columns: 1fr;
            }
        }

        .card {
            background-color: var(--card-bg);
            border: 1px solid var(--border);
            border-radius: 12px;
            padding: 24px;
            box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1), 0 2px 4px -1px rgba(0, 0, 0, 0.06);
        }

        h2 {
            font-size: 1.1rem;
            font-weight: 600;
            margin-top: 0;
            margin-bottom: 20px;
            color: var(--text);
            display: flex;
            align-items: center;
            gap: 8px;
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
            transition: border-color 0.2s, box-shadow 0.2s;
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

        button:hover {
            background-color: var(--accent-hover);
        }

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
        <h1>JRL Protocol</h1>
        <p class="subtitle">Platform Tagged URL Compression Engine (JRL:[PLATFORM]JRL)</p>
    </header>

    <div class="container">
        <!-- 압축 패널 -->
        <div class="card">
            <h2>🔗 Generate JRL</h2>
            <label for="urlInput">Source URL</label>
            <textarea id="urlInput" rows="3" placeholder="https://github.com/...">https://github.com/7109jun/--/blob/main/JRL.md</textarea>
            
            <button onclick="generatePlatformJRL()">Generate JRL Tag</button>
            
            <label style="margin-top: 16px;">Result</label>
            <div class="result-box" id="jrlOutput">대기 중...</div>
        </div>

        <!-- 복원 패널 -->
        <div class="card">
            <h2>🔓 Restore JRL</h2>
            <label for="jrlInput">JRL Code</label>
            <input type="text" id="jrlInput" placeholder="JRL:GITHUBJRL">
            
            <button onclick="restorePlatformJRL()">Restore URL</button>
            
            <label style="margin-top: 16px;">Result</label>
            <div class="result-box" id="urlOutput">대기 중...</div>
        </div>
    </div>

    <footer>
        &copy; 2026 JRL Architecture. "JUN RRRR LOVE"
    </footer>

    <script>
        // 로컬 DB 연동
        const db = JSON.parse(localStorage.getItem('jrl_platform_db') || '{}');

        function generatePlatformJRL() {
            const urlInput = document.getElementById('urlInput').value.trim();
            const outputBox = id => document.getElementById(id);

            if (!urlInput) {
                outputBox('jrlOutput').innerText = "URL을 입력해주세요.";
                return;
            }

            let platformName = "WEB"; // 기본값

            try {
                // URL 파싱을 통해 도메인 추출
                let urlObj = new URL(urlInput);
                let hostname = urlObj.hostname.toLowerCase(); // 예: github.com, youtube.com

                // 서브도메인 및 확장자(.com, .co.kr 등) 제거하고 순수 플랫폼 명칭 추출
                let parts = hostname.replace('www.', '').split('.');
                if (parts.length > 0) {
                    platformName = parts[0].toUpperCase(); // 예: github, naver -> GITHUB, NAVER
                }
            } catch (e) {
                platformName = "UNKNOWN";
            }

            // 규격 완성: JRL:[PLATFORM]JRL (최대 13글자 제한 방어를 위해 플랫폼명은 최대 6글자로 절단)
            if (platformName.length > 6) {
                platformName = platformName.substring(0, 6);
            }

            const finalJRL = `JRL:${platformName}JRL`;

            // DB에 매핑 저장 (동일 플랫폼이라도 최신 입력된 URL로 갱신)
            db[finalJRL] = urlInput;
            localStorage.setItem('jrl_platform_db', JSON.stringify(db));

            outputBox('jrlOutput').innerHTML = `<strong>${finalJRL}</strong>`;
        }

        function restorePlatformJRL() {
            const jrlInput = document.getElementById('jrlInput').value.trim().toUpperCase();
            const outputBox = document.getElementById('urlOutput');

            if (!jrlInput) {
                outputBox.innerText = "JRL 코드를 입력해주세요.";
                return;
            }

            if (db[jrlInput]) {
                const originalUrl = db[jrlInput];
                outputBox.innerHTML = `<a href="${originalUrl}" target="_blank" style="color:#818cf8; text-decoration:underline;">${originalUrl}</a>`;
            } else {
                outputBox.innerText = "데이터베이스에 존재하지 않는 JRL 태그입니다.";
            }
        }
    </script>
</body>
</html>
