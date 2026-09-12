const OIDS = [
  ['sysDescr', '.1.3.6.1.2.1.1.1.0', '문자열'],
  ['sysName', '.1.3.6.1.2.1.1.5.0', '문자열'],
  ['팩1 SOC', '.1.3.6.1.2.1.32.1.3.0', '%'],
  ['팩1 전압', '.1.3.6.1.2.1.32.1.8.0', '0.1 V (540 → 54.0 V)'],
  ['팩1 전류', '.1.3.6.1.2.1.32.1.2.0', '0.1 A'],
  ['팩1 용량', '.1.3.6.1.2.1.32.1.4.0', '0.1 Ah'],
  ['팩1 SOH', '.1.3.6.1.2.1.32.1.9.0', '%'],
  ['팩1 셀1', '.1.3.6.1.2.1.32.1.1.1.0', 'mV'],
  ['팩1 온도1', '.1.3.6.1.2.1.32.1.5.1.0', '0.1 ℃'],
  ['팩1 최고셀', '.1.3.6.1.2.1.32.1.11.0', 'mV'],
  ['팩1 최저셀', '.1.3.6.1.2.1.32.1.12.0', 'mV'],
  ['팩1 충전릴레이', '.1.3.6.1.2.1.32.1.6.4.0', '0/1'],
  ['팩1 방전릴레이', '.1.3.6.1.2.1.32.1.6.5.0', '0/1'],
  ['팩1 Fault', '.1.3.6.1.2.1.32.1.6.1.0', '플래그'],
  ['팩1 Protect', '.1.3.6.1.2.1.32.1.10.0', '플래그'],
  ['팩1 Warning', '.1.3.6.1.2.1.32.1.6.2.0', '플래그'],
  ['팩2 SOC', '.1.3.6.1.2.1.32.2.3.0', '%'],
  ['팩2 전압', '.1.3.6.1.2.1.32.2.8.0', '0.1 V'],
  ['팩2 전류', '.1.3.6.1.2.1.32.2.2.0', '0.1 A'],
  ['팩2 셀1', '.1.3.6.1.2.1.32.2.1.1.0', 'mV'],
  ['팩2 온도1', '.1.3.6.1.2.1.32.2.5.1.0', '0.1 ℃']
];

async function ensureAuth() {
  const r = await fetch('/api/me', { credentials: 'include' });
  const j = await r.json();
  if (!j.authenticated) location.href = 'login.html';
}

function fillTable() {
  const tb = document.getElementById('oids');
  tb.innerHTML = OIDS.map(function (x) {
    return '<tr data-oid="' + x[1] + '"><td>' + x[0] + '</td><td><code>' + x[1] + '</code></td><td>' + x[2] + '</td></tr>';
  }).join('');
  tb.querySelectorAll('tr').forEach(function (tr) {
    tr.onclick = function () {
      document.getElementById('oid').value = tr.getAttribute('data-oid');
    };
  });
}

document.getElementById('go').onclick = async function () {
  const oid = document.getElementById('oid').value.trim();
  const r = await fetch('/api/snmp-get', {
    method: 'POST', credentials: 'include',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ oid: oid })
  });
  const t = await r.text();
  document.getElementById('out').textContent = 'HTTP ' + r.status + '\n' + t;
};

ensureAuth().then(function () {
  fillTable();
  document.getElementById('go').click();
});
