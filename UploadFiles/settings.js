async function ensureAuth() {
  const r = await fetch('/api/me', { credentials: 'include' });
  const j = await r.json();
  if (!j.authenticated) location.href = 'login.html';
}

function val(id) { return document.getElementById(id).value; }
function set(id, v) { document.getElementById(id).value = v || ''; }

async function load() {
  const r = await fetch('/api/network-config', { credentials: 'include' });
  const j = await r.json();
  set('name', j.name);
  set('time', j.time);
  set('ip', j.ip);
  set('subnet', j.subnet);
  set('gateway', j.gateway);
  set('webPort', j.webPort);
  document.getElementById('webEnabled').checked = !!j.webEnabled;
  set('ssid', j.ssid);
  document.getElementById('mac').textContent = j.mac || '-';
  document.getElementById('ver').textContent = j.version || '-';
}

document.getElementById('save').onclick = async function () {
  const body = {
    name: val('name'),
    time: val('time'),
    ip: val('ip'),
    subnet: val('subnet'),
    gateway: val('gateway'),
    webEnabled: document.getElementById('webEnabled').checked,
    webPort: parseInt(val('webPort') || '80', 10),
    ssid: val('ssid')
  };
  if (val('pass')) body.pass = val('pass');
  if (val('userid') && val('passwd')) {
    body.userid = val('userid');
    body.passwd = val('passwd');
  }
  const r = await fetch('/api/network-config', {
    method: 'POST', credentials: 'include',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(body)
  });
  const j = await r.json();
  document.getElementById('msg').textContent = j.reboot ? '저장 후 재부팅합니다. 새 주소로 다시 접속하십시오.' : (j.ok ? '저장했습니다.' : '실패');
};

ensureAuth().then(load);
