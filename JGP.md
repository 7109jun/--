<img width="1214" height="1096" alt="스크린샷 2026-08-12 125028" src="https://github.com/user-attachments/assets/0c7d091d-926a-469d-9393-16c1c1a5b196" />
게임 파일 폴더 뭐시깽이.
```<!DOCTYPE html>
<html lang="ko">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>JGP PACKAGER — J Game Package</title>
<style>
:root{
  --bg0:#0c0e12; --bg1:#12151b; --bg2:#171b23; --bg3:#080a0d;
  --line:#232936; --line2:#333c4e;
  --tx:#d8dde8; --dim:#8a93a6; --faint:#5b6375;
  --acc:#f7a83d; --acc2:#ffd28a; --accd:#7a5010;
  --ok:#4ad08a; --err:#ff6262; --warn:#f7c948;
  --s0:#ff7364; --s1:#53b6ff; --s2:#45d48a; --s3:#e8c15a; --s4:#a08cff;
  --disp:Bahnschrift,"Avenir Next Condensed","Arial Narrow","Segoe UI",system-ui,sans-serif;
  --body:"Segoe UI",system-ui,-apple-system,"Malgun Gothic",sans-serif;
  --mono:ui-monospace,"Cascadia Code","JetBrains Mono",Consolas,"SF Mono",Menlo,monospace;
}
*{margin:0;padding:0;box-sizing:border-box}
html,body{height:100%}
body{
  font-family:var(--body); color:var(--tx); background:
    radial-gradient(900px 480px at 88% -10%, rgba(247,168,61,.07), transparent 60%),
    radial-gradient(760px 420px at -8% 112%, rgba(83,182,255,.05), transparent 60%),
    var(--bg0);
  display:flex; flex-direction:column; min-height:100vh;
}
body::before{content:""; position:fixed; inset:0; pointer-events:none; z-index:0;
  background:
    repeating-linear-gradient(0deg, transparent 0 31px, rgba(255,255,255,.020) 31px 32px),
    repeating-linear-gradient(90deg, transparent 0 31px, rgba(255,255,255,.020) 31px 32px);
}
body::after{content:""; position:fixed; inset:0; pointer-events:none; z-index:0;
  background:repeating-linear-gradient(0deg, rgba(255,255,255,.010) 0 1px, transparent 1px 3px);
}
::selection{background:#f7a83d55}
::-webkit-scrollbar{width:10px;height:10px}
::-webkit-scrollbar-thumb{background:#262c3a;border-radius:5px;border:2px solid var(--bg0)}
::-webkit-scrollbar-track{background:transparent}
button{font-family:inherit}
:focus-visible{outline:1px solid var(--acc2);outline-offset:1px}

/* ── header ─────────────────────────────── */
header{position:relative;z-index:2;display:flex;align-items:center;gap:18px;
  padding:18px 30px 16px;border-bottom:1px solid var(--line);
  background:linear-gradient(180deg,#131720,#0d0f14)}
header::after{content:"";position:absolute;left:0;right:0;top:0;height:2px;
  background:linear-gradient(90deg,var(--s0),var(--s1),var(--s2),var(--s3),var(--s4),var(--acc));opacity:.75}
.logo{display:grid;grid-template-columns:1fr 1fr;grid-template-rows:repeat(3,11px);
  grid-template-areas:"e i" "e s" "c d";gap:3px;width:40px;height:44px;flex:none}
.logo i{border-radius:2px}
.logo .le{grid-area:e;background:var(--s0);box-shadow:0 0 10px #ff736444}
.logo .li{grid-area:i;background:var(--s1);box-shadow:0 0 10px #53b6ff44}
.logo .ls{grid-area:s;background:var(--s2);box-shadow:0 0 10px #45d48a44}
.logo .lc{grid-area:c;background:var(--s3);box-shadow:0 0 10px #e8c15a44}
.logo .ld{grid-area:d;background:var(--s4);box-shadow:0 0 10px #a08cff44}
.titles h1{font-family:var(--disp);font-size:27px;font-weight:700;letter-spacing:4px;line-height:1}
.titles p{font-family:var(--mono);font-size:10.5px;letter-spacing:2.2px;color:var(--dim);margin-top:5px}
.titles p b{color:var(--acc);font-weight:600}
.hdr-right{margin-left:auto;display:flex;align-items:center;gap:9px;font-family:var(--mono);font-size:11px;color:var(--dim);letter-spacing:1px}
.led{width:9px;height:9px;border-radius:50%;background:var(--acc);box-shadow:0 0 8px var(--acc)}
.led.busy{animation:blink .5s steps(2) infinite}
.led.ok{background:var(--ok);box-shadow:0 0 8px var(--ok)}
.led.err{background:var(--err);box-shadow:0 0 8px var(--err)}
@keyframes blink{50%{opacity:.25}}

/* ── tabs ───────────────────────────────── */
nav{position:relative;z-index:2;display:flex;gap:2px;padding:0 30px;border-bottom:1px solid var(--line);background:#0e1116}
nav button{position:relative;background:none;border:none;color:var(--dim);cursor:pointer;
  font-family:var(--disp);font-size:13px;letter-spacing:2.5px;padding:13px 22px 12px;transition:color .15s}
nav button:hover{color:var(--tx)}
nav button.active{color:var(--acc2)}
nav button::after{content:"";position:absolute;left:14px;right:14px;bottom:-1px;height:2px;background:var(--acc);
  transform:scaleX(0);transition:transform .22s ease}
nav button.active::after{transform:scaleX(1)}

/* ── layout ─────────────────────────────── */
main{position:relative;z-index:1;flex:1;width:100%;max-width:1280px;margin:0 auto;padding:22px 30px 96px}
.tabpanel{display:none}
.tabpanel.active{display:block;animation:fade .25s ease}
@keyframes fade{from{opacity:0;transform:translateY(6px)}to{opacity:1;transform:none}}

/* ── buttons ────────────────────────────── */
.btn{font-family:var(--mono);font-size:12px;padding:7px 14px;background:#1a1f28;color:var(--tx);
  border:1px solid var(--line2);border-radius:3px;cursor:pointer;transition:all .15s;letter-spacing:.4px}
.btn:hover{border-color:#4d5872;background:#212836;transform:translateY(-1px)}
.btn:active{transform:translateY(0)}
.btn.primary{background:var(--acc);border-color:var(--acc);color:#1a1206;font-weight:700}
.btn.primary:hover{background:#ffbb59;border-color:#ffbb59}
.btn.big{font-family:var(--disp);font-size:14px;letter-spacing:2px;padding:12px 30px}
.btn:disabled{opacity:.4;pointer-events:none}
.btn.mini{padding:2px 8px;font-size:10.5px}

/* ── create tab ─────────────────────────── */
.create-top{display:flex;flex-wrap:wrap;align-items:flex-end;gap:14px;margin-bottom:16px}
.field label{display:block;font-family:var(--mono);font-size:10px;letter-spacing:1.6px;color:var(--faint);margin-bottom:5px}
.field input{font-family:var(--mono);font-size:13px;color:var(--acc2);background:var(--bg3);
  border:1px solid var(--line2);border-radius:3px;padding:8px 12px;width:270px}
.chip{font-family:var(--mono);font-size:11px;color:var(--dim);border:1px solid var(--line);
  background:var(--bg1);border-radius:3px;padding:8px 13px;letter-spacing:.4px}
.chip b{color:var(--tx);font-weight:600}
.hintline{width:100%;font-family:var(--mono);font-size:10.5px;color:var(--faint);letter-spacing:.5px}
.hintline em{color:var(--dim);font-style:normal}

.buckets{display:grid;grid-template-columns:1fr 1fr;gap:14px}
.bucket{--sec:#888;position:relative;background:var(--bg1);border:1px solid var(--line);
  border-left:3px solid var(--sec);border-radius:4px;padding:12px 14px 13px;
  transition:box-shadow .18s,border-color .18s,background .18s}
.bucket:hover{border-color:var(--line2);border-left-color:var(--sec);box-shadow:0 5px 20px rgba(0,0,0,.35)}
.bucket.drag{border-color:var(--sec);background:#151a23;box-shadow:0 0 0 1px var(--sec),0 8px 26px rgba(0,0,0,.45)}
.bucket.exe{grid-column:1/-1}
.b-head{display:flex;justify-content:space-between;align-items:baseline;gap:10px}
.b-name{font-family:var(--disp);font-size:15px;font-weight:700;letter-spacing:2.5px;color:var(--sec)}
.b-path{font-family:var(--mono);font-size:11px;color:var(--dim);margin-left:9px}
.b-meta{font-family:var(--mono);font-size:11px;color:var(--dim);white-space:nowrap}
.b-meta b{color:var(--tx);font-weight:600}
.b-hint{font-family:var(--mono);font-size:10.5px;color:var(--faint);margin:3px 0 8px;letter-spacing:.4px}
.b-list{max-height:148px;overflow:auto;border-top:1px dashed var(--line);margin-top:2px}
.b-list:empty{border-top:none}
.frow{display:flex;gap:10px;align-items:center;padding:3.5px 5px;font-family:var(--mono);font-size:11.5px;color:#b9c1d0;border-radius:2px}
.frow:hover{background:#1b212c}
.fname{flex:1;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.fsize{color:var(--faint);flex:none}
.fx{flex:none;background:none;border:none;color:var(--faint);cursor:pointer;font-size:13px;line-height:1;padding:0 3px}
.fx:hover{color:var(--err)}
.more{color:var(--faint);font-size:10.5px;padding:4px 5px}
.b-actions{display:flex;gap:8px;margin-top:10px}
.exerow{display:flex;align-items:center;gap:12px;font-family:var(--mono);font-size:12.5px;margin:8px 0 2px;flex-wrap:wrap}
.exerow .tgt{color:var(--s0);font-weight:700}
.exerow .arr{color:var(--faint)}
.exerow .src{color:var(--tx)}
.exerow .sz{color:var(--dim)}
.exe-empty{color:var(--faint)}

.actionbar{display:flex;align-items:center;gap:18px;margin-top:18px;flex-wrap:wrap}
.progwrap{flex:1;min-width:240px;max-width:460px;display:none}
.progwrap.on{display:block}
.proglabel{font-family:var(--mono);font-size:10.5px;color:var(--dim);margin-bottom:5px;letter-spacing:.4px}
.prog{height:10px;background:var(--bg3);border:1px solid var(--line);border-radius:3px;overflow:hidden}
.prog i{display:block;height:100%;width:0;background:repeating-linear-gradient(45deg,var(--acc) 0 8px,#b97c22 8px 16px);transition:width .12s linear}
.prog.busy i{animation:slide .8s linear infinite}
@keyframes slide{to{background-position:22.6px 0}}

.result{display:none;margin-top:16px;border:1px solid #6b5322;border-radius:4px;background:linear-gradient(180deg,#171510,#12110d);padding:16px 20px}
.result.on{display:flex;flex-wrap:wrap;gap:14px 34px;align-items:center;animation:fade .3s ease}
.r-item .rl{font-family:var(--mono);font-size:9.5px;letter-spacing:1.8px;color:var(--faint)}
.r-item .rv{font-family:var(--disp);font-size:26px;font-weight:700;letter-spacing:1px;margin-top:3px}
.r-item .rv.acc{color:var(--acc)}
.r-item .rv.sm{font-size:15px;color:var(--tx);font-family:var(--mono);font-weight:400;padding-top:7px}
.r-btns{margin-left:auto;display:flex;gap:10px}

/* ── console ────────────────────────────── */
.console{margin-top:18px;background:var(--bg3);border:1px solid var(--line);border-radius:4px;
  height:168px;overflow:auto;padding:9px 12px;font-family:var(--mono);font-size:11px;line-height:1.65}
.console .ln{white-space:pre-wrap;word-break:break-all;animation:fade .2s ease}
.console .t{color:#4d5568}
.console .i{color:#aab3c5}
.console .ok{color:var(--ok)}
.console .err{color:var(--err)}
.console .w{color:var(--warn)}

/* ── open tab ───────────────────────────── */
.drop{border:1px dashed var(--line2);border-radius:4px;background:var(--bg1);text-align:center;
  padding:44px 20px;cursor:pointer;transition:all .18s}
.drop:hover,.drop.drag{border-color:var(--acc);background:#161610}
.drop .d1{font-family:var(--disp);font-size:16px;letter-spacing:3px;color:var(--acc2)}
.drop .d2{font-family:var(--mono);font-size:11px;color:var(--dim);margin-top:9px}
.open-info{display:none;margin-bottom:14px}
.open-info.on{display:block;animation:fade .25s}
.oi-top{display:flex;flex-wrap:wrap;gap:10px;align-items:center;margin-bottom:12px}
.oi-name{font-family:var(--mono);font-size:14px;color:var(--acc2);font-weight:600}
.oi-meta{display:flex;flex-wrap:wrap;gap:8px;margin-bottom:14px}
.oi-meta .chip{padding:6px 11px}
.oi-search{display:flex;align-items:center;gap:12px;margin-bottom:14px}
.oi-search input{flex:1;max-width:430px;font-family:var(--mono);font-size:12px;color:var(--tx);
  background:var(--bg3);border:1px solid var(--line2);border-radius:3px;padding:8px 12px}
.oi-search input:focus{border-color:var(--acc)}
#oi-shown{font-family:var(--mono);font-size:11px;color:var(--dim)}
.oi-actions{display:flex;gap:10px;flex-wrap:wrap;align-items:center}

.ftable{width:100%;border-collapse:collapse;font-family:var(--mono);font-size:12px;margin-top:16px}
.ftable th{font-size:9.5px;letter-spacing:1.8px;color:var(--faint);text-align:left;padding:7px 10px;border-bottom:1px solid var(--line2);font-weight:600}
.ftable td{padding:5.5px 10px;border-bottom:1px solid #1a1f28}
.ftable tbody tr.file:hover td{background:#161c26}
.ftable tr.grp td{background:#11141b;font-weight:700;letter-spacing:1.5px;font-size:11.5px;border-bottom:1px solid var(--line)}
.ftable .r{text-align:right}
.dot{display:inline-block;width:8px;height:8px;border-radius:1px;margin-right:8px;vertical-align:baseline}
.m{display:inline-block;font-size:9.5px;letter-spacing:1px;padding:1.5px 7px;border-radius:2px}
.m.def{color:var(--acc2);border:1px solid #6b5322}
.m.sto{color:#9aa3b5;border:1px solid var(--line2)}
.crc{color:#626d82}
.fpath{color:#c4cbda}

/* ── spec tab ───────────────────────────── */
.spech2{font-family:var(--disp);font-size:17px;letter-spacing:2.5px;color:var(--acc2);margin:24px 0 10px}
.spech2:first-child{margin-top:4px}
.specp{font-size:13px;color:var(--dim);line-height:1.7;margin-bottom:10px;max-width:860px}
.specp b{color:var(--tx)}
pre.diagram{font-family:var(--mono);font-size:11.5px;color:#b9c1d0;background:var(--bg1);
  border:1px solid var(--line);border-radius:4px;padding:14px 16px;overflow:auto;line-height:1.6;margin-bottom:8px}
table.spec{border-collapse:collapse;font-family:var(--mono);font-size:11.5px;margin-bottom:6px}
table.spec th{background:#161a23;color:var(--acc2);letter-spacing:1px;font-size:10px}
table.spec th,table.spec td{border:1px solid var(--line);padding:5.5px 11px;text-align:left}
table.spec td:first-child{color:var(--s1)}
table.spec td:nth-child(2){color:var(--s3)}
#hexdump{font-family:var(--mono);font-size:11.5px;color:#b9c1d0;background:var(--bg3);
  border:1px solid var(--line);border-radius:4px;padding:13px 16px;overflow:auto;line-height:1.7;white-space:pre}

/* ── status bar ─────────────────────────── */
#statusbar{position:fixed;left:0;right:0;bottom:0;z-index:5;display:flex;align-items:center;gap:26px;
  padding:9px 30px;background:rgba(11,13,17,.94);border-top:1px solid var(--line);
  font-family:var(--mono);font-size:11px;letter-spacing:.4px;backdrop-filter:blur(4px)}
.st span{color:var(--faint);letter-spacing:1.6px;font-size:9.5px;margin-right:7px}
.st b{color:#e9edf5;font-weight:600}
.st b.acc{color:var(--acc)}
#sb-msg{margin-left:auto;color:var(--acc2)}
#sb-msg.err{color:var(--err)}
#sb-msg.ok{color:var(--ok)}

@media (max-width:900px){
  .buckets{grid-template-columns:1fr}
  header{flex-wrap:wrap}
  #statusbar{flex-wrap:wrap;gap:12px 22px}
}
</style>
</head>
<body>

<header>
  <div class="logo"><i class="le"></i><i class="li"></i><i class="ls"></i><i class="lc"></i><i class="ld"></i></div>
  <div class="titles">
    <h1>JGP PACKAGER</h1>
    <p>J GAME PACKAGE · <b>BINARY FORMAT v1</b> · SOLID BLOCKS · CRC32 · RAW DEFLATE</p>
  </div>
  <div class="hdr-right"><span id="led" class="led"></span><span id="ledText">READY</span></div>
</header>

<nav>
  <button data-tab="create" class="active">CREATE JGP</button>
  <button data-tab="open">OPEN JGP</button>
  <button data-tab="spec">FORMAT SPEC</button>
</nav>

<main>
  <!-- ══ CREATE ══ -->
  <section id="tab-create" class="tabpanel active">
    <div class="create-top">
      <div class="field">
        <label>PACKAGE NAME</label>
        <input id="pkgName" value="MyGame" spellcheck="false" autocomplete="off">
      </div>
      <div class="chip">FILES <b id="c-files">0</b></div>
      <div class="chip">ORIGINAL <b id="c-orig">0 B</b></div>
      <div class="chip">섹션 <b>game.exe · image/ · sound/ · code/ · data/</b></div>
      <div class="hintline">① EXE 선택 → ② 리소스 추가 (드래그&amp;드롭 지원, 폴더명 <em>image·sound·code·data</em> 자동 인식) → ③ CREATE. 섹션만 존재하며 별도 INFO/VIDEO 섹션 없음.</div>
    </div>

    <div class="buckets" id="buckets"></div>

    <div class="actionbar">
      <button id="btnBuild" class="btn primary big">▸ CREATE JGP</button>
      <div class="progwrap" id="pw-c">
        <div class="proglabel" id="pl-c">building…</div>
        <div class="prog busy" id="pb-c"><i></i></div>
      </div>
    </div>

    <div class="result" id="result">
      <div class="r-item"><div class="rl">PACKAGE</div><div class="rv sm" id="r-name">—</div></div>
      <div class="r-item"><div class="rl">PACKED</div><div class="rv acc" id="r-packed">—</div></div>
      <div class="r-item"><div class="rl">COMPRESSION</div><div class="rv" id="r-ratio">—</div></div>
      <div class="r-item"><div class="rl">FILES / BLOCKS</div><div class="rv sm" id="r-counts">—</div></div>
      <div class="r-btns">
        <button class="btn primary" id="btnDownload">↓ Download .jgp</button>
        <button class="btn" id="btnInspect">Open in viewer →</button>
      </div>
    </div>

    <div class="console" id="log-c"></div>
  </section>

  <!-- ══ OPEN ══ -->
  <section id="tab-open" class="tabpanel">
    <div class="drop" id="dropzone">
      <div class="d1">DROP .JGP HERE / CLICK TO SELECT</div>
      <div class="d2">헤더 · 인덱스 · 블록을 실제 파싱합니다 — 매직 "JGP\x01", CRC32 검증, 규격 위반 경로 거부</div>
    </div>

    <div class="open-info" id="openInfo">
      <div class="oi-top">
        <span class="oi-name" id="oi-name">—</span>
        <button class="btn mini" id="btnClosePkg">✕ close</button>
      </div>
      <div class="oi-meta" id="oi-meta"></div>
      <div class="oi-search">
        <input id="oi-filter" placeholder="인덱스 검색 — 예: image/player, .ogg, save" autocomplete="off" spellcheck="false">
        <span id="oi-shown"></span>
      </div>
      <div class="oi-actions">
        <button class="btn primary big" id="btnExtract">▸ EXTRACT JGP</button>
        <button class="btn" id="btnExtractZip">Extract as ZIP (fallback)</button>
        <button class="btn" id="btnExeDl" style="display:none">↓ game.exe</button>
        <button class="btn" id="btnVerify">Verify integrity (CRC32)</button>
        <div class="progwrap" id="pw-o" style="flex:1;max-width:380px">
          <div class="proglabel" id="pl-o">…</div>
          <div class="prog busy" id="pb-o"><i></i></div>
        </div>
      </div>
    </div>

    <div id="tableWrap"></div>
    <div class="console" id="log-o"></div>
  </section>

  <!-- ══ SPEC ══ -->
  <section id="tab-spec" class="tabpanel">
    <h2 class="spech2">JGP v1 — BINARY FORMAT</h2>
    <p class="specp">게임 배포 전용 패키지 포맷. ZIP과 달리 <b>파일당 로컬 헤더가 없고</b>, 작은 파일들은 섹션 단위
      <b>솔리드 블록</b>으로 묶어 하나의 raw DEFLATE 스트림에 저장한다. 모든 정수 필드는 <b>little-endian</b>,
      문자열은 <b>UTF-8</b>. 이 문서만으로 C/C++ 디코더를 구현할 수 있다.</p>
<pre class="diagram">┌────────────────┬──────────────────────────────────────┬────────────────┐
│  HEADER 64 B   │  BLOCK PAYLOAD  (block 0 … block N)  │     INDEX      │
└────────────────┴──────────────────────────────────────┴────────────────┘
 offset 0x00      offset 0x40 ~                          header.indexOffset

 BLOCK PAYLOAD는 프레이밍 바이트가 전혀 없다(0 오버헤드).
 블록 위치/크기는 전부 INDEX가 가진다.
 저장 트리: &lt;root&gt;/game.exe · image/ · sound/ · code/ · data/  — 정확히 5개, 다른 섹션 금지.</pre>

    <h2 class="spech2">HEADER (64 bytes)</h2>
    <table class="spec">
      <tr><th>OFFSET</th><th>TYPE</th><th>FIELD</th><th>설명</th></tr>
      <tr><td>0x00</td><td>u32</td><td>magic</td><td>0x0150474A — 바이트 순서 <code>4A 47 50 01</code> ("JGP\x01")</td></tr>
      <tr><td>0x04</td><td>u16</td><td>version</td><td>= 1</td></tr>
      <tr><td>0x06</td><td>u16</td><td>flags</td><td>bit0 = 인덱스가 raw DEFLATE로 압축됨</td></tr>
      <tr><td>0x08</td><td>u32</td><td>entryCount</td><td>파일 수</td></tr>
      <tr><td>0x0C</td><td>u32</td><td>blockCount</td><td>블록 수</td></tr>
      <tr><td>0x10</td><td>u64</td><td>indexOffset</td><td>INDEX의 절대 오프셋</td></tr>
      <tr><td>0x18</td><td>u64</td><td>indexSize</td><td>저장된 INDEX 바이트 수</td></tr>
      <tr><td>0x20</td><td>u64</td><td>totalUnpacked</td><td>전체 원본 크기 합</td></tr>
      <tr><td>0x28</td><td>u32</td><td>headerCrc</td><td>바이트 0x00~0x27의 CRC32</td></tr>
      <tr><td>0x2C</td><td>—</td><td>reserved</td><td>0으로 채움 (0x3F까지)</td></tr>
    </table>

    <h2 class="spech2">INDEX — block record (32 B × blockCount)</h2>
    <table class="spec">
      <tr><th>OFF</th><th>TYPE</th><th>FIELD</th><th>설명</th></tr>
      <tr><td>0</td><td>u8</td><td>method</td><td>0 = STORE, 1 = DEFLATE(raw, RFC 1951)</td></tr>
      <tr><td>1</td><td>u8+u16</td><td>reserved</td><td>0</td></tr>
      <tr><td>4</td><td>u32</td><td>crc32</td><td>복원된 블록 전체의 CRC32</td></tr>
      <tr><td>8</td><td>u64</td><td>unpackedSize</td><td>복원 크기</td></tr>
      <tr><td>16</td><td>u64</td><td>fileOffset</td><td>파일 내 페이로드 오프셋</td></tr>
      <tr><td>24</td><td>u64</td><td>packedSize</td><td>저장된 페이로드 크기</td></tr>
    </table>

    <h2 class="spech2">INDEX — entry record (28 B + nameLen × entryCount)</h2>
    <table class="spec">
      <tr><th>OFF</th><th>TYPE</th><th>FIELD</th><th>설명</th></tr>
      <tr><td>0</td><td>u8</td><td>section</td><td>0=EXE(root), 1=IMAGE, 2=SOUND, 3=CODE, 4=DATA</td></tr>
      <tr><td>1</td><td>u8</td><td>flags</td><td>0</td></tr>
      <tr><td>2</td><td>u16</td><td>nameLen</td><td>UTF-8 이름 바이트 수</td></tr>
      <tr><td>4</td><td>u32</td><td>blockId</td><td>소속 블록</td></tr>
      <tr><td>8</td><td>u32</td><td>crc32</td><td>이 파일 원본의 CRC32</td></tr>
      <tr><td>12</td><td>u64</td><td>offsetInBlock</td><td>블록 복원 데이터 내 오프셋</td></tr>
      <tr><td>20</td><td>u64</td><td>size</td><td>원본 크기</td></tr>
      <tr><td>28</td><td>u8[]</td><td>name</td><td>섹션 내부 경로. '/' 구분, '..'·절대경로·'\' 금지. section 0은 정확히 "game.exe" 1개</td></tr>
    </table>

    <h2 class="spech2">IMPLEMENTATION NOTES</h2>
    <p class="specp">
      • CRC32는 ZIP/PNG와 동일한 IEEE 802.3 (poly 0xEDB88320, init/xorout 0xFFFFFFFF).<br>
      • 인코더는 DEFLATE 결과가 원본보다 크면 STORE로 저장 — 디코더는 두 메서드 모두 지원해야 한다.<br>
      • <b>DEFLATE 포맷 주의:</b> JGP는 <b>Raw Deflate(RFC 1951)</b>만을 사용한다. Zlib 헤더/트레일러(RFC 1950)가 포함되어서는 안 된다.<br>
      • C/C++ 구현 시 반드시 <code>inflateInit2(&amp;s, -15)</code> / <code>deflateInit2(&amp;s, lv, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY)</code> 사용.<br>
      • JavaScript 구현 시 반드시 <code>CompressionStream('deflate-raw')</code> / <code>DecompressionStream('deflate-raw')</code> 사용. ('deflate'는 브라우저에 따라 Zlib 래퍼를 포함할 수 있으므로 금지)<br>
      • 솔리드 블록의 단일 파일 추출 = 블록 1회 복원 후 [offsetInBlock, +size) 슬라이스. STORE 블록은 해당 오프셋을 직접 slice.<br>
      • 인덱스 로드 후 경로→엔트리 해시맵 구성: 단일 파일 접근은 인덱스 조회 + 블록 1회 읽기로 끝난다.<br>
      • 이미 압축된 미디어(png/jpg/ogg/mp3 등)만 모인 블록은 인코더가 DEFLATE를 건너뛰어 STORE로 저장 — 속도·크기 모두 확보.<br>
      • 추출 트리는 항상 game.exe + image/ + sound/ + code/ + data/ 를 생성한다 (빈 섹션 디렉터리 포함).</p>

    <h2 class="spech2">LIVE HEADER — 현재 열린 패키지</h2>
    <div id="hexdump">— 패키지 미열림 (Open JGP 탭에서 .jgp를 열면 64바이트 헤더를 여기에 덤프합니다)</div>
  </section>
</main>

<footer id="statusbar">
  <div class="st"><span>FILES</span><b id="sb-files">0</b></div>
  <div class="st"><span>ORIGINAL</span><b id="sb-orig">0 B</b></div>
  <div class="st"><span>PACKED</span><b id="sb-packed">—</b></div>
  <div class="st"><span>COMPRESSION</span><b id="sb-ratio" class="acc">—</b></div>
  <div id="sb-msg"></div>
</footer>

<script>
'use strict';
/* ════════════════════════════════════════════════════════════════
   JGP v1 — format constants (바이너리 규격은 변경 금지)
   ════════════════════════════════════════════════════════════════ */
const MAGIC=0x0150474A, VERSION=1, HDR_SIZE=64;
const M_STORE=0, M_DEFLATE=1;
const SOLO_MIN=256*1024, SOLID_MAX=Math.round(1.5*1024*1024), SOLID_MAX_FILES=512;
const SECTIONS=[
  {name:'EXE',  dir:'',      color:'var(--s0)', hex:'#ff7364'},
  {name:'IMAGE',dir:'image', color:'var(--s1)', hex:'#53b6ff'},
  {name:'SOUND',dir:'sound', color:'var(--s2)', hex:'#45d48a'},
  {name:'CODE', dir:'code',  color:'var(--s3)', hex:'#e8c15a'},
  {name:'DATA', dir:'data',  color:'var(--s4)', hex:'#a08cff'},
];
const STORE_EXT=/\.(png|jpe?g|jfif|gif|webp|avif|mp3|ogg|oga|opus|m4a|aac|flac|mp4|webm|zip|7z|rar|gz|tgz|pak|apk)$/i;

/* 
 * [수정] CompressionStream 지원 여부 및 Raw Deflate 사용 가능 여부 체크
 * 'deflate-raw'는 Chrome 80+, Safari 16.4+, Firefox 113+ 에서 지원됩니다.
 * JGP v1 규격은 Raw Deflate(RFC 1951)를 강제하므로 'deflate-raw' 필수입니다.
 */
const HAS_RAW_DEFLATE = (() => {
  try {
    new CompressionStream('deflate-raw');
    return true;
  } catch (e) {
    return false;
  }
})();

/* ── utils ─────────────────────────────── */
const CRC_T=(()=>{const t=new Uint32Array(256);for(let n=0;n<256;n++){let c=n;for(let k=0;k<8;k++)c=(c&1)?(0xEDB88320^(c>>>1)):(c>>>1);t[n]=c>>>0;}return t;})();
function crc32(u8,start=0,end=-1,prev=0){if(end<0)end=u8.length;let c=(prev^0xFFFFFFFF)>>>0;
  for(let i=start;i<end;i++)c=CRC_T[(c^u8[i])&0xFF]^(c>>>8);return(c^0xFFFFFFFF)>>>0;}

/**
 * [수정] Streams API를 통한 압축/해제 파이프라인
 * 규격 준수를 위해 반드시 'deflate-raw'를 사용합니다.
 */
async function csBytes(u8, stream) {
  const readable = new Blob([u8]).stream().pipeThrough(stream);
  return new Uint8Array(await new Response(readable).arrayBuffer());
}

const deflateRaw = u8 => {
  if (!HAS_RAW_DEFLATE) throw new Error("Browser does not support deflate-raw");
  return csBytes(u8, new CompressionStream('deflate-raw'));
};

const inflateRaw = u8 => {
  if (!HAS_RAW_DEFLATE) throw new Error("Browser does not support deflate-raw");
  return csBytes(u8, new DecompressionStream('deflate-raw'));
};

const tick=()=>new Promise(r=>setTimeout(r,0));
function fmt(n){if(!isFinite(n))return'—';if(n<1024)return n+' B';
  const u=['KB','MB','GB','TB'];let i=-1;do{n/=1024;i++;}while(n>=1024&&i<u.length-1);
  return(n>=100?n.toFixed(0):n>=10?n.toFixed(1):n.toFixed(2))+' '+u[i];}
function ratioStr(orig,packed){if(!orig)return'—';const r=(1-packed/orig)*100;
  return(r>0?'+':'')+r.toFixed(1)+'%';}
function saveBlob(name,blob){const a=document.createElement('a');a.href=URL.createObjectURL(blob);
  a.download=name;document.body.appendChild(a);a.click();
  setTimeout(()=>{URL.revokeObjectURL(a.href);a.remove();},4000);}
const baseName=n=>(n.replace(/\.jgp$/i,'').replace(/[\\/:*?"<>|]/g,'').trim())||'game';
const $=id=>document.getElementById(id);

/* ── console / LED / status ────────────── */
function log(pane,cls,msg){const el=$(pane);const d=new Date();
  const p=x=>String(x).padStart(2,'0');
  const ln=document.createElement('div');ln.className='ln';
  ln.innerHTML=`<span class="t">[${p(d.getHours())}:${p(d.getMinutes())}:${p(d.getSeconds())}] </span><span class="${cls}">${msg.replace(/</g,'&lt;')}</span>`;
  el.appendChild(ln);while(el.children.length>260)el.removeChild(el.firstChild);
  el.scrollTop=el.scrollHeight;}
const logC=(c,m)=>log('log-c',c,m), logO=(c,m)=>log('log-o',c,m);
function setLED(state,text){const led=$('led');led.className='led'+(state?' '+state:'');$('ledText').textContent=text;}
function setMsg(text,cls=''){$('sb-msg').textContent=text;$('sb-msg').className=cls;}
function setStatus(files,orig,packed,ratio){
  $('sb-files').textContent=files; $('sb-orig').textContent=fmt(orig);
  $('sb-packed').textContent=packed==null?'—':fmt(packed);
  $('sb-ratio').textContent=ratio==null?'—':ratio;}
function setProg(w,l,p,frac,text){const wrap=$(w);wrap.classList.add('on');
  $(l).textContent=text;$(p).querySelector('i').style.width=(frac*100).toFixed(1)+'%';}
function hideProg(w){$(w).classList.remove('on');}

/* ── state ─────────────────────────────── */
const state={exe:null,buckets:[null,new Map(),new Map(),new Map(),new Map()]};
let building=false,busy=false,lastBuild=null,model=null,activeTab='create';
let blkCache={m:null,bi:-1,raw:null}; // 마지막 inflate 블록 캐시(개별 파일 반복 추출 고속화)
function clearBlockCache(){blkCache={m:null,bi:-1,raw:null};}

/* ════════════════════════════════════════
   CREATE — UI
   ════════════════════════════════════════ */
function renderBuckets(){
  const wrap=$('buckets');wrap.innerHTML='';
  const exe=document.createElement('div');exe.className='bucket exe';exe.style.setProperty('--sec','var(--s0)');
  exe.innerHTML=`<div class="b-head"><div><span class="b-name">EXE</span><span class="b-path">game.exe</span></div>
    <div class="b-meta">메인 실행 파일 · 패키지 루트에 <b>game.exe</b>로 저장</div></div>
    <div class="exerow" id="exeRow"></div>
    <div class="b-actions"><button class="btn" data-act="pick-exe">＋ 게임 EXE 선택</button></div>`;
  wrap.appendChild(exe);
  wireDnD(exe,0);
  for(let s=1;s<=4;s++){
    const S=SECTIONS[s];
    const card=document.createElement('div');card.className='bucket';card.dataset.sec=s;
    card.style.setProperty('--sec',S.color);
    const hints=['','','png · jpg · webp · gif · bmp …','wav · ogg · mp3 …','dll · js · lua · wasm …','save · font · config · …'][s];
    card.innerHTML=`<div class="b-head"><div><span class="b-name">${S.name}</span><span class="b-path">${S.dir}/</span></div>
      <div class="b-meta" id="meta-${s}"></div></div>
      <div class="b-hint">${hints}</div>
      <div class="b-list" id="list-${s}"></div>
      <div class="b-actions">
        <button class="btn" data-act="files" data-sec="${s}">＋ Files</button>
        <button class="btn" data-act="folder" data-sec="${s}">＋ Folder</button>
      </div>`;
    wrap.appendChild(card);
    wireDnD(card,s);
  }
  renderExe();for(let s=1;s<=4;s++)renderBucket(s);
}
function renderExe(){
  const r=$('exeRow');
  if(!state.exe){r.innerHTML='<span class="exe-empty">선택된 EXE 없음 — 생성하려면 게임 실행 파일이 필요합니다</span>';return;}
  r.innerHTML=`<span class="tgt">game.exe</span><span class="arr">←</span>
    <span class="src">${state.exe.name.replace(/</g,'&lt;')}</span>
    <span class="sz">(${fmt(state.exe.size)})</span>
    <button class="fx" id="exeClear" title="제거">×</button>`;
  $('exeClear').onclick=()=>{state.exe=null;renderExe();updateCreateStats();logC('w','EXE 제거됨');};
}
function renderBucket(s){
  const m=state.buckets[s],list=$('list-'+s);
  let n=0,sz=0;for(const f of m.values()){n++;sz+=f.size;}
  $('meta-'+s).innerHTML=`<b>${n}</b> files · <b>${fmt(sz)}</b>`;
  list.innerHTML='';const keys=[...m.keys()].sort();const cap=200;
  keys.slice(0,cap).forEach(k=>{
    const row=document.createElement('div');row.className='frow';
    row.innerHTML=`<span class="fname" title="${k.replace(/"/g,'&quot;')}">${k.replace(/</g,'&lt;')}</span>
      <span class="fsize">${fmt(m.get(k).size)}</span>
      <button class="fx" data-k="${encodeURIComponent(k)}" title="제거">×</button>`;
    list.appendChild(row);
  });
  if(keys.length>cap){const d=document.createElement('div');d.className='more';
    d.textContent=`+ ${keys.length-cap} more…`;list.appendChild(d);}
}
function refreshBuckets(){for(let s=1;s<=4;s++)renderBucket(s);updateCreateStats();}
function createStats(){let n=state.exe?1:0,sz=state.exe?state.exe.size:0;
  for(let s=1;s<=4;s++)for(const f of state.buckets[s].values()){n++;sz+=f.size;}
  return{n,sz};}
function updateCreateStats(){
  const {n,sz}=createStats();
  $('c-files').textContent=n;$('c-orig').textContent=fmt(sz);
  if(activeTab==='create'){
    setStatus(n,sz,lastBuild?lastBuild.blob.size:null,
      lastBuild?ratioStr(sz,lastBuild.blob.size):null);
  }
}

/* ── routing / adding ──────────────────── */
const KEYMAP={image:1,images:1,img:1,graphics:1,sprite:1,sprites:1,
  sound:2,sounds:2,audio:2,music:2,bgm:2,sfx:2,
  code:3,script:3,scripts:3,
  data:4,save:4,saves:4,font:4,fonts:4,config:4};
function guessSec(name){const ext=(name.match(/\.([a-z0-9]+)$/i)||[])[1]?.toLowerCase();
  if(ext==='exe')return 0;
  if(['png','jpg','jpeg','jfif','gif','webp','avif','bmp','svg','tga','ico'].includes(ext))return 1;
  if(['wav','ogg','oga','mp3','opus','m4a','aac','flac','wma'].includes(ext))return 2;
  if(['dll','so','js','mjs','ts','lua','py','wasm','c','cpp','h','hpp','cs','asm','ini'].includes(ext))return 3;
  return 4;}
function warnExeName(f){if(!/\.exe$/i.test(f.name))
  logC('w',`"${f.name}" — .exe 확장자가 아니지만 패키지에는 game.exe로 저장됩니다`);}
function routeEntry(rel,file,forced,batch){
  let parts=rel.split(/[\\/]+/).map(s=>s.trim()).filter(s=>s&&s!=='.'&&s!=='..');
  if(!parts.length)return;
  let sec=forced;
  const head=parts[0].toLowerCase();
  if(parts.length>1&&sec==null&&KEYMAP[head]!=null){sec=KEYMAP[head];parts=parts.slice(1);}
  else if(parts.length>1&&sec!=null&&KEYMAP[head]===sec){parts=parts.slice(1);}
  if(sec==null)sec=guessSec(file.name);
  if(!parts.length)return;
  if(sec===0){if(state.exe)batch.r++;else batch.a++;state.exe=file;warnExeName(file);return;}
  const path=parts.join('/');
  if(state.buckets[sec].has(path))batch.r++;else batch.a++;
  state.buckets[sec].set(path,file);
}
function logBatch(pane,total,batch,forced){
  if(!total)return;
  const where=forced!=null?` → ${forced===0?'EXE':SECTIONS[forced].name+'/'}`:' → 자동 분류';
  log(pane,'i',`${total}개 추가${where} (신규 ${batch.a}${batch.r?`, 교체 ${batch.r}`:''})`);
}
function pickFiles(sec,folder){
  const inp=document.createElement('input');inp.type='file';
  if(folder){inp.setAttribute('webkitdirectory','');inp.setAttribute('directory','');}
  else if(sec!==0)inp.multiple=true;
  inp.onchange=()=>{
    const fl=[...inp.files];if(!fl.length)return;
    const batch={a:0,r:0};
    if(sec===0&&!folder){
      if(state.exe)batch.r++;else batch.a++;
      state.exe=fl[0];
      if(fl.length>1)logC('w',`EXE는 단일 파일 — 나머지 ${fl.length-1}개 무시`);
      warnExeName(fl[0]);
      renderExe();refreshBuckets();logBatch('log-c',1,batch,0);return;
    }
    if(folder){for(const f of fl)routeEntry(f.webkitRelativePath||f.name,f,sec,batch);}
    else{for(const f of fl)routeEntry(f.name,f,sec,batch);}
    renderExe();refreshBuckets();logBatch('log-c',fl.length,batch,sec);
  };
  inp.click();
}
async function walkEntry(entry,rel,onFile){
  if(entry.isFile){
    try{const f=await new Promise((res,rej)=>entry.file(res,rej));onFile(f,rel?rel+'/'+entry.name:entry.name);}
    catch(e){logC('w','읽기 실패: '+entry.fullPath);}
  }else if(entry.isDirectory){
    const reader=entry.createReader();
    for(;;){const batch=await new Promise((res,rej)=>reader.readEntries(res,rej));
      if(!batch.length)break;
      const nrel=rel?rel+'/'+entry.name:entry.name;
      for(const e of batch)await walkEntry(e,nrel,onFile);}
  }
}
async function handleDrop(dt,forced,pane){
  const entries=[];
  if(dt.items)for(const it of dt.items){const en=it.webkitGetAsEntry&&it.webkitGetAsEntry();if(en)entries.push(en);}
  const got=[];
  if(entries.length){for(const en of entries)await walkEntry(en,'',(f,rel)=>got.push([rel,f]));}
  else{for(const f of dt.files)got.push([f.name,f]);}
  if(forced===0){ /* EXE 버킷은 단일 파일만 */
    if(!got.length)return;
    const f=got[0][1];const batch={a:0,r:0};
    if(state.exe)batch.r++;else batch.a++;
    state.exe=f;
    if(got.length>1)logC('w',`EXE 버킷은 단일 파일만 받음 — ${got.length-1}개 무시`);
    warnExeName(f);
    renderExe();refreshBuckets();logBatch(pane,1,batch,0);return;
  }
  const batch={a:0,r:0};
  for(const[rel,f]of got)routeEntry(rel,f,forced,batch);
  renderExe();refreshBuckets();logBatch(pane,got.length,batch,forced);
}
function wireDnD(el,sec){
  el.addEventListener('dragover',e=>{e.preventDefault();e.stopPropagation();
    e.dataTransfer.dropEffect='copy';el.classList.add('drag');});
  el.addEventListener('dragleave',()=>el.classList.remove('drag'));
  el.addEventListener('drop',async e=>{e.preventDefault();e.stopPropagation();
    el.classList.remove('drag');await handleDrop(e.dataTransfer,sec,'log-c');});
}
$('buckets').addEventListener('click',e=>{
  const b=e.target.closest('button');if(!b)return;
  const act=b.dataset.act;
  if(act==='pick-exe'){pickFiles(0,false);return;}
  if(act==='files'||act==='folder'){pickFiles(+b.dataset.sec,act==='folder');return;}
  if(b.classList.contains('fx')&&b.dataset.k){
    const sec=+b.closest('.bucket').dataset.sec;
    state.buckets[sec].delete(decodeURIComponent(b.dataset.k));
    refreshBuckets();
  }
});
window.addEventListener('dragover',e=>e.preventDefault());
window.addEventListener('drop',e=>e.preventDefault());

/* ════════════════════════════════════════
   CREATE — build real .jgp
   ════════════════════════════════════════ */
function buildHeader(o){const b=new Uint8Array(HDR_SIZE);const dv=new DataView(b.buffer);
  dv.setUint32(0,MAGIC,true);dv.setUint16(4,VERSION,true);dv.setUint16(6,o.flags,true);
  dv.setUint32(8,o.entryCount,true);dv.setUint32(12,o.blockCount,true);
  dv.setBigUint64(16,BigInt(o.indexOffset),true);dv.setBigUint64(24,BigInt(o.indexSize),true);
  dv.setBigUint64(32,BigInt(o.totalUnpacked),true);
  dv.setUint32(40,crc32(b,0,40),true);return b;}
function buildIndex(blocks,entries){
  const enc=new TextEncoder();const names=entries.map(e=>enc.encode(e.name));
  let size=blocks.length*32;entries.forEach((e,i)=>size+=28+names[i].length);
  const b=new Uint8Array(size);const dv=new DataView(b.buffer);let p=0;
  for(const bl of blocks){dv.setUint8(p,bl.method);dv.setUint32(p+4,bl.crc,true);
    dv.setBigUint64(p+8,BigInt(bl.unpacked),true);dv.setBigUint64(p+16,BigInt(bl.off),true);
    dv.setBigUint64(p+24,BigInt(bl.packed),true);p+=32;}
  entries.forEach((e,i)=>{dv.setUint8(p,e.sec);dv.setUint16(p+2,names[i].length,true);
    dv.setUint32(p+4,e.blockId,true);dv.setUint32(p+8,e.crc,true);
    dv.setBigUint64(p+12,BigInt(e.off),true);dv.setBigUint64(p+20,BigInt(e.size),true);
    p+=28;b.set(names[i],p);p+=names[i].length;});
  return b;}

async function buildPackage(){
  if(building)return;
  if(!state.exe){logC('err','게임 EXE를 먼저 선택하세요.');setMsg('EXE 필요','err');return;}
  
  // [추가] Raw Deflate 미지원 브라우저 차단
  if(!HAS_RAW_DEFLATE){
    logC('err','현재 브라우저는 JGP v1 규격(Raw Deflate)을 지원하지 않습니다. Chrome, Edge, Safari 최신 버전 또는 Firefox 113+를 사용해주세요.');
    setMsg('UNSUPPORTED BROWSER','err');setLED('err','ERROR');
    return;
  }

  building=true;$('btnBuild').disabled=true;setLED('busy','PACKING');
  const t0=performance.now();
  try{
    /* 계획: exe 단독 블록 + 섹션별 솔리드/단독 블록 */
    const items=[{sec:0,name:'game.exe',file:state.exe}];
    for(let s=1;s<=4;s++){const m=state.buckets[s];
      for(const k of [...m.keys()].sort())items.push({sec:s,name:k,file:m.get(k)});}
    const plans=[[items[0]]];
    let i=1;
    for(let s=1;s<=4;s++){
      let solid=[],sz=0;
      const flush=()=>{if(solid.length){plans.push(solid);solid=[];sz=0;}};
      while(i<items.length&&items[i].sec===s){
        const it=items[i++];
        if(it.file.size>=SOLO_MIN){flush();plans.push([it]);}
        else{solid.push(it);sz+=it.file.size;
          if(sz>=SOLID_MAX||solid.length>=SOLID_MAX_FILES)flush();}
      }
      flush();
    }
    const solidN=plans.filter(p=>p.length>1).length;
    logC('i',`빌드 시작 — 파일 ${items.length}개 → 블록 ${plans.length}개 (솔리드 ${solidN})`);

    /* 블록 실체화 */
    const parts=[];let cursor=HDR_SIZE;
    const blockRecs=[],entryRecs=[];
    setProg('pw-c','pl-c','pb-c',0,'preparing…');
    for(let bi=0;bi<plans.length;bi++){
      const plan=plans[bi];
      const total=plan.reduce((a,it)=>a+it.file.size,0);
      const raw=new Uint8Array(total);
      let o=0;
      for(const it of plan){
        const buf=new Uint8Array(await it.file.arrayBuffer());
        raw.set(buf,o);
        entryRecs.push({sec:it.sec,name:it.name,blockId:bi,
          crc:crc32(raw,o,o+buf.length),off:o,size:buf.length});
        o+=buf.length;
      }
      const bCrc=crc32(raw);
      let packed=raw,method=M_STORE;
      /* 이미 압축된 포맷뿐인 블록(단독/솔리드 무관)은 DEFLATE 시도 자체를 스킵 */
      const skipDef=plan.every(it=>STORE_EXT.test(it.name));
      if(total>0&&!skipDef){
        try {
          const d=await deflateRaw(raw);
          if(d.length<raw.length){packed=d;method=M_DEFLATE;}
        } catch(e) {
          // 압축 스트림 오류 시 STORE로 폴백 (안전 장치)
          logC('w', `Block ${bi} 압축 실패, STORE로 대체: ${e.message}`);
        }
      }
      blockRecs.push({method,crc:bCrc,unpacked:total,off:cursor,packed:packed.length});
      parts.push(packed);cursor+=packed.length;
      setProg('pw-c','pl-c','pb-c',(bi+1)/plans.length,
        `block ${bi+1}/${plans.length} · ${plan.length===1?plan[0].name:'solid ×'+plan.length} · ${method?'DEFLATE-RAW':'STORE'}`);
      await tick();
    }

    /* 인덱스 + 헤더 + 최종 Blob (파츠 조립 — 전체 재복사 없음) */
    const indexBytes=buildIndex(blockRecs,entryRecs);
    let idx=indexBytes,flags=0;
    try {
      const d=await deflateRaw(indexBytes);
      if(d.length<indexBytes.length){idx=d;flags|=1;}
    } catch(e) {
      logC('w', `인덱스 압축 실패, STORE로 대체: ${e.message}`);
    }
    
    const totalUnpacked=entryRecs.reduce((a,e)=>a+e.size,0);
    const header=buildHeader({flags,entryCount:entryRecs.length,blockCount:blockRecs.length,
      indexOffset:cursor,indexSize:idx.length,totalUnpacked});
    const blob=new Blob([header,...parts,idx],{type:'application/x-jgp'});

    const rawName=($('pkgName').value.trim()||'MyGame').replace(/[\\/:*?"<>|]/g,'');
    const fname=rawName+'.jgp';
    lastBuild={blob,fname,entryRecs,blockRecs,totalUnpacked};

    const ratio=ratioStr(totalUnpacked,blob.size);
    logC('ok',`완료 — ${fname} · ${fmt(blob.size)} (${ratio}) · ${(performance.now()-t0).toFixed(0)}ms`);
    $('r-name').textContent=fname;
    $('r-packed').textContent=fmt(blob.size);
    $('r-ratio').textContent=ratio;
    $('r-counts').textContent=`${entryRecs.length} / ${blockRecs.length}`;
    $('result').classList.add('on');
    setMsg('PACKAGE READY','ok');setLED('ok','DONE');
    updateCreateStats();
  }catch(err){
    logC('err','빌드 실패: '+(err&&err.message||err));
    setMsg('BUILD FAILED','err');setLED('err','ERROR');
  }finally{
    building=false;$('btnBuild').disabled=false;
    setTimeout(()=>hideProg('pw-c'),900);
  }
}
$('btnBuild').onclick=buildPackage;
$('btnDownload').onclick=()=>{if(lastBuild)saveBlob(lastBuild.fname,lastBuild.blob);};
$('btnInspect').onclick=()=>{if(!lastBuild)return;
  switchTab('open');
  openModel(new File([lastBuild.blob],lastBuild.fname)).catch(e=>logO('err',String(e)));};

/* ════════════════════════════════════════
   OPEN — real parser
   ════════════════════════════════════════ */
function parseIndex(u8,blockCount,entryCount){
  const dv=new DataView(u8.buffer,u8.byteOffset,u8.byteLength);
  const blocks=[];let p=0;
  for(let i=0;i<blockCount;i++){
    blocks.push({method:dv.getUint8(p),crc:dv.getUint32(p+4,true),
      unpacked:Number(dv.getBigUint64(p+8,true)),
      off:Number(dv.getBigUint64(p+16,true)),
      packed:Number(dv.getBigUint64(p+24,true))});
    p+=32;}
  const entries=[];const dec=new TextDecoder();
  for(let i=0;i<entryCount;i++){
    const nameLen=dv.getUint16(p+2,true);
    entries.push({sec:dv.getUint8(p),blockId:dv.getUint32(p+4,true),
      crc:dv.getUint32(p+8,true),
      off:Number(dv.getBigUint64(p+12,true)),
      size:Number(dv.getBigUint64(p+20,true)),
      name:dec.decode(u8.subarray(p+28,p+28+nameLen))});
    p+=28+nameLen;}
  return{blocks,entries};}

async function openModel(file){
  try{
    clearBlockCache();
    if(file.size<HDR_SIZE)throw new Error('파일이 너무 작습니다 (JGP 아님)');
    const hb=new Uint8Array(await file.slice(0,HDR_SIZE).arrayBuffer());
    const dv=new DataView(hb.buffer);
    if(dv.getUint32(0,true)!==MAGIC)throw new Error('매직 불일치 — JGP 파일이 아닙니다');
    const version=dv.getUint16(4,true);
    if(version!==VERSION)throw new Error(`지원하지 않는 포맷 버전 v${version}`);
    const flags=dv.getUint16(6,true);
    const entryCount=dv.getUint32(8,true),blockCount=dv.getUint32(12,true);
    const indexOffset=Number(dv.getBigUint64(16,true)),indexSize=Number(dv.getBigUint64(24,true));
    const totalUnpacked=Number(dv.getBigUint64(32,true));
    if(crc32(hb,0,40)!==dv.getUint32(40,true))logO('w','헤더 CRC 불일치 — 헤더 손상 가능성');
    if(indexOffset<HDR_SIZE||indexSize===0)throw new Error('잘못된 인덱스 오프셋/크기');
    if(indexOffset+indexSize>file.size)throw new Error('인덱스 오프셋이 파일 범위를 벗어남');
    let idx=new Uint8Array(await file.slice(indexOffset,indexOffset+indexSize).arrayBuffer());
    if(flags&1){
      if(!HAS_RAW_DEFLATE) throw new Error('인덱스 압축 해제 불가: 브라우저가 deflate-raw를 지원하지 않습니다');
      try{idx=await inflateRaw(idx);}catch(e){throw new Error('인덱스 압축 해제 실패 (손상 또는 호환성 문제)');}
    }
    const {blocks,entries}=parseIndex(idx,blockCount,entryCount);
    blocks.forEach((b,i)=>{if(b.method>1)throw new Error(`블록 ${i}: 알 수 없는 메서드 ${b.method}`);
      if(b.off+b.packed>file.size)throw new Error(`블록 ${i}: 범위 초과`);});
    entries.forEach((e,i)=>{
      if(e.sec>4)throw new Error(`엔트리 ${i}: 잘못된 섹션 ${e.sec}`);
      if(!e.name||e.name.includes('\\')||e.name.startsWith('/')||
         e.name.split('/').some(s=>!s||s==='..'))
        throw new Error(`엔트리 ${i}: 규격 위반 경로 "${e.name}" ('/' 구분만 허용, '..'·절대경로·'\\' 금지)`);
      if(e.blockId>=blocks.length)throw new Error(`엔트리 ${i}: 잘못된 블록 참조`);
      const b=blocks[e.blockId];
      if(e.off+e.size>b.unpacked)throw new Error(`엔트리 ${i}: 블록 범위 초과`);});
    /* 규격 일관성 경고 */
    const seen=new Set();let dups=0,exeN=0,wrongExe=0;
    for(const e of entries){const k=e.sec+'/'+e.name;
      if(seen.has(k))dups++;seen.add(k);
      if(e.sec===0){exeN++;if(e.name!=='game.exe')wrongExe++;}}
    if(dups)logO('w',`중복 경로 ${dups}개 — 추출 시 마지막 항목으로 덮어써집니다`);
    if(exeN===0)logO('w','game.exe 없음 — 규격상 루트에 game.exe 1개가 있어야 합니다');
    if(exeN>1)logO('w',`game.exe 엔트리 ${exeN}개 — 규격 위반 (1개여야 함)`);
    if(wrongExe)logO('w','EXE 섹션 엔트리 이름이 "game.exe"가 아님');
    if(file.size>indexOffset+indexSize)
      logO('w',`인덱스 뒤에 여분 바이트 ${file.size-(indexOffset+indexSize)}B`);
    const byBlock=Array.from({length:blockCount},()=>[]);
    entries.forEach(e=>byBlock[e.blockId].push(e));
    const pathMap=new Map(); /* 경로→엔트리 O(1) 인덱스 */
    entries.forEach((e,i)=>pathMap.set(pathOf(e),i));
    model={file,version,flags,entryCount,blockCount,indexOffset,indexSize,
      totalUnpacked,blocks,entries,byBlock,pathMap};
    $('oi-filter').value='';
    renderOpen();renderHex();
    logO('ok',`${file.name} 열림 — 파일 ${entryCount} · 블록 ${blockCount} · 인덱스${flags&1?' (압축)':''} · 매직/CRC 확인 완료`);
    setMsg('PACKAGE OPEN','ok');setLED('ok','OPEN');
  }catch(err){
    model=null;renderOpen();
    logO('err','열기 실패: '+err.message);
    setMsg('OPEN FAILED','err');setLED('err','ERROR');
  }
}
function pathOf(e){return e.sec===0?e.name:SECTIONS[e.sec].dir+'/'+e.name;}
function renderOpen(){
  const has=!!model;
  $('dropzone').style.display=has?'none':'block';
  $('openInfo').classList.toggle('on',has);
  $('tableWrap').innerHTML='';
  if(!has){if(activeTab==='open')setStatus(0,0,null,null);return;}
  const m=model;
  $('oi-name').textContent=m.file.name;
  const defN=m.blocks.filter(b=>b.method===M_DEFLATE).length;
  $('oi-meta').innerHTML=
    `<div class="chip">SIZE <b>${fmt(m.file.size)}</b></div>`+
    `<div class="chip">VERSION <b>v${m.version}</b></div>`+
    `<div class="chip">FILES <b>${m.entryCount}</b></div>`+
    `<div class="chip">BLOCKS <b>${m.blockCount}</b> (deflate-raw ${defN} · store ${m.blockCount-defN})</div>`+
    `<div class="chip">ORIGINAL <b>${fmt(m.totalUnpacked)}</b></div>`+
    `<div class="chip">INDEX @ <b>0x${m.indexOffset.toString(16).toUpperCase()}</b> (${m.indexSize} B)</div>`;
  $('btnExeDl').style.display=m.entries.some(e=>e.sec===0)?'':'none';
  renderTable();
  if(activeTab==='open')setStatus(m.entryCount,m.totalUnpacked,m.file.size,
    ratioStr(m.totalUnpacked,m.file.size));
}
function renderTable(){
  const wrap=$('tableWrap');
  if(!model){wrap.innerHTML='';return;}
  const m=model;
  const q=($('oi-filter').value||'').trim().toLowerCase();
  let html=`<table class="ftable"><thead><tr>
    <th>PATH</th><th>METHOD</th><th class="r">SIZE</th><th class="r">BLOCK</th><th class="r">CRC32</th><th></th>
    </tr></thead><tbody>`;
  let shown=0;
  for(let s=0;s<=4;s++){
    const idxs=[];m.entries.forEach((e,idx)=>{if(e.sec===s)idxs.push(idx);});
    const filtered=q?idxs.filter(i=>pathOf(m.entries[i]).toLowerCase().includes(q)):idxs;
    if(!filtered.length)continue;
    filtered.sort((a,b)=>m.entries[a].name<m.entries[b].name?-1:1);
    const sz=filtered.reduce((a,i)=>a+m.entries[i].size,0);
    const label=s===0?'ROOT · game.exe':`${SECTIONS[s].name} · ${SECTIONS[s].dir}/`;
    html+=`<tr class="grp"><td colspan="6" style="color:${SECTIONS[s].hex}">
      ${label} <span style="color:var(--faint);font-weight:400">— ${filtered.length} files · ${fmt(sz)}</span></td></tr>`;
    for(const i of filtered){
      const e=m.entries[i];const b=m.blocks[e.blockId];
      html+=`<tr class="file"><td class="fpath"><span class="dot" style="background:${SECTIONS[s].hex}"></span>${e.name.replace(/</g,'&lt;')}</td>
        <td><span class="m ${b.method?'def':'sto'}">${b.method?'DEFLATE-RAW':'STORE'}</span></td>
        <td class="r">${fmt(e.size)}</td>
        <td class="r" style="color:#626d82">#${e.blockId}</td>
        <td class="r crc">${e.crc.toString(16).padStart(8,'0').toUpperCase()}</td>
        <td class="r"><button class="btn mini" data-dl="${i}">↓ file</button></td></tr>`;
      shown++;
    }
  }
  if(!shown)html+=`<tr><td colspan="6" style="color:var(--faint);padding:18px">검색 결과 없음 — "${q.replace(/</g,'&lt;')}"</td></tr>`;
  html+='</tbody></table>';
  wrap.innerHTML=html;
  $('oi-shown').textContent=`${shown} / ${m.entryCount} entries`;
}
$('oi-filter').addEventListener('input',renderTable);
$('tableWrap').addEventListener('click',async e=>{
  const b=e.target.closest('button[data-dl]');if(!b||!model)return;
  const entry=model.entries[+b.dataset.dl];
  try{
    const bytes=await readEntryBytes(model,entry);
    const crc=crc32(bytes);
    if(crc!==entry.crc)logO('w',`CRC 불일치: ${pathOf(entry)} (손상 가능성 — 그대로 저장합니다)`);
    saveBlob(entry.name.split('/').pop(),new Blob([bytes]));
    logO('i',`개별 추출: ${pathOf(entry)} (${fmt(entry.size)}) — 원본 바이트 그대로`);
  }catch(err){logO('err',err.message);}
});

/* dropzone */
const dz=$('dropzone');
dz.onclick=()=>{const inp=document.createElement('input');inp.type='file';
  inp.accept='.jgp,application/x-jgp';
  inp.onchange=()=>{if(inp.files[0])openModel(inp.files[0]);};inp.click();};
dz.addEventListener('dragover',e=>{e.preventDefault();dz.classList.add('drag');});
dz.addEventListener('dragleave',()=>dz.classList.remove('drag'));
dz.addEventListener('drop',e=>{e.preventDefault();dz.classList.remove('drag');
  const f=[...(e.dataTransfer.files||[])].find(f=>/\.jgp$/i.test(f.name))||e.dataTransfer.files[0];
  if(f)openModel(f);});
$('btnClosePkg').onclick=()=>{model=null;clearBlockCache();$('oi-filter').value='';
  renderOpen();renderHex();setMsg('');setLED('','READY');};
$('btnExeDl').onclick=async()=>{
  if(!model)return;
  const e=model.entries.find(x=>x.sec===0);
  if(!e){logO('w','game.exe 엔트리가 없습니다');return;}
  try{
    const bytes=await readEntryBytes(model,e);
    if(crc32(bytes)!==e.crc)logO('w','game.exe CRC 불일치 — 손상 가능성');
    saveBlob('game.exe',new Blob([bytes]));
    logO('ok',`game.exe 추출 (${fmt(e.size)}) — 원본 바이트 그대로, 실행 가능`);
  }catch(err){logO('err',err.message);}
};

/* ── block / entry readers ─────────────── */
async function readBlockRaw(m,b){
  const chunk=new Uint8Array(await m.file.slice(b.off,b.off+b.packed).arrayBuffer());
  if(b.method===M_STORE){if(chunk.length!==b.unpacked)throw new Error('블록 크기 불일치 (손상)');return chunk;}
  if(!HAS_RAW_DEFLATE) throw new Error('DEFLATE 블록 해제 불가: 브라우저가 deflate-raw를 지원하지 않습니다');
  let raw;try{raw=await inflateRaw(chunk);}
  catch(e){throw new Error('DEFLATE 스트림 손상 또는 호환성 문제 — 압축 해제 실패');}
  if(raw.length!==b.unpacked)throw new Error('블록 복원 크기 불일치 (손상)');
  return raw;}
async function readEntryBytes(m,e){
  const b=m.blocks[e.blockId];
  if(b.method===M_STORE) /* STORE: 인덱스 오프셋으로 직접 slice — 블록 전체 읽기 불필요 */
    return new Uint8Array(await m.file.slice(b.off+e.off,b.off+e.off+e.size).arrayBuffer());
  if(blkCache.m===m&&blkCache.bi===e.blockId){
    const raw=blkCache.raw;
    if(e.off+e.size>raw.length)throw new Error('엔트리 범위 초과');
    return raw.subarray(e.off,e.off+e.size);}
  const raw=await readBlockRaw(m,b);
  blkCache={m,bi:e.blockId,raw};
  if(e.off+e.size>raw.length)throw new Error('엔트리 범위 초과');
  return raw.subarray(e.off,e.off+e.size);}

/* ── verify ────────────────────────────── */
async function verifyAll(){
  if(!model||busy)return;busy=true;setLED('busy','VERIFY');
  setProg('pw-o','pl-o','pb-o',0,'scanning…');
  let okF=0,badF=0,badB=0;
  try{
    for(let bi=0;bi<model.blocks.length;bi++){
      const b=model.blocks[bi];const list=model.byBlock[bi];
      try{
        const raw=await readBlockRaw(model,b);
        if(crc32(raw)!==b.crc){badB++;badF+=list.length;
          logO('err',`블록 #${bi}: 블록 CRC 불일치 — ${list.length}개 파일 손상`);}
        else for(const e of list){
          if(crc32(raw,e.off,e.off+e.size)===e.crc)okF++;
          else{badF++;logO('err',`CRC 불일치: ${pathOf(e)}`);}
        }
      }catch(err){badB++;badF+=list.length;
        logO('err',`블록 #${bi}: ${err.message}`);}
      setProg('pw-o','pl-o','pb-o',(bi+1)/model.blocks.length,
        `verify block ${bi+1}/${model.blocks.length}`);
      await tick();
    }
    if(badF===0){logO('ok',`무결성 검증 통과 — 파일 ${okF}개 전부 CRC32 OK`);setMsg('INTEGRITY OK','ok');setLED('ok','VERIFIED');}
    else{logO('err',`검증 완료: 정상 ${okF} · 손상 ${badF} (블록 ${badB}개)`);setMsg('CORRUPT DETECTED','err');setLED('err','CORRUPT');}
  }finally{busy=false;setTimeout(()=>hideProg('pw-o'),900);}
}
$('btnVerify').onclick=verifyAll;

