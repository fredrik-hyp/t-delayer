'use strict';
const $ = (selector, root = document) => root.querySelector(selector);
const state = {instances: [], connected: false, target: null, busy: new Set(), editing: null, authenticated: false, authGeneration: 0};
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
  resourceMeter('#storage-meter', storage?.used_percent);
  resourceMeter('#cpu-meter', cpu?.usage_percent);
}
function bufferSummary(instance) {
  // Buffer bounds are shared by all outputs; playback positions can differ.
  const output = instance.outputs.find(output => output.ready && !output.stale &&
    output.status.startpos && output.status.lastpos &&
    Number.isFinite(Date.parse(output.status.startpos)) && Number.isFinite(Date.parse(output.status.lastpos)) &&
    Date.parse(output.status.lastpos) >= Date.parse(output.status.startpos));
  let buffered = '—', oldest = '—', latest = '—';
  if (output) {
    const seconds = Math.floor((Date.parse(output.status.lastpos) - Date.parse(output.status.startpos)) / 1000);
    const days = Math.floor(seconds / 86400);
    buffered = `${days ? `${days}d ` : ''}${Math.floor(seconds / 3600) % 24}h ${Math.floor(seconds / 60) % 60}m ${seconds % 60}s`;
    oldest = timeText(output.status.startpos);
    latest = timeText(output.status.lastpos);
  }
  return `<dl class="buffer-summary" aria-label="Instance buffer" title="Recorded buffer bounds, independent of each output's playback position.">
    <div><dt>Total buffered time</dt><dd>${buffered}</dd></div>
    <div><dt>Oldest in buffer (UTC)</dt><dd>${oldest}</dd></div>
    <div><dt>Latest in buffer (UTC)</dt><dd>${latest}</dd></div></dl>`;
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
  $('#overview').replaceChildren();
  $('#command-form').reset();
  $('#configuration-form').reset();
  $('#password-form').reset();
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
}
function outputRow(instance, output) {
  const stat = output.status, metric = output.metrics;
  const enabled = state.connected && output.ready && stat.startpos && stat.currpos && !state.busy.has(instance.name);
  const disabled = enabled ? '' : 'disabled';
  const label = instance.process_state === 'stopped' || instance.process_state === 'exited' ? 'Stopped' : output.stale ? 'Waiting for fresh status' : outputLabels[stat.state] || 'Waiting';
  const badge = output.stale ? 'stale' : (stat.state || '').toLowerCase();
  const delay = stat.lastpos && stat.currpos ? Math.max(0, (new Date(stat.lastpos) - new Date(stat.currpos)) / 1000) : null;
  const button = (action, text) => `<button class="output-command" data-action="${action}" data-path="${output.pathnumber}" ${disabled}>${text}</button>`;
  return `<tr><td><span class="output-name">${escapeHTML(output.name)}</span><span class="sub">#${output.pathnumber} · ${escapeHTML(output.host)}:${output.port}<br>TTL ${output.ttl} · ${output.rawudp ? 'Raw UDP' : 'RTP preserved'}</span></td>
    <td><span class="badge ${badge}">${label}</span><span class="sub">${duration(delay)} behind buffer end</span></td>
    <td class="time">${timeText(stat.currpos)}<span class="sub">From ${timeText(stat.startpos)}<br>To ${timeText(stat.lastpos)}</span></td>
    <td>${bps(metric.bitrateout)}<span class="sub">In ${bps(metric.bitratein)}<br>UDP-drops ${escapeHTML(metric.udpdrops ?? '—')}</span></td>
    <td><div class="actions">${button('play','Play')}${button('pause','Pause')}${button('jump','Jump / schedule')}${button('playto','Play until')}</div>
    <span class="sub">Start ${timeText(stat.startplayat)} · Stop ${timeText(stat.playto)}</span></td></tr>`;
}
function render() {
  const focused = document.activeElement;
  const focusKey = focused?.dataset.action ? [focused.closest('[data-instance]')?.dataset.instance, focused.dataset.action, focused.dataset.path] : null;
  const openLogs = new Set([...document.querySelectorAll('details[open]')].map(el => el.closest('[data-instance]').dataset.instance));
  $('#overview').innerHTML = `<div><strong>${state.instances.filter(i => i.process_state === 'running').length}/${state.instances.length}</strong><span>INSTANCES RUNNING</span></div><div><strong>${state.instances.reduce((sum,i) => sum + i.outputs.length,0)}</strong><span>OUTPUTS</span></div>`;
  $('#instances').innerHTML = state.instances.map(instance => {
    const running = instance.pid !== null;
    const disabled = state.busy.has(instance.name) ? 'disabled' : '';
    return `<section class="instance" data-instance="${escapeHTML(instance.name)}"><div class="instance-header"><div><div class="instance-title"><h2>${escapeHTML(instance.name)}</h2><span class="badge ${instance.process_state}">${processLabels[instance.process_state]}</span></div><div class="source">INPUT ${escapeHTML(instance.source.host)}:${instance.source.port} · Retention ${Math.round(instance.msbuffer / 60000)} min · ${running ? `PID ${instance.pid}` : `Exit ${instance.last_exit_code ?? '—'}`} · Autostart ${instance.autostart ? 'on' : 'off'}</div></div><div class="instance-buttons"><button class="config-command" data-action="configure" ${running || !state.connected ? 'disabled' : disabled} title="${running ? 'Stop the instance to change its input and outputs' : 'Edit input and outputs'}">Configure</button><button data-action="${running ? 'stop' : 'start'}" class="${running ? 'danger' : 'primary'}" ${disabled}>${running ? 'Stop instance' : 'Start instance'}</button></div></div>
      ${bufferSummary(instance)}
      <div class="table-scroll"><table><thead><tr><th>Output</th><th>Status</th><th>Recorded position / buffer</th><th>Traffic</th><th>Output controls</th></tr></thead><tbody>${instance.outputs.map(o => outputRow(instance,o)).join('')}</tbody></table></div>
      ${instance.pending.length ? `<div class="queue">Command queue: ${instance.pending.map(c => `#${c.id} ${escapeHTML(c.action)} → output ${c.pathnumber}`).join(' · ')}</div>` : ''}
      <details ${openLogs.has(instance.name) ? 'open' : ''}><summary>Recent operational messages</summary><div class="logs">${instance.logs.map(log => `${timeText(log.timestamp)} ${escapeHTML(log.text)}`).join('\n') || 'No messages yet.'}</div></details></section>`;
  }).join('') || '<section class="empty"><h2>Create your first instance</h2><p>Select New instance to configure an input and one or more outputs.</p></section>';
  if (focusKey) {
    const section = [...document.querySelectorAll('[data-instance]')].find(el => el.dataset.instance === focusKey[0]);
    const next = section && [...section.querySelectorAll('button')].find(el => el.dataset.action === focusKey[1] && el.dataset.path === focusKey[2]);
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
    document.body.classList.remove('disconnected');
    $('#connection').textContent = `● Connected · ${new Date(result.sampled_at).toISOString().slice(11, 19)} UTC`;
    $('#connection').className = 'online';
    render();
  } catch (error) {
    if (!state.authenticated || generation !== state.authGeneration) return;
    state.connected = false;
    renderSystem(null, true);
    $('#add-instance').disabled = true;
    document.body.classList.add('disconnected');
    $('#connection').textContent = `Status unavailable · ${error.message}`;
    $('#connection').className = 'offline';
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
    setUtcInput(form, 'jumptopos', action === 'jump' ? output.status.currpos || output.status.startpos : null);
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
for (const selector of ['.close','.cancel']) $(selector).addEventListener('click',() => $('#command-dialog').close());

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
restoreSession();
(async function poll() { await refresh(); setTimeout(poll,2000); })();
