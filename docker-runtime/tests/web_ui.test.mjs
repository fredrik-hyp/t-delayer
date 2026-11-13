/** Browser regression checks. Requires Playwright and Chromium; never contacts the real API. */
import assert from 'node:assert/strict';
import {test} from 'node:test';
import {createServer} from 'node:http';
import {readFile} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
const {chromium} = await import(process.env.PLAYWRIGHT_MODULE || 'playwright');
const screenshots = process.env.UI_SCREENSHOT_DIR || tmpdir();
const web = new URL('../web/',import.meta.url);
const csp = "default-src 'self'; script-src 'self'; style-src 'self'; frame-ancestors 'none'; base-uri 'none'; form-action 'self'";
const oldest = '2026-11-11T12:00:00.000Z';
const newest = '2026-11-12T12:00:00.000Z';
function output(pathnumber, position, mode = 'Play') {
  return {pathnumber, name:['Program','Backup','Output 2'][pathnumber] || `Output ${pathnumber}`, host:`239.1.0.${pathnumber+1}`,port:1234,ttl:7,rawudp:pathnumber===1,ready:true,stale:false,
    status:{state:mode,startpos:oldest,lastpos:newest,currpos:position,startplayat:null,playto:null},metrics:{bitratein:9.8e6,bitrateout:4.6e6,udpdrops:0}};
}
function fixture() {
  return {sampled_at:newest,system:{storage:{available_bytes:6.1*1024**3,total_bytes:37.3*1024**3,used_percent:83.7,path:'/home/video'},cpu:{usage_percent:14},network:{rx_bps:9.83e6,tx_bps:9.75e6,interfaces:['eth0','eth1']}},instances:[
    {name:'st1',pid:123,process_state:'running',last_exit_code:null,autostart:true,source:{host:'239.10.0.1',port:1234},msbuffer:86400_000,outputs:[output(0,'2026-11-11T11:00:00Z','Pause'),output(1,newest,'Live'),output(2,'2026-11-12T02:40:00Z')],pending:[],logs:[{timestamp:newest,text:'Recording active'}]},
    {name:'st2',pid:null,process_state:'stopped',last_exit_code:0,autostart:false,source:{host:'239.10.0.2',port:1234},msbuffer:3600_000,outputs:[{...output(0,null),ready:false,status:{}}],pending:[],logs:[]},
  ]};
}