/* ── extract: File System Access (원본 바이트 그대로 → game.exe 실행 가능) ── */
async function extractFS(m){
  const dir=await window.showDirectoryPicker({mode:'readwrite'});
  const rootName=baseName(m.file.name);
  const root=await dir.getDirectoryHandle(rootName,{create:true});
  /* 규격 구조 보장: 빈 섹션이어도 4개 디렉터리 항상 생성 */
  for(const d of['image','sound','code','data'])await root.getDirectoryHandle(d,{create:true});
  let done=0;const corrupt=[];
  for(let bi=0;bi<m.blocks.length;bi++){
    const b=m.blocks[bi];const list=m.byBlock[bi];
    const raw=b.method===M_STORE?null:await readBlockRaw(m,b);
    for(const e of list){
      let bytes;
      if(b.method===M_STORE)
        bytes=new Uint8Array(await m.file.slice(b.off+e.off,b.off+e.off+e.size).arrayBuffer());
      else bytes=raw.subarray(e.off,e.off+e.size);
      if(crc32(bytes)!==e.crc)corrupt.push(pathOf(e));
      const parts=pathOf(e).split('/');
      let d=root;
      for(const seg of parts.slice(0,-1))d=await d.getDirectoryHandle(seg,{create:true});
      const fh=await d.getFileHandle(parts[parts.length-1],{create:true});
      const w=await fh.createWritable();await w.write(bytes);await w.close();
      done++;
      setProg('pw-o','pl-o','pb-o',done/Math.max(1,m.entryCount),
        `extract ${done}/${m.entryCount} — ${pathOf(e)}`);
      if(done%8===0)await tick();
    }
  }
  logO('ok',`복원 완료 — 파일 ${done}개 → ${rootName}/ (game.exe · image/ · sound/ · code/ · data/)`);
  logO('ok',`전 파일 원본 바이트 그대로 복원(CRC32 검증) — ${rootName}/game.exe 실행 가능`);
  if(corrupt.length){corrupt.slice(0,10).forEach(p=>logO('err','CRC 불일치(손상): '+p));
    if(corrupt.length>10)logO('err',`… 외 ${corrupt.length-10}개`);}
  return rootName;
}
/* ── extract: minimal ZIP writer (STORE) — 범용 폴백. 바이트 정확 보존 ── */
async function extractZip(m){
  const enc=new TextEncoder();const rootName=baseName(m.file.name);
  const parts=[];const central=[];let off=0;let done=0;const corrupt=[];
  const dosT=0,dosD=0x0021; // 1980-01-01 고정 → 결정적 출력
  const addDir=name=>{ /* 디렉터리 엔트리 (빈 섹션 구조 보존용) */
    const nameB=enc.encode(name);
    const lh=new Uint8Array(30);const dv=new DataView(lh.buffer);
    dv.setUint32(0,0x04034b50,true);dv.setUint16(4,20,true);
    dv.setUint16(10,dosT,true);dv.setUint16(12,dosD,true);
    dv.setUint16(26,nameB.length,true);
    parts.push(lh,nameB);
    const ch=new Uint8Array(46);const cv=new DataView(ch.buffer);
    cv.setUint32(0,0x02014b50,true);cv.setUint16(4,20,true);cv.setUint16(6,20,true);
    cv.setUint16(10,0,true);cv.setUint16(12,dosT,true);cv.setUint16(14,dosD,true);
    cv.setUint16(28,nameB.length,true);cv.setUint32(38,0x10,true);cv.setUint32(42,off,true);
    central.push(ch,nameB);
    off+=30+nameB.length;};
  addDir(rootName+'/image/');addDir(rootName+'/sound/');
  addDir(rootName+'/code/');addDir(rootName+'/data/');
  for(let bi=0;bi<m.blocks.length;bi++){
    const b=m.blocks[bi];const list=m.byBlock[bi];
    const raw=b.method===M_STORE?null:await readBlockRaw(m,b);
    for(const e of list){
      const bytes=b.method===M_STORE
        ?new Uint8Array(await m.file.slice(b.off+e.off,b.off+e.off+e.size).arrayBuffer())
        :raw.subarray(e.off,e.off+e.size);
      if(crc32(bytes)!==e.crc)corrupt.push(pathOf(e));
      const nameB=enc.encode(rootName+'/'+pathOf(e));
      const lh=new Uint8Array(30);const dv=new DataView(lh.buffer);
      dv.setUint32(0,0x04034b50,true);dv.setUint16(4,20,true);
      dv.setUint16(8,0,true); /* method = STORE */
      dv.setUint16(10,dosT,true);dv.setUint16(12,dosD,true);
      dv.setUint32(14,e.crc,true);dv.setUint32(18,e.size,true);dv.setUint32(22,e.size,true);
      dv.setUint16(26,nameB.length,true);
      parts.push(lh,nameB,bytes);
      const ch=new Uint8Array(46);const cv=new DataView(ch.buffer);
      cv.setUint32(0,0x02014b50,true);cv.setUint16(4,20,true);cv.setUint16(6,20,true);
      cv.setUint16(10,0,true); /* method = STORE */
      cv.setUint16(12,dosT,true);cv.setUint16(14,dosD,true);
      cv.setUint32(16,e.crc,true);cv.setUint32(20,e.size,true);cv.setUint32(24,e.size,true);
      cv.setUint16(28,nameB.length,true);cv.setUint32(42,off,true);
      central.push(ch,nameB);
      off+=30+nameB.length+e.size;
      done++;
      setProg('pw-o','pl-o','pb-o',done/Math.max(1,m.entryCount),
        `zip ${done}/${m.entryCount} — ${pathOf(e)}`);
      if(done%8===0)await tick();
    }
  }
  const cdSize=central.reduce((a,p)=>a+p.length,0);
  const eocd=new Uint8Array(22);const ev=new DataView(eocd.buffer);
  ev.setUint32(0,0x06054b50,true);ev.setUint16(8,m.entryCount,true);ev.setUint16(10,m.entryCount,true);
  ev.setUint32(12,cdSize,true);ev.setUint32(16,off,true);
  const blob=new Blob([...parts,...central,eocd],{type:'application/zip'});
  saveBlob(rootName+'_unpacked.zip',blob);
  logO('ok',`ZIP 폴백 저장 — ${rootName}_unpacked.zip (${fmt(blob.size)}) · 파일 ${done}개 · STORE(바이트 정확)`);
  logO('ok',`ZIP 해제 후 ${rootName}/game.exe 실행 가능`);
  if(corrupt.length)logO('w',`CRC 불일치 파일 ${corrupt.length}개 포함됨`);
}
async function extractAll(forceZip){
  if(!model||busy)return;
  const canFS=typeof window.showDirectoryPicker==='function';
  if(!canFS&&!forceZip)logO('w','File System Access API 미지원 브라우저 — ZIP 폴백을 사용합니다');
  busy=true;setLED('busy','EXTRACT');
  setProg('pw-o','pl-o','pb-o',0,'starting…');
  try{
    if(!forceZip&&canFS){
      const rootName=await extractFS(model);
      setMsg('EXTRACTED → '+rootName+'/','ok');setLED('ok','EXTRACTED');
    }else{
      await extractZip(model);
      setMsg('ZIP SAVED','ok');setLED('ok','EXTRACTED');
    }
  }catch(err){
    if(err&&err.name==='AbortError'){logO('i','추출 취소됨');setLED('','READY');}
    else{logO('err','추출 실패: '+(err&&err.message||err));setMsg('EXTRACT FAILED','err');setLED('err','ERROR');}
  }finally{busy=false;setTimeout(()=>hideProg('pw-o'),900);}
}
$('btnExtract').onclick=()=>extractAll(false);
$('btnExtractZip').onclick=()=>extractAll(true);

