<!DOCTYPE html>
<html lang="ko">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>JTAudio (.jta) Studio v1.2</title>
<style>
  :root{
    --bg:#0b0d12; --panel:#12161f; --panel-2:#171c28; --border:#232a3a;
    --text:#e6e9f0; --muted:#8891a5; --accent:#7dd3c0; --accent-2:#8ea9ff;
    --warn:#f0a55a; --bad:#f0708a; --good:#7dd3a0;
    --mono:'SFMono-Regular',Consolas,'Liberation Mono',Menlo,monospace;
  }
  *{box-sizing:border-box;}
  body{ margin:0; background:var(--bg); color:var(--text);
    font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Pretendard,Roboto,sans-serif; min-height:100vh; }
  header{ padding:28px 32px 18px; border-bottom:1px solid var(--border);
    display:flex; align-items:baseline; gap:14px; flex-wrap:wrap; }
  header h1{ font-size:22px; margin:0; letter-spacing:.3px;} header h1 span{ color:var(--accent); }
  header p{ margin:0; color:var(--muted); font-size:13px; }
  .wrap{ max-width:1180px; margin:0 auto; padding:24px 32px 60px; }
  .grid{ display:grid; grid-template-columns:1fr 1fr; gap:20px; }
  @media (max-width:920px){ .grid{ grid-template-columns:1fr; } }
  .card{ background:var(--panel); border:1px solid var(--border); border-radius:14px; padding:20px 22px; margin-bottom:20px; }
  .card h2{ font-size:15px; margin:0 0 14px; display:flex; align-items:center; gap:8px; }
  .card h2 .tag{ font-size:10px; font-weight:700; letter-spacing:.5px; text-transform:uppercase; color:var(--accent);
    background:rgba(125,211,192,.1); border:1px solid rgba(125,211,192,.3); padding:2px 7px; border-radius:20px; }
  .drop{ border:1.5px dashed var(--border); border-radius:12px; padding:22px; text-align:center; color:var(--muted);
    font-size:13px; cursor:pointer; transition:.15s; background:var(--panel-2); }
  .drop:hover, .drop.drag{ border-color:var(--accent); color:var(--text); }
  input[type=file]{ display:none; }
  .row{ display:flex; gap:10px; flex-wrap:wrap; margin-top:12px; align-items:flex-end; }
  label.field{ font-size:12px; color:var(--muted); display:flex; flex-direction:column; gap:5px; flex:1; min-width:120px; }
  label.checkline{ font-size:12px; color:var(--muted); display:flex; align-items:center; gap:7px; flex-direction:row; }
  input[type=text], select, textarea{ background:var(--panel-2); border:1px solid var(--border); color:var(--text);
    padding:8px 10px; border-radius:8px; font-size:13px; outline:none; font-family:inherit; }
  textarea{ font-family:var(--mono); font-size:11.5px; resize:vertical; min-height:110px; width:100%; }
  input[type=text]:focus, select:focus, textarea:focus{ border-color:var(--accent); }
  button{ background:var(--accent); color:#06110f; border:none; border-radius:9px; padding:10px 16px;
    font-size:13px; font-weight:700; cursor:pointer; transition:.15s; letter-spacing:.2px; }
  button:hover{ filter:brightness(1.08); transform:translateY(-1px); }
  button:disabled{ background:#333a48; color:#6b7385; cursor:not-allowed; transform:none; }
  button.ghost{ background:transparent; color:var(--accent-2); border:1px solid var(--border); }
  button.ghost:hover{ border-color:var(--accent-2); }
  button.small{ padding:6px 11px; font-size:11.5px; }
  .stat-grid{ display:grid; grid-template-columns:repeat(auto-fit,minmax(120px,1fr)); gap:10px; margin-top:14px;}
  .stat{ background:var(--panel-2); border:1px solid var(--border); border-radius:10px; padding:12px 14px; }
  .stat .v{ font-size:19px; font-weight:700; font-family:var(--mono); }
  .stat .l{ font-size:11px; color:var(--muted); margin-top:2px; text-transform:uppercase; letter-spacing:.4px;}
  .good{ color:var(--good); } .warn{ color:var(--warn); } .bad{ color:var(--bad); }
  pre.preview{ background:#080a0f; border:1px solid var(--border); border-radius:10px; padding:14px;
    font-family:var(--mono); font-size:11px; line-height:1.55; color:#a6f0da;
    max-height:220px; overflow:auto; white-space:pre-wrap; word-break:break-all; margin-top:12px; }
  pre.preview.full{ max-height:520px; }
  table.meta{ width:100%; border-collapse:collapse; font-size:12.5px; margin-top:10px; }
  table.meta td{ padding:6px 4px; border-bottom:1px solid var(--border); }
  table.meta td:first-child{ color:var(--muted); width:38%; }
  .bar-outer{ height:8px; background:var(--panel-2); border-radius:5px; overflow:hidden; margin-top:8px; border:1px solid var(--border);}
  .bar-inner{ height:100%; background:linear-gradient(90deg,var(--accent),var(--accent-2)); width:0%; transition:width .4s; }
  audio{ width:100%; margin-top:12px; filter:invert(0.9) hue-rotate(180deg); border-radius:8px; }
  .muted{ color:var(--muted); font-size:12px; }
  .divider{ border-top:1px dashed var(--border); margin:16px 0; }
  .footer-note{ color:var(--muted); font-size:11.5px; margin-top:30px; line-height:1.6; border-top:1px solid var(--border); padding-top:16px;}
  .badge{ display:inline-block; padding:2px 8px; border-radius:20px; font-size:11px; font-weight:700; }
  .badge.ok{ background:rgba(125,211,160,.12); color:var(--good); border:1px solid rgba(125,211,160,.35); }
  .badge.no{ background:rgba(240,112,138,.12); color:var(--bad); border:1px solid rgba(240,112,138,.35); }
  .badge.info{ background:rgba(142,169,255,.12); color:var(--accent-2); border:1px solid rgba(142,169,255,.35); }
  .badge.warn2{ background:rgba(240,165,90,.12); color:var(--warn); border:1px solid rgba(240,165,90,.35); }
  .spec-toggle{ cursor:pointer; color:var(--accent-2); font-size:12.5px; user-select:none; }
  #specBody{ display:none; margin-top:14px; }
  #specBody h3{ font-size:13px; color:var(--accent); margin:18px 0 6px; }
  #specBody h4{ font-size:12.5px; color:var(--accent-2); margin:14px 0 4px; }
  #specBody p, #specBody li{ font-size:12.5px; color:#c3c9d6; line-height:1.7; }
  #specBody code{ background:var(--panel-2); padding:1px 6px; border-radius:5px; font-family:var(--mono); font-size:11.5px; color:var(--accent); }
  #specBody pre{ background:#080a0f; border:1px solid var(--border); padding:12px; border-radius:8px; overflow:auto; font-size:11.5px; color:#c3c9d6;}
  #specBody table{ width:100%; border-collapse:collapse; font-size:12px; margin:8px 0 14px;}
  #specBody table td, #specBody table th{ border:1px solid var(--border); padding:6px 8px; text-align:left; }
  #specBody table th{ color:var(--accent-2); background:var(--panel-2); }
</style>
</head>
<body>

<header>
  <h1>JT<span>Audio</span> Studio <span style="font-size:11px;color:var(--muted);font-weight:400;">v1.2</span></h1>
  <p>.jta — 바이너리 없이, 텍스트만으로 저장·전송하는 손실 오디오 포맷 · 압축률·보존성·편집 용이성 강화판</p>
</header>

<div class="wrap">

  <div class="grid">

    <!-- ENCODE -->
    <div class="card">
      <h2>1. 인코딩 <span class="tag">Audio → .jta</span></h2>
      <div class="drop" id="dropEncode">
        🎵 MP3 / WAV 파일을 여기로 드래그하거나 클릭해서 업로드
        <input type="file" id="fileInput" accept=".mp3,.wav,audio/*">
      </div>
      <div id="srcInfo" class="muted" style="margin-top:10px;"></div>

      <div class="row">
        <label class="field">제목
          <input type="text" id="metaTitle" placeholder="untitled">
        </label>
        <label class="field">아티스트
          <input type="text" id="metaArtist" placeholder="unknown">
        </label>
      </div>
      <div class="row">
        <label class="field">품질 (보존성 — 샘플 비트 해상도)
          <select id="qualitySel">
            <option value="6">매우 적음 · 6bit (용량 최소, 손실이 꽤 있음)</option>
            <option value="8">적음 · 8bit (손실이 있음)</option>
            <option value="10">중간 · 10bit (손실이 조금 있음)</option>
            <option value="12" selected>높음 · 12bit (권장, 손실이 거의 없음)</option>
            <option value="16">매우 높음 · 16bit (거의 다 저장, 용량이 많음)</option>
            <option value="20">무손실급 · 20bit (사실상 원본과 동일, 용량 최대)</option>
          </select>
        </label>
        <label class="field">청크 크기 (스트리밍 단위)
          <select id="chunkSel">
            <option value="1024">1024 샘플 (촘촘함, git diff 세밀)</option>
            <option value="2048" selected>2048 샘플 (권장)</option>
            <option value="4096">4096 샘플 (더 큰 청크)</option>
          </select>
        </label>
      </div>
      <div class="row">
        <label class="checkline"><input type="checkbox" id="noiseShapeChk"> 실험적 노이즈 셰이핑 사용 (기본 비활성 — 6-3절 참고, 측정상 RMS 손실률이 오히려 늘 수 있음)</label>
      </div>
      <div class="row">
        <button id="encodeBtn" disabled>⚙️ .jta 로 인코딩 (압축률 자동 최적화)</button>
        <button id="dlBtn" class="ghost" disabled>⬇ .jta 다운로드</button>
        <button id="copyBtn" class="ghost" disabled>📋 텍스트 복사</button>
      </div>

      <div id="encodeStats" style="display:none;">
        <div class="stat-grid">
          <div class="stat"><div class="v" id="statOrig">-</div><div class="l">원본 크기</div></div>
          <div class="stat"><div class="v" id="statJta">-</div><div class="l">.jta 크기</div></div>
          <div class="stat"><div class="v" id="statRatio">-</div><div class="l">압축비 (원본 대비)</div></div>
          <div class="stat"><div class="v" id="statLoss">-</div><div class="l">추정 손실률 (RMS)</div></div>
        </div>
        <div class="bar-outer"><div class="bar-inner" id="ratioBar"></div></div>
        <div class="muted" id="codecChoice" style="margin-top:10px;"></div>
        <div class="row" style="margin-top:6px;">
          <button class="ghost small" id="togglePreviewBtn">전체 보기</button>
          <span class="muted" id="lineCountLabel"></span>
        </div>
        <pre class="preview" id="jtaPreview"></pre>
      </div>
    </div>

    <!-- DECODE -->
    <div class="card">
      <h2>2. 디코딩 &amp; 재생 <span class="tag">.jta → PCM</span></h2>
      <div class="drop" id="dropDecode">
        📄 .jta 텍스트 파일을 여기로 드래그하거나 클릭해서 업로드
        <input type="file" id="jtaInput" accept=".jta,text/plain">
      </div>

      <div class="divider"></div>
      <div class="muted" style="margin-bottom:6px;">또는 .jta 텍스트를 아래에 직접 붙여넣기:</div>
      <textarea id="pasteArea" placeholder="#JTA1.0
[HEADER]
...
여기에 .jta 텍스트 전체를 붙여넣으세요"></textarea>
      <div class="row">
        <button id="pastePlayBtn">📋 붙여넣은 텍스트 재생</button>
        <button id="pasteClearBtn" class="ghost small">지우기</button>
      </div>

      <div class="row">
        <button id="decodeBtn" class="ghost" disabled>▶ 업로드한 파일 디코딩 후 재생</button>
      </div>
      <audio id="player" controls style="display:none;"></audio>

      <div id="decodeStats" style="display:none;">
        <div class="stat-grid">
          <div class="stat"><div class="v" id="dChecksum">-</div><div class="l">체크섬</div></div>
          <div class="stat"><div class="v" id="dChunks">-</div><div class="l">청크 수</div></div>
          <div class="stat"><div class="v" id="dDuration">-</div><div class="l">길이</div></div>
          <div class="stat"><div class="v" id="dSize">-</div><div class="l">복원 PCM 크기</div></div>
        </div>
        <table class="meta" id="metaTable"></table>
      </div>
    </div>
  </div>

  <div class="card">
    <div class="spec-toggle" id="specToggle">▸ JTAudio v1.2 규격 전체 문서 보기 / 접기</div>
    <div id="specBody"></div>
  </div>

  <div class="footer-note">
    JTAudio는 MP3의 대체제가 아닙니다 — 텍스트만 허용되는 환경(마크다운, JSON, 이메일, 채팅, Git 저장소,
    AI 프롬프트)에서 오디오를 손실 압축하여 담아 보내기 위한 규격입니다. v1.2는 압축률(82문자 알파벳,
    비용 기반 RLE, mid/side, 적응형 델타)과 편집 용이성(주석, 공백 허용 파싱)을 강화했고, 보존성은
    20bit "무손실급" 옵션으로 확장했습니다 — 기존 v1.0/v1.1 파일과 디코더는 전혀 영향받지 않습니다.
  </div>
</div>

<script>
/* ============================================================
   JTAudio v1.2 — Reference Codec (browser build)
   Ported verbatim from a Node.js module validated with a
   regression + measurement test suite (see companion spec doc,
   section 7, for the actual measured numbers).
   ============================================================ */
const ALPHABET64 =
  "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz.-";
const RESERVED_CHARS = new Set(['"', "'", '`', '\\', '#', '@', '[', ']', '~', '=', '^', '|']);
function buildAlphabet82(){
  let s = "";
  for (let code = 0x21; code <= 0x7E; code++){
    const ch = String.fromCharCode(code);
    if (!RESERVED_CHARS.has(ch)) s += ch;
  }
  return s;
}
const ALPHABET82 = buildAlphabet82();
const RLE_MARK = "~", REF_MARK = "=", CH_SEP = "|", ORDER2_MARK = "^";

function buildDigitTable(alphabet){ const val = {}; for (let i=0;i<alphabet.length;i++) val[alphabet[i]] = i; return val; }
const DIGIT_VAL_64 = buildDigitTable(ALPHABET64);
const DIGIT_VAL_82 = buildDigitTable(ALPHABET82);

const CODEC_TABLE = {
  "JTA-DPCM-RLE64":  { alphabet: ALPHABET64, digitVal: DIGIT_VAL_64, allowOrder2: false },
  "JTA-DPCM2-RLE64": { alphabet: ALPHABET64, digitVal: DIGIT_VAL_64, allowOrder2: true  },
  "JTA-DPCM2-RLE82": { alphabet: ALPHABET82, digitVal: DIGIT_VAL_82, allowOrder2: true  },
};
const KNOWN_STEREO_MODES = new Set(["discrete", "mid_side"]);

function makeVarintCodec(alphabet, digitVal){
  const N = alphabet.length, half = N >> 1, bits = Math.log2(half);
  function encodeVarint(uint){
    let out = ""; let v = uint >>> 0;
    do{ let chunk = v % half; v = Math.floor(v/half); if (v > 0) chunk += half; out += alphabet[chunk]; } while (v > 0);
    return out;
  }
  function decodeVarint(str, pos){
    let result = 0, mult = 1, p = pos;
    while (true){
      const d = digitVal[str[p++]];
      const cont = d >= half;
      const payload = cont ? d - half : d;
      result += payload*mult; mult *= half;
      if (!cont) break;
    }
    return [result >>> 0, p];
  }
  return { encodeVarint, decodeVarint, half, bitsPerChar: bits };
}
const VARINT64 = makeVarintCodec(ALPHABET64, DIGIT_VAL_64);
const VARINT82 = makeVarintCodec(ALPHABET82, DIGIT_VAL_82);

function zigzag(n){ return n >= 0 ? n*2 : -n*2 - 1; }
function unzigzag(u){ return (u % 2 === 0) ? u/2 : -(u+1)/2; }

function quantize(float32, bits){
  const max = (1 << (bits-1)) - 1;
  const out = new Int32Array(float32.length);
  for (let i=0;i<float32.length;i++){ let s = Math.max(-1, Math.min(1, float32[i])); out[i] = Math.round(s*max); }
  return out;
}
function quantizeNoiseShaped(float32, bits){
  const max = (1 << (bits-1)) - 1;
  const out = new Int32Array(float32.length);
  let error = 0;
  for (let i=0;i<float32.length;i++){
    let target = float32[i] + error;
    target = Math.max(-1, Math.min(1, target));
    const q = Math.round(target*max);
    out[i] = q;
    error = target - (q/max);
  }
  return out;
}
function dequantize(ints, bits){
  const max = (1 << (bits-1)) - 1;
  const out = new Float32Array(ints.length);
  for (let i=0;i<ints.length;i++) out[i] = ints[i]/max;
  return out;
}

function encodeDiffs(diffs, varint){
  let out = ""; let i = 0;
  while (i < diffs.length){
    const d = diffs[i];
    let runLen = 1, j = i+1;
    while (j < diffs.length && diffs[j] === d){ runLen++; j++; }
    const single = varint.encodeVarint(zigzag(d));
    const individualCost = single.length * runLen;
    let rleCost = Infinity;
    if (runLen >= 2) rleCost = 1 + varint.encodeVarint(runLen).length + single.length;
    if (rleCost < individualCost) out += RLE_MARK + varint.encodeVarint(runLen) + single;
    else out += single.repeat(runLen);
    i += runLen;
  }
  return out;
}
function decodeDiffs(str, count, pos, varint){
  const diffs = new Array(count); let n = 0;
  while (n < count){
    if (str[pos] === RLE_MARK){
      pos++; let runLen, dz;
      [runLen, pos] = varint.decodeVarint(str, pos); [dz, pos] = varint.decodeVarint(str, pos);
      const d = unzigzag(dz);
      for (let k=0;k<runLen;k++) diffs[n++] = d;
    } else { let z; [z, pos] = varint.decodeVarint(str, pos); diffs[n++] = unzigzag(z); }
  }
  return { diffs, pos };
}

function order1Diffs(ints, prevSample){
  const diffs = new Array(ints.length); let prev = prevSample;
  for (let i=0;i<ints.length;i++){ diffs[i] = ints[i]-prev; prev = ints[i]; }
  return diffs;
}
function order1Reconstruct(diffs, prevSample){
  const out = new Int32Array(diffs.length); let prev = prevSample;
  for (let i=0;i<diffs.length;i++){ prev = prev + diffs[i]; out[i] = prev; }
  return out;
}
function order2Diffs(ints, prevSample, prevPrevSample){
  const diffs = new Array(ints.length);
  let lastDelta = prevSample - prevPrevSample, prevA = prevSample;
  for (let i=0;i<ints.length;i++){
    const delta = ints[i]-prevA;
    diffs[i] = delta - lastDelta;
    lastDelta = delta; prevA = ints[i];
  }
  return diffs;
}
function order2Reconstruct(diffs, prevSample, prevPrevSample){
  const out = new Int32Array(diffs.length);
  let lastDelta = prevSample - prevPrevSample, prevA = prevSample;
  for (let i=0;i<diffs.length;i++){
    const delta = lastDelta + diffs[i];
    const sample = prevA + delta;
    out[i] = sample; lastDelta = delta; prevA = sample;
  }
  return out;
}

function encodeChannelChunk(ints, prevSample, prevPrevSample, allowOrder2, varint){
  const d1 = order1Diffs(ints, prevSample);
  const t1 = encodeDiffs(d1, varint);
  let best = t1, bestIsOrder2 = false;
  if (allowOrder2){
    const d2 = order2Diffs(ints, prevSample, prevPrevSample);
    const t2 = encodeDiffs(d2, varint);
    if (t2.length < t1.length){ best = t2; bestIsOrder2 = true; }
  }
  const text = (bestIsOrder2 ? ORDER2_MARK : "") + best;
  const lastSample = ints.length ? ints[ints.length-1] : prevSample;
  const lastPrevSample = ints.length >= 2 ? ints[ints.length-2] : prevSample;
  return { text, lastSample, lastPrevSample };
}
function decodeChannelChunk(str, count, prevSample, prevPrevSample, varint){
  let pos = 0, order2 = false;
  if (str[0] === ORDER2_MARK){ order2 = true; pos = 1; }
  const { diffs } = decodeDiffs(str, count, pos, varint);
  const samples = order2 ? order2Reconstruct(diffs, prevSample, prevPrevSample) : order1Reconstruct(diffs, prevSample);
  const lastSample = count ? samples[count-1] : prevSample;
  const lastPrevSample = count >= 2 ? samples[count-2] : prevSample;
  return { samples, lastSample, lastPrevSample };
}

function toMidSide(L, R){
  const M = new Int32Array(L.length), S = new Int32Array(L.length);
  for (let i=0;i<L.length;i++){ const side = L[i]-R[i]; const mid = R[i] + (side>>1); M[i]=mid; S[i]=side; }
  return [M, S];
}
function fromMidSide(M, S){
  const L = new Int32Array(M.length), R = new Int32Array(M.length);
  for (let i=0;i<M.length;i++){ const side = S[i]; const r = M[i]-(side>>1); const l = side+r; L[i]=l; R[i]=r; }
  return [L, R];
}

const CRC_TABLE = (() => {
  const t = new Uint32Array(256);
  for (let n=0;n<256;n++){ let c=n; for (let k=0;k<8;k++) c = (c&1) ? (0xEDB88320 ^ (c>>>1)) : (c>>>1); t[n]=c>>>0; }
  return t;
})();
function crc32(str){
  let c = 0xFFFFFFFF;
  for (let i=0;i<str.length;i++){ const byte = str.charCodeAt(i)&0xff; c = CRC_TABLE[(c^byte)&0xff] ^ (c>>>8); }
  return ((c ^ 0xFFFFFFFF) >>> 0).toString(16).padStart(8,"0").toUpperCase();
}
function normalizeLineEndings(text){ return text.replace(/\r\n/g, "\n"); }

function parseKeyValueBlock(block){
  const obj = {};
  for (const rawLine of block.split("\n")){
    const line = rawLine.trim();
    if (line === "" || line.startsWith("#")) continue;
    const i = line.indexOf("=");
    if (i === -1) continue;
    obj[line.slice(0,i).trim()] = line.slice(i+1).trim();
  }
  return obj;
}

function encodeJTA(channelsIn, opts){
  const sampleRate = opts.sampleRate;
  const bits = opts.bits || 12;
  const chunkSamples = opts.chunkSamples || 2048;
  const codec = opts.codec || "JTA-DPCM-RLE64";
  const stereoMode = opts.stereoMode || "discrete";
  const quantizeMode = opts.quantizeMode || "direct";
  if (!CODEC_TABLE[codec]) throw new Error("unknown codec (encoder): " + codec);
  if (!KNOWN_STEREO_MODES.has(stereoMode)) throw new Error("unknown stereo_mode (encoder): " + stereoMode);
  if (stereoMode === "mid_side" && channelsIn.length !== 2) throw new Error("stereo_mode=mid_side requires exactly 2 channels");
  const { allowOrder2, alphabet } = CODEC_TABLE[codec];
  const varint = alphabet === ALPHABET82 ? VARINT82 : VARINT64;

  const totalSamples = channelsIn[0].length;
  const nCh = channelsIn.length;
  const qFn = quantizeMode === "noise_shaped" ? quantizeNoiseShaped : quantize;
  let qch = channelsIn.map(c => qFn(c, bits));
  if (stereoMode === "mid_side") qch = toMidSide(qch[0], qch[1]);

  const chunkLines = [];
  const seen = new Map();
  let prev = new Array(nCh).fill(0), prevPrev = new Array(nCh).fill(0);
  let idx = 0;
  for (let offset=0; offset<totalSamples; offset+=chunkSamples){
    const len = Math.min(chunkSamples, totalSamples-offset);
    const parts = [];
    for (let ch=0; ch<nCh; ch++){
      const slice = qch[ch].subarray(offset, offset+len);
      const { text, lastSample, lastPrevSample } = encodeChannelChunk(slice, prev[ch], prevPrev[ch], allowOrder2, varint);
      parts.push(text);
      prevPrev[ch] = lastPrevSample; prev[ch] = lastSample;
    }
    const payload = parts.join(CH_SEP);
    let line;
    if (seen.has(payload)) line = REF_MARK + seen.get(payload);
    else { seen.set(payload, idx); line = payload; }
    const t0 = (offset/sampleRate).toFixed(3);
    const t1 = (Math.min(offset+len, totalSamples)/sampleRate).toFixed(3);
    chunkLines.push(`@CHUNK ${idx} samples=${len} time=${t0}-${t1}\n${line}`);
    idx++;
  }

  const streamBody = normalizeLineEndings(chunkLines.join("\n"));
  const streamCrc = crc32(streamBody);
  const meta = opts.meta || {};
  const metaLines = Object.entries(meta).map(([k,v]) => `${k}=${v}`).join("\n");

  const header = [
    `#JTA1.0`, `[HEADER]`,
    `title=${meta.title || "untitled"}`,
    `sample_rate=${sampleRate}`,
    `channels=${nCh}`,
    `bit_depth=${bits}`,
    `chunk_samples=${chunkSamples}`,
    `total_samples=${totalSamples}`,
    `duration=${(totalSamples/sampleRate).toFixed(3)}`,
    `codec=${codec}`,
    `stereo_mode=${stereoMode}`,
    `quantize_mode=${quantizeMode}`,
    `loss_target=2.0`,
    `created=${new Date().toISOString()}`,
    `[/HEADER]`
  ].join("\n");
  const metaBlock = [`[META]`, metaLines, `[/META]`].join("\n");
  const streamBlock = [`[STREAM]`, streamBody, `[/STREAM]`].join("\n");
  const endBlock = [`[END]`, `chunks=${idx}`, `total_samples=${totalSamples}`, `checksum=CRC32:${streamCrc}`, `[/END]`].join("\n");

  return [header, metaBlock, streamBlock, endBlock].join("\n\n") + "\n";
}

function parseBlock(text, name){
  const start = text.indexOf(`[${name}]`);
  const end = text.indexOf(`[/${name}]`);
  if (start === -1 || end === -1) throw new Error(`.jta 파일 구조가 올바르지 않습니다: [${name}] 블록을 찾을 수 없습니다`);
  return text.slice(start + name.length + 2, end).trim();
}

function decodeJTA(textRaw){
  const text = normalizeLineEndings(textRaw);
  if (!text.startsWith("#JTA1.0")) throw new Error("지원하지 않는 JTAudio 버전입니다 (알 수 없는 매직 문자열)");
  const headerBlock = parseBlock(text, "HEADER");
  const metaBlock = parseBlock(text, "META");
  const streamBlock = parseBlock(text, "STREAM");
  const endBlock = parseBlock(text, "END");

  const header = parseKeyValueBlock(headerBlock);
  const meta = parseKeyValueBlock(metaBlock);
  const end = parseKeyValueBlock(endBlock);

  const codec = header.codec || "JTA-DPCM-RLE64";
  const stereoMode = header.stereo_mode || "discrete";
  if (!CODEC_TABLE[codec]) throw new Error(`지원하지 않는 codec입니다: "${codec}". 이 디코더는 ${Object.keys(CODEC_TABLE).join(", ")}만 지원합니다.`);
  if (!KNOWN_STEREO_MODES.has(stereoMode)) throw new Error(`지원하지 않는 stereo_mode입니다: "${stereoMode}".`);
  const { alphabet } = CODEC_TABLE[codec];
  const varint = alphabet === ALPHABET82 ? VARINT82 : VARINT64;

  const expectedCrc = (end.checksum || "").replace("CRC32:", "");
  const actualCrc = crc32(streamBlock);
  const checksumOk = expectedCrc === actualCrc;

  const sampleRate = parseInt(header.sample_rate,10);
  const nCh = parseInt(header.channels,10);
  const bits = parseInt(header.bit_depth,10);
  const totalSamples = parseInt(header.total_samples,10);
  if (stereoMode === "mid_side" && nCh !== 2) throw new Error("stereo_mode=mid_side인데 channels가 2가 아닙니다");

  const channels = [];
  for (let c=0;c<nCh;c++) channels.push(new Int32Array(totalSamples));

  const lines = streamBlock.split("\n");
  const chunkPayloads = [];
  let prev = new Array(nCh).fill(0), prevPrev = new Array(nCh).fill(0);
  let writeOffset = 0, i = 0;
  while (i < lines.length){
    const marker = lines[i];
    if (!marker.startsWith("@CHUNK")){ i++; continue; }
    const m = marker.match(/@CHUNK (\d+) samples=(\d+) time=/);
    const chunkIdx = parseInt(m[1],10);
    const len = parseInt(m[2],10);
    const payloadLine = lines[i+1];
    let payload;
    if (payloadLine.startsWith(REF_MARK)){
      const refIdx = parseInt(payloadLine.slice(1),10);
      if (!(refIdx >= 0 && refIdx < chunkIdx) || chunkPayloads[refIdx] === undefined){
        throw new Error(`청크 ${chunkIdx}의 백레퍼런스가 유효하지 않습니다 (참조 대상=${refIdx})`);
      }
      payload = chunkPayloads[refIdx];
    } else payload = payloadLine;
    chunkPayloads[chunkIdx] = payload;

    const parts = payload.split(CH_SEP);
    for (let ch=0; ch<nCh; ch++){
      const { samples, lastSample, lastPrevSample } = decodeChannelChunk(parts[ch], len, prev[ch], prevPrev[ch], varint);
      channels[ch].set(samples, writeOffset);
      prevPrev[ch] = lastPrevSample; prev[ch] = lastSample;
    }
    writeOffset += len;
    i += 2;
  }

  let intChannels = channels;
  if (stereoMode === "mid_side") intChannels = fromMidSide(channels[0], channels[1]);

  const floatChannels = intChannels.map(c => dequantize(c, bits));
  return { sampleRate, channels: floatChannels, header, meta, end, checksumOk,
           chunkCount: parseInt(end.chunks,10), codec, stereoMode,
           quantizeMode: header.quantize_mode || "direct" };
}

/* ============================================================
   UI wiring
   ============================================================ */
const audioCtx = new (window.AudioContext || window.webkitAudioContext)();
let sourceBuffer = null, sourceFile = null;
let lastJtaText = null, lastJtaMeta = null, previewExpanded = false;

function fmtBytes(n){
  if (n < 1024) return n + " B";
  if (n < 1024*1024) return (n/1024).toFixed(1) + " KB";
  return (n/1024/1024).toFixed(2) + " MB";
}
function wireDrop(dropEl, inputEl, onFile){
  dropEl.addEventListener("click", () => inputEl.click());
  inputEl.addEventListener("change", e => { if (e.target.files[0]) onFile(e.target.files[0]); });
  ["dragover","dragenter"].forEach(ev => dropEl.addEventListener(ev, e => { e.preventDefault(); dropEl.classList.add("drag"); }));
  ["dragleave","drop"].forEach(ev => dropEl.addEventListener(ev, e => { e.preventDefault(); dropEl.classList.remove("drag"); }));
  dropEl.addEventListener("drop", e => { if (e.dataTransfer.files[0]) onFile(e.dataTransfer.files[0]); });
}

wireDrop(document.getElementById("dropEncode"), document.getElementById("fileInput"), async file => {
  sourceFile = file;
  const arrBuf = await file.arrayBuffer();
  try{ sourceBuffer = await audioCtx.decodeAudioData(arrBuf.slice(0)); }
  catch(err){ document.getElementById("srcInfo").textContent = "⚠ 디코딩 실패: 지원되지 않는 오디오 형식일 수 있습니다."; return; }
  document.getElementById("srcInfo").innerHTML =
    `<b>${file.name}</b> · ${fmtBytes(file.size)} · ${sourceBuffer.sampleRate}Hz · ` +
    `${sourceBuffer.numberOfChannels}ch · ${sourceBuffer.duration.toFixed(2)}s`;
  document.getElementById("encodeBtn").disabled = false;
});

document.getElementById("encodeBtn").addEventListener("click", () => {
  if (!sourceBuffer) return;
  const bits = parseInt(document.getElementById("qualitySel").value, 10);
  const chunkSamples = parseInt(document.getElementById("chunkSel").value, 10);
  const useNoiseShaping = document.getElementById("noiseShapeChk").checked;
  const title = document.getElementById("metaTitle").value.trim() || (sourceFile ? sourceFile.name.replace(/\.[^.]+$/,'') : "untitled");
  const artist = document.getElementById("metaArtist").value.trim() || "unknown";

  const channels = [];
  for (let c=0;c<sourceBuffer.numberOfChannels;c++) channels.push(sourceBuffer.getChannelData(c));
  const meta = { title, artist, source_file: sourceFile ? sourceFile.name : "unknown" };
  const baseOpts = {
    sampleRate: sourceBuffer.sampleRate, bits, chunkSamples,
    codec: "JTA-DPCM2-RLE82", quantizeMode: useNoiseShaping ? "noise_shaped" : "direct", meta
  };

  // v1.2 compression upgrade: try discrete vs mid_side (stereo only) and
  // keep whichever is smaller -- measured per-file, never assumed.
  let bestText = encodeJTA(channels, { ...baseOpts, stereoMode: "discrete" });
  let chosenMode = "discrete";
  if (channels.length === 2){
    const msText = encodeJTA(channels, { ...baseOpts, stereoMode: "mid_side" });
    if (msText.length < bestText.length){ bestText = msText; chosenMode = "mid_side"; }
  }
  const jtaText = bestText;

  const decoded = decodeJTA(jtaText);
  let sumSq=0, sumRef=0;
  const ref = channels[0], got = decoded.channels[0];
  for (let i=0;i<ref.length;i++){ const e=ref[i]-got[i]; sumSq+=e*e; sumRef+=ref[i]*ref[i]; }
  const rmsPct = sumRef>0 ? 100*Math.sqrt(sumSq/sumRef) : 0;

  lastJtaText = jtaText;
  lastJtaMeta = { title, artist };

  const origBytes = sourceFile.size;
  const jtaBytes = new Blob([jtaText]).size;
  const ratio = jtaBytes / origBytes;

  document.getElementById("statOrig").textContent = fmtBytes(origBytes);
  document.getElementById("statJta").textContent = fmtBytes(jtaBytes);
  const ratioEl = document.getElementById("statRatio");
  ratioEl.textContent = ratio.toFixed(2) + "x";
  ratioEl.className = "v " + (ratio <= 1 ? "good" : ratio <= 3 ? "warn" : "bad");
  const lossEl = document.getElementById("statLoss");
  lossEl.textContent = rmsPct.toFixed(3) + "%";
  lossEl.className = "v " + (rmsPct <= 2 ? "good" : rmsPct <= 5 ? "warn" : "bad");
  document.getElementById("ratioBar").style.width = Math.min(100, ratio*33) + "%";

  document.getElementById("codecChoice").innerHTML =
    `<span class="badge info">codec=JTA-DPCM2-RLE82</span> ` +
    `<span class="badge info">stereo_mode=${chosenMode}</span> ` +
    (useNoiseShaping ? `<span class="badge warn2">quantize_mode=noise_shaped (실험적)</span> ` : ``) +
    `— 채널·청크마다 1차/2차 델타, 82문자 알파벳, 비용 기반 RLE, 백레퍼런스를 모두 적용하고` +
    (channels.length===2 ? ` discrete/mid_side도 실제 인코딩 후 더 작은 쪽을 선택했습니다.` : ` 최선의 결과를 선택했습니다.`);

  previewExpanded = false;
  updatePreview();
  document.getElementById("encodeStats").style.display = "block";
  document.getElementById("dlBtn").disabled = false;
  document.getElementById("copyBtn").disabled = false;
});

function updatePreview(){
  const lines = lastJtaText.split("\n");
  const pre = document.getElementById("jtaPreview");
  const btn = document.getElementById("togglePreviewBtn");
  if (previewExpanded){
    pre.textContent = lastJtaText;
    pre.classList.add("full");
    btn.textContent = "미리보기만 보기";
  } else {
    pre.textContent = lines.slice(0,14).join("\n") + (lines.length>14 ? "\n... (미리보기)" : "");
    pre.classList.remove("full");
    btn.textContent = "전체 보기";
  }
  document.getElementById("lineCountLabel").textContent = `전체 ${lines.length}줄 / ${fmtBytes(new Blob([lastJtaText]).size)}`;
}
document.getElementById("togglePreviewBtn").addEventListener("click", () => { previewExpanded = !previewExpanded; updatePreview(); });

document.getElementById("dlBtn").addEventListener("click", () => {
  if (!lastJtaText) return;
  const blob = new Blob([lastJtaText], { type: "text/plain;charset=utf-8" });
  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = (lastJtaMeta.title || "output").replace(/[^\w\-]+/g,"_") + ".jta";
  a.click();
  URL.revokeObjectURL(url);
});
document.getElementById("copyBtn").addEventListener("click", async () => {
  if (!lastJtaText) return;
  try{
    await navigator.clipboard.writeText(lastJtaText);
    const btn = document.getElementById("copyBtn");
    const orig = btn.textContent;
    btn.textContent = "✓ 복사됨";
    setTimeout(() => btn.textContent = orig, 1400);
  }catch(err){ alert("클립보드 복사 실패: 브라우저 권한을 확인해주세요."); }
});

function renderDecoded(decoded){
  const nCh = decoded.channels.length;
  const len = decoded.channels[0].length;
  const buf = audioCtx.createBuffer(nCh, len, decoded.sampleRate);
  for (let c=0;c<nCh;c++) buf.copyToChannel(decoded.channels[c], c);

  const wavBlob = bufferToWavBlob(buf);
  const url = URL.createObjectURL(wavBlob);
  const player = document.getElementById("player");
  player.src = url;
  player.style.display = "block";
  player.play().catch(()=>{});

  document.getElementById("dChecksum").innerHTML = decoded.checksumOk
    ? '<span class="badge ok">✓ 유효</span>' : '<span class="badge no">✗ 손상됨</span>';
  document.getElementById("dChunks").textContent = decoded.chunkCount;
  document.getElementById("dDuration").textContent = decoded.header.duration + "s";
  document.getElementById("dSize").textContent = fmtBytes(len*nCh*4);

  const metaTable = document.getElementById("metaTable");
  metaTable.innerHTML = "";
  const rows = [
    ["제목", decoded.meta.title], ["아티스트", decoded.meta.artist], ["원본 파일", decoded.meta.source_file],
    ["샘플레이트", decoded.header.sample_rate + " Hz"], ["채널", decoded.header.channels],
    ["비트 해상도", decoded.header.bit_depth + " bit"], ["코덱", decoded.codec], ["스테레오 모드", decoded.stereoMode],
    ["양자화 모드", decoded.quantizeMode], ["생성 시각", decoded.header.created],
  ];
  for (const [k,v] of rows){
    if (!v) continue;
    const tr = document.createElement("tr");
    tr.innerHTML = `<td>${k}</td><td>${v}</td>`;
    metaTable.appendChild(tr);
  }
  document.getElementById("decodeStats").style.display = "block";
}

let uploadedJtaText = null;
wireDrop(document.getElementById("dropDecode"), document.getElementById("jtaInput"), async file => {
  uploadedJtaText = await file.text();
  document.getElementById("decodeBtn").disabled = false;
  document.getElementById("decodeBtn").textContent = `▶ "${file.name}" 디코딩 후 재생`;
});
document.getElementById("decodeBtn").addEventListener("click", () => {
  if (!uploadedJtaText) return;
  try{ renderDecoded(decodeJTA(uploadedJtaText)); }
  catch(err){ alert("디코딩 실패: " + err.message); }
});
document.getElementById("pastePlayBtn").addEventListener("click", () => {
  const text = document.getElementById("pasteArea").value;
  if (!text.trim()) { alert(".jta 텍스트를 먼저 붙여넣어주세요."); return; }
  try{ renderDecoded(decodeJTA(text)); }
  catch(err){ alert("디코딩 실패: " + err.message); }
});
document.getElementById("pasteClearBtn").addEventListener("click", () => { document.getElementById("pasteArea").value = ""; });

function bufferToWavBlob(buf){
  const nCh = buf.numberOfChannels, len = buf.length, sr = buf.sampleRate;
  const bytesPerSample = 2, blockAlign = nCh * bytesPerSample, dataSize = len * blockAlign;
  const ab = new ArrayBuffer(44 + dataSize);
  const view = new DataView(ab);
  function writeStr(offset, str){ for (let i=0;i<str.length;i++) view.setUint8(offset+i, str.charCodeAt(i)); }
  writeStr(0,"RIFF"); view.setUint32(4, 36+dataSize, true); writeStr(8,"WAVE");
  writeStr(12,"fmt "); view.setUint32(16,16,true); view.setUint16(20,1,true);
  view.setUint16(22,nCh,true); view.setUint32(24,sr,true);
  view.setUint32(28, sr*blockAlign, true); view.setUint16(32, blockAlign, true); view.setUint16(34,16,true);
  writeStr(36,"data"); view.setUint32(40,dataSize,true);
  let offset = 44;
  const chData = []; for (let c=0;c<nCh;c++) chData.push(buf.getChannelData(c));
  for (let i=0;i<len;i++){
    for (let c=0;c<nCh;c++){
      let s = Math.max(-1, Math.min(1, chData[c][i]));
      view.setInt16(offset, s<0 ? s*0x8000 : s*0x7FFF, true);
      offset += 2;
    }
  }
  return new Blob([ab], { type: "audio/wav" });
}

document.getElementById("specToggle").addEventListener("click", () => {
  const el = document.getElementById("specBody");
  el.style.display = el.style.display === "block" ? "none" : "block";
});
document.getElementById("specBody").innerHTML = `
<h3>1. v1.2에서 무엇이 바뀌었나</h3>
<table>
<tr><th>영역</th><th>변경</th><th>영향</th></tr>
<tr><td>압축률</td><td>82문자 알파벳(codec=JTA-DPCM2-RLE82), 비용 기반 RLE</td><td>실측 6~40% 추가 절감(신호에 따라 다름)</td></tr>
<tr><td>보존성</td><td>bit_depth 20 옵션(무손실급) 추가</td><td>손실을 사실상 0에 근접시킴</td></tr>
<tr><td>편집 용이성</td><td>주석(#) 문법, 공백 허용 key=value 파싱</td><td>사람·AI가 헤더를 정렬·주석 처리 가능</td></tr>
<tr><td>실험적 기능</td><td>quantize_mode=noise_shaped</td><td>측정 결과 RMS 개선 미확인 — 기본 비활성</td></tr>
</table>

<h3>2. 82문자 알파벳</h3>
<p>인쇄 가능한 ASCII(0x21~0x7E, 94자)에서 따옴표류(<code>" ' \` \\</code>), 구조 마커(<code># @ [ ]</code>),
예약 기호(<code>~ = ^ |</code>) 12개를 제외한 82개 문자를 오름차순으로 사용합니다. 문자당 페이로드가
5bit→약 5.36bit로 늘어 같은 데이터를 더 적은 문자로 표현합니다. 220Hz 순음 테스트에서
RLE64 대비 <b>6.25% 추가 절감</b>을 확인했습니다.</p>

<h3>3. 비용 기반 RLE</h3>
<p>v1.1까지는 "델타가 4회 이상 반복되면 무조건 RLE" 고정 규칙을 썼습니다. 이는 델타값이 2문자 이상
필요한 경우 비효율적이었습니다(3회 반복 시 개별 인코딩 6글자 vs RLE 4글자인데도 RLE를 안 씀).
v1.2는 매 반복 구간마다 실제 글자 수를 계산해 항상 더 짧은 쪽을 선택합니다. RLE 토큰 형식 자체는
그대로라 디코더 변경은 필요 없습니다. 실측: 반복 구간 하나당 최대 33% 절감(신호에 따라 다름, 반복이
없으면 개선 없음/손해 없음).</p>

<h3>4. mid/side + 적응형 델타 (v1.1에서 계승, v1.2 알파벳과 결합)</h3>
<p>상관된 스테레오 220~880Hz 합성음 기준, discrete → mid_side + 적응형 2차 델타 + RLE82 + 비용 기반
RLE를 모두 적용하면 v1.0 대비 <b>40.7% 절감</b>을 실측했습니다. 인코더는 discrete/mid_side를 둘 다
실제로 인코딩해 본 뒤 더 작은 쪽을 선택하므로, 상관관계가 낮은 신호에서 mid_side가 불리하면 자동으로
discrete가 선택됩니다.</p>

<h3>5. 보존성: bit_depth와 quantize_mode</h3>
<p>손실은 오직 bit_depth(양자화)에서만 발생하며, 위의 압축 개선들은 엔트로피 코딩 단계이므로
손실률에 영향을 주지 않습니다(mid_side 적용 후에도 RMS 오차가 순수 양자화 오차 수준인 0.05~0.06%로
유지됨을 확인). 20bit "무손실급" 옵션을 추가해 원본에 가장 가깝게 저장할 수 있게 했습니다.</p>
<p><b>실험적 quantize_mode=noise_shaped</b>: 1차 오차 피드백 양자화를 구현했으나, 300+700Hz 합성음
8bit 기준 실측 결과 RMS 오차가 direct 0.98% → noise_shaped 1.38%로 오히려 늘었습니다. 이론상
노이즈 셰이핑은 "오차 총량"이 아니라 "오차의 주파수 분포"를 바꾸는 기법이라 RMS 단일 지표로는
개선이 보이지 않을 수 있으나, 스펙트럼 분석 도구가 없어 청감상 이득을 검증하지 못했습니다. 과장된
주장을 피하기 위해 <b>기본값에서 제외하고 실험적 opt-in으로만 제공</b>합니다.</p>

<h3>6. 사람·AI 편집 용이성</h3>
<p>HEADER/META/END 블록에서 <code>#</code>으로 시작하는 줄은 주석으로 처리됩니다(값에 <code>=</code>가
있어도 안전 — <code>#</code> 확인이 <code>=</code> 분리보다 먼저 일어남). <code>key = value</code>처럼
공백을 넣어 정렬해도 정확히 파싱됩니다. STREAM 블록에는 주석을 지원하지 않습니다(데이터와 모호해질 수
있어서).</p>

<h3>7. 하위 호환성</h3>
<ul>
<li>v1.0/v1.1 파일 → v1.2 디코더: 완전히 정상 재생 (회귀 테스트 확인).</li>
<li>v1.2가 codec=JTA-DPCM2-RLE82 또는 stereo_mode=mid_side를 사용한 파일 → codec 필드를 검증하지
않는 구버전 디코더에서는 정상 재생을 보장하지 않습니다(의도된 것 — 새 기능은 codec으로 반드시
선언되어야 하고, 이를 검증하는 디코더는 조용히 깨지는 대신 명확한 오류를 냅니다).</li>
</ul>

<h3>8. 다음 성장 방향 (제안만, 미구현)</h3>
<p>적응형 청크 크기, 3차 이상 델타, 3채널 이상 조인트 디코릴레이션은 실익을 실측할 도구가 아직 없어
이번 버전에는 포함하지 않았습니다. 자세한 표/실측 수치/의사결정 근거는 별첨 규격서(JTAudio_Spec_v1.2.md)를
참고하세요.</p>
`;
</script>
</body>
</html>