test('production cockpit integrates design and existing API contracts',async t => {
  const server = createServer(async (req,res) => {
    try {
      const path = new URL(req.url,'http://localhost').pathname;
      const file = new URL(path === '/' ? 'index.html' : path.slice(1),web);
      if (!file.href.startsWith(web.href)) throw new Error('Outside web root');
      const data = await readFile(file);
      res.writeHead(200,{'Content-Type':file.pathname.endsWith('.css') ? 'text/css' : file.pathname.endsWith('.js') ? 'text/javascript' : 'text/html','Content-Security-Policy':csp});
      res.end(data);
    } catch { res.writeHead(404); res.end(); }
  });
  await new Promise(resolve => server.listen(0,'127.0.0.1',resolve));
  const browser = await chromium.launch({headless:true,...(process.env.CHROMIUM_PATH ? {executablePath:process.env.CHROMIUM_PATH} : {})});
  t.after(async () => {await browser.close(); await new Promise(resolve => server.close(resolve));});
  const page = await browser.newPage({viewport:{width:1440,height:1100}});
  page.setDefaultTimeout(5000);
  const errors = [];
  page.on('pageerror',e => errors.push(e.message));
  page.on('console',msg => {if (msg.type()==='error' && /Content Security Policy|Refused to/.test(msg.text())) errors.push(msg.text());});
  let data = fixture(), authenticated = false, offline = false, expire = false;
  const requests = [];
  await page.route('**/api/**',async route => {
    const req = route.request(), path = new URL(req.url()).pathname;
    const body = req.postDataJSON();
    if (req.method()==='POST') requests.push({path,body,headers:req.headers()});
    let result;
    if (path === '/api/auth/session') result={authenticated,username:'admin'};
    else if (path === '/api/auth/login') {authenticated=true;result={authenticated:true,username:'admin'};}
    else if (path === '/api/auth/logout') {authenticated=false;result={};}
    else if (path === '/api/status') {
      if (expire) {await route.fulfill({status:401,json:{error:'Session expired'}});return;}
      if (offline) {await route.fulfill({status:503,json:{error:'Service unavailable'}});return;}
      result=data;
    } else if (path === '/api/configuration') result={instances:[]};
    else if (path === '/api/auth/password') result={authenticated:true,username:'admin'};
    else if (path.endsWith('/commands')) result={command_id:42};
    else if (path.includes('/configuration/import')) result={imported:['st2']};
    else result={saved:true};
    await route.fulfill({json:result});
  });
  const base = `http://127.0.0.1:${server.address().port}`;
  await page.goto(base);
  await t.test('session login, actual metrics, expired playback and process controls',async () => {
    await page.locator('#login-submit:not([disabled])').waitFor();
    assert.equal(await page.locator('#dashboard').isVisible(),false);
    await page.screenshot({path:join(screenshots,'delayer-login.png'),fullPage:true});
    await page.locator('#login-username').fill('admin');
    await page.locator('#login-password').fill('test-password');
    await page.locator('#login-submit').click();
    await page.locator('.instance-card').first().waitFor();
    assert.equal(await page.locator('#running-summary').textContent(),'1 / 2');
    assert.equal(await page.locator('#storage-summary').textContent(),'6.1 GiB');
    assert.equal(await page.locator('.output-state[data-state="expired"]').count(),1);
    assert.equal(await page.locator('[data-instance="st1"] [data-action="configure"]').isDisabled(),true);
    assert.equal(await page.locator('[data-instance="st2"] [data-action="start"]').isEnabled(),true);
    assert.equal(await page.locator('[data-instance="st1"] [data-output="0"] [data-action="play"]').isDisabled(),true);
    assert.equal(await page.locator('.timeline-marker').count(),2);
    await page.screenshot({path:join(screenshots,'delayer-cockpit-desktop.png'),fullPage:true});
  });
  await t.test('timeline selection, tooltip and recovery prefill stay in UTC',async () => {
    const marker = page.locator('.timeline-marker[data-path="2"]');
    await marker.focus();
    assert.equal(await page.locator('[data-output="2"]').getAttribute('data-selected'),'true');
    await marker.click();
    assert.equal(await page.locator('#command-dialog').isVisible(),true);
    assert.equal(await page.locator('[name="jumptopos_date"]').inputValue(),'2026-11-12');
    await page.screenshot({path:join(screenshots,'delayer-command-dialog.png'),fullPage:true});
    await page.locator('#command-dialog [data-close-command]').first().click();
    assert.equal(await page.locator('#command-dialog').isVisible(),false);
    await page.locator('.timeline-expired [data-action="jump"]').click();
    assert.equal(await page.locator('[name="jumptopos_date"]').inputValue(),'2026-11-11');
    assert.equal(await page.locator('[name="jumptopos_time"]').inputValue(),'12:00:00');
    await page.locator('#command-form button[type="submit"]').click();
    await page.locator('#command-dialog').waitFor({state:'hidden'});
    const request=requests.findLast(r => r.path.endsWith('/outputs/0/commands'));
    assert.deepEqual(request.body,{action:'jump',jumptopos:oldest});
    assert.equal(request.headers['x-delayer-control'],'1');
  });
  await t.test('independent play/pause and wall-clock scheduling preserve API payloads',async () => {
    await page.locator('[data-output="2"] [data-action="pause"]').click();
    await page.waitForFunction(() => document.querySelector('#notice').textContent.includes('#42'));
    assert.deepEqual(requests.findLast(r => r.path.endsWith('/outputs/2/commands')).body,{action:'pause'});
    await page.locator('.timeline-marker[data-path="2"]').click();
    await page.locator('[name="startplayat_date"]').fill('2026-11-13');
    await page.locator('[name="startplayat_time"]').fill('14:30:00');
    await page.locator('#command-form button[type="submit"]').click();
    await page.locator('#command-dialog').waitFor({state:'hidden'});
    assert.equal(requests.findLast(r => r.path.endsWith('/outputs/2/commands')).body.startplayat,'2026-11-13T14:30:00.000Z');
    await page.locator('[data-output="2"] [data-action="playto"]').click();
    assert.equal(await page.locator('#jump-field').isVisible(),false);
    await page.locator('#command-form button[type="submit"]').click();
    await page.locator('#command-dialog').waitFor({state:'hidden'});
    assert.deepEqual(requests.findLast(r => r.path.endsWith('/outputs/2/commands')).body,{action:'playto'});
  });
  await t.test('ring view, tabs, resources, activity drawer and compact mode',async () => {
    await page.locator('[data-buffer-view="ring"]').click();
    assert.equal(await page.locator('.buffer-output').count(),3);
    await page.locator('.buffer-output[data-path="2"]').click();
    assert.equal(await page.locator('[data-output="2"]').getAttribute('data-selected'),'true');
    await page.screenshot({path:join(screenshots,'delayer-cockpit-ring.png'),fullPage:true});
    await page.locator('#density-toggle').click();
    assert.equal(await page.locator('body').getAttribute('class'),'compact');
    await page.locator('#tab-resources').click();
    assert.equal(await page.locator('#cpu-value').textContent(),'14.0%');
    await page.screenshot({path:join(screenshots,'delayer-resources.png'),fullPage:true});
    await page.locator('#tab-activity').click();
    assert.match(await page.locator('#activity-list').textContent(),/Recording active/);
    await page.locator('#notifications-button').click();
    assert.equal(await page.locator('#notification-drawer').isVisible(),true);
    await page.keyboard.press('Escape');
    await page.waitForFunction(() => document.querySelector('#notifications-button').getAttribute('aria-expanded') === 'false');
    await page.locator('#tab-instances').click();
  });
  await t.test('configuration, password and JSON tools retain the production forms',async () => {
    await page.locator('[data-instance="st2"] [data-action="configure"]').click();
    assert.equal(await page.locator('[name="instance_name"]').inputValue(),'st2');
    await page.locator('#add-output').click();
    assert.equal(await page.locator('.output-fields').count(),2);
    await page.locator('#configuration-dialog [data-close-config]').first().click();
    await page.locator('#change-password').click();
    await page.locator('#current-password').fill('test-password');
    await page.locator('#password-dialog [data-close-password]').first().click();
    await page.waitForFunction(() => document.querySelector('#current-password').value === '');
    await page.locator('#tab-configuration').click();
    await page.locator('[data-global-action="import"]').click();
    await page.locator('#import-json').fill(JSON.stringify({name:'st2',source:{host:'239.10.0.2',port:1234},outputs:[]}));
    assert.match(await page.locator('#import-preview').textContent(),/1 updated/);
    await page.locator('#import-dialog [data-close-import]').first().click();
    await page.locator('[data-global-action="export"]').click();
    await page.locator('#tab-instances').click();
  });
  await t.test('responsive layout has no horizontal overflow in either visualization',async () => {
    for (const width of [1440,1100,1024,768,375,320]) {
      await page.setViewportSize({width,height:1000});
      for (const view of ['timeline','ring']) {
        await page.locator(`[data-buffer-view="${view}"]`).click();
        assert.equal(await page.evaluate(() => document.documentElement.scrollWidth <= innerWidth),true,`${view} at ${width}px`);
      }
    }
    await page.setViewportSize({width:375,height:1000});
    await page.locator('[data-buffer-view="timeline"]').click();
    await page.screenshot({path:join(screenshots,'delayer-cockpit-mobile.png'),fullPage:true});
  });
  await t.test('polling keeps selection, keyboard focus and open logs',async () => {
    await page.setViewportSize({width:1440,height:1100});
    await page.locator('[data-instance="st1"] summary').click();
    await page.locator('.timeline-marker[data-path="2"]').focus();
    await page.waitForTimeout(2200);
    assert.equal(await page.locator('[data-instance="st1"] details').getAttribute('open'),'');
    assert.equal(await page.locator('[data-output="2"]').getAttribute('data-selected'),'true');
    assert.equal(await page.evaluate(() => document.activeElement.dataset.path),'2');
  });
  await t.test('instance badge follows fresh multicast input independently of buffered output traffic',async () => {
    const card=page.locator('[data-instance="st1"]');
    for (const output of data.instances[0].outputs) output.metrics.bitratein=0;
    await page.waitForFunction(() => document.querySelector('[data-instance="st1"] .state-chip').textContent==='No input');
    assert.equal(await card.locator('.state-chip').getAttribute('data-state'),'no-input');
    // Inspect both phases deterministically instead of sampling a moving blink.
    const phases=await card.evaluate(el=>{
      const chip=el.querySelector('.state-chip');
      const animations=[...el.getAnimations(),...chip.getAnimations()];
      const duration=animations.map(animation=>animation.effect.getTiming().duration);
      animations.forEach(animation=>{animation.pause();animation.currentTime=0;});
      const on={border:getComputedStyle(el).borderLeftColor,opacity:getComputedStyle(chip).opacity};
      animations.forEach(animation=>animation.currentTime=500);
      const off={border:getComputedStyle(el).borderLeftColor,opacity:getComputedStyle(chip).opacity};
      animations.forEach(animation=>animation.currentTime=0);
      return {duration,on,off};
    });
    assert.deepEqual(phases,{duration:[1000,1000],on:{border:'rgb(239, 116, 116)',opacity:'1'},off:{border:'rgba(0, 0, 0, 0)',opacity:'0.25'}});
    assert.equal(await card.locator('.state-chip').evaluate(el=>getComputedStyle(el).color),'rgb(255, 159, 171)');
    assert.deepEqual(await card.locator('.state-chip').evaluate(el=>({name:getComputedStyle(el).animationName,duration:getComputedStyle(el).animationDuration})),{name:'no-input-blink',duration:'1s'});
    await page.emulateMedia({reducedMotion:'reduce'});
    assert.equal(await card.locator('.state-chip').evaluate(el=>getComputedStyle(el).animationName),'none');
    assert.equal(await card.evaluate(el=>getComputedStyle(el).animationName),'none');
    await page.emulateMedia({reducedMotion:'no-preference'});
    assert.equal(await card.evaluate(el=>getComputedStyle(el).borderLeftColor),'rgb(239, 116, 116)');
    assert.equal(await card.locator('[data-action="stop"]').isEnabled(),true);
    assert.equal(await card.locator('[data-output="2"] [data-action="play"]').isEnabled(),true);
    assert.equal(await page.locator('#running-summary').textContent(),'1 / 2');
    assert.equal(await page.locator('#health-value').textContent(),'Needs attention');
    await card.locator('.instance-header').screenshot({path:join(screenshots,'delayer-no-input.png')});
    // Any fresh positive rate restores Running, even when output traffic is zero.
    data.instances[0].outputs[1].metrics.bitratein=1;
    data.instances[0].outputs[1].metrics.bitrateout=0;
    await page.waitForFunction(() => document.querySelector('[data-instance="st1"] .state-chip').textContent==='Running');
    assert.equal(await card.locator('.state-chip').evaluate(el=>getComputedStyle(el).animationName),'none');
    assert.equal(await card.evaluate(el=>getComputedStyle(el).animationName),'none');
    // Missing or stale measurements must not be interpreted as measured silence.
    for (const output of data.instances[0].outputs) output.metrics.bitratein=null;
    await page.waitForTimeout(2200);
    assert.equal(await card.locator('.state-chip').textContent(),'Running');
    for (const output of data.instances[0].outputs) {output.metrics.bitratein=0;output.stale=true;output.ready=false;}
    await page.waitForFunction(() => !document.querySelector('.timeline-marker'));
    assert.equal(await card.locator('.state-chip').textContent(),'Running');
    // A process with fresh zero input can have no recorded buffer yet.
    data.instances[0].process_state='starting';
    for (const output of data.instances[0].outputs) {output.stale=false;output.status={state:'Live'};}
    await page.waitForFunction(() => document.querySelector('[data-instance="st1"] .state-chip').textContent==='No input');
    assert.equal(await card.locator('.timeline-marker').count(),0);
    data.instances[0].pid=null;data.instances[0].process_state='stopped';
    await page.waitForFunction(() => document.querySelector('[data-instance="st1"] .state-chip').textContent==='Stopped');
    data=fixture();
    await page.waitForFunction(() => document.querySelector('[data-instance="st1"] .state-chip').textContent==='Running');
  });
  await t.test('stale and disconnected status hide geometry and disable writes',async () => {
    for (const output of data.instances[0].outputs) {output.stale=true;output.ready=false;}
    await page.waitForTimeout(2200);
    assert.equal(await page.locator('.timeline-marker').count(),0);
    assert.equal(await page.locator('[data-output="2"] [data-action="play"]').isDisabled(),true);
    offline=true;
    await page.waitForFunction(() => document.querySelector('#connection').classList.contains('offline'));
    assert.equal(await page.locator('[data-instance="st2"] [data-action="start"]').isDisabled(),true);
    assert.equal(await page.locator('#storage-summary').textContent(),'—');
    offline=false;data=fixture();
    await page.waitForFunction(() => document.querySelector('.timeline-marker'));
  });
  await t.test('untrusted server text is escaped, eight clustered outputs fit and empty states render',async () => {
    data.instances[0].outputs=Array.from({length:8},(_,index)=>output(index,newest));
    data.instances[0].outputs[0].name='<img src=x onerror="alert(1)">';
    data.instances[0].logs[0].text='<script>throw new Error("unsafe")</script>';
    await page.waitForFunction(() => document.querySelectorAll('.timeline-marker').length === 8);
    assert.equal(await page.locator('#instances img, #instances script, #activity-list script').count(),0);
    assert.equal(await page.locator('.timeline-scale').first().getAttribute('data-lanes'),'8');
    assert.equal(await page.evaluate(() => document.documentElement.scrollWidth <= innerWidth),true);
    data.instances=[];
    await page.waitForFunction(() => document.querySelector('#instances').textContent.includes('Create your first instance'));
  });
  await t.test('session restoration, password changes and logout use the existing endpoints',async () => {
    data=fixture();
    await page.reload();
    await page.locator('.instance-card').first().waitFor();
    assert.equal(await page.locator('#login-view').isVisible(),false);
    await page.locator('#change-password').click();
    await page.locator('#current-password').fill('test-password');
    await page.locator('#new-password').fill('new-test-password');
    await page.locator('#confirm-password').fill('different-password');
    await page.locator('#password-form button[type="submit"]').click();
    assert.match(await page.locator('#password-error').textContent(),/do not match/);
    await page.locator('#confirm-password').fill('new-test-password');
    await page.locator('#password-form button[type="submit"]').click();
    await page.locator('#password-dialog').waitFor({state:'hidden'});
    assert.deepEqual(requests.findLast(r => r.path === '/api/auth/password').body,{
      current_password:'test-password',new_password:'new-test-password',confirm_password:'new-test-password',
    });
    await page.locator('#logout').click();
    await page.locator('#login-view').waitFor();
    assert.equal(await page.locator('#notification-count').textContent(),'0');
    await page.locator('#login-username').fill('admin');
    await page.locator('#login-password').fill('new-test-password');
    await page.locator('#login-submit').click();
    await page.locator('.instance-card').first().waitFor();
  });
  await t.test('recovery with subsecond buffer bounds chooses a playable whole second',async () => {
    for (const output of data.instances[0].outputs) output.status.startpos='2026-11-11T12:00:00.123456Z';
    await page.waitForTimeout(2200);
    await page.locator('.timeline-expired [data-action="jump"]').click();
    assert.equal(await page.locator('[name="jumptopos_time"]').inputValue(),'12:00:01');
    await page.locator('#command-form button[type="submit"]').click();
    await page.locator('#command-dialog').waitFor({state:'hidden'});
    assert.equal(requests.findLast(r => r.path.endsWith('/outputs/0/commands')).body.jumptopos,'2026-11-11T12:00:01.000Z');
  });
  await t.test('expired sessions clear operational data and dialogs',async () => {
    await page.locator('#notifications-button').click();
    expire=true;
    await page.locator('#login-view').waitFor();
    assert.equal(await page.locator('#notification-drawer').isVisible(),false);
    assert.equal(await page.locator('.instance-card').count(),0);
    assert.match(await page.locator('#login-error').textContent(),/expired/);
    assert.equal(await page.locator('#notification-count').textContent(),'0');
  });
  assert.deepEqual(errors,[],'No JavaScript or CSP errors');
});