/* ════════════════════════════════════════
   SPEC — live hexdump
   ════════════════════════════════════════ */
async function renderHex(){
  const el=$('hexdump');
  if(!model){el.textContent='— 패키지 미열림 (Open JGP 탭에서 .jgp를 열면 64바이트 헤더를 여기에 덤프합니다)';return;}
  const hb=new Uint8Array(await model.file.slice(0,64).arrayBuffer());
  let out='';
  for(let r=0;r<4;r++){
    const row=[...hb.subarray(r*16,r*16+16)];
    const hex=row.map(b=>b.toString(16).padStart(2,'0').toUpperCase()).join(' ');
    const asc=row.map(b=>(b>=32&&b<127)?String.fromCharCode(b):'·').join('');
    out+=(r*16).toString(16).padStart(4,'0').toUpperCase()+'  '+hex+'  |'+asc+'|\n';
  }
  const dv=new DataView(hb.buffer);
  out+=`\n magic=4A 47 50 01 ✓   version=${dv.getUint16(4,true)}   flags=0x${dv.getUint16(6,true).toString(16)}`
     +`\n entries=${dv.getUint32(8,true)}   blocks=${dv.getUint32(12,true)}`
     +`\n indexOffset=0x${Number(dv.getBigUint64(16,true)).toString(16).toUpperCase()}   indexSize=${Number(dv.getBigUint64(24,true))}`
     +`\n totalUnpacked=${fmt(Number(dv.getBigUint64(32,true)))}   headerCrc=0x${dv.getUint32(40,true).toString(16).padStart(8,'0').toUpperCase()}`;
  el.textContent=out;
}

