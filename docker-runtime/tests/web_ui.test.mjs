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
  await page.clock.setFixedTime(new Date(newest));
  page.setDefaultTimeout(5000);
  const errors = [];
  page.on('pageerror',e => errors.push(e.message));
  page.on('console',msg => {if (msg.type()==='error' && /Content Security Policy|Refused to/.test(msg.text())) errors.push(msg.text());});
  let data = fixture(), authenticated = false, offline = false, expire = false, stopError = false, commandError = false, releaseStop = null;
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
    } else if (path.endsWith('/stop')) {
      if (stopError) {await route.fulfill({status:503,json:{error:'Could not stop instance. Try again.'}});return;}
      await new Promise(resolve => {releaseStop=resolve;});
      result={stopped:true};
    } else if (path === '/api/configuration') result={instances:[]};
    else if (path === '/api/auth/password') result={authenticated:true,username:'admin'};
    else if (path.endsWith('/commands')) {
      if (commandError) {await route.fulfill({status:503,json:{error:'Command unavailable'}});return;}
      result={command_id:42};
    }
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
  await t.test('stop confirmation identifies the instance and defaults to keeping it running',async () => {
    const trigger = page.locator('[data-instance="st1"] [data-action="stop"]');
    const dialog = page.locator('#stop-dialog');
    const cancel = dialog.getByRole('button',{name:'Keep running',exact:true});
    const confirm = page.locator('#stop-submit');
    const stopRequests = () => requests.filter(r => r.path.endsWith('/stop'));
    const before = stopRequests().length;
    await trigger.click();
    assert.equal(await dialog.getAttribute('role'),'alertdialog');
    assert.equal(await page.locator('#stop-title').textContent(),'Stop st1?');
    assert.equal(await confirm.textContent(),'Stop st1');
    assert.match(await page.locator('#stop-warning').textContent(),/Recording and all outputs will stop immediately/);
    assert.equal(await cancel.evaluate(el => el === document.activeElement),true);
    await page.screenshot({path:join(screenshots,'delayer-stop-dialog.png'),fullPage:true});
    // Polling must not dismiss the dialog or break focus restoration.
    await page.waitForTimeout(2200);
    await page.keyboard.press('Enter');
    await dialog.waitFor({state:'hidden'});
    assert.equal(await trigger.evaluate(el => el === document.activeElement),true);
    assert.equal(stopRequests().length,before);
    await trigger.click();
    await page.keyboard.press('Escape');
    await dialog.waitFor({state:'hidden'});
    await trigger.click();
    await dialog.getByRole('button',{name:'Cancel stopping instance',exact:true}).click();
    await dialog.waitFor({state:'hidden'});
    assert.equal(stopRequests().length,before);
    await page.setViewportSize({width:320,height:640});
    await trigger.click();
    assert.equal(await dialog.evaluate(el => el.scrollWidth <= el.clientWidth),true);
    const bounds = await dialog.boundingBox();
    assert.ok(bounds.x >= 0 && bounds.x + bounds.width <= 320);
    await page.screenshot({path:join(screenshots,'delayer-stop-dialog-mobile.png'),fullPage:true});
    await cancel.click();
    await page.setViewportSize({width:1440,height:1100});
    stopError=true;
    await trigger.click();
    await confirm.click();
    await page.locator('#stop-error').waitFor({state:'visible'});
    assert.equal(await dialog.isVisible(),true);
    assert.match(await page.locator('#stop-error').textContent(),/Could not stop instance/);
    assert.equal(await confirm.isEnabled(),true);
    stopError=false;
    try {
      await confirm.click();
      await page.waitForFunction(() => document.querySelector('#stop-submit').textContent === 'Stopping…');
      assert.equal(await confirm.isDisabled(),true);
      assert.equal(await cancel.isDisabled(),true);
      await page.keyboard.press('Escape');
      assert.equal(await dialog.isVisible(),true);
      assert.equal(stopRequests().length,before+2);
      assert.deepEqual(stopRequests().at(-1).body,{});
      assert.equal(stopRequests().at(-1).path,'/api/instances/st1/stop');
      await page.waitForTimeout(50);
      releaseStop();
      await dialog.waitFor({state:'hidden'});
      assert.equal(await trigger.evaluate(el => el === document.activeElement),true);
    } finally {releaseStop?.();}
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
  await t.test('date and optional time fields use fixed formats across locales and validate input',async () => {
    for (const [locale, timezoneId] of [['sv-SE','Europe/Stockholm'],['en-US','America/New_York']]) {
      const context = await browser.newContext({locale,timezoneId,viewport:{width:1440,height:1100}});
      try {
        const localPage = await context.newPage();
        localPage.on('pageerror',e => errors.push(e.message));
        await localPage.clock.setFixedTime(new Date(newest));
        const commands = [];
        await localPage.route('**/api/**',async route => {
          const path = new URL(route.request().url()).pathname;
          let result = {authenticated:true,username:'admin'};
          if (path === '/api/status') result = fixture();
          else if (path.endsWith('/commands')) {commands.push(route.request().postDataJSON());result={command_id:42};}
          await route.fulfill({json:result});
        });
        await localPage.goto(base);
        await localPage.locator('.timeline-marker[data-path="2"]').click();
        const form = localPage.locator('#command-form');
        const submit = form.locator('button[type="submit"]');
        const keys = ['jumptopos','startplayat','playto'];
        for (const key of keys) {
          const date = form.locator(`[name="${key}_date"]`);
          assert.equal(await date.getAttribute('type'),'text');
          assert.equal(await date.getAttribute('placeholder'),'yyyy-mm-dd');
        }
        for (const key of ['startplayat','playto']) {
          const time = form.locator(`[name="${key}_time"]`);
          assert.equal(await time.getAttribute('type'),'text');
          assert.equal(await time.getAttribute('placeholder'),'hh:mm:ss');
          await time.focus();
          await time.blur();
          assert.equal(await time.inputValue(),'','a focused empty field must not acquire an example time');
        }
        const clocks = localPage.locator('[data-utc-now]');
        assert.equal(await clocks.count(),2);
        assert.deepEqual(await clocks.allTextContents(),['2026-11-12 12:00:00 UTC','2026-11-12 12:00:00 UTC']);
        assert.equal(await form.locator('[name="jumptopos_date"]').inputValue(),'2026-11-12');
        await localPage.locator('#command-dialog').screenshot({path:join(screenshots,`delayer-fixed-dates-${locale}.png`)});
        await localPage.setViewportSize({width:320,height:800});
        assert.equal(await localPage.locator('#command-dialog').evaluate(el=>el.scrollWidth<=el.clientWidth),true,'field clocks fit the mobile dialog');
        await localPage.setViewportSize({width:1440,height:1100});
        for (const [key, invalid] of [['jumptopos','2026-02-30'],['startplayat','2026-13-01'],['playto','2026-04-31'],['jumptopos','16/11/2026'],['jumptopos','2026-2-01']]) {
          const date = form.locator(`[name="${key}_date"]`);
          await date.fill(invalid);
          assert.equal(await date.evaluate(el => el.validity.valid),false,`${invalid} in ${locale}`);
          assert.equal(await date.getAttribute('aria-invalid'),'true');
          assert.equal(await localPage.locator(key === 'jumptopos' ? '#recorded-time-error' : '#date-format-error').isVisible(),true);
          assert.equal(await submit.isDisabled(),true);
          await form.evaluate(el => el.dispatchEvent(new Event('submit',{bubbles:true,cancelable:true})));
          assert.equal(commands.length,0,'invalid dates never reach the API');
          await date.fill(key === 'jumptopos' ? '2026-11-12' : '');
        }
        for (const key of ['startplayat','playto']) {
          const time = form.locator(`[name="${key}_time"]`);
          for (const invalid of ['24:00:00','12:60:00','12:30:60','1:30:00','12.30.00']) {
            await time.fill(invalid);
            assert.equal(await time.evaluate(el => el.validity.valid),false,`${invalid} in ${locale}`);
            assert.equal(await time.getAttribute('aria-invalid'),'true');
            assert.equal(await localPage.locator('#time-format-error').isVisible(),true);
            assert.equal(await submit.isDisabled(),true);
            await form.evaluate(el => el.dispatchEvent(new Event('submit',{bubbles:true,cancelable:true})));
            assert.equal(commands.length,0,'malformed times never reach the API');
          }
          for (const valid of ['00:00','23:59:59']) {
            await time.fill(valid);
            assert.equal(await time.evaluate(el => el.validity.valid),true);
          }
          await time.fill('');
        }
        assert.equal(await localPage.locator('#time-format-error').isVisible(),false);
        const recorded = form.locator('[name="jumptopos_date"]');
        await recorded.fill('2024-02-29');
        assert.equal(await recorded.evaluate(el => el.validity.valid),true,'a leap day is a valid date');
        await recorded.fill('2026-02-29');
        assert.equal(await recorded.evaluate(el => el.validity.valid),false,'a leap day in a non-leap year is invalid');
        await recorded.fill('2026-11-12');
        assert.equal(await localPage.locator('#date-format-error').isVisible(),false);
        await form.locator('[name="startplayat_date"]').fill('2026-11-13');
        await form.locator('[name="startplayat_time"]').fill('01:30');
        await form.locator('[name="playto_date"]').fill('2026-11-13');
        await form.locator('[name="playto_time"]').fill('02:30:00');
        await localPage.clock.setFixedTime(new Date('2026-11-12T12:00:01Z'));
        await localPage.waitForFunction(()=>[...document.querySelectorAll('[data-utc-now]')].every(clock=>clock.textContent==='2026-11-12 12:00:01 UTC'));
        assert.equal(await form.locator('[name="startplayat_time"]').inputValue(),'01:30','ticking clocks do not change the scheduled start');
        assert.equal(await form.locator('[name="playto_time"]').inputValue(),'02:30:00','ticking clocks do not change the scheduled stop');
        assert.equal(commands.length,0);
        await submit.click();
        await localPage.locator('#command-dialog').waitFor({state:'hidden'});
        assert.deepEqual(commands,[{action:'jump',jumptopos:'2026-11-12T02:40:00.000Z',startplayat:'2026-11-13T01:30:00.000Z',playto:'2026-11-13T02:30:00.000Z'}]);
      } finally {await context.close();}
    }
  });
  await t.test('recorded time errors take precedence over scheduling errors and follow live buffer bounds',async () => {
    const dialog = page.locator('#command-dialog');
    const form = page.locator('#command-form');
    const recordedError = page.locator('#recorded-time-error');
    const scheduleError = page.locator('#schedule-error');
    const stopError = page.locator('#stop-time-error');
    const submit = form.locator('button[type="submit"]');
    const commands = () => requests.filter(r=>r.path.endsWith('/commands'));
    const before=commands().length;
    data=fixture();data.instances[0].msbuffer=30*60_000;
    for (const output of data.instances[0].outputs) {
      Object.assign(output.status,{startpos:'2026-11-16T13:40:21Z',lastpos:'2026-11-16T14:10:39Z',currpos:'2026-11-16T14:00:00Z'});
    }
    try {
      await page.clock.setFixedTime(new Date('2026-11-16T14:11:51Z'));
      await page.waitForFunction(()=>document.querySelector('[data-instance="st1"] [data-output="2"] .output-position strong').textContent==='2026-11-16 14:00:00 UTC');
      await page.locator('.timeline-marker[data-path="2"]').click();
      await form.locator('[name="jumptopos_time"]').fill('14:45:29');
      await form.locator('[name="startplayat_date"]').fill('2026-11-26');
      await form.locator('[name="startplayat_time"]').fill('14:15:35');
      assert.equal(await recordedError.isVisible(),true);
      assert.match(await recordedError.textContent(),/within the current buffer: 2026-11-16 13:40:21 UTC – 2026-11-16 14:10:39 UTC/);
      assert.equal(await scheduleError.isVisible(),false,'do not calculate expiry for a nonexistent recording');
      assert.equal(await form.locator('[name="jumptopos_date"]').getAttribute('aria-invalid'),'true');
      assert.equal(await form.locator('[name="jumptopos_time"]').getAttribute('aria-invalid'),'true');
      assert.equal(await form.locator('[name="startplayat_time"]').getAttribute('aria-invalid'),'false');
      assert.equal(await submit.isDisabled(),true);
      await form.evaluate(el=>el.dispatchEvent(new Event('submit',{bubbles:true,cancelable:true})));
      assert.equal(commands().length,before);
      await dialog.screenshot({path:join(screenshots,'delayer-recorded-time-priority-error.png')});
      await form.locator('[name="playto_date"]').fill('2026-11-16');
      await form.locator('[name="playto_time"]').fill('14:00:00');
      assert.equal(await stopError.isVisible(),false,'later stop checks also wait for valid Recorded time');
      await form.locator('[name="jumptopos_time"]').fill('13:30:00');
      assert.equal(await recordedError.isVisible(),true,'times before the oldest packet are also invalid');
      assert.equal(await scheduleError.isVisible(),false);
      await form.locator('[name="jumptopos_time"]').fill('14:00:00');
      assert.equal(await recordedError.isVisible(),false);
      assert.match(await scheduleError.textContent(),/2026-11-16 14:30:00 UTC/,'expiry is evaluated after correcting Recorded time');
      assert.equal(await stopError.isVisible(),true);
      for (const key of ['startplayat','playto']) {
        await form.locator(`[name="${key}_date"]`).fill('');
        await form.locator(`[name="${key}_time"]`).fill('');
      }
      for (const boundary of ['13:40:21','14:10:39']) {
        await form.locator('[name="jumptopos_time"]').fill(boundary);
        assert.equal(await recordedError.isVisible(),false,'both exact buffer boundaries are valid');
        assert.equal(await submit.isEnabled(),true);
      }
      await form.locator('[name="jumptopos_time"]').fill('13:40:21');
      for (const output of data.instances[0].outputs) {
        output.status.startpos='2026-11-16T13:40:22Z';
        output.status.lastpos='2026-11-16T14:11:40Z';
      }
      await recordedError.waitFor({state:'visible'});
      assert.match(await page.locator('#command-help').textContent(),/13:40:22 UTC – 2026-11-16 14:11:40 UTC/);
      assert.equal(await form.locator('[name="jumptopos_time"]').inputValue(),'13:40:21','polling does not replace user input');
      await form.locator('[name="jumptopos_time"]').fill('14:00:00');
      assert.equal(await submit.isEnabled(),true);
      await form.locator('[name="jumptopos_date"]').fill('');
      assert.match(await recordedError.textContent(),/enter both a date and a time/);
      assert.equal(await submit.isDisabled(),true);
      await form.locator('[name="jumptopos_date"]').fill('2026-11-16');
      offline=true;
      await page.waitForFunction(()=>document.querySelector('#recorded-time-error').textContent.includes('buffer is unavailable'));
      assert.equal(await submit.isDisabled(),true);
      assert.equal(await scheduleError.isVisible(),false);
    } finally {
      offline=false;data=fixture();
      await page.clock.setFixedTime(new Date(newest));
      if (await dialog.isVisible()) await page.keyboard.press('Escape');
      await page.waitForFunction(()=>document.querySelector('[data-instance="st1"] [data-output="2"] .output-position strong').textContent==='2026-11-12 02:40:00 UTC');
    }
  });
  await t.test('recorded time explains the buffer and expired schedules cannot be submitted',async () => {
    const dialog = page.locator('#command-dialog');
    const error = page.locator('#schedule-error');
    const submit = page.locator('#command-form button[type="submit"]');
    const commands = () => requests.filter(r => r.path.endsWith('/commands'));
    const before = commands().length;
    await page.locator('.timeline-marker[data-path="2"]').click();
    assert.match(await page.locator('#dialog-title').textContent(), /time in the buffer/);
    assert.match(await page.locator('#command-help').textContent(), /Currently in buffer:/);
    assert.match(await page.locator('#recorded-time-help').textContent(), /when the video was recorded/);
    assert.equal(await error.isVisible(),false);
    await page.locator('[name="startplayat_date"]').fill('2026-11-13');
    assert.equal(await error.isVisible(),false,'an incomplete start does not produce an expiry error');
    await page.locator('[name="startplayat_time"]').fill('02:39:59');
    assert.equal(await error.isVisible(),false,'starting just before expiry is safe');
    assert.equal(await submit.isEnabled(),true);
    await page.locator('[name="startplayat_time"]').fill('02:40:00');
    assert.equal(await error.isVisible(),true,'the expiry boundary is an error');
    assert.equal(await error.getAttribute('role'),'alert');
    assert.match(await error.textContent(), /2026-11-13 02:40:00 UTC/);
    assert.match(await error.textContent(), /current 24-hour buffer\./);
    assert.match(await error.textContent(), /earlier playback start or a later recorded time/);
    assert.equal(await submit.isDisabled(),true);
    assert.equal(await page.locator('[name="startplayat_time"]').getAttribute('aria-invalid'),'true');
    await page.keyboard.press('Enter');
    assert.equal(await dialog.isVisible(),true);
    assert.equal(commands().length,before,'Enter cannot submit an expired schedule');
    // Bypassing the disabled button and native constraints still cannot send.
    await page.locator('#command-form').evaluate(form => form.dispatchEvent(new Event('submit',{bubbles:true,cancelable:true})));
    assert.equal(commands().length,before);
    await dialog.screenshot({path:join(screenshots,'delayer-schedule-buffer-error.png')});
    await page.setViewportSize({width:320,height:800});
    assert.equal(await dialog.evaluate(el => el.scrollWidth <= el.clientWidth),true);
    await error.scrollIntoViewIfNeeded();
    await dialog.screenshot({path:join(screenshots,'delayer-schedule-buffer-error-mobile.png')});
    await page.setViewportSize({width:1440,height:1100});
    await page.locator('[name="jumptopos_time"]').fill('03:00:00');
    assert.equal(await error.isVisible(),false,'changing the recorded position recalculates expiry');
    assert.equal(await submit.isEnabled(),true);
    data.instances[0].msbuffer=12*3600_000;
    await error.waitFor({state:'visible'});
    assert.equal(await submit.isDisabled(),true);
    assert.match(await error.textContent(), /2026-11-12 15:00:00 UTC/,'polling uses updated retention');
    assert.match(await error.textContent(), /current 12-hour buffer\./);
    for (const [minutes, label] of [[60, '1-hour buffer'], [30, '30-minute buffer']]) {
      data.instances[0].msbuffer=minutes*60_000;
      await page.waitForFunction(label => document.querySelector('#schedule-error').textContent.includes(`current ${label}.`), label);
      assert.equal(await submit.isDisabled(),true);
    }
    data.instances[0].msbuffer=86400_000;
    await error.waitFor({state:'hidden'});
    assert.equal(await submit.isEnabled(),true);
    await page.locator('[name="startplayat_time"]').fill('');
    assert.equal(await error.isVisible(),false);
    await page.locator('[name="startplayat_time"]').fill('03:00:00');
    assert.equal(await error.isVisible(),true);
    assert.equal(await submit.isDisabled(),true);
    await page.locator('[name="startplayat_time"]').fill('02:59:59');
    assert.equal(await error.isVisible(),false);
    assert.equal(await submit.isEnabled(),true);
    await submit.click();
    await dialog.waitFor({state:'hidden'});
    assert.deepEqual(requests.findLast(r => r.path.endsWith('/outputs/2/commands')).body,{
      action:'jump',jumptopos:'2026-11-12T03:00:00.000Z',startplayat:'2026-11-13T02:59:59.000Z',
    });
    await page.locator('.timeline-marker[data-path="2"]').click();
    assert.equal(await error.isVisible(),false,'reopening clears the previous error');
    assert.equal(await submit.isEnabled(),true);
    await page.keyboard.press('Escape');
    await page.locator('[data-output="2"] [data-action="playto"]').click();
    assert.equal(await error.isVisible(),false,'stop-only commands have no schedule error');
    await page.keyboard.press('Escape');
  });
  await t.test('stop time must be after the scheduled start and errors update together',async () => {
    const dialog = page.locator('#command-dialog');
    const error = page.locator('#stop-time-error');
    const expiryError = page.locator('#schedule-error');
    const submit = page.locator('#command-form button[type="submit"]');
    const commands = () => requests.filter(r => r.path.endsWith('/commands'));
    const before = commands().length;
    await page.locator('.timeline-marker[data-path="2"]').click();
    await page.locator('[name="startplayat_date"]').fill('2026-11-12');
    await page.locator('[name="startplayat_time"]').fill('14:00:00');
    await page.locator('[name="playto_date"]').fill('2026-11-12');
    assert.equal(await error.isVisible(),false,'an incomplete stop has no comparison error');
    await page.locator('[name="playto_time"]').fill('13:59:59');
    assert.equal(await error.isVisible(),true);
    assert.equal(await error.getAttribute('role'),'alert');
    assert.match(await error.textContent(), /must be after Start playback at \(2026-11-12 14:00:00 UTC\)/);
    assert.equal(await page.locator('[name="playto_time"]').getAttribute('aria-invalid'),'true');
    assert.equal(await submit.isDisabled(),true);
    await page.keyboard.press('Enter');
    await page.locator('#command-form').evaluate(form => form.dispatchEvent(new Event('submit',{bubbles:true,cancelable:true})));
    assert.equal(commands().length,before,'neither Enter nor forced submit can send an invalid stop');
    await error.scrollIntoViewIfNeeded();
    await dialog.screenshot({path:join(screenshots,'delayer-stop-time-error.png')});
    await page.setViewportSize({width:320,height:800});
    await error.scrollIntoViewIfNeeded();
    assert.equal(await dialog.evaluate(el => el.scrollWidth <= el.clientWidth),true);
    await dialog.screenshot({path:join(screenshots,'delayer-stop-time-error-mobile.png')});
    await page.setViewportSize({width:1440,height:1100});
    await page.locator('[name="playto_time"]').fill('14:00:00');
    assert.equal(await submit.isDisabled(),true,'equal times are also invalid');
    await page.locator('[name="playto_date"]').fill('2026-11-13');
    await page.locator('[name="playto_time"]').fill('01:00:00');
    assert.equal(await error.isVisible(),false,'a stop on the next day can have an earlier time of day');
    assert.equal(await submit.isEnabled(),true);
    await page.locator('[name="startplayat_date"]').fill('2026-11-13');
    await page.locator('[name="startplayat_time"]').fill('03:00:00');
    assert.equal(await error.isVisible(),true,'editing the start recalculates the stop error');
    assert.equal(await expiryError.isVisible(),true);
    await page.locator('[name="playto_time"]').fill('04:00:00');
    assert.equal(await error.isVisible(),false);
    assert.equal(await submit.isDisabled(),true,'fixing the stop does not bypass the buffer error');
    await page.locator('[name="startplayat_time"]').fill('01:30:00');
    assert.equal(await submit.isEnabled(),true);
    await page.locator('[name="playto_time"]').fill('01:00:00');
    assert.equal(await error.isVisible(),true);
    await page.locator('[name="startplayat_time"]').fill('');
    await page.locator('[name="startplayat_date"]').fill('');
    assert.equal(await error.isVisible(),false,'clearing the scheduled start clears the comparison');
    await page.locator('[name="startplayat_date"]').fill('2026-11-12');
    await page.locator('[name="startplayat_time"]').fill('14:00:00');
    await page.locator('[name="playto_date"]').fill('2026-11-12');
    assert.equal(await error.isVisible(),true);
    await page.locator('[name="playto_date"]').fill('');
    await page.locator('[name="playto_time"]').fill('');
    assert.equal(await error.isVisible(),false,'clearing the optional stop clears the error');
    assert.equal(await submit.isEnabled(),true);
    await page.locator('[name="playto_date"]').fill('2026-11-13');
    await page.locator('[name="playto_time"]').fill('01:00:00');
    await submit.click();
    await dialog.waitFor({state:'hidden'});
    assert.deepEqual(commands().at(-1).body,{
      action:'jump',jumptopos:'2026-11-12T02:40:00.000Z',startplayat:'2026-11-12T14:00:00.000Z',playto:'2026-11-13T01:00:00.000Z',
    });
    await page.locator('[data-output="2"] [data-action="playto"]').click();
    assert.equal(await error.isVisible(),false,'stop-only dialog does not inherit the error');
    assert.equal(await page.locator('[name="playto_time"]').getAttribute('aria-invalid'),'false');
    await page.keyboard.press('Escape');
  });
  await t.test('wall-clock fields require future times and revalidate as time passes',async () => {
    const dialog = page.locator('#command-dialog');
    const form = page.locator('#command-form');
    const submit = form.locator('button[type="submit"]');
    const startError = page.locator('#schedule-error');
    const stopError = page.locator('#stop-time-error');
    const before = requests.filter(r => r.path.endsWith('/commands')).length;
    await page.locator('.timeline-marker[data-path="2"]').click();
    for (const key of ['startplayat','playto']) {
      assert.equal(await form.locator(`[name="${key}_date"]`).inputValue(),'');
      assert.equal(await form.locator(`[name="${key}_time"]`).inputValue(),'');
    }
    assert.match(await page.locator('#schedule-field legend').textContent(), /future time/);
    assert.match(await page.locator('#end-field legend').textContent(), /future time/);
    await form.locator('[name="startplayat_date"]').fill('2026-11-12');
    for (const time of ['11:59:59','12:00:00']) {
      await form.locator('[name="startplayat_time"]').fill(time);
      assert.match(await startError.textContent(), /must be in the future \(UTC\)/);
      assert.equal(await submit.isDisabled(),true);
    }
    await form.evaluate(el => el.dispatchEvent(new Event('submit',{bubbles:true,cancelable:true})));
    assert.equal(requests.filter(r => r.path.endsWith('/commands')).length,before);
    await form.locator('[name="startplayat_time"]').fill('12:00:05');
    assert.equal(await startError.isVisible(),false);
    await form.locator('[name="playto_date"]').fill('2026-11-11');
    await form.locator('[name="playto_time"]').fill('23:59:59');
    assert.match(await stopError.textContent(), /must be in the future \(UTC\)/);
    await form.locator('[name="playto_date"]').fill('2026-11-12');
    await form.locator('[name="playto_time"]').fill('12:00:00');
    assert.equal(await submit.isDisabled(),true);
    await form.locator('[name="playto_time"]').fill('12:00:10');
    assert.equal(await stopError.isVisible(),false);
    assert.equal(await submit.isEnabled(),true);
    try {
      await page.clock.setFixedTime(new Date('2026-11-12T12:00:05Z'));
      await page.waitForFunction(() => document.querySelector('#schedule-error').textContent.includes('must be in the future'));
      assert.equal(await submit.isDisabled(),true,'an open form cannot retain a start that has passed');
      await page.clock.setFixedTime(new Date('2026-11-12T12:00:10Z'));
      await page.waitForFunction(() => document.querySelector('#stop-time-error').textContent.includes('must be in the future'));
    } finally {await page.clock.setFixedTime(new Date(newest));}
    await page.keyboard.press('Escape');
    data.instances[0].outputs[2].status.playto='2026-11-12T11:59:59Z';
    await page.waitForTimeout(2200);
    await page.locator('[data-output="2"] [data-action="playto"]').click();
    assert.equal(await form.locator('[name="playto_date"]').inputValue(),'','expired stop times are not prefilled');
    assert.equal(await form.locator('[name="playto_time"]').inputValue(),'');
    await form.locator('[name="playto_date"]').fill('2026-11-12');
    await form.locator('[name="playto_time"]').fill('11:59:59');
    assert.match(await stopError.textContent(), /must be in the future/,'stop-only commands also require a future time');
    assert.equal(await submit.isDisabled(),true);
    await form.evaluate(el => el.dispatchEvent(new Event('submit',{bubbles:true,cancelable:true})));
    assert.equal(requests.filter(r => r.path.endsWith('/commands')).length,before);
    await form.locator('[name="playto_date"]').fill('');
    await form.locator('[name="playto_time"]').fill('');
    assert.equal(await stopError.isVisible(),false);
    assert.equal(await submit.isEnabled(),true);
    data.instances[0].outputs[2].status.playto=null;
    await page.keyboard.press('Escape');
  });
  await t.test('countdowns tick from start to stop and disappear when not applicable',async () => {
    const row=page.locator('[data-instance="st1"] [data-output="2"]');
    const timer=row.locator('.output-countdown');
    const target=data.instances[0].outputs[2];
    const waitText=text=>page.waitForFunction(text=>{
      const timer=document.querySelector('[data-instance="st1"] [data-output="2"] .output-countdown');
      return !timer.hidden && timer.textContent===text;
    },text);
    try {
      await timer.waitFor({state:'hidden'});
      target.status.state='Pause';
      target.status.playto='2026-11-12T12:00:30Z';
      await page.waitForTimeout(2200);
      assert.equal(await timer.isVisible(),false,'a paused output without a start has no countdown');
      target.status.startplayat='2026-11-12T12:00:10Z';
      await waitText('Starts in 00:00:10');
      assert.equal(await timer.getAttribute('role'),'timer');
      assert.equal(await timer.getAttribute('data-phase'),'start');
      assert.equal(await timer.getAttribute('aria-live'),'off');
      await row.screenshot({path:join(screenshots,'delayer-countdown-start.png')});
      const jump=row.locator('[data-action="jump"]');
      await jump.focus();
      await page.clock.setFixedTime(new Date('2026-11-12T12:00:01Z'));
      await waitText('Starts in 00:00:09');
      assert.equal(await jump.evaluate(el=>el===document.activeElement),true,'a countdown tick preserves keyboard focus');
      await page.clock.setFixedTime(new Date('2026-11-12T12:00:09.500Z'));
      await waitText('Starts in 00:00:01');
      await page.clock.setFixedTime(new Date('2026-11-12T12:00:10Z'));
      await waitText('Starts in 00:00:00');
      assert.equal(await timer.getAttribute('data-phase'),'start','the deadline alone does not imply playback has started');
      target.status.state='Play';target.status.startplayat=null;
      await waitText('Stops in 00:00:20');
      assert.equal(await timer.getAttribute('data-phase'),'stop');
      await row.screenshot({path:join(screenshots,'delayer-countdown-stop.png')});
      await page.clock.setFixedTime(new Date('2026-11-12T12:00:11Z'));
      await waitText('Stops in 00:00:19');
      await page.locator('[data-buffer-view="ring"]').click();
      assert.equal(await timer.textContent(),'Stops in 00:00:19');
      await page.setViewportSize({width:320,height:800});
      assert.equal(await row.evaluate(el=>el.scrollWidth<=el.clientWidth),true);
      await row.screenshot({path:join(screenshots,'delayer-countdown-mobile.png')});
      await page.setViewportSize({width:1440,height:1100});
      target.status.playto=null;
      await timer.waitFor({state:'hidden'});
      assert.equal(await timer.textContent(),'');
      assert.equal(await timer.evaluate(el=>getComputedStyle(el).display),'none','no empty countdown space remains');
      target.status.state='Live';target.status.playto='2026-11-12T12:00:30Z';
      await waitText('Stops in 00:00:19');
      target.status.playto='2026-11-12T12:00:10Z';
      await waitText('Stops in 00:00:00');
      target.status.state='Pause';
      await timer.waitFor({state:'hidden'});
      target.status.startplayat='2026-11-12T12:01:00Z';
      target.status.playto='2026-11-12T12:02:00Z';
      await waitText('Starts in 00:00:49');
      target.ready=false;target.stale=true;
      await timer.waitFor({state:'hidden'});
      target.ready=true;target.stale=false;
      await waitText('Starts in 00:00:49');
      offline=true;
      await timer.waitFor({state:'hidden'});
      offline=false;
      await waitText('Starts in 00:00:49');
      data.instances[0].pid=null;data.instances[0].process_state='stopped';
      await timer.waitFor({state:'hidden'});
    } finally {
      data=fixture();offline=false;
      await page.clock.setFixedTime(new Date(newest));
      await page.setViewportSize({width:1440,height:1100});
      await page.locator('[data-buffer-view="timeline"]').click();
      await page.waitForFunction(()=>document.querySelector('[data-instance="st1"] [data-output="2"] .output-state').textContent==='Delayed');
    }
  });
  await t.test('scheduled times keep steady colors according to fresh playback state',async () => {
    const row = page.locator('[data-instance="st1"] [data-output="2"]');
    const start = row.locator('.output-start-time');
    const stop = row.locator('.output-stop-time');
    const target = data.instances[0].outputs[2];
    const waitStates = async (startState, stopState) => page.waitForFunction(([startState,stopState]) => {
      const row=document.querySelector('[data-instance="st1"] [data-output="2"]');
      return row.querySelector('.output-start-time').dataset.scheduleState===startState
        && row.querySelector('.output-stop-time').dataset.scheduleState===stopState;
    },[startState,stopState]);
    const color = locator => locator.evaluate(el => getComputedStyle(el).color);
    const animation = locator => locator.evaluate(el => getComputedStyle(el).animationName);
    const checkSteady = async locator => {
      assert.equal(await animation(locator),'none');
      assert.equal(await locator.evaluate(el=>el.getAnimations().length),0);
      assert.equal(await locator.evaluate(el=>getComputedStyle(el).opacity),'1');
    };
    try {
      await page.emulateMedia({reducedMotion:'no-preference'});
      target.status.state='Pause';
      target.status.startplayat='2026-11-12T12:01:00Z';
      target.status.playto='2026-11-12T12:02:00Z';
      await waitStates('waiting','set');
      assert.equal(await color(start),'rgb(243, 189, 91)');
      assert.equal(await color(stop),'rgb(83, 211, 155)');
      assert.equal(await animation(stop),'none');
      assert.match(await start.getAttribute('title'),/Waiting for scheduled playback start/);
      await checkSteady(start);
      await checkSteady(stop);
      await row.screenshot({path:join(screenshots,'delayer-scheduled-start-waiting.png')});
      await page.emulateMedia({reducedMotion:'reduce'});
      assert.equal(await animation(start),'none');
      assert.equal(await color(start),'rgb(243, 189, 91)');
      await page.emulateMedia({reducedMotion:'no-preference'});
      await page.clock.setFixedTime(new Date('2026-11-12T12:01:01Z'));
      await waitStates('idle','set');
      assert.equal(await color(stop),'rgb(83, 211, 155)','passing the start deadline does not imply playback has started');
      target.status.state='Play';
      target.status.startplayat=null;
      await waitStates('idle','active');
      assert.equal(await color(stop),'rgb(239, 116, 116)');
      assert.match(await stop.getAttribute('title'),/Playback active/);
      await checkSteady(stop);
      await row.screenshot({path:join(screenshots,'delayer-scheduled-stop-active.png')});
      await page.emulateMedia({reducedMotion:'reduce'});
      assert.equal(await animation(stop),'none');
      assert.equal(await color(stop),'rgb(239, 116, 116)');
      await page.emulateMedia({reducedMotion:'no-preference'});
      target.status.state='Pause';
      await waitStates('idle','set');
      assert.equal(await color(stop),'rgb(83, 211, 155)');
      target.status.state='Live';
      await waitStates('idle','active');
      target.ready=false;target.stale=true;
      await waitStates('idle','set');
      assert.equal(await animation(stop),'none','stale status does not report active playback');
      target.ready=true;target.stale=false;
      await waitStates('idle','active');
      offline=true;
      await waitStates('idle','set');
      await checkSteady(stop);
      offline=false;
      await waitStates('idle','active');
      data.instances[0].pid=null;
      data.instances[0].process_state='stopped';
      await waitStates('idle','set');
      await checkSteady(stop);
      target.status.playto=null;
      await waitStates('idle','unset');
      assert.equal(await animation(stop),'none');
    } finally {
      data=fixture();offline=false;
      await page.clock.setFixedTime(new Date(newest));
      await page.emulateMedia({reducedMotion:'no-preference'});
      await page.waitForFunction(()=>document.querySelector('[data-instance="st1"] [data-output="2"] .output-state').textContent==='Delayed');
    }
  });
  await t.test('Play and Pause require a temporary, single-use arm for the selected output',async () => {
    const row=page.locator('[data-instance="st1"] [data-output="2"]');
    const other=page.locator('[data-instance="st1"] [data-output="1"]');
    const arm=row.locator('[data-action="arm"]');
    const play=row.locator('[data-action="play"]');
    const pause=row.locator('[data-action="pause"]');
    const commands=()=>requests.filter(r=>r.path.endsWith('/commands'));
    const before=commands().length;
    assert.equal(await play.isDisabled(),true);
    assert.equal(await pause.isDisabled(),true);
    assert.equal(await arm.getAttribute('aria-pressed'),'false');
    // Even a stale DOM button enabled outside the renderer cannot skip arming.
    await play.evaluate(el=>{el.disabled=false;el.click();});
    assert.equal(commands().length,before);
    assert.equal(await play.isDisabled(),true);
    await arm.focus();
    await page.keyboard.press('Space');
    assert.equal(await arm.getAttribute('aria-pressed'),'true');
    assert.equal(await arm.locator('.arm-label').textContent(),'Armed · 20s');
    assert.equal(await play.isEnabled(),true);
    assert.equal(await pause.isEnabled(),true);
    assert.equal(await other.locator('[data-action="play"]').isDisabled(),true);
    await page.waitForTimeout(2200);
    assert.equal(await arm.getAttribute('aria-pressed'),'true','polling keeps the guard open');
    assert.equal(await arm.evaluate(el=>el===document.activeElement),true);
    await row.screenshot({path:join(screenshots,'delayer-playback-armed.png')});
    await row.locator('.row-actions').screenshot({path:join(screenshots,'delayer-playback-armed-controls.png')});
    await page.keyboard.press('Space');
    assert.equal(await play.isDisabled(),true,'a second activation closes the lid');
    assert.equal(commands().length,before);
    await arm.click();
    await other.locator('[data-action="arm"]').click();
    assert.equal(await play.isDisabled(),true,'only one output can be armed');
    await page.keyboard.press('Escape');
    assert.equal(await other.locator('[data-action="pause"]').isDisabled(),true);
    await arm.click();
    await page.locator('#tab-resources').click();
    await page.locator('#tab-instances').click();
    assert.equal(await play.isDisabled(),true,'changing tabs closes the lid');
    await arm.click();
    await page.evaluate(()=>window.dispatchEvent(new Event('blur')));
    assert.equal(await play.isDisabled(),true,'leaving the window closes the lid');
    await arm.click();
    await page.keyboard.press('Tab');
    assert.equal(await play.evaluate(el=>el===document.activeElement),true);
    await page.waitForTimeout(10000);
    assert.equal(await play.isEnabled(),true,'arming lasts longer than the previous eight-second window');
    await page.waitForFunction(()=>document.querySelector('[data-instance="st1"] [data-output="2"] [data-action="play"]').disabled,null,{timeout:15000});
    assert.equal(await arm.getAttribute('aria-pressed'),'false','arming expires without a command');
    assert.equal(await arm.evaluate(el=>el===document.activeElement),true);
    assert.equal(commands().length,before);
    await arm.click();
    await page.keyboard.press('Tab');
    assert.equal(await play.evaluate(el=>el===document.activeElement),true);
    await page.keyboard.press('Enter');
    await page.waitForFunction(()=>document.querySelector('#notice').textContent.includes('#42'));
    assert.equal(commands().length,before+1);
    assert.deepEqual(commands().at(-1).body,{action:'play'});
    assert.equal(await play.isDisabled(),true,'a command consumes the arm');
    assert.equal(await pause.isDisabled(),true);
    await arm.click();
    commandError=true;
    await pause.click();
    await page.waitForFunction(()=>document.querySelector('#notice').textContent==='Command unavailable');
    commandError=false;
    assert.equal(await pause.isDisabled(),true,'a failed command also consumes the arm');
    assert.equal(await arm.getAttribute('aria-pressed'),'false');
    assert.equal(commands().length,before+2);
    await row.screenshot({path:join(screenshots,'delayer-playback-locked.png')});
    await row.locator('.row-actions').screenshot({path:join(screenshots,'delayer-playback-locked-controls.png')});
    // Expired recordings still require recovery before Play, even when armed.
    const expired=page.locator('[data-instance="st1"] [data-output="0"]');
    await expired.locator('[data-action="arm"]').click();
    assert.equal(await expired.locator('[data-action="play"]').isDisabled(),true);
    assert.equal(await expired.locator('[data-action="pause"]').isEnabled(),true);
    await page.keyboard.press('Escape');
    await page.setViewportSize({width:320,height:800});
    await arm.click();
    await row.screenshot({path:join(screenshots,'delayer-playback-mobile-320.png')});
    assert.equal(await row.evaluate(el=>el.scrollWidth<=el.clientWidth),true);
    await page.keyboard.press('Escape');
    await page.setViewportSize({width:1440,height:1100});
  });
  await t.test('independent play/pause and wall-clock scheduling preserve API payloads',async () => {
    await page.locator('[data-output="2"] [data-action="arm"]').click();
    await page.locator('[data-output="2"] [data-action="pause"]').click();
    await page.waitForFunction(() => document.querySelector('#notice').textContent.includes('#42'));
    assert.deepEqual(requests.findLast(r => r.path.endsWith('/outputs/2/commands')).body,{action:'pause'});
    await page.locator('.timeline-marker[data-path="2"]').click();
    await page.locator('[name="startplayat_date"]').fill('2026-11-13');
    await page.locator('[name="startplayat_time"]').fill('01:30:00');
    await page.locator('#command-form button[type="submit"]').click();
    await page.locator('#command-dialog').waitFor({state:'hidden'});
    assert.equal(requests.findLast(r => r.path.endsWith('/outputs/2/commands')).body.startplayat,'2026-11-13T01:30:00.000Z');
    await page.locator('[data-output="2"] [data-action="playto"]').click();
    assert.equal(await page.locator('#jump-field').isVisible(),false);
    await page.locator('#command-form button[type="submit"]').click();
    await page.locator('#command-dialog').waitFor({state:'hidden'});
    assert.deepEqual(requests.findLast(r => r.path.endsWith('/outputs/2/commands')).body,{action:'playto'});
  });
  await t.test('ring view, tabs, resources and activity drawer',async () => {
    await page.locator('[data-buffer-view="ring"]').click();
    assert.equal(await page.locator('.buffer-output').count(),3);
    await page.locator('.buffer-output[data-path="2"]').click();
    assert.equal(await page.locator('[data-output="2"]').getAttribute('data-selected'),'true');
    await page.screenshot({path:join(screenshots,'delayer-cockpit-ring.png'),fullPage:true});
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
    for (const width of [1440,1280,1100,1051,1024,768,640,375,320]) {
      await page.setViewportSize({width,height:1000});
      for (const view of ['timeline','ring']) {
        await page.locator(`[data-buffer-view="${view}"]`).click();
        assert.equal(await page.evaluate(() => document.documentElement.scrollWidth <= innerWidth),true,`${view} at ${width}px`);
        const layouts=await page.locator('.row-actions').evaluateAll(groups=>groups.map(group=>{
          const box=selector=>group.querySelector(selector).getBoundingClientRect();
          const guard=box('.playback-guard'),jump=box('[data-action="jump"]'),until=box('[data-action="playto"]');
          return Math.abs(jump.left-until.left)<1 && Math.abs(jump.width-until.width)<1 &&
            Math.abs(guard.top-jump.top)<1 && Math.abs(guard.bottom-until.bottom)<1 &&
            guard.right<=jump.left && jump.bottom<until.top;
        }));
        assert.ok(layouts.every(Boolean),`aligned control groups in ${view} at ${width}px`);
        if (view==='ring' && width>=1100) {
          const centered=await page.locator('.output-row').evaluateAll(rows=>rows.map(row=>{
            const info=row.querySelector('.output-position').getBoundingClientRect();
            const controls=row.querySelector('.row-actions').getBoundingClientRect();
            return Math.abs((info.top+info.bottom)-(controls.top+controls.bottom))<2 && controls.left>=info.right;
          }));
          assert.ok(centered.every(Boolean),`ring controls beside playback information at ${width}px`);
        }
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
    assert.equal(await card.locator('[data-output="2"] [data-action="play"]').isDisabled(),true);
    await card.locator('[data-output="2"] [data-action="arm"]').click();
    assert.equal(await card.locator('[data-output="2"] [data-action="play"]').isEnabled(),true);
    await page.keyboard.press('Escape');
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
    await page.locator('[data-instance="st1"] [data-output="2"] [data-action="arm"]').click();
    for (const output of data.instances[0].outputs) {output.stale=true;output.ready=false;}
    await page.waitForTimeout(2200);
    assert.equal(await page.locator('.timeline-marker').count(),0);
    assert.equal(await page.locator('[data-output="2"] [data-action="play"]').isDisabled(),true);
    assert.equal(await page.locator('[data-output="2"] [data-action="arm"]').getAttribute('aria-pressed'),'false');
    offline=true;
    await page.waitForFunction(() => document.querySelector('#connection').classList.contains('offline'));
    assert.equal(await page.locator('[data-instance="st2"] [data-action="start"]').isDisabled(),true);
    assert.equal(await page.locator('#storage-summary').textContent(),'—');
    offline=false;data=fixture();
    await page.waitForFunction(() => document.querySelector('.timeline-marker'));
    assert.equal(await page.locator('[data-output="2"] [data-action="play"]').isDisabled(),true,'reconnection does not re-arm');
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
    await page.keyboard.press('Escape');
    await page.locator('[data-instance="st1"] [data-action="stop"]').click();
    expire=true;
    await page.locator('#login-view').waitFor();
    assert.equal(await page.locator('#notification-drawer').isVisible(),false);
    assert.equal(await page.locator('#stop-dialog').isVisible(),false);
    assert.equal(await page.locator('.instance-card').count(),0);
    assert.match(await page.locator('#login-error').textContent(),/expired/);
    assert.equal(await page.locator('#notification-count').textContent(),'0');
  });
  assert.deepEqual(errors,[],'No JavaScript or CSP errors');
});
