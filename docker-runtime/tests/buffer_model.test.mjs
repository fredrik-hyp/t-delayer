import assert from 'node:assert/strict';
import {test} from 'node:test';
import {classifyPlayback, buildMarkerLayout, formatDelay, scheduledBufferExpiry, formatCountdown} from '../web/buffer-model.js';

const bounds = {oldestMs: 10_000, newestMs: 110_000};
test('countdowns keep full seconds, clamp elapsed deadlines and support days', () => {
  assert.equal(formatCountdown(0),'00:00:00');
  assert.equal(formatCountdown(-1000),'00:00:00');
  assert.equal(formatCountdown(1),'00:00:01');
  assert.equal(formatCountdown(999),'00:00:01');
  assert.equal(formatCountdown(1001),'00:00:02');
  assert.equal(formatCountdown(229_000),'00:03:49');
  assert.equal(formatCountdown(3661_000),'01:01:01');
  assert.equal(formatCountdown(25*3600_000),'1 d 01:00:00');
  assert.equal(formatCountdown(NaN),'—');
  assert.equal(formatCountdown(Infinity),'—');
});

test('scheduled playback is invalid at or after the recorded position expires', () => {
  const recorded = Date.parse('2026-11-16T09:30:00Z');
  const now = Date.parse('2026-11-16T09:55:00Z');
  const retention = 3600_000;
  const expires = Date.parse('2026-11-16T10:30:00Z');
  assert.equal(scheduledBufferExpiry(recorded, expires - 1, retention, now), null);
  assert.equal(scheduledBufferExpiry(recorded, expires, retention, now), expires);
  assert.equal(scheduledBufferExpiry(recorded, expires + 86400_000, retention, now), expires);
  // A buffer that is still filling must use configured retention, not its span.
  assert.equal(scheduledBufferExpiry(recorded, now + 10 * 60_000, retention, now), null);
  assert.equal(scheduledBufferExpiry(recorded, now + 10 * 60_000, 30 * 60_000, now), recorded + 30 * 60_000);
  for (const args of [
    [NaN, expires, retention, now], [recorded, NaN, retention, now],
    [recorded, expires, undefined, now], [recorded, expires, 0, now],
    [recorded, expires, -1, now], [recorded, expires, retention, NaN],
    [recorded, now, 1000, now], [recorded, now - 1, 1000, now],
  ]) assert.equal(scheduledBufferExpiry(...args), null);
});

test('buffer boundaries distinguish expired, delayed, live and unavailable positions', () => {
  assert.equal(classifyPlayback(9_999,10_000,110_000).state,'expired');
  assert.equal(classifyPlayback(10_000,10_000,110_000).state,'available');
  assert.equal(classifyPlayback(108_000,10_000,110_000).state,'live');
  assert.equal(classifyPlayback(110_000,10_000,110_000).state,'live');
  assert.equal(classifyPlayback(112_001,10_000,110_000).state,'invalid');
  for (const args of [[NaN,10_000,110_000],[20_000,10_000,10_000],[20_000,NaN,110_000]]) {
    assert.equal(classifyPlayback(...args).state,'invalid');
  }
});
test('nearby output labels occupy separate lanes while stems retain their real positions', () => {
  const outputs = [10_000,10_100,109_000,110_000,9_000,NaN].map((positionMs,id) => ({id,positionMs}));
  const markers = buildMarkerLayout(outputs,bounds);
  assert.equal(markers[0].percent,0);
  assert.equal(markers[3].percent,100);
  assert.notEqual(markers[0].lane,markers[1].lane);
  assert.notEqual(markers[2].lane,markers[3].lane);
  assert.equal(markers[4].percent,null);
  assert.equal(markers[5].percent,null);
  assert.deepEqual(markers.map(m => m.id),outputs.map(m => m.id));
});
test('delay formatting supports multi-day recordings', () => {
  assert.equal(formatDelay(24*3600_000),'1 d');
  assert.equal(formatDelay(25*3600_000),'1 d 1 h');
  assert.equal(formatDelay(90_000),'1 m');
});