/* ════════════════════════════════════════
   TABS / INIT
   ════════════════════════════════════════ */
function switchTab(name){
  activeTab=name;
  document.querySelectorAll('nav button').forEach(b=>b.classList.toggle('active',b.dataset.tab===name));
  document.querySelectorAll('.tabpanel').forEach(p=>p.classList.toggle('active',p.id==='tab-'+name));
  if(name==='create')updateCreateStats();
  else if(name==='open'&&model)setStatus(model.entryCount,model.totalUnpacked,model.file.size,
    ratioStr(model.totalUnpacked,model.file.size));
  else if(name==='open')setStatus(0,0,null,null);
}
document.querySelectorAll('nav button').forEach(b=>b.onclick=()=>switchTab(b.dataset.tab));

renderBuckets();
updateCreateStats();
logC('i','JGP PACKAGER v1 — 게임 배포 전용 바이너리 패키지 (ZIP 아님)');
logC('i','구성: game.exe · image/ · sound/ · code/ · data/ — 섹션 4개 + 실행파일, 그 외 섹션 없음');
logC('i','압축: 섹션 단위 솔리드 블록 + RAW DEFLATE(RFC 1951) · 무결성: CRC32 (파일/블록 단위)');
if(!HAS_RAW_DEFLATE)logC('err','⚠ 현재 브라우저는 deflate-raw를 지원하지 않습니다. CREATE 기능이 제한됩니다.');
setLED('','READY');
</script>
</body>
</html>```
