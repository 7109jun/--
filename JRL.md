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
    <title>JRL (JUN RRRR LOVE) - 최적화된 양방향 변환기</title>
    <style>
        :root {
            --bg-color: #050505;
            --panel-bg: #0f0f0f;
            --primary: #ff003c;
            --secondary: #00f0ff;
            --text: #f0f0f0;
        }

        body {
            background-color: var(--bg-color);
            color: var(--text);
            font-family: 'Courier New', Courier, monospace;
            margin: 0;
            padding: 40px;
            display: flex;
            flex-direction: column;
            align-items: center;
        }

        header {
            text-align: center;
            margin-bottom: 40px;
        }

        h1 {
            color: var(--primary);
            text-shadow: 0 0 10px rgba(255, 0, 60, 0.5);
            font-size: 2.2rem;
            margin: 0;
        }

        p.subtitle {
            color: #777;
            font-size: 0.9rem;
            margin-top: 10px;
        }

        .container {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 30px;
            width: 100%;
            max-width: 1000px;
        }

        .panel {
            background-color: var(--panel-bg);
            border: 2px solid var(--primary);
            border-radius: 8px;
            padding: 25px;
            box-shadow: 0 0 20px rgba(255, 0, 60, 0.15);
            position: relative;
        }

        .panel::before {
            content: '';
            position: absolute;
            top: 0; left: 0; width: 100%; height: 3px;
            background: linear-gradient(90deg, var(--primary), var(--secondary));
        }

        h2 {
            color: var(--secondary);
            font-size: 1.2rem;
            margin-top: 0;
            border-bottom: 1px dashed #333;
            padding-bottom: 10px;
        }

        label {
            display: block;
            margin-top: 15px;
            font-size: 0.85rem;
            color: #aaa;
        }

        textarea, input[type="text"] {
            width: 100%;
            background: #000;
            border: 1px solid #444;
            color: var(--text);
            padding: 12px;
            margin-top: 5px;
            box-sizing: border-box;
            font-family: monospace;
            border-radius: 4px;
            resize: vertical;
        }

        textarea:focus, input[type="text"]:focus {
            border-color: var(--secondary);
            outline: none;
            box-shadow: 0 0 8px rgba(0, 240, 255, 0.3);
        }

        button {
            background: linear-gradient(135deg, var(--primary), #990022);
            color: white;
            border: none;
            padding: 12px 20px;
            margin-top: 20px;
            width: 100%;
            font-weight: bold;
            font-family: monospace;
            cursor: pointer;
            border-radius: 4px;
            transition: all 0.2s ease;
        }

        button:hover {
            background: linear-gradient(135deg, #ff1a48, var(--primary));
            box-shadow: 0 0 12px rgba(255, 0, 60, 0.7);
        }

        .result-box {
            margin-top: 20px;
            padding: 15px;
            background: #000;
            border: 1px dashed var(--secondary);
            min-height: 50px;
            word-break: break-all;
            font-size: 1.1rem;
            color: var(--secondary);
        }

        .error {
            color: var(--primary) !important;
            border-color: var(--primary) !important;
        }

        .spec-info {
            margin-top: 40px;
            font-size: 0.75rem;
            color: #555;
            text-align: center;
            border-top: 1px solid #111;
            padding-top: 15px;
            width: 100%;
            max-width: 1000px;
        }
    </style>
</head>
<body>

    <header>
        <h1>🔪 JRL OPTIMIZED ENGINE</h1>
        <p class="subtitle">규격: [JRL:][고유ID][JRL] (전체 글자수 13글자 제한 완벽 준수)</p>
    </header>

    <div class="container">
        <!-- 압축 패널 -->
        <div class="panel">
            <h2>[단계 1] 사지절단 압축 (URL ➔ JRL)</h2>
            <label for="urlInput">대상 URL 입력</label>
            <textarea id="urlInput" rows="3" placeholder="https://example.com/very/long/path">https://very-long-annoying-domain.com/path/to/page?id=12345&token=abcdef</textarea>
            
            <button onclick="compressToJRL()">JRL로 봉인하기</button>
            
            <label>생성된 JRL 규격</label>
            <div class="result-box" id="jrlOutput">대기 중...</div>
        </div>

        <!-- 복원 패널 -->
        <div class="panel">
            <h2>[단계 2] 부활 의식 (JRL ➔ URL)</h2>
            <label for="jrlInput">JRL 입력 (예: JRL:A1B2JRL)</label>
            <input type="text" id="jrlInput" placeholder="JRL:고유IDJRL 형식 입력">
            
            <button onclick="restoreFromJRL()">원래 URL로 부활시키기</button>
            
            <label>복원된 원본 URL</label>
            <div class="result-box" id="urlOutput">대기 중...</div>
        </div>
    </div>

    <div class="spec-info">
        JRL SYSTEM v2.0 — "JUN RRRR LOVE" ARCHITECTURE.
    </div>

    <script>
        // 로컬 데이터베이스 및 카운터 불러오기 (충돌 방지 최적화)
        let db = JSON.parse(localStorage.getItem('jrl_opt_db') || '{}');
        let counter = parseInt(localStorage.getItem('jrl_opt_counter') || '1000', 10);

        // 이미 등록된 URL인지 확인하는 역방향 룩업맵 생성
        let reverseMap = {};
        for (let key in db) {
            reverseMap[db[key]] = key;
        }

        function compressToJRL() {
            const url = document.getElementById('urlInput').value.trim();
            const outputBox = document.getElementById('jrlOutput');

            if (!url) {
                outputBox.className = "result-box error";
                outputBox.innerText = "[오류] URL이 비어 있습니다.";
                return;
            }

            // 이미 압축된 적 있는 URL이라면 기존 JRL을 재활용 (중복 생성 방지 최적화)
            if (reverseMap[url]) {
                const existingJRL = reverseMap[url];
                outputBox.className = "result-box";
                outputBox.innerHTML = `<strong>${existingJRL}</strong> <span style="font-size:0.8rem; color:#888;">(캐시 재사용, 총 ${existingJRL.length}글자)</span>`;
                return;
            }

            // 고유 ID 생성 (카운터를 36진수로 변환해 짧고 고유하게 유지)
            let uniqueId = counter.toString(36).toUpperCase();
            counter++;
            localStorage.setItem('jrl_opt_counter', counter);

            // 전체 JRL 구조 조합: "JRL:" (4자) + uniqueId (최대 6자) + "JRL" (3자) = 최대 13자
            if (uniqueId.length > 6) {
                uniqueId = uniqueId.slice(-6); // 넘치면 뒤 6자리만 사용
            }
            const finalJRL = `JRL:${uniqueId}JRL`;

            // 규격 최종 검수
            if (finalJRL.length > 13) {
                outputBox.className = "result-box error";
                outputBox.innerText = `[치명적 오류] 규격 위반! (${finalJRL.length}글자 > 13글자 제한)`;
                return;
            }

            // DB 저장
            db[finalJRL] = url;
            reverseMap[url] = finalJRL;
            localStorage.setItem('jrl_opt_db', JSON.stringify(db));

            // 출력
            outputBox.className = "result-box";
            outputBox.innerHTML = `<strong>${finalJRL}</strong> <span style="font-size:0.8rem; color:#888;">(총 ${finalJRL.length}글자)</span>`;
        }

        function restoreFromJRL() {
            let jrlQuery = document.getElementById('jrlInput').value.trim().toUpperCase();
            const outputBox = document.getElementById('urlOutput');

            if (!jrlQuery) {
                outputBox.className = "result-box error";
                outputBox.innerText = "[오류] JRL을 입력해주세요.";
                return;
            }

            if (jrlQuery.length > 13) {
                outputBox.className = "result-box error";
                outputBox.innerText = "[오류] 입력된 JRL이 13글자 규격 제한을 초과했습니다.";
                return;
            }

            if (db[jrlQuery]) {
                const originalUrl = db[jrlQuery];
                outputBox.className = "result-box";
                outputBox.innerHTML = `<a href="${originalUrl}" target="_blank" style="color:var(--secondary); text-decoration:underline;">${originalUrl}</a>`;
            } else {
                outputBox.className = "result-box error";
                outputBox.innerText = "[오류] 데이터베이스에 존재하지 않는 JRL 영혼입니다.";
            }
        }
    </script>
</body>
</html>```
