import {classifyPlayback, formatDelay, buildMarkerLayout} from './buffer-model.js';

'use strict';
const $ = (selector, root = document) => root.querySelector(selector);
const state = {instances: [], connected: false, target: null, busy: new Set(), editing: null, importing: null, authenticated: false, authGeneration: 0, bufferView: 'timeline', selectedOutputs: new Map(), activity: []};
const processLabels = {running: 'Running', starting: 'Starting / waiting for status', stopped: 'Stopped', exited: 'Process exited'};
const outputLabels = {Live: 'Live', Play: 'Delayed', Pause: 'Paused'};
const escapeHTML = value => String(value ?? '').replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
const timeText = value => value ? new Date(value).toISOString().slice(0, 19).replace('T', ' ') + ' UTC' : '—';
const bps = value => value == null ? '—' : `${(Number(value) / 1000000).toFixed(2)} Mbit/s`;
const duration = value => value == null ? '—' : `${Number(value).toFixed(1)} s`;
function byteSize(value) {
  if (value == null || !Number.isFinite(value)) return '—';
  const units = ['B', 'KiB', 'MiB', 'GiB', 'TiB', 'PiB'];
  const index = Math.min(Math.floor(Math.log2(Math.max(1, value)) / 10), units.length - 1);
  return `${(value / 1024 ** index).toFixed(index ? 1 : 0)} ${units[index]}`;
}
function resourceMeter(id, value) {
  const meter = $(id);
  const available = value != null && Number.isFinite(value);
  const percent = available ? Math.max(0, Math.min(100, value)) : 0;
  $('span', meter).style.width = `${percent}%`;
  meter.classList.toggle('warning', available && percent >= 85);
  meter.classList.toggle('critical', available && percent >= 95);
  if (available) meter.setAttribute('aria-valuenow', percent.toFixed(1));
  else meter.removeAttribute('aria-valuenow');
  meter.setAttribute('aria-valuetext', available ? `${percent.toFixed(1)}%` : 'Unavailable');
}
function renderSystem(metrics, offline = false) {
  const {storage, cpu, network} = metrics || {};
  $('#system-status').textContent = offline ? 'Status unavailable' : storage && cpu && network ? '● Live · every 2 seconds' : 'Some metrics unavailable · retrying';
  $('.system-panel').classList.toggle('unavailable', offline);
  $('#storage-value').textContent = byteSize(storage?.available_bytes);
  $('#storage-detail').textContent = storage ? `of ${byteSize(storage.total_bytes)} total · ${storage.path}` : 'Recording storage unavailable';
  $('#cpu-value').textContent = cpu?.usage_percent == null ? '—' : `${cpu.usage_percent.toFixed(1)}%`;
  $('#cpu-detail').textContent = cpu ? 'Across all system CPUs' : 'Waiting for CPU sample';
  $('#network-in').textContent = bps(network?.rx_bps);
  $('#network-out').textContent = bps(network?.tx_bps);
  $('#network-detail').textContent = network ? `${network.interfaces.join(', ') || 'No interfaces'} · excluding loopback` : 'Waiting for network sample';
  $('#storage-summary').textContent = byteSize(storage?.available_bytes);
  $('#storage-summary-detail').textContent = storage ? `of ${byteSize(storage.total_bytes)} total available` : 'Storage unavailable';
  $('#network-summary').textContent = `${network?.rx_bps == null ? '—' : (network.rx_bps / 1e6).toFixed(1)} / ${network?.tx_bps == null ? '—' : (network.tx_bps / 1e6).toFixed(1)}`;
  resourceMeter('#storage-meter', storage?.used_percent);
  resourceMeter('#cpu-meter', cpu?.usage_percent);
}
function setUtcInput(form, key, value) {
  const utc = value ? new Date(value).toISOString() : '';
  form.elements[`${key}_date`].value = utc.slice(0, 10);
  form.elements[`${key}_time`].value = utc.slice(11, 19);
}
function readUtcInput(form, key, label) {
  const date = form.elements[`${key}_date`].value;
  const time = form.elements[`${key}_time`].value;
  if (!date && !time) return null;
  if (!date || !time) {
    form.elements[`${key}_${date ? 'time' : 'date'}`].focus();
    throw new Error(`${label}: enter both a date and a time in UTC, or leave both blank.`);
  }
  // Date and time fields supply wall-clock values; interpret them together in UTC.
  return new Date(`${date}T${time}Z`).toISOString();
}

