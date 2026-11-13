import assert from 'node:assert/strict';
import {test} from 'node:test';
import {classifyPlayback, buildMarkerLayout, formatDelay} from '../web/buffer-model.js';

const bounds = {oldestMs: 10_000, newestMs: 110_000};
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
