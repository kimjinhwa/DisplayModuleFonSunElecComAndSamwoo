#ifndef WEB_PAGES_H
#define WEB_PAGES_H

/* 부트스트랩 페이지만 펌웨어에 넣는다. 본문 UI는 SPIFFS UploadFiles. */

static const char FILE_UPLOAD_HTML[] = R"HTML(<!DOCTYPE html>
<html lang="ko"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>파일 업로드</title>
<style>
body{font-family:"Malgun Gothic","맑은 고딕",Arial,sans-serif;background:#121820;margin:0;color:#e8eaee}
.bar{background:#263343;color:#fff;padding:12px 16px;font-weight:700}
.box{max-width:640px;margin:24px auto;background:#1c2734;border-radius:10px;padding:16px}
button{background:#2f5f8f;color:#fff;border:0;padding:8px 16px;border-radius:6px;cursor:pointer}
a{color:#9fc4e8}
#log{white-space:pre-wrap;font:13px Consolas,monospace;background:#121820;padding:8px;min-height:80px;margin-top:12px}
.warn{color:#f0c0b0;font-size:13px;margin:10px 0}
</style></head><body>
<div class="bar">리튬 BMS · 파일 업로드</div>
<div class="box">
<h1>웹 화면 파일 업로드</h1>
<p>HTML/CSS/JS 를 올립니다. 여러 파일을 한 번에 선택할 수 있습니다.</p>
<div class="warn">펌웨어 <code>.bin</code> 은 이 화면으로 올리지 마십시오. 앱에서 SSID/PASS 전송 후 <b>update</b> → Wi‑Fi OTA 를 사용합니다.</div>
<input id="f" type="file" multiple>
<p><button id="go">업로드</button> <a href="/" style="margin-left:12px">모니터</a></p>
<div id="log"></div>
</div>
<script>
const log=document.getElementById('log');
document.getElementById('go').onclick=async()=>{
  const files=document.getElementById('f').files;
  if(!files.length){log.textContent='파일을 고르십시오.';return;}
  log.textContent='';
  for(const file of files){
    const fd=new FormData();
    fd.append('update',file,file.name);
    try{
      const r=await fetch('/upload',{method:'POST',body:fd});
      const t=await r.text();
      log.textContent+=file.name+'  HTTP '+r.status+'  '+t+'\n';
    }catch(e){log.textContent+=file.name+'  FAIL '+e+'\n';}
  }
};
</script></body></html>
)HTML";

static const char SERVER_INDEX_HTML[] = R"HTML(<!DOCTYPE html>
<html lang="ko"><head><meta charset="UTF-8"><title>펌웨어</title></head>
<body><p>이더넷 펌웨어 업로드는 쓰지 않습니다. BLE <code>update</code> 후 Wi‑Fi OTA 를 사용하십시오.</p>
<p><a href="/fileUpload">HTML 업로드</a> · <a href="/">모니터</a></p></body></html>
)HTML";

#endif
