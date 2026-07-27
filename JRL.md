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
    <title>JRL - Pure Dismemberment Protocol</title>
    <style>
        :root {
            --bg: #0f1115;
            --card-bg: #161920;
            --accent: #ff3333;
            --accent-hover: #cc0000;
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
            color: var(--accent);
            letter-spacing: -0.025em;
        }

        p.subtitle {
            color: var(--text-muted);
            font-size: 0.95rem;
            margin-top: 8px;
        }

        .container {
            width: 100%;
            max-width: 500px;
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

        textarea {
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

        textarea:focus {
            border-color: var(--accent);
            outline: none;
            box-shadow: 0 0 0 3px rgba(255, 51, 51, 0.15);
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
            font-size: 1.1rem;
            color: #ff6666;
            font-family: monospace;
            text-align: center;
            font-weight: bold;
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
        <p class="subtitle">Pure URL Dismemberment Engine (Max 13 Chars)</p>
    </header>

    <div class="container">
        <div class="card">
            <h2>🔪 사지분해 압축기</h2>
            <label for="urlInput">Source URL</label>
            <textarea id="urlInput" rows="3" placeholder="https://...">https://very-long-annoying-domain.com/path/to/page?id=12345&token=abcdef</textarea>
            
            <button onclick="dismemberURL()">사지분해 실행</button>
            
            <label style="margin-top: 16px;">JRL 결과 (13글자 엄수)</label>
            <div class="result-box" id="jrlOutput">JRL:--------JRL</div>
        </div>
    </div>

    <footer>
        &copy; 2026 JRL Architecture. No Database, Just Gore.
    </footer>

    <script>
        function dismemberURL() {
            const url = document.getElementById('urlInput').value.trim();
            const outputBox = document.getElementById('jrlOutput');

            if (!url) {
                outputBox.innerText = "URL이 비어있다 형제여.";
                return;
            }

            // 1. 프로토콜 및 온갖 특수문자/슬래시를 무자비하게 사지분해(제거)
            let rawBones = url
                .replace(/^https?:\/\/(www\.)?/, '')
                .replace(/[\/\?&=\-\._%]/g, '');

            // 2. 전체 규격 'JRL:'(4자) + 'JRL'(3자) = 7글자를 제외한 
            // 남은 가용 공간은 최대 6글자 (13 - 7 = 6)
            let bodyPart = rawBones.substring(0, 6).toUpperCase();

            // 만약 찌꺼기가 모자라면 X로 채움
            while (bodyPart.length < 6) {
                bodyPart += 'X';
            }

            // 3. 최종 JRL 규격 완성
            const finalJRL = `JRL:${bodyPart}JRL`;

            outputBox.innerText = finalJRL;
        }
    </script>
</body>
</html>
