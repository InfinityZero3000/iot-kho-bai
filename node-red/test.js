// node test.js — chay logic cac function node voi flow/env gia lap
const assert = require('assert');
const nodes = require('./flows.json');
const ctx = {};
const status = [];
const run = (id, msg, env = { BLYNK_TOKEN: 'T' }) =>
  new Function('msg', 'flow', 'env', 'node', nodes.find(n => n.id === id).func)(
    msg,
    { get: k => ctx[k], set: (k, v) => (ctx[k] = v) },
    { get: k => env[k] },
    { status: s => status.push(s), warn() {} });
const state = (topic, payload) => run('f1', { topic, payload });

// f1: trang thai
assert.strictEqual(state('kho/khac', 1), null);                      // topic la -> bo qua
assert.strictEqual(state('kho/status', 'online')[5].payload, 'ESP32 online');
state('kho/zone/A', { pir: 0, on: 1 });
let o = state('kho/zone/B', { pir: 1, on: 1 });
assert.deepStrictEqual([o[0].payload, o[1].payload, o[3].payload, o[4].payload],
  ['SÁNG (giữ đèn)', 'SÁNG (có xe nâng)', 33, 2]);
assert.strictEqual(state('kho/zone/C', null)[2].payload, 'Tắt');      // payload hong khong lam sap
assert.strictEqual(state('kho/status', 'offline')[0].payload, 'Không rõ');
state('kho/status', 'online');

// f2: dien nang (2 khu sang, 150W, 1 gio) -> dung 0.3 kWh, co so 0.45 kWh
ctx.e = { used: 0, base: 0, t: Date.now() - 3600000 };
run('f2', {});
assert.strictEqual(ctx.e.used, 0);                                    // qua 120s -> khong cong don
ctx.e.t = Date.now() - 60000;
run('f2', {});
assert(Math.abs(ctx.e.used - 2 * 150 / 60 / 1000) < 1e-6, ctx.e.used); // 2 khu * 150W * 1 phut

// f3: URL Blynk
const u = run('f3', {}).url;
assert(/token=T&V0=1&V1=1&V2=0&V3=1&V4=33&V5=/.test(u), u);
assert.strictEqual(run('f3', {}, {}), null);                          // thieu token
assert.strictEqual(status.at(-1).text, 'thiếu BLYNK_TOKEN');

// f4/f5: poll che do tu Blynk
assert(run('f4', {}).url.endsWith('&V10'));
assert.strictEqual(run('f5', { payload: '1' }).payload, 'on');
assert.strictEqual(run('f5', { payload: '' }), null);                 // rong/loi HTTP -> bo qua
assert.strictEqual(run('f5', { payload: '{"error":1}' }), null);

// f6: mo phong ESP32
const sim = m => run('f6', m)[0].slice(1).map(x => x.payload.on).join('');
ctx.pos = 0; ctx.simMode = undefined;
assert.strictEqual(sim({ topic: '' }), '010');                          // tick: xe nang sang khu B
assert.strictEqual(sim({ topic: 'kho/cmd/mode', payload: 'on' }), '111');
assert.strictEqual(sim({ topic: 'kho/cmd/mode', payload: 'off' }), '000');
assert.strictEqual(sim({ topic: 'kho/cmd/mode', payload: 'auto' }), '010');
// f7/f8: cau noi Wokwi (HiveMQ) <-> kho/*
const W = 'huit-khobai-nht/';
assert.strictEqual(run('f7', { topic: W + 'khu1/chedo', payload: 'AUTO' }), null);   // topic khong lien quan
let z = run('f7', { topic: W + 'khu2/pir', payload: 1 });                            // mqtt in auto-detect: "1" -> so 1
assert.deepStrictEqual([z.topic, z.payload], ['kho/zone/B', { pir: 1, on: 0 }]);
z = run('f7', { topic: W + 'khu2/den', payload: 'ON' });
assert.deepStrictEqual(z.payload, { pir: 1, on: 1 });                                // nho pir cua lan truoc
assert.strictEqual(run('f7', { topic: W + 'khu3/den', payload: 'ON' }).topic, 'kho/zone/C');
assert.strictEqual(run('f7', { topic: W + 'trangthai', payload: 'online' }).topic, 'kho/status');
assert.strictEqual(ctx.wokwiOn, true);
assert.deepStrictEqual(run('f6', { topic: '' }), null);                              // Wokwi online -> sim im lang
run('f7', { topic: W + 'trangthai', payload: 'offline' });
assert.notStrictEqual(run('f6', { topic: '' }), null);                               // Wokwi offline -> sim chay lai
const cmds = run('f8', { payload: 'on' })[0];
assert.deepStrictEqual(cmds.map(c => c.topic + '=' + c.payload),
  [1, 2, 3].map(n => W + 'khu' + n + '/lenh=ON'));
assert.strictEqual(run('f8', { payload: 'x' }), null);

console.log('OK');
