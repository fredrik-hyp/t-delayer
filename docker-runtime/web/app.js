'use strict';
const $ = (selector, root = document) => root.querySelector(selector);
const state = {instances: [], connected: false, target: null, busy: new Set(), editing: null, authenticated: false, authGeneration: 0};
const processLabels = {running: 'I drift', starting: 'Startar / väntar på status', stopped: 'Stoppad', exited: 'Processen avslutades'};
const outputLabels = {Live: 'Live', Play: 'Fördröjd', Pause: 'Pausad'};
const escapeHTML = value => String(value ?? '').replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
const timeText = value => value ? new Date(value).toLocaleString('sv-SE') : '—';
const bps = value => value == null ? '—' : `${(Number(value) / 1000000).toFixed(2)} Mbit/s`;
const duration = value => value == null ? '—' : `${Number(value).toFixed(1)} s`;
const localInput = value => {
  if (!value) return '';
  const date = new Date(value);
  return new Date(date.getTime() - date.getTimezoneOffset() * 60000).toISOString().slice(0, 19);
};
$('#timezone').textContent = Intl.DateTimeFormat().resolvedOptions().timeZone;

async function api(path, body) {
  const generation = state.authGeneration;
  const response = await fetch(new URL(path, window.location.origin), {cache: 'no-store', credentials: 'same-origin', signal: AbortSignal.timeout(body ? 35000 : 10000),
    ...(body ? {method: 'POST', headers: {'Content-Type': 'application/json', 'X-Delayer-Control': '1'}, body: JSON.stringify(body)} : {})});
  if (!response.ok) {
    let message = `HTTP ${response.status}`;
    try { message = (await response.json()).error || message; } catch (_) { /* nginx may return HTML */ }
    if (response.status === 401 && path !== '/api/auth/login' && state.authenticated && generation === state.authGeneration) {
      showLogin('Din session har gått ut. Logga in igen.');
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
  state.busy.clear();
  state.target = null;
  state.editing = null;
  for (const dialog of document.querySelectorAll('dialog[open]')) dialog.close();
  $('#instances').replaceChildren();
  $('#overview').replaceChildren();
  $('#command-form').reset();
  $('#configuration-form').reset();
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
  $('#password-visibility').textContent = 'Visa';
  $('#password-visibility').setAttribute('aria-label','Visa lösenord');
  $('#password-visibility').setAttribute('aria-pressed','false');
  $('#login-error').textContent = message;
  $('#login-submit').disabled = false;
  $('#login-submit').textContent = 'Logga in';
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
  $('#connection').textContent = 'Ansluter…';
  $('#session-user').textContent = session.username;
  document.body.classList.remove('signed-out','disconnected');
}
$('#password-visibility').addEventListener('click',() => {
  const visible = $('#login-password').type === 'password';
  $('#login-password').type = visible ? 'text' : 'password';
  $('#password-visibility').textContent = visible ? 'Dölj' : 'Visa';
  $('#password-visibility').setAttribute('aria-label',visible ? 'Dölj lösenord' : 'Visa lösenord');
  $('#password-visibility').setAttribute('aria-pressed',String(visible));
});
$('#login-form').addEventListener('submit', async event => {
  event.preventDefault();
  if ($('#login-submit').disabled) return;
  $('#login-submit').disabled = true;
  $('#login-submit').textContent = 'Loggar in…';
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
    $('#login-submit').textContent = 'Logga in';
  }
});
$('#logout').addEventListener('click', async () => {
  $('#logout').disabled = true;
  try {
    await api('/api/auth/logout',{});
    showLogin();
    $('#login-username').focus();
  } catch (error) { notify(`Det gick inte att logga ut: ${error.message}`,true); }
  finally { $('#logout').disabled = false; }
});
async function restoreSession() {
  try {
    const session = await api('/api/auth/session');
    if (session.authenticated) {
      signedIn(session);
      await refresh();
    } else showLogin();
  } catch (_) { showLogin('Kunde inte nå tjänsten. Försök logga in igen.'); }
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
  const label = instance.process_state === 'stopped' || instance.process_state === 'exited' ? 'Stoppad' : output.stale ? 'Väntar på färsk status' : outputLabels[stat.state] || 'Väntar';
  const badge = output.stale ? 'stale' : (stat.state || '').toLowerCase();
  const delay = stat.lastpos && stat.currpos ? Math.max(0, (new Date(stat.lastpos) - new Date(stat.currpos)) / 1000) : null;
  const button = (action, text) => `<button class="output-command" data-action="${action}" data-path="${output.pathnumber}" ${disabled}>${text}</button>`;
  return `<tr><td><span class="output-name">${escapeHTML(output.name)}</span><span class="sub">#${output.pathnumber} · ${escapeHTML(output.host)}:${output.port}<br>TTL ${output.ttl} · ${output.rawudp ? 'Raw UDP' : 'RTP bevaras'}</span></td>
    <td><span class="badge ${badge}">${label}</span><span class="sub">${duration(delay)} efter buffertslut</span></td>
    <td class="time">${timeText(stat.currpos)}<span class="sub">Från ${timeText(stat.startpos)}<br>Till ${timeText(stat.lastpos)}</span></td>
    <td>${bps(metric.bitrateout)}<span class="sub">In ${bps(metric.bitratein)}<br>UDP-drops ${escapeHTML(metric.udpdrops ?? '—')}</span></td>
    <td><div class="actions">${button('play','Spela')}${button('pause','Pausa')}${button('jump','Hoppa / schemalägg')}${button('playto','Spela till')}</div>
    <span class="sub">Start ${timeText(stat.startplayat)} · Stopp ${timeText(stat.playto)}</span></td></tr>`;
}
function render() {
  const focused = document.activeElement;
  const focusKey = focused?.dataset.action ? [focused.closest('[data-instance]')?.dataset.instance, focused.dataset.action, focused.dataset.path] : null;
  const openLogs = new Set([...document.querySelectorAll('details[open]')].map(el => el.closest('[data-instance]').dataset.instance));
  $('#overview').innerHTML = `<div><strong>${state.instances.filter(i => i.process_state === 'running').length}/${state.instances.length}</strong><span>INSTANSER I DRIFT</span></div><div><strong>${state.instances.reduce((sum,i) => sum + i.outputs.length,0)}</strong><span>OUTPUTS</span></div>`;
  $('#instances').innerHTML = state.instances.map(instance => {
    const running = instance.pid !== null;
    const disabled = state.busy.has(instance.name) ? 'disabled' : '';
    return `<section class="instance" data-instance="${escapeHTML(instance.name)}"><div class="instance-header"><div><div class="instance-title"><h2>${escapeHTML(instance.name)}</h2><span class="badge ${instance.process_state}">${processLabels[instance.process_state]}</span></div><div class="source">INPUT ${escapeHTML(instance.source.host)}:${instance.source.port} · Buffert ${Math.round(instance.msbuffer / 60000)} min · ${running ? `PID ${instance.pid}` : `Exit ${instance.last_exit_code ?? '—'}`} · Autostart ${instance.autostart ? 'på' : 'av'}</div></div><div class="instance-buttons"><button class="config-command" data-action="configure" ${running || !state.connected ? 'disabled' : disabled} title="${running ? 'Stoppa instansen för att ändra input och outputs' : 'Ändra input och outputs'}">Konfigurera</button><button data-action="${running ? 'stop' : 'start'}" class="${running ? 'danger' : 'primary'}" ${disabled}>${running ? 'Stoppa instans' : 'Starta instans'}</button></div></div>
      <div class="table-scroll"><table><thead><tr><th>Output</th><th>Status</th><th>Inspelad position / buffert</th><th>Trafik</th><th>Styrning per output</th></tr></thead><tbody>${instance.outputs.map(o => outputRow(instance,o)).join('')}</tbody></table></div>
      ${instance.pending.length ? `<div class="queue">Kommandokö: ${instance.pending.map(c => `#${c.id} ${escapeHTML(c.action)} → output ${c.pathnumber}`).join(' · ')}</div>` : ''}
      <details ${openLogs.has(instance.name) ? 'open' : ''}><summary>Senaste driftmeddelanden</summary><div class="logs">${instance.logs.map(log => `${timeText(log.timestamp)} ${escapeHTML(log.text)}`).join('\n') || 'Inga meddelanden ännu.'}</div></details></section>`;
  }).join('') || '<section class="empty"><h2>Skapa din första instans</h2><p>Välj Ny instans för att konfigurera input och en eller flera outputs.</p></section>';
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
    $('#add-instance').disabled = false;
    document.body.classList.remove('disconnected');
    $('#connection').textContent = `● Ansluten · ${new Date(result.sampled_at).toLocaleTimeString('sv-SE')}`;
    $('#connection').className = 'online';
    render();
  } catch (error) {
    if (!state.authenticated || generation !== state.authGeneration) return;
    state.connected = false;
    $('#add-instance').disabled = true;
    document.body.classList.add('disconnected');
    $('#connection').textContent = `Status saknas · ${error.message}`;
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
    notify(path == null ? `${name}: ${body.action === 'start' ? 'start begärd' : 'stoppad'}.` : `${name} / output ${path}: kommando #${result.command_id} köat. Status uppdateras när det har hanterats.`);
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
  if (action === 'stop' && !confirm(`Stoppa ${name}? Inspelning och samtliga outputs för instansen stoppas.`)) return;
  if (action === 'jump' || action === 'playto') {
    state.target = {name,path,action};
    const output = state.instances.find(i => i.name === name).outputs.find(o => o.pathnumber === path);
    const form = $('#command-form');
    form.reset();
    $('#target-name').textContent = `${name} / #${path} ${output.name}`;
    $('#dialog-title').textContent = action === 'jump' ? 'Hoppa till inspelad tid' : 'Ange sluttid';
    $('#command-help').textContent = action === 'jump' ? `Tillgängligt: ${timeText(output.status.startpos)} – ${timeText(output.status.lastpos)}` : 'Outputen pausas vid valt klockslag i verklig tid. Ett tomt fält tar bort sluttiden.';
    $('#jump-field').hidden = action !== 'jump';
    $('#schedule-field').hidden = action !== 'jump';
    form.elements.jumptopos.required = action === 'jump';
    form.elements.jumptopos.min = localInput(output.status.startpos);
    form.elements.jumptopos.max = localInput(output.status.lastpos);
    form.elements.jumptopos.value = action === 'jump' ? localInput(output.status.currpos || output.status.startpos) : '';
    form.elements.playto.value = localInput(output.status.playto);
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
  const body = {action: target.action};
  for (const key of ['jumptopos','startplayat','playto']) {
    const value = form.elements[key].value;
    if (value && (target.action === 'jump' || key === 'playto')) body[key] = new Date(value).toISOString();
  }
  const submit = $('button[type=submit]',form);
  submit.disabled = true;
  try {
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
    <label>Namn<input data-field="name" required maxlength="100" value="${escapeHTML(output.name || '')}"></label>
    <label>Multicastadress<input data-field="host" required placeholder="239.20.0.1" value="${escapeHTML(output.host || '')}" autocomplete="off"></label>
    <label>Port<input data-field="port" type="number" required min="1" max="65535" value="${output.port || 1234}"></label>
    <label>TTL<input data-field="ttl" type="number" required min="1" max="255" value="${output.ttl || 7}"></label>
    </div><div class="output-options"><label class="checkbox"><input data-field="rawudp" type="checkbox" ${output.rawudp ? 'checked' : ''}>Raw UDP · strippa RTP</label><button type="button" data-remove-output>Ta bort output</button></div>`;
  $('#configuration-outputs').append(fieldset);
  numberOutputs();
}
function numberOutputs() {
  const rows = [...document.querySelectorAll('.output-fields')];
  rows.forEach((row,index) => {
    $('legend',row).textContent = `Output #${index}${index === 0 ? ' · primär' : ''}`;
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
  $('#configuration-title').textContent = instance ? `Konfigurera ${instance.name}` : 'Ny instans';
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
    notify(`${body.name}: konfiguration sparad. Instansen kan startas när du är redo.`);
    await refresh();
  } catch (error) { $('#configuration-error').textContent = error.message; }
  finally { submit.disabled = false; }
});
restoreSession();
(async function poll() { await refresh(); setTimeout(poll,2000); })();
