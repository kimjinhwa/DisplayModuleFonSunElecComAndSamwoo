async function ensureAuth() {
  const r = await fetch('/api/me', { credentials: 'include' });
  const j = await r.json();
  if (!j.authenticated) location.href = 'login.html';
}

function led(on, cls) {
  return '<div class="dot ' + (on ? cls : '') + '"></div>';
}

function packCard(i, p) {
  const fail = p.ok ? '' : ' fail';
  const rawCells = (p.cells || []).slice(0, p.cellNum || 16);
  const valid = rawCells.filter(function (mv) { return mv >= 100; });
  const h = valid.length ? Math.max.apply(null, valid) : 0;
  const l = valid.length ? Math.min.apply(null, valid) : 0;
  p = Object.assign({}, p, {
    hvol: h / 1000,
    lvol: l / 1000,
    diff: h - l
  });
  const cells = rawCells.map(function (mv, n) {
    const t = (p.temps && (n === 0 || n === 3 || n === 7 || n === 11))
      ? ('<br>' + (p.temps[[0, 3, 7, 11].indexOf(n)] || 0).toFixed(1) + '℃')
      : '';
    return '<div class="cell">#' + (n + 1) + '<br>' + (mv / 1000).toFixed(3) + 'V' + t + '</div>';
  }).join('');
  return (
    '<section class="card">' +
    '<h2>PACK ' + (i + 1) + (p.ok ? '' : ' · 통신 실패') + '</h2>' +
    '<div class="big' + fail + '">' + (p.ok ? p.volt.toFixed(1) + ' V' : '-- V') + '</div>' +
    '<div class="soc"><i style="width:' + (p.soc || 0) + '%"></i></div>' +
    '<div class="kv">' +
    '<div>SOC ' + (p.soc || 0) + '%</div><div>SOH ' + (p.soh || 0) + '%</div>' +
    '<div>전류 ' + (p.ok ? p.amp.toFixed(1) : '--') + ' A</div><div>용량 ' + (p.capacity || 0).toFixed(1) + ' Ah</div>' +
    '<div>평균온도 ' + (p.ok ? p.temp.toFixed(1) : '--') + ' ℃</div><div>셀차 ' + (p.diff || 0) + ' mV</div>' +
    '<div>HVOL ' + (p.hvol || 0).toFixed(3) + ' V</div><div>LVOL ' + (p.lvol || 0).toFixed(3) + ' V</div>' +
    '<div>상태 ' + (p.alarm || '') + '</div><div>셀 ' + (p.cellNum || 0) + '</div>' +
    '</div>' +
    '<div class="leds">' +
    '<div class="led">' + led(p.comm, 'on-ok') + '통신</div>' +
    '<div class="led">' + led(p.charge, 'on-chg') + '충전</div>' +
    '<div class="led">' + led(p.discharge, 'on-dsg') + '방전</div>' +
    '<div class="led">' + led(p.warn, 'on-warn') + '경고</div>' +
    '</div>' +
    '<div class="cells">' + cells + '</div>' +
    '</section>'
  );
}

async function refresh() {
  const r = await fetch('/api/bms', { credentials: 'include' });
  const j = await r.json();
  document.getElementById('title').textContent = j.name || '리튬 BMS';
  document.getElementById('meta').textContent = '  ' + (j.ip || '') + '  v' + (j.version || '') + '  ERR ' + (j.err || 0);
  document.getElementById('packs').innerHTML = (j.packs || []).map(function (p, i) { return packCard(i, p); }).join('');
}

document.getElementById('logout').onclick = async function () {
  await fetch('/api/logout', { method: 'POST', credentials: 'include' });
  location.href = 'login.html';
};

ensureAuth().then(function () {
  refresh();
  setInterval(refresh, 2000);
});