async function api(path, body) {
  const generation = state.authGeneration;
  const response = await fetch(new URL(path, window.location.origin), {cache: 'no-store', credentials: 'same-origin', signal: AbortSignal.timeout(body ? 35000 : 10000),
    ...(body ? {method: 'POST', headers: {'Content-Type': 'application/json', 'X-Delayer-Control': '1'}, body: JSON.stringify(body)} : {})});
  if (!response.ok) {
    let message = `HTTP ${response.status}`;
    try { message = (await response.json()).error || message; } catch (_) { /* nginx may return HTML */ }
    if (response.status === 401 && path !== '/api/auth/login' && state.authenticated && generation === state.authGeneration) {
      showLogin('Your session has expired. Sign in again.');
    }
    throw new Error(message);
  }
  return response.json();
}
function showLogin(message = '') {
  state.authGeneration++;
  state.authenticated = false;
  state.connected = false;
  state.instances = [];
  renderSystem(null);
  state.busy.clear();
  state.target = null;
  state.editing = null;
  for (const dialog of document.querySelectorAll('dialog[open]')) dialog.close();
  $('#instances').replaceChildren();
  state.activity = [];
  state.selectedOutputs.clear();
  render();
  $('#command-form').reset();
  $('#configuration-form').reset();
  $('#password-form').reset();
  $('#import-form').reset();
  $('#configuration-outputs').replaceChildren();
  $('#notice').hidden = true;
  $('#dashboard').hidden = true;
  $('#dashboard-footer').hidden = true;
  $('#user-menu').hidden = true;
  $('#connection').hidden = true;
  $('#session-user').textContent = '';
  $('#login-view').hidden = false;
  $('#login-password').value = '';
  $('#login-password').type = 'password';
  $('#password-visibility').textContent = 'Show';
  $('#password-visibility').setAttribute('aria-label','Show password');
  $('#password-visibility').setAttribute('aria-pressed','false');
  $('#login-error').textContent = message;
  $('#login-submit').disabled = false;
  $('#login-submit').textContent = 'Sign in';
  document.body.classList.add('signed-out');
}
function signedIn(session) {
  state.authGeneration++;
  state.authenticated = true;
  $('#login-password').value = '';
  $('#login-view').hidden = true;
  $('#dashboard').hidden = false;
  $('#dashboard-footer').hidden = false;
  $('#user-menu').hidden = false;
  $('#connection').hidden = false;
  $('#connection').textContent = 'Connecting…';
  $('#session-user').textContent = session.username;
  document.body.classList.remove('signed-out','disconnected');
}
$('#password-visibility').addEventListener('click',() => {
  const visible = $('#login-password').type === 'password';
  $('#login-password').type = visible ? 'text' : 'password';
  $('#password-visibility').textContent = visible ? 'Hide' : 'Show';
  $('#password-visibility').setAttribute('aria-label',visible ? 'Hide password' : 'Show password');
  $('#password-visibility').setAttribute('aria-pressed',String(visible));
});
$('#login-form').addEventListener('submit', async event => {
  event.preventDefault();
  if ($('#login-submit').disabled) return;
  $('#login-submit').disabled = true;
  $('#login-submit').textContent = 'Signing in…';
  $('#login-error').textContent = '';
  try {
    const session = await api('/api/auth/login',{username:$('#login-username').value.trim(),password:$('#login-password').value});
    signedIn(session);
    await refresh();
  } catch (error) {
    $('#login-error').textContent = error.message;
    $('#login-password').value = '';
    $('#login-password').focus();
  } finally {
    $('#login-submit').disabled = false;
    $('#login-submit').textContent = 'Sign in';
  }
});
$('#logout').addEventListener('click', async () => {
  $('#logout').disabled = true;
  try {
    await api('/api/auth/logout',{});
    showLogin();
    $('#login-username').focus();
  } catch (error) { notify(`Could not sign out: ${error.message}`,true); }
  finally { $('#logout').disabled = false; }
});
$('#change-password').addEventListener('click', () => {
  $('#password-form').reset();
  $('#password-error').textContent = '';
  $('#password-dialog').showModal();
  $('#current-password').focus();
});
for (const button of document.querySelectorAll('[data-close-password]')) {
  button.addEventListener('click', () => $('#password-dialog').close());
}
$('#password-dialog').addEventListener('close', () => $('#password-form').reset());
$('#password-form').addEventListener('submit', async event => {
  event.preventDefault();
  const form = event.target;
  const submit = $('button[type=submit]', form);
  if (submit.disabled) return;
  $('#password-error').textContent = '';
  if (form.elements.new_password.value !== form.elements.confirm_password.value) {
    $('#password-error').textContent = 'New passwords do not match.';
    $('#confirm-password').focus();
    return;
  }
  submit.disabled = true;
  submit.textContent = 'Saving…';
  try {
    const session = await api('/api/auth/password', {
      current_password: form.elements.current_password.value,
      new_password: form.elements.new_password.value,
      confirm_password: form.elements.confirm_password.value
    });
    signedIn(session);
    $('#password-dialog').close();
    notify('Password changed. Other browser sessions have been signed out.');
    await refresh();
  } catch (error) {
    $('#password-error').textContent = error.message;
    $('#current-password').value = '';
    $('#current-password').focus();
  } finally {
    submit.disabled = false;
    submit.textContent = 'Save password';
  }
});
async function restoreSession() {
  try {
    const session = await api('/api/auth/session');
    if (session.authenticated) {
      signedIn(session);
      await refresh();
    } else showLogin();
  } catch (_) { showLogin('Could not reach the service. Try signing in again.'); }
}
function notify(message, error = false) {
  $('#notice').hidden = false;
  $('#notice').className = error ? 'error' : '';
  $('#notice').textContent = message;
  state.activity.unshift({title: error ? 'Command failed' : 'Control room', message, error, timestamp: new Date().toISOString()});
  state.activity = state.activity.slice(0,100);
  renderActivity();
}
const markerColors = ['#f3bd5b', '#53d39b', '#6aa8ff', '#bc96f2', '#f18ba8', '#66cbd1', '#e59e63', '#b8c67a'];
function hasNoInput(instance) {
  if (!state.connected || instance.pid == null || ['stopped','exited'].includes(instance.process_state)) return false;
  // Every output reports the same shared multicast input rate. Playback can
  // continue from disk with zero input, so output bitrate is not a signal here.
  const rates = instance.outputs.filter(output => output.stale === false)
    .map(output => output.metrics?.bitratein)
    .filter(rate => typeof rate === 'number' && Number.isFinite(rate) && rate >= 0);
  return rates.length > 0 && rates.every(rate => rate === 0);
}
function bufferBounds(instance) {
  if (!state.connected || instance.pid == null) return null;
  const output = instance.outputs.find(o => o.ready && !o.stale &&
    Number.isFinite(Date.parse(o.status.startpos)) && Number.isFinite(Date.parse(o.status.lastpos)) &&
    Date.parse(o.status.lastpos) > Date.parse(o.status.startpos));
  return output ? {oldestMs: Date.parse(output.status.startpos), newestMs: Date.parse(output.status.lastpos)} : null;
}
function playback(instance, output) {
  if (instance.pid == null) return {state: 'stopped', label: 'Stopped', delayMs: null};
  const bounds = bufferBounds(instance);
  if (!state.connected || !output.ready || output.stale || !bounds) return {state: 'invalid', label: 'Waiting for status', delayMs: null};
  const result = classifyPlayback(Date.parse(output.status.currpos), bounds.oldestMs, bounds.newestMs);
  const label = result.state === 'expired' ? 'Expired' : result.state === 'invalid' ? 'Unavailable' : outputLabels[output.status.state] || 'Waiting';
  return {...result, label};
}
function outputEnabled(instance, output) {
  return state.connected && instance.pid != null && output.ready && !output.stale &&
    Boolean(bufferBounds(instance)) && Number.isFinite(Date.parse(output.status.currpos)) && !state.busy.has(instance.name);
}
function markerData(instance, bounds) {
  return buildMarkerLayout(instance.outputs.map((output, index) => ({
    ...output, positionMs: output.ready && !output.stale ? Date.parse(output.status.currpos) : NaN,
    color: markerColors[index % markerColors.length],
  })), bounds);
}
function bufferSummary(instance) {
  const bounds = bufferBounds(instance);
  const heading = `<div class="timeline-heading"><h3>${state.bufferView === 'ring' ? 'Ring buffer' : 'Recording timeline'}</h3><span class="timeline-duration">${bounds ? `${formatDelay(bounds.newestMs - bounds.oldestMs)} recorded` : '—'}</span></div>`;
  if (!bounds) return `<section class="buffer-section" aria-label="Recording buffer">${heading}<p class="buffer-empty">${state.connected ? 'No current recording window available.' : 'Recording status unavailable. Reconnecting…'}</p></section>`;
  const markers = markerData(instance, bounds);
  const selected = state.selectedOutputs.get(instance.name);
  const button = (marker, className, content) => `<button type="button" class="${className} output-command" data-action="jump" data-path="${marker.pathnumber}" data-color="${marker.color}" aria-haspopup="dialog" aria-pressed="${selected === marker.pathnumber}" aria-label="Jump or schedule ${escapeHTML(marker.name)}" ${outputEnabled(instance, marker) ? '' : 'disabled'}>${content}</button>`;
  const endpoints = `<div class="timeline-bounds"><div class="timeline-endpoint"><span>Oldest available</span><time datetime="${new Date(bounds.oldestMs).toISOString()}">${timeText(bounds.oldestMs)}</time></div><div class="timeline-endpoint timeline-live"><span>Latest recorded</span><time datetime="${new Date(bounds.newestMs).toISOString()}">${timeText(bounds.newestMs)}</time></div></div>`;
  const expired = markers.filter(m => m.state === 'expired').map(m => `<div class="timeline-expired-item"><span>${escapeHTML(m.name)} · Position overwritten</span>${button(m,'secondary-button','Recover')}</div>`).join('');
  if (state.bufferView === 'ring') {
    let previousAngle = -Infinity;
    const ringMarkers = markers.filter(m => m.percent !== null).sort((a,b) => a.percent - b.percent).map(m => {
      const angle = -150 + m.percent * 3;
      if (angle - previousAngle < 30) return '';
      previousAngle = angle;
      return `<span data-angle="${angle}">${button(m,'buffer-marker',`#${m.pathnumber}`)}</span>`;
    }).join('');
    const legend = markers.map(m => `<button type="button" class="buffer-output" data-action="select" data-path="${m.pathnumber}" data-color="${m.color}" data-state="${m.state}" aria-pressed="${selected === m.pathnumber}"><span class="buffer-output-symbol">${m.state === 'expired' ? '!' : m.pathnumber}</span><span class="buffer-output-name">${escapeHTML(m.name)}</span><span class="buffer-output-status">${escapeHTML(playback(instance,m).label)}${m.delayMs == null || m.state === 'live' ? '' : ` · ${formatDelay(m.delayMs)}`}</span></button>`).join('');
    return `<section class="buffer-section ring-section" aria-label="Ring buffer">${heading}<div class="ring-layout"><div class="buffer-visual"><div class="buffer-shell"><div class="buffer-ring" aria-hidden="true"></div><div class="buffer-center"><strong>${formatDelay(bounds.newestMs-bounds.oldestMs)}</strong><span>Recorded window</span></div>${ringMarkers}</div></div><div class="ring-details">${endpoints}<div class="buffer-legend">${legend}</div></div></div><div class="timeline-expired">${expired}</div></section>`;
  }
  const visible = markers.filter(m => m.percent !== null);
  const lanes = Math.max(1,...visible.map(m => m.lane + 1));
  const heads = visible.map(m => `<div class="timeline-playhead" data-position="${m.percent}" data-lane="${m.lane}" data-color="${m.color}" data-state="${m.state}"><span class="timeline-stem" aria-hidden="true"></span>${button(m,'timeline-marker',`<span class="timeline-symbol">#${m.pathnumber}</span><strong class="timeline-marker-name">${escapeHTML(m.name)}</strong><span class="timeline-marker-status">${m.status.state === 'Pause' ? 'Paused' : m.state === 'live' ? 'Live' : `−${formatDelay(m.delayMs)}`}</span>`)}<span class="timeline-tooltip" role="tooltip">${timeText(m.positionMs)} · ${escapeHTML(playback(instance,m).label)} · ${duration(m.delayMs / 1000)} behind buffer end</span></div>`).join('');
  const ticks = [25,50,75].map(p => `<time class="timeline-tick" data-tick="${p}">${new Date(bounds.oldestMs+(bounds.newestMs-bounds.oldestMs)*p/100).toISOString().slice(11,19)}</time>`).join('');
  return `<section class="buffer-section" aria-label="Recording timeline">${heading}<div class="timeline-scale" data-lanes="${lanes}"><div class="timeline-axis" aria-hidden="true"></div>${heads}</div>${endpoints.replace('<div class="timeline-bounds">',`<div class="timeline-bounds">${ticks}`)}<div class="timeline-expired">${expired}</div></section>`;
}
function outputRow(instance, output) {
  const stat = output.status, metric = output.metrics;
  const position = playback(instance, output);
  const enabled = outputEnabled(instance, output);
  const button = (action, text, unavailable = false) => `<button class="output-command" data-action="${action}" data-path="${output.pathnumber}" ${enabled && !unavailable ? '' : 'disabled'}>${text}</button>`;
  return `<article class="output-row" data-output="${output.pathnumber}" data-selected="${state.selectedOutputs.get(instance.name) === output.pathnumber}">
    <div class="output-name"><strong>${escapeHTML(output.name)}</strong><span>#${output.pathnumber} · ${escapeHTML(output.host)}:${output.port} · TTL ${output.ttl} · ${output.rawudp ? 'Raw UDP' : 'RTP preserved'}</span><span>${bps(metric.bitrateout)} out · ${bps(metric.bitratein)} in · UDP-drops ${escapeHTML(metric.udpdrops ?? '—')}</span></div>
    <span class="output-state" data-state="${position.state === 'live' && stat.state === 'Pause' ? 'available' : position.state}">${position.label}</span>
    <div class="output-position"><strong>${timeText(stat.currpos)}</strong><span>${position.delayMs == null ? 'Last reported position' : `${duration(position.delayMs / 1000)} behind buffer end`}</span>${position.state === 'expired' ? '<span class="output-warning">Position overwritten · recovery required</span>' : ''}<span>Start ${timeText(stat.startplayat)} · Stop ${timeText(stat.playto)}</span></div>
    <div class="row-actions">${button('play','Play',position.state === 'expired')}${button('pause','Pause')}${button('jump',position.state === 'expired' ? 'Recover / schedule' : 'Jump / schedule')}${button('playto','Play until')}</div></article>`;
}
function renderActivity() {
  const entries = [...state.activity, ...state.instances.flatMap(instance => instance.logs.map(log => ({timestamp: log.timestamp, title: instance.name, message: log.text})))].sort((a,b) => Date.parse(b.timestamp)-Date.parse(a.timestamp)).slice(0,100);
  const content = entries.map(item => `<li class="activity-item" data-tone="${item.error ? 'warning' : 'success'}"><i class="activity-dot" aria-hidden="true"></i><div><b>${escapeHTML(item.title)}</b><span>${escapeHTML(item.message)}</span></div><time>${timeText(item.timestamp)}</time></li>`).join('') || '<li class="empty">No operational messages yet.</li>';
  $('#activity-list').innerHTML = content;
  $('#notification-list').innerHTML = content;
  $('#notification-count').textContent = entries.length;
  $('#activity-tab-count').textContent = entries.length;
}
function render() {
  const focused = document.activeElement;
  const focusKey = focused?.dataset.action ? [focused.closest('[data-instance]')?.dataset.instance, focused.dataset.action, focused.dataset.path, focused.className] : null;
  const openLogs = new Set([...document.querySelectorAll('#instances details[open]')].map(el => el.closest('[data-instance]').dataset.instance));
  const running = state.instances.filter(i => i.process_state === 'running').length;
  const outputs = state.instances.flatMap(i => i.outputs);
  $('#running-summary').textContent = `${running} / ${state.instances.length}`;
  $('#output-summary').textContent = outputs.length;
  $('#active-output-summary').textContent = `${state.connected ? outputs.filter(o => o.ready && !o.stale && ['Live','Play'].includes(o.status.state)).length : '—'} active`;
  $('#instance-tab-count').textContent = state.instances.length;
  const warning = state.instances.some(i => hasNoInput(i) || i.process_state === 'exited' || i.outputs.some(o => o.stale));
  $('#health-value').textContent = !state.connected ? 'Unavailable' : warning ? 'Needs attention' : state.instances.some(i => i.process_state === 'starting') ? 'Starting' : 'Connected';
  $('#health-detail').textContent = !state.connected ? 'Reconnecting to service' : warning ? 'Check instance status' : 'Status updates every 2 seconds';
  $('.health-card').classList.toggle('unavailable', !state.connected || warning);
  $('#instances').innerHTML = state.instances.map(instance => {
    const running = instance.pid != null;
    const noInput = hasNoInput(instance);
    const displayState = noInput ? 'no-input' : instance.process_state;
    const label = noInput ? 'No input' : processLabels[instance.process_state] || 'Unknown';
    const disabled = state.busy.has(instance.name) || !state.connected ? 'disabled' : '';
    return `<article class="instance-card" data-instance="${escapeHTML(instance.name)}" data-state="${displayState}"><header class="instance-header"><div class="instance-identity"><span class="state-chip" data-state="${displayState}">${label}</span><div><h2>${escapeHTML(instance.name)}</h2><p>INPUT ${escapeHTML(instance.source.host)}:${instance.source.port} · Retention ${formatDelay(instance.msbuffer)} · ${running ? `PID ${instance.pid}` : `Exit ${instance.last_exit_code ?? '—'}`} · Autostart ${instance.autostart ? 'on' : 'off'}</p></div></div><div class="instance-header-actions"><button class="config-command" data-action="export" ${disabled}>Export JSON</button><button class="config-command" data-action="import" ${running ? 'disabled' : disabled} title="Stop the instance before importing">Import JSON</button><button class="config-command" data-action="configure" ${running ? 'disabled' : disabled} title="${running ? 'Stop the instance to change its input and outputs' : 'Edit input and outputs'}">Configure</button><button data-action="${running ? 'stop' : 'start'}" class="${running ? 'danger' : 'primary'}" ${disabled}>${running ? 'Stop instance' : 'Start instance'}</button></div></header>
      <div class="instance-content" data-visualization="${state.bufferView}">${bufferSummary(instance)}<section class="outputs-section" aria-label="Outputs"><div class="outputs-heading"><h3>Outputs · ${instance.outputs.length}</h3></div><div class="output-list">${instance.outputs.map(o => outputRow(instance,o)).join('')}</div></section></div>
      ${instance.pending.length ? `<div class="queue">Command queue: ${instance.pending.map(c => `#${c.id} ${escapeHTML(c.action)} → output ${c.pathnumber}`).join(' · ')}</div>` : ''}
      <details ${openLogs.has(instance.name) ? 'open' : ''}><summary>Recent operational messages</summary><div class="logs">${instance.logs.map(log => `${timeText(log.timestamp)} ${escapeHTML(log.text)}`).join('\n') || 'No messages yet.'}</div></details><footer class="instance-footer"><span>${noInput ? 'No multicast input detected' : running ? 'Recording continues independently of output playback' : 'Instance stopped · no active recording'}</span><span>All times UTC</span></footer></article>`;
  }).join('') || '<section class="utility-panel empty"><h2>Create your first instance</h2><p>Select New instance to configure an input and one or more outputs.</p></section>';
  // Set dynamic geometry through CSSOM so the strict style-src CSP stays intact.
  for (const el of document.querySelectorAll('[data-position]')) {
    el.style.setProperty('--position',`${el.dataset.position}%`);
    el.style.setProperty('--lane',el.dataset.lane);
  }
  for (const el of document.querySelectorAll('[data-color]')) el.style.setProperty('--marker-color',el.dataset.color);
  for (const el of document.querySelectorAll('[data-lanes]')) el.style.setProperty('--lanes',el.dataset.lanes);
  for (const el of document.querySelectorAll('[data-angle]')) $('button',el).style.setProperty('--marker-angle',`${el.dataset.angle}deg`);
  for (const el of document.querySelectorAll('[data-tick]')) el.style.setProperty('--tick-position',`${el.dataset.tick}%`);
  for (const [index,tooltip] of [...document.querySelectorAll('.timeline-tooltip')].entries()) {
    tooltip.id = `playhead-tooltip-${index}`;
    $('.timeline-marker',tooltip.parentElement).setAttribute('aria-describedby',tooltip.id);
  }
  renderActivity();
  if (focusKey) {
    const section = [...document.querySelectorAll('[data-instance]')].find(el => el.dataset.instance === focusKey[0]);
    const next = section && [...section.querySelectorAll('button')].find(el => el.dataset.action === focusKey[1] && el.dataset.path === focusKey[2] && el.className === focusKey[3]);
    next?.focus({preventScroll: true});
  }
}
async function refresh() {
  if (!state.authenticated) return;
  const generation = state.authGeneration;
  try {
    const result = await api('/api/status');
    if (!state.authenticated || generation !== state.authGeneration) return;
    state.instances = result.instances;
    state.connected = true;
    renderSystem(result.system);
    $('#add-instance').disabled = false;
    $('#export-all').disabled = false;
    $('#import-all').disabled = false;
    document.body.classList.remove('disconnected');
    $('#connection').textContent = `Connected · ${new Date(result.sampled_at).toISOString().slice(11, 19)} UTC`;
    $('#connection').className = 'connection-pill online';
    for (const button of document.querySelectorAll('[data-global-action]')) button.disabled = false;
    render();
  } catch (error) {
    if (!state.authenticated || generation !== state.authGeneration) return;
    state.connected = false;
    renderSystem(null, true);
    $('#add-instance').disabled = true;
    $('#export-all').disabled = true;
    $('#import-all').disabled = true;
    document.body.classList.add('disconnected');
    $('#connection').textContent = `Status unavailable · ${error.message}`;
    $('#connection').className = 'connection-pill offline';
    for (const button of document.querySelectorAll('[data-global-action]')) button.disabled = button.dataset.globalAction !== 'password';
    render();
    document.querySelectorAll('.output-command,.config-command').forEach(button => button.disabled = true);
  }
}
async function send(name, path, body) {
  state.busy.add(name);
  render();
  try {
    const url = path == null ? `/api/instances/${name}/${body.action}` : `/api/instances/${name}/outputs/${path}/commands`;
    const result = await api(url, path == null ? {} : body);
    notify(path == null ? `${name}: ${body.action === 'start' ? 'start requested' : 'stopped'}.` : `${name} / output ${path}: command #${result.command_id} queued. Status will update once it has been processed.`);
  } finally {
    state.busy.delete(name);
    await refresh();
  }
}
$('#instances').addEventListener('click', async event => {
  const button = event.target.closest('button[data-action]');
  if (!button || button.disabled) return;
  const name = button.closest('[data-instance]').dataset.instance;
  const action = button.dataset.action;
  const path = button.dataset.path == null ? null : Number(button.dataset.path);
  if (path != null) selectOutput(name,path);
  if (action === 'select') return;
  if (action === 'export') {
    try { await downloadConfiguration(name); } catch (error) { notify(error.message,true); }
    return;
  }
  if (action === 'import') {
    openImport(name);
    return;
  }
  if (action === 'configure') {
    openConfiguration(state.instances.find(instance => instance.name === name));
    return;
  }
  if (action === 'stop' && !confirm(`Stop ${name}? Recording and all outputs for this instance will stop.`)) return;
  if (action === 'jump' || action === 'playto') {
    state.target = {name,path,action};
    const output = state.instances.find(i => i.name === name).outputs.find(o => o.pathnumber === path);
    const form = $('#command-form');
    form.reset();
    $('#target-name').textContent = `${name} / #${path} ${output.name}`;
    $('#dialog-title').textContent = action === 'jump' ? 'Jump to recorded time (UTC)' : 'Set stop time (UTC)';
    $('#command-help').textContent = action === 'jump' ? `Available: ${timeText(output.status.startpos)} – ${timeText(output.status.lastpos)}` : 'The output pauses at the selected wall-clock time in UTC. Leave both date and time blank to clear the stop time.';
    $('#jump-field').hidden = action !== 'jump';
    $('#schedule-field').hidden = action !== 'jump';
    form.elements.jumptopos_date.required = action === 'jump';
    form.elements.jumptopos_time.required = action === 'jump';
    form.elements.jumptopos_date.min = output.status.startpos?.slice(0, 10) || '';
    form.elements.jumptopos_date.max = output.status.lastpos?.slice(0, 10) || '';
    const position = playback(state.instances.find(i => i.name === name),output);
    // Date/time fields use whole seconds. Round the lower bound upward so
    // subsecond timestamps cannot prefill a position that is already expired.
    const start = output.status.startpos;
    const fractionalStart = /\.\d*[1-9]\d*(?:Z|[+-])/.test(start || '');
    const earliest = Math.floor(Date.parse(start) / 1000) * 1000 + (fractionalStart ? 1000 : 0);
    const current = position.state === 'expired' ? earliest : Math.max(earliest, Date.parse(output.status.currpos) || earliest);
    setUtcInput(form, 'jumptopos', action === 'jump' ? current : null);
    setUtcInput(form, 'playto', output.status.playto);
    $('#form-error').textContent = '';
    $('#command-dialog').showModal();
    return;
  }
  try { await send(name,path,{action}); } catch (error) { notify(error.message,true); }
});
$('#command-form').addEventListener('submit', async event => {
  event.preventDefault();
  const target = state.target;
  const form = event.target;
  const submit = $('button[type=submit]',form);
  submit.disabled = true;
  try {
    const body = {action: target.action};
    for (const [key,label] of [['jumptopos','Recorded time'],['startplayat','Start playback at'],['playto','Stop at wall-clock time']]) {
      if (target.action !== 'jump' && key !== 'playto') continue;
      const value = readUtcInput(form, key, label);
      if (value) body[key] = value;
    }
    if (target.action === 'jump') {
      const status = state.instances.find(i => i.name === target.name).outputs.find(o => o.pathnumber === target.path).status;
      const position = Date.parse(body.jumptopos);
      if (position < Date.parse(status.startpos) || position > Date.parse(status.lastpos)) {
        throw new Error('Recorded time must be within the available buffer (UTC).');
      }
    }
    await send(target.name,target.path,body);
    $('#command-dialog').close();
  } catch (error) { $('#form-error').textContent = error.message; }
  finally { submit.disabled = false; }
});
for (const button of document.querySelectorAll('[data-close-command]')) button.addEventListener('click',() => $('#command-dialog').close());

function outputFields(output = {}) {
  const fieldset = document.createElement('fieldset');
  fieldset.className = 'output-fields';
  fieldset.innerHTML = `<legend>Output</legend><div class="config-grid">
    <label>Name<input data-field="name" required maxlength="100" value="${escapeHTML(output.name || '')}"></label>
    <label>Multicast address<input data-field="host" required placeholder="239.20.0.1" value="${escapeHTML(output.host || '')}" autocomplete="off"></label>
    <label>Port<input data-field="port" type="number" required min="1" max="65535" value="${output.port || 1234}"></label>
    <label>TTL<input data-field="ttl" type="number" required min="1" max="255" value="${output.ttl || 7}"></label>
    </div><div class="output-options"><label class="checkbox"><input data-field="rawudp" type="checkbox" ${output.rawudp ? 'checked' : ''}>Raw UDP · strip RTP</label><button type="button" data-remove-output>Remove output</button></div>`;
  $('#configuration-outputs').append(fieldset);
  numberOutputs();
}
function numberOutputs() {
  const rows = [...document.querySelectorAll('.output-fields')];
  rows.forEach((row,index) => {
    $('legend',row).textContent = `Output #${index}${index === 0 ? ' · primary' : ''}`;
    $('[data-remove-output]',row).disabled = rows.length === 1;
  });
  $('#add-output').disabled = rows.length >= 8;
}
function openConfiguration(instance = null) {
  state.editing = instance?.name || null;
  const form = $('#configuration-form');
  form.reset();
  form.elements.instance_name.value = instance?.name || '';
  form.elements.instance_name.readOnly = Boolean(instance);
  form.elements.buffer_minutes.value = instance ? instance.msbuffer / 60000 : 60;
  form.elements.autostart.checked = instance?.autostart || false;
  form.elements.source_host.value = instance?.source.host || '';
  form.elements.source_port.value = instance?.source.port || 1234;
  $('#configuration-title').textContent = instance ? `Configure ${instance.name}` : 'New instance';
  $('#configuration-outputs').replaceChildren();
  for (const output of instance?.outputs || [{name:'Program'}]) outputFields(output);
  $('#configuration-error').textContent = '';
  $('#configuration-dialog').showModal();
}
$('#add-instance').addEventListener('click',() => openConfiguration());
$('#add-output').addEventListener('click',() => {
  const count = document.querySelectorAll('.output-fields').length;
  if (count < 8) outputFields({name:`Output ${count}`});
});
$('#configuration-outputs').addEventListener('click',event => {
  const button = event.target.closest('[data-remove-output]');
  if (button && !button.disabled) {
    button.closest('fieldset').remove();
    numberOutputs();
  }
});
for (const button of document.querySelectorAll('[data-close-config]')) button.addEventListener('click',() => $('#configuration-dialog').close());
$('#configuration-form').addEventListener('submit', async event => {
  event.preventDefault();
  const form = event.target;
  const body = {name:form.elements.instance_name.value.trim(),
    msbuffer:Math.round(Number(form.elements.buffer_minutes.value) * 60000),
    autostart:form.elements.autostart.checked,
    source:{host:form.elements.source_host.value.trim(),port:Number(form.elements.source_port.value)},
    outputs:[...document.querySelectorAll('.output-fields')].map(row => ({
      name:$('[data-field="name"]',row).value.trim(),host:$('[data-field="host"]',row).value.trim(),
      port:Number($('[data-field="port"]',row).value),ttl:Number($('[data-field="ttl"]',row).value),rawudp:$('[data-field="rawudp"]',row).checked}))};
  const submit = $('button[type=submit]',form);
  submit.disabled = true;
  try {
    await api(state.editing ? `/api/instances/${state.editing}/configuration` : '/api/instances',body);
    $('#configuration-dialog').close();
    notify(`${body.name}: configuration saved. Start the instance when you are ready.`);
    await refresh();
  } catch (error) { $('#configuration-error').textContent = error.message; }
  finally { submit.disabled = false; }
});
async function downloadConfiguration(name = null) {
  const generation = state.authGeneration;
  const data = await api(name ? `/api/instances/${name}/configuration` : '/api/configuration');
  if (!state.authenticated || generation !== state.authGeneration) return;
  const url = URL.createObjectURL(new Blob([JSON.stringify(data,null,2) + '\n'], {type:'application/json'}));
  const link = document.createElement('a');
  link.href = url;
  link.download = name ? `${name}.json` : 'instances.json';
  document.body.append(link);
  link.click();
  link.remove();
  setTimeout(() => URL.revokeObjectURL(url),1000);
}
function openImport(name = null) {
  state.importing = name;
  $('#import-form').reset();
  $('#import-title').textContent = name ? `Import ${name}` : 'Import JSON';
  $('#import-help').textContent = name
    ? `Load one complete instance object with name "${name}" (or an instances object containing only that instance).`
    : 'Load one complete instance object or an object containing an "instances" list, as produced by Export all JSON.';
  $('#import-preview').textContent = '';
  $('#import-error').textContent = '';
  $('#import-dialog').showModal();
}
function importBody() {
  const text = $('#import-json').value;
  if (new Blob([text]).size > 1024 * 1024) throw new Error('JSON must be at most 1 MiB.');
  const body = JSON.parse(text);
  if (!body || Array.isArray(body) || typeof body !== 'object') throw new Error('Use a JSON object.');
  const items = Object.hasOwn(body,'instances') ? body.instances : [body];
  if (!Array.isArray(items) || items.some(item => !item || typeof item !== 'object' || Array.isArray(item) || typeof item.name !== 'string' || !item.source || !Array.isArray(item.outputs))) {
    throw new Error('Each instance needs a name, source and outputs.');
  }
  if (state.importing && (items.length !== 1 || items[0].name !== state.importing)) {
    throw new Error(`Import exactly one configuration with name ${state.importing}.`);
  }
  return {body,items};
}
function previewImport() {
  $('#import-error').textContent = '';
  $('#import-preview').textContent = '';
  if (!$('#import-json').value.trim()) return;
  try {
    const {items} = importBody();
    const existing = items.filter(item => state.instances.some(instance => instance.name === item.name));
    const running = existing.filter(item => state.instances.some(instance => instance.name === item.name && instance.pid !== null));
    $('#import-preview').textContent = `${items.length - existing.length} new, ${existing.length} updated.${existing.length ? ` Update: ${existing.map(item => item.name).join(', ')}.` : ''}${running.length ? ` Stop first: ${running.map(item => item.name).join(', ')}.` : ''}`;
  } catch (error) { $('#import-error').textContent = error.message; }
}
$('#export-all').addEventListener('click',async () => {
  try { await downloadConfiguration(); } catch (error) { notify(error.message,true); }
});
$('#import-all').addEventListener('click',() => openImport());
$('#import-json').addEventListener('input',previewImport);
$('#import-file').addEventListener('change',async event => {
  const file = event.target.files[0];
  if (!file) return;
  const generation = state.authGeneration;
  try {
    if (file.size > 1024 * 1024) throw new Error('JSON must be at most 1 MiB.');
    const text = await file.text();
    if (!state.authenticated || generation !== state.authGeneration || !$('#import-dialog').open) return;
    $('#import-json').value = text.replace(/^\uFEFF/,'');
    previewImport();
  } catch (error) { $('#import-error').textContent = error.message; }
});
for (const button of document.querySelectorAll('[data-close-import]')) button.addEventListener('click',() => $('#import-dialog').close());
$('#import-form').addEventListener('submit',async event => {
  event.preventDefault();
  const submit = $('button[type=submit]',event.target);
  if (submit.disabled) return;
  submit.disabled = true;
  $('#import-error').textContent = '';
  try {
    const {body} = importBody();
    const result = await api(state.importing ? `/api/instances/${state.importing}/configuration/import` : '/api/configuration/import',body);
    $('#import-dialog').close();
    notify(`Configuration imported for ${result.imported.length} instance(s). Start instances when ready.`);
    await refresh();
  } catch (error) { $('#import-error').textContent = error.message; }
  finally { submit.disabled = false; }
});
function selectOutput(name, path) {
  state.selectedOutputs.set(name,path);
  const card = [...document.querySelectorAll('[data-instance]')].find(el => el.dataset.instance === name);
  if (!card) return;
  for (const row of card.querySelectorAll('[data-output]')) row.dataset.selected = String(Number(row.dataset.output) === path);
  for (const button of card.querySelectorAll('.timeline-marker,.buffer-marker,.buffer-output')) button.setAttribute('aria-pressed', String(Number(button.dataset.path) === path));
}
$('#instances').addEventListener('focusin', event => {
  const marker = event.target.closest('.timeline-marker,.buffer-marker,.buffer-output');
  if (marker) selectOutput(marker.closest('[data-instance]').dataset.instance, Number(marker.dataset.path));
});
$('#instances').addEventListener('click', event => {
  const row = event.target.closest('[data-output]');
  if (row && !event.target.closest('button')) selectOutput(row.closest('[data-instance]').dataset.instance, Number(row.dataset.output));
});
$('#instances').addEventListener('keydown', event => {
  if (event.key === 'Escape') {
    const head = event.target.closest('.timeline-playhead');
    if (head) head.dataset.tooltipDismissed = 'true';
  }
});
$('#instances').addEventListener('focusout', event => {
  const head = event.target.closest('.timeline-playhead');
  if (head) delete head.dataset.tooltipDismissed;
});
$('#instances').addEventListener('pointerout', event => {
  const head = event.target.closest('.timeline-playhead');
  if (head && !head.contains(event.relatedTarget)) delete head.dataset.tooltipDismissed;
});
const tabs = [...document.querySelectorAll('[role="tab"]')];
function switchTab(tab) {
  for (const candidate of tabs) {
    const active = candidate === tab;
    candidate.setAttribute('aria-selected',String(active));
    candidate.tabIndex = active ? 0 : -1;
    document.getElementById(candidate.getAttribute('aria-controls')).hidden = !active;
  }
}
for (const tab of tabs) {
  tab.addEventListener('click',() => switchTab(tab));
  tab.addEventListener('keydown',event => {
    const index = tabs.indexOf(tab);
    const target = event.key === 'ArrowRight' ? tabs[(index+1)%tabs.length] : event.key === 'ArrowLeft' ? tabs[(index+tabs.length-1)%tabs.length] : event.key === 'Home' ? tabs[0] : event.key === 'End' ? tabs.at(-1) : null;
    if (target) { event.preventDefault(); switchTab(target); target.focus(); }
  });
}
for (const button of document.querySelectorAll('[data-buffer-view]')) button.addEventListener('click', () => {
  state.bufferView = button.dataset.bufferView;
  for (const candidate of document.querySelectorAll('[data-buffer-view]')) candidate.setAttribute('aria-pressed',String(candidate === button));
  render();
});
$('#density-toggle').addEventListener('click',event => {
  const compact = document.body.classList.toggle('compact');
  event.currentTarget.setAttribute('aria-pressed',String(compact));
});
$('#notifications-button').addEventListener('click', () => {
  $('#notification-drawer').showModal();
  $('#notifications-button').setAttribute('aria-expanded','true');
});
$('[data-drawer-close]').addEventListener('click',() => $('#notification-drawer').close());
$('#notification-drawer').addEventListener('close',() => $('#notifications-button').setAttribute('aria-expanded','false'));
for (const button of document.querySelectorAll('[data-global-action]')) button.addEventListener('click', () => {
  const target = {new: '#add-instance', import: '#import-all', export: '#export-all', password: '#change-password'}[button.dataset.globalAction];
  $(target).click();
});
function updateClock() {
  const now = new Date();
  $('#utc-clock').dateTime = now.toISOString();
  $('#utc-clock').textContent = `${now.toISOString().slice(11,19)} UTC`;
}
updateClock();
setInterval(updateClock,1000);

restoreSession();
(async function poll() { await refresh(); setTimeout(poll,2000); })();
