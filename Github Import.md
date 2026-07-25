# Github에서 파일들을 찾기가 너무 귀찮아서
> 그냥 Github 저장소 주소를 넣으면.
>바로바로
> 가져와서
> 파일별로 나눠주고
> 복사가능하게 해주는
> 미리보기 코드 보기 기능도 있는!
## HTML을 제작했습니다.
# 코드:
```<!DOCTYPE html>
<html lang="ko">
<head>
    <meta charset="UTF-8">
    <title>GitHub 저장소 전체 파일 및 내용 조회기</title>
    <!-- Marked.js (마크다운 렌더링용) -->
    <script src="https://cdn.jsdelivr.net/npm/marked/marked.min.js"></script>
    <style>
        :root {
            --bg-color: #0d1117;
            --container-bg: #161b22;
            --border-color: #30363d;
            --text-color: #c9d1d9;
            --accent-color: #58a6ff;
            --button-bg: #238636;
            --button-hover: #2ea043;
            --tab-bg: #21262d;
            --tab-active: #30363d;
            --code-bg: #0d1117;
            --copy-bg: #21262d;
            --copy-hover: #30363d;
        }

        * { box-sizing: border-box; margin: 0; padding: 0; }
        
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Helvetica, Arial, sans-serif;
            background-color: var(--bg-color);
            color: var(--text-color);
            padding: 40px 20px;
            display: flex;
            justify-content: center;
        }

        .main-wrapper {
            width: 100%;
            max-width: 1100px;
        }

        .container {
            background-color: var(--container-bg);
            border: 1px solid var(--border-color);
            border-radius: 12px;
            padding: 30px;
            box-shadow: 0 8px 24px rgba(0, 0, 0, 0.4);
        }

        h2 {
            font-size: 1.8rem;
            margin-bottom: 10px;
            color: #ffffff;
            border-bottom: 1px solid var(--border-color);
            padding-bottom: 15px;
        }

        p {
            color: #8b949e;
            margin-bottom: 20px;
            font-size: 0.95rem;
        }

        code {
            background-color: rgba(110, 118, 129, 0.4);
            padding: 2px 6px;
            border-radius: 4px;
            font-family: ui-monospace, SFMono-Regular, SF Mono, Menlo, Consolas, monospace;
        }

        .input-group {
            display: flex;
            gap: 12px;
            margin-bottom: 20px;
        }

        input[type="text"] {
            flex: 1;
            background-color: var(--bg-color);
            border: 1px solid var(--border-color);
            border-radius: 6px;
            padding: 12px 16px;
            color: var(--text-color);
            font-size: 1rem;
            outline: none;
            transition: border-color 0.2s;
        }

        input[type="text"]:focus {
            border-color: var(--accent-color);
            box-shadow: 0 0 0 3px rgba(88, 166, 255, 0.3);
        }

        button.fetch-btn {
            background-color: var(--button-bg);
            color: #ffffff;
            border: none;
            border-radius: 6px;
            padding: 0 24px;
            font-size: 1rem;
            font-weight: 600;
            cursor: pointer;
            transition: background-color 0.2s, transform 0.1s;
        }

        button.fetch-btn:hover { background-color: var(--button-hover); }
        button.fetch-btn:active { transform: scale(0.98); }

        #status {
            font-weight: 500;
            color: var(--accent-color);
            margin-bottom: 15px;
            min-height: 24px;
            font-size: 0.95rem;
        }

        #result {
            background-color: var(--bg-color);
            border: 1px solid var(--border-color);
            border-radius: 8px;
            padding: 20px;
            max-height: 700px;
            overflow-y: auto;
            font-family: ui-monospace, SFMono-Regular, SF Mono, Menlo, Consolas, monospace;
            font-size: 0.9rem;
            line-height: 1.6;
        }

        .file-block {
            margin-bottom: 25px;
            border: 1px solid var(--border-color);
            border-radius: 6px;
            background-color: var(--container-bg);
            overflow: hidden;
        }

        .file-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            background-color: var(--tab-bg);
            padding: 10px 15px;
            border-bottom: 1px solid var(--border-color);
            flex-wrap: wrap;
            gap: 10px;
        }

        .file-title {
            font-weight: 700;
            color: var(--accent-color);
            display: flex;
            align-items: center;
            gap: 8px;
        }

        .header-actions {
            display: flex;
            align-items: center;
            gap: 8px;
        }

        .tab-buttons {
            display: flex;
            gap: 5px;
        }

        .action-btn {
            background: var(--copy-bg);
            border: 1px solid var(--border-color);
            color: var(--text-color);
            padding: 4px 10px;
            font-size: 0.8rem;
            border-radius: 4px;
            cursor: pointer;
            transition: background-color 0.2s;
        }

        .action-btn:hover {
            background-color: var(--copy-hover);
        }

        .tab-btn.active {
            background-color: var(--accent-color);
            color: #0d1117;
            border-color: var(--accent-color);
            font-weight: bold;
        }

        .view-panel {
            padding: 15px;
            white-space: pre-wrap;
            word-break: break-all;
            background-color: var(--code-bg);
        }

        /* 마크다운 렌더링 영역 전용 스타일링 */
        .markdown-body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Helvetica, Arial, sans-serif;
            color: var(--text-color);
            line-height: 1.6;
        }
        .markdown-body h1, .markdown-body h2, .markdown-body h3 { border-bottom: 1px solid var(--border-color); padding-bottom: .3em; margin-top: 24px; margin-bottom: 16px; }
        .markdown-body pre { background-color: var(--tab-bg); padding: 12px; border-radius: 6px; overflow-x: auto; }
        .markdown-body code { background-color: rgba(110, 118, 129, 0.4); padding: 2px 4px; border-radius: 4px; }
        .markdown-body blockquote { border-left: 0.25em solid var(--border-color); padding: 0 1em; color: #8b949e; }

        .folder-item {
            color: #79c0ff;
            font-weight: bold;
            padding: 4px 0;
            margin-bottom: 10px;
        }

        .error { color: #f85149 !important; font-weight: bold; }

        /* 스크롤바 디자인 */
        ::-webkit-scrollbar { width: 8px; height: 8px; }
        ::-webkit-scrollbar-track { background: var(--bg-color); }
        ::-webkit-scrollbar-thumb { background: var(--border-color); border-radius: 4px; }
        ::-webkit-scrollbar-thumb:hover { background: #484f58; }
    </style>
</head>
<body>

    <div class="main-wrapper">
        <div class="container">
            <h2>GitHub 저장소 전체 파일 및 내용 조회기</h2>
            <p>예시: <code>https://github.com/octocat/Hello-World</code></p>
            
            <div class="input-group">
                <input type="text" id="repoUrl" placeholder="GitHub 저장소 URL을 입력하세요">
                <button class="fetch-btn" onclick="fetchAllFilesAndContent()">전체 가져오기</button>
            </div>

            <div id="status"></div>
            <div id="result">결과가 여기에 표시됩니다.</div>
        </div>
    </div>

    <script>
        async function fetchAllFilesAndContent() {
            const urlInput = document.getElementById('repoUrl').value.trim();
            const resultDiv = document.getElementById('result');
            const statusDiv = document.getElementById('status');
            
            resultDiv.innerHTML = '';
            statusDiv.textContent = '저장소 구조를 분석하는 중...';

            try {
                const match = urlInput.match(/github\.com\/([^\/]+)\/([^\/]+)/);
                if (!match) throw new Error('유효한 GitHub 저장소 URL이 아닙니다.');
                
                const owner = match[1];
                let repo = match[2].replace(/\.git$/, '');

                const repoInfoRes = await fetch(`https://api.github.com/repos/${owner}/${repo}`);
                if (!repoInfoRes.ok) throw new Error('저장소를 찾을 수 없거나 접근할 수 없습니다.');
                const repoData = await repoInfoRes.json();
                const defaultBranch = repoData.default_branch || 'main';

                statusDiv.textContent = '모든 파일 목록(Tree)을 불러오는 중...';
                const treeUrl = `https://api.github.com/repos/${owner}/${repo}/git/trees/${defaultBranch}?recursive=1`;
                const treeRes = await fetch(treeUrl);
                if (!treeRes.ok) throw new Error('파일 트리를 가져오는데 실패했습니다.');
                
                const treeData = await treeRes.json();
                treeData.tree.sort((a, b) => a.path.localeCompare(b.path));

                const files = treeData.tree.filter(item => item.type === 'blob');
                resultDiv.innerHTML = `<div style="margin-bottom:15px; color:#8b949e;">[저장소: ${owner}/${repo}] (총 폴더/파일 수: ${treeData.tree.length}개, 실제 파일 수: ${files.length}개)</div>`;

                let count = 0;
                for (const item of treeData.tree) {
                    const parts = item.path.split('/');
                    const depth = parts.length - 1;
                    const fileName = parts[parts.length - 1];
                    const indentPx = depth * 20;

                    if (item.type === 'tree') {
                        const folderDiv = document.createElement('div');
                        folderDiv.className = 'folder-item';
                        folderDiv.style.marginLeft = `${indentPx}px`;
                        folderDiv.innerHTML = `📁 ${fileName}`;
                        resultDiv.appendChild(folderDiv);
                    } else {
                        count++;
                        statusDiv.textContent = `파일 읽는 중... (${count}/${files.length}) : ${item.path}`;

                        let fileContent = '';
                        try {
                            const fileRes = await fetch(item.url);
                            if (fileRes.ok) {
                                const fileJson = await fileRes.json();
                                if (fileJson.encoding === 'base64' && fileJson.content) {
                                    const binString = atob(fileJson.content.replace(/\s/g, ''));
                                    const bytes = Uint8Array.from(binString, (m) => m.codePointAt(0));
                                    fileContent = new TextDecoder().decode(bytes);
                                }
                            }
                        } catch (e) {
                            fileContent = '(파일 내용을 읽어오지 못했습니다)';
                        }

                        // 파일 박스 생성 컨테이너
                        const fileBlock = document.createElement('div');
                        fileBlock.className = 'file-block';
                        fileBlock.style.marginLeft = `${indentPx}px`;

                        // 상단 바 (파일 이름 + 복사 버튼 + 탭 전환 버튼)
                        const isMarkdown = fileName.toLowerCase().endsWith('.md');
                        const fileHeader = document.createElement('div');
                        fileHeader.className = 'file-header';
                        
                        let tabButtonsHtml = `<div class="tab-buttons"><button class="action-btn tab-btn active" onclick="switchTab(this, 'code')">코드 보기</button>`;
                        if (isMarkdown) {
                            tabButtonsHtml += `<button class="action-btn tab-btn" onclick="switchTab(this, 'preview')">미리보기</button>`;
                        }
                        tabButtonsHtml += `</div>`;

                        fileHeader.innerHTML = `
                            <div class="file-title">📄 ${item.path}</div>
                            <div class="header-actions">
                                <button class="action-btn copy-btn" onclick="copyContent(this)">📋 복사</button>
                                ${tabButtonsHtml}
                            </div>
                        `;
                        fileBlock.appendChild(fileHeader);

                        // 코드 보기 패널 (# 포함 원본 텍스트)
                        const codePanel = document.createElement('div');
                        codePanel.className = 'view-panel code-view';
                        codePanel.textContent = fileContent; 
                        fileBlock.appendChild(codePanel);

                        // 마크다운 미리보기 패널 (마크다운 파일인 경우만 생성)
                        if (isMarkdown) {
                            const previewPanel = document.createElement('div');
                            previewPanel.className = 'view-panel markdown-body preview-view';
                            previewPanel.style.display = 'none';
                            previewPanel.innerHTML = marked.parse(fileContent);
                            fileBlock.appendChild(previewPanel);
                        }

                        // 복사 기능에서 참조할 수 있도록 원본 텍스트를 데이터 속성으로 저장
                        fileBlock.dataset.rawContent = fileContent;

                        resultDiv.appendChild(fileBlock);
                    }
                }

                statusDiv.textContent = `완료! 총 ${files.length}개의 파일과 내용을 모두 가져왔습니다.`;

            } catch (error) {
                statusDiv.textContent = '';
                resultDiv.className = 'error';
                resultDiv.textContent = `에러 발생: ${error.message}`;
            }
        }

        // 탭 전환 함수 (코드 보기 ↔ 미리보기)
        function switchTab(btn, mode) {
            const fileBlock = btn.closest('.file-block');
            const codeView = fileBlock.querySelector('.code-view');
            const previewView = fileBlock.querySelector('.preview-view');
            
            // 버튼 활성화 상태 변경
            fileBlock.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
            btn.classList.add('active');

            if (mode === 'code') {
                codeView.style.display = 'block';
                if (previewView) previewView.style.display = 'none';
            } else if (mode === 'preview') {
                codeView.style.display = 'none';
                if (previewView) previewView.style.display = 'block';
            }
        }

        // 복사 기능 함수
        async function copyContent(btn) {
            const fileBlock = btn.closest('.file-block');
            const content = fileBlock.dataset.rawContent || '';

            try {
                await navigator.clipboard.writeText(content);
                const originalText = btn.textContent;
                btn.textContent = '✅ 복사 완료!';
                btn.style.backgroundColor = '#238636';
                btn.style.color = '#ffffff';

                setTimeout(() => {
                    btn.textContent = originalText;
                    btn.style.backgroundColor = '';
                    btn.style.color = '';
                }, 1500);
            } catch (err) {
                alert('클립보드 복사에 실패했습니다.');
            }
        }
    </script>

</body>
</html>
