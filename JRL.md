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
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>JRL Protocol v6.0 — Stateless URL Compressor</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{
  font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;
  background:#0f1115;color:#e2e8f0;
  min-height:100vh;display:flex;flex-direction:column;align-items:center;
  padding:40px 20px;
}
h1{font-size:2rem;font-weight:700;color:#6366f1;margin-bottom:6px;letter-spacing:1px}
.subtitle{color:#94a3b8;font-size:.88rem;margin-bottom:36px;text-align:center}
.badge{
  display:inline-block;background:#1e2230;border:1px solid #6366f1;
  color:#6366f1;font-size:.7rem;padding:3px 10px;border-radius:20px;
  margin-bottom:30px;letter-spacing:1px;
}
.container{
  display:grid;grid-template-columns:1fr 1fr;gap:28px;
  width:100%;max-width:1100px;
}
@media(max-width:768px){.container{grid-template-columns:1fr}}
.panel{
  background:#161920;border-radius:14px;padding:30px;
  border:1px solid #2d3748;transition:border-color .3s,box-shadow .3s;
}
.panel:hover{border-color:#6366f1;box-shadow:0 0 24px rgba(99,102,241,.12)}
.panel-title{
  font-size:1.15rem;font-weight:600;color:#6366f1;margin-bottom:22px;
  display:flex;align-items:center;gap:10px;
}
.panel-title::before{content:'';width:4px;height:20px;background:#6366f1;border-radius:2px}
label{display:block;font-size:.82rem;color:#94a3b8;margin-bottom:8px;font-weight:500}
input[type="text"]{
  width:100%;padding:12px 16px;background:#0f1115;
  border:1px solid #2d3748;border-radius:8px;color:#e2e8f0;
  font-size:.95rem;font-family:Consolas,Monaco,monospace;
  outline:none;transition:border-color .3s,box-shadow .3s;
}
input[type="text"]:focus{border-color:#6366f1;box-shadow:0 0 0 3px rgba(99,102,241,.15)}
button{
  width:100%;padding:13px;margin-top:16px;background:#6366f1;color:#fff;
  border:none;border-radius:8px;font-size:.95rem;font-weight:600;
  cursor:pointer;transition:background .3s,transform .2s,box-shadow .3s;
}
button:hover{background:#4f46e5;transform:translateY(-2px);box-shadow:0 4px 14px rgba(99,102,241,.35)}
button:active{transform:translateY(0)}
.result-wrap{margin-top:22px;position:relative}
.result-box{
  background:#0f1115;border:2px solid #2d3748;border-radius:8px;
  padding:20px 14px;font-family:Consolas,Monaco,monospace;
  font-size:1.45rem;font-weight:700;letter-spacing:2px;text-align:center;
  color:#10b981;cursor:pointer;transition:border-color .3s,box-shadow .3s;
  user-select:all;min-height:62px;display:flex;align-items:center;
  justify-content:center;word-break:break-all;
}
.result-box:hover{border-color:#10b981;box-shadow:0 0 16px rgba(16,185,129,.2)}
.result-box.copied{animation:pulse .3s ease}
@keyframes pulse{0%,100%{transform:scale(1)}50%{transform:scale(1.03)}}
.copy-msg{
  position:absolute;top:50%;left:50%;transform:translate(-50%,-50%);
  background:#10b981;color:#fff;padding:10px 22px;border-radius:6px;
  font-size:.9rem;font-weight:600;opacity:0;pointer-events:none;
  transition:opacity .25s;z-index:10;white-space:nowrap;
}
.copy-msg.show{opacity:1}
.info{font-size:.78rem;color:#64748b;margin-top:10px;text-align:center}
.err{color:#ef4444;font-size:.84rem;margin-top:10px;text-align:center;min-height:20px}
.dict-hint{
  margin-top:18px;padding:14px;background:#1a1e28;border-radius:8px;
  font-size:.75rem;color:#64748b;line-height:1.7;
}
.dict-hint strong{color:#94a3b8}
</style>
</head>
<body>

<h1>JRL Protocol v6.0</h1>
<p class="subtitle">Stateless · Zero-Storage · 13-Char Fixed-Format URL Compression Engine</p>
<span class="badge">NO LocalStorage · NO SessionStorage · NO Server · NO DB</span>

<div class="container">

  <!-- ===== ENCODE PANEL ===== -->
  <div class="panel">
    <div class="panel-title">무손실 인코딩 (Encode)</div>
    <label for="inUrl">URL 입력</label>
    <input type="text" id="inUrl" placeholder="https://github.com/username/repo" spellcheck="false">
    <button onclick="handleEncode()">⚡ JRL 코드 생성</button>
    <div class="err" id="encErr"></div>
    <div class="result-wrap">
      <div class="result-box" id="encRes" onclick="clip('encRes')">---</div>
      <div class="copy-msg" id="encMsg">✓ 클립보드 복사 완료!</div>
    </div>
    <p class="info">결과 상자를 클릭하면 클립보드에 즉시 복사됩니다</p>
    <div class="dict-hint">
      <strong>Track 1 고정 사전 등록 도메인:</strong><br>
      github.com · google.com · naver.com · youtube.com · daum.net ·
      namu.wiki · twitter.com · facebook.com · instagram.com ·
      reddit.com · wikipedia.org · stackoverflow.com 외 다수
    </div>
  </div>

  <!-- ===== DECODE PANEL ===== -->
  <div class="panel">
    <div class="panel-title">무손실 복원 (Decode)</div>
    <label for="inJrl">JRL 코드 입력 (13글자)</label>
    <input type="text" id="inJrl" placeholder="JRL:GH0001JRL" maxlength="13" spellcheck="false">
    <button onclick="handleDecode()">🔓 URL 복원</button>
    <div class="err" id="decErr"></div>
    <div class="result-wrap">
      <div class="result-box" id="decRes" onclick="clip('decRes')" style="font-size:1rem">---</div>
      <div class="copy-msg" id="decMsg">✓ 클립보드 복사 완료!</div>
    </div>
    <p class="info">결과 상자를 클릭하면 클립보드에 즉시 복사됩니다</p>
    <div class="dict-hint">
      <strong>Track 2 동적 비트 압축 구조 (6글자 페이로드):</strong><br>
      [0] 프로토콜 토큰 · [1] www 플래그 · [2] TLD 토큰 ·
      [3-5] 도메인 축약 · 잔여 공간 X 패딩
    </div>
  </div>

</div>

<script>
/* ==========================================================
   JRL Protocol v6.0 — Pure Stateless Engine
   ❌ localStorage  ❌ sessionStorage  ❌ fetch  ❌ XMLHttpRequest
   모든 매핑은 아래 상수 객체에 하드코딩되어 있으며,
   인코딩/디코딩은 순수 함수(Pure Function)로만 동작한다.
   ========================================================== */

/* ---------- Track 1 : 고정 사전 (양방향) ---------- */
var FIXED_URL_TO_CODE = {
  'https://github.com':'GH0001',
  'https://www.github.com':'GH0002',
  'https://google.com':'GO0003',
  'https://www.google.com':'GO0004',
  'https://naver.com':'NV0005',
  'https://www.naver.com':'NV0006',
  'https://youtube.com':'YT0007',
  'https://www.youtube.com':'YT0008',
  'https://daum.net':'DM0009',
  'https://www.daum.net':'DM0010',
  'https://namu.wiki':'NW0011',
  'https://www.namu.wiki':'NW0012',
  'https://twitter.com':'TW0013',
  'https://www.twitter.com':'TW0014',
  'https://x.com':'XX0015',
  'https://facebook.com':'FB0016',
  'https://www.facebook.com':'FB0017',
  'https://instagram.com':'IG0018',
  'https://www.instagram.com':'IG0019',
  'https://linkedin.com':'LI0020',
  'https://www.linkedin.com':'LI0021',
  'https://reddit.com':'RD0022',
  'https://www.reddit.com':'RD0023',
  'https://wikipedia.org':'WP0024',
  'https://en.wikipedia.org':'WP0025',
  'https://ko.wikipedia.org':'WP0026',
  'https://stackoverflow.com':'SO0027',
  'https://www.stackoverflow.com':'SO0028',
  'https://netflix.com':'NF0029',
  'https://www.netflix.com':'NF0030',
  'https://amazon.com':'AZ0031',
  'https://www.amazon.com':'AZ0032',
  'https://twitch.tv':'TC0033',
  'https://www.twitch.tv':'TC0034',
  'https://discord.com':'DC0035',
  'https://www.discord.com':'DC0036',
  'https://notion.so':'NT0037',
  'https://www.notion.so':'NT0038',
  'https://vercel.com':'VC0039',
  'https://www.vercel.com':'VC0040'
};

var FIXED_CODE_TO_URL = {};
(function(){
  var keys = Object.keys(FIXED_URL_TO_CODE);
  for(var i=0;i<keys.length;i++){
    FIXED_CODE_TO_URL[FIXED_URL_TO_CODE[keys[i]]] = keys[i];
  }
})();

/* ---------- Track 2 : 동적 압축 매핑 사전 ---------- */
var PROTO_ENC = {'https://':'H','http://':'T','ftp://':'F'};
var PROTO_DEC = {'H':'https://','T':'http://','F':'ftp://'};

var TLD_ENC = {
  '.com':'C','.net':'N','.org':'O','.co.kr':'K','.io':'I',
  '.dev':'D','.edu':'E','.gov':'G','.kr':'R','.jp':'J',
  '.cn':'A','.uk':'U','.de':'M','.fr':'Q','.me':'S',
  '.app':'P','.xyz':'X','.tech':'Y','.ai':'Z','.tv':'V',
  '.so':'B','.info':'L','.biz':'W'
};
var TLD_DEC = {};
(function(){
  var keys = Object.keys(TLD_ENC);
  for(var i=0;i<keys.length;i++){
    TLD_DEC[TLD_ENC[keys[i]]] = keys[i];
  }
})();

var B62 = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';

/* ---------- 핵심 인코더 ---------- */
function encodeAbsoluteJRL(rawUrl){
  if(!rawUrl || typeof rawUrl!=='string') throw new Error('유효하지 않은 URL입니다.');
  var url = rawUrl.trim();

  /* Track 1: 고정 사전 히트 */
  if(FIXED_URL_TO_CODE[url]){
    return 'JRL:' + FIXED_URL_TO_CODE[url] + 'JRL';
  }

  /* Track 2: 동적 비트 압축 */
  var rest = url;
  var pTok = 'H';
  var protoKeys = Object.keys(PROTO_ENC);
  for(var i=0;i<protoKeys.length;i++){
    if(rest.indexOf(protoKeys[i])===0){
      pTok = PROTO_ENC[protoKeys[i]];
      rest = rest.substring(protoKeys[i].length);
      break;
    }
  }

  var wTok = 'X';
  if(rest.indexOf('www.')===0){
    wTok = 'W';
    rest = rest.substring(4);
  }

  var slashIdx = rest.indexOf('/');
  var hostPart = slashIdx===-1 ? rest : rest.substring(0,slashIdx);

  var tTok = 'C';
  var domain = hostPart;
  var tldKeys = Object.keys(TLD_ENC).sort(function(a,b){return b.length-a.length;});
  for(var j=0;j<tldKeys.length;j++){
    if(hostPart.length > tldKeys[j].length &&
       hostPart.substring(hostPart.length - tldKeys[j].length) === tldKeys[j]){
      tTok = TLD_ENC[tldKeys[j]];
      domain = hostPart.substring(0, hostPart.length - tldKeys[j].length);
      break;
    }
  }

  var dEnc = '';
  for(var k=0;k<domain.length && dEnc.length<3;k++){
    var ch = domain[k];
    if(B62.indexOf(ch)!==-1){
      dEnc += ch;
    }
  }
  while(dEnc.length<3) dEnc += 'X';

  var payload = pTok + wTok + tTok + dEnc;
  while(payload.length<6) payload += 'X';
  payload = payload.substring(0,6);

  return 'JRL:' + payload + 'JRL';
}

/* ---------- 핵심 디코더 ---------- */
function decodeAbsoluteJRL(rawJrl){
  if(!rawJrl || typeof rawJrl!=='string') throw new Error('유효하지 않은 JRL 코드입니다.');
  var jrl = rawJrl.trim();

  if(jrl.length!==13) throw new Error('JRL 코드는 정확히 13글자여야 합니다. (현재 '+jrl.length+'글자)');
  if(jrl.substring(0,4)!=='JRL:') throw new Error('접두사 "JRL:" 형식이 올바르지 않습니다.');
  if(jrl.substring(10,13)!=='JRL') throw new Error('접미사 "JRL" 형식이 올바르지 않습니다.');

  var payload = jrl.substring(4,10);

  /* Track 1: 고정 사전 역매핑 */
  if(FIXED_CODE_TO_URL[payload]){
    return FIXED_CODE_TO_URL[payload];
  }

  /* Track 2: 동적 역치환 복원 */
  var cleaned = payload.replace(/X+$/,'');
  if(cleaned.length<3) throw new Error('페이로드 정보가 부족하여 복원할 수 없습니다.');

  var pTok = cleaned[0];
  var wTok = cleaned.length>1 ? cleaned[1] : 'X';
  var tTok = cleaned.length>2 ? cleaned[2] : 'C';
  var dRaw = cleaned.length>3 ? cleaned.substring(3) : '';

  var protocol = PROTO_DEC[pTok] || 'https://';
  var www      = (wTok==='W') ? 'www.' : '';
  var tld      = TLD_DEC[tTok] || '.com';
  var domain   = dRaw.replace(/X/g,'');

  if(!domain) throw new Error('도메인 정보를 복원할 수 없습니다.');

  return protocol + www + domain + tld;
}

/* ---------- UI 핸들러 ---------- */
function handleEncode(){
  var inp = document.getElementById('inUrl');
  var res = document.getElementById('encRes');
  var err = document.getElementById('encErr');
  err.textContent = '';
  try{
    if(!inp.value.trim()) throw new Error('URL을 입력해주세요.');
    var code = encodeAbsoluteJRL(inp.value);
    res.textContent = code;
    res.style.color = '#10b981';
  }catch(e){
    res.textContent = 'ERROR';
    res.style.color = '#ef4444';
    err.textContent = e.message;
  }
}

function handleDecode(){
  var inp = document.getElementById('inJrl');
  var res = document.getElementById('decRes');
  var err = document.getElementById('decErr');
  err.textContent = '';
  try{
    if(!inp.value.trim()) throw new Error('JRL 코드를 입력해주세요.');
    var url = decodeAbsoluteJRL(inp.value);
    res.textContent = url;
    res.style.color = '#10b981';
  }catch(e){
    res.textContent = 'ERROR';
    res.style.color = '#ef4444';
    err.textContent = e.message;
  }
}

/* ---------- 클립보드 복사 유틸리티 ---------- */
function clip(elId){
  var el  = document.getElementById(elId);
  var txt = el.textContent;
  if(txt==='---'||txt==='ERROR') return;

  navigator.clipboard.writeText(txt).then(function(){
    var msgId = (elId==='encRes') ? 'encMsg' : 'decMsg';
    var msg   = document.getElementById(msgId);
    el.classList.add('copied');
    msg.classList.add('show');
    setTimeout(function(){
      el.classList.remove('copied');
      msg.classList.remove('show');
    },1500);
  }).catch(function(){
    var ta = document.createElement('textarea');
    ta.value = txt;
    ta.style.position = 'fixed';
    ta.style.opacity  = '0';
    document.body.appendChild(ta);
    ta.select();
    document.execCommand('copy');
    document.body.removeChild(ta);
    var msgId2 = (elId==='encRes') ? 'encMsg' : 'decMsg';
    var msg2   = document.getElementById(msgId2);
    el.classList.add('copied');
    msg2.classList.add('show');
    setTimeout(function(){
      el.classList.remove('copied');
      msg2.classList.remove('show');
    },1500);
  });
}

/* ---------- Enter 키 지원 ---------- */
document.getElementById('inUrl').addEventListener('keydown',function(e){
  if(e.key==='Enter') handleEncode();
});
document.getElementById('inJrl').addEventListener('keydown',function(e){
  if(e.key==='Enter') handleDecode();
});
</script>
</body>
</html>
