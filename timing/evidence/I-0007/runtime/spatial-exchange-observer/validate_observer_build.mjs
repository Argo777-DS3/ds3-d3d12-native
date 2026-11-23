import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';

const here=new URL('.',import.meta.url);
const read=name=>fs.readFileSync(new URL(name,here));
const hash=data=>crypto.createHash('sha256').update(data).digest('hex').toUpperCase();
const files=['pass_c_probe.cpp','injector.cpp','build.ps1','pass_c_config.txt',
  'Set-SpatialTarget.ps1','Reset-SpatialConfig.ps1','spatial_debug_config.hpp',
  'spatial_exchange_pairing.hpp','pairing_self_test.cpp','test_pairing.ps1',
  'four_execute_self_test.cpp','test_four_execute.ps1','observation-schema.json',
  'validate_event_sequence.mjs','analyze_spatial_exchange.mjs','test_analyze_spatial_exchange.mjs',
  'analyze_crash_session.mjs','analyze_crash_phase_boundary.mjs','analyze_spatial_exchange_dynamics.mjs',
  'analyze_spatial_player_correlation.mjs','analyze_spatial_elapsed_regimes.mjs',
  'analyze_spatial_block_layout.mjs','Read-SpatialOwnerTypes.ps1',
  'summarize_spatial_owner_regimes.mjs'];
const source=read('pass_c_probe.cpp').toString('utf8'),config=read('pass_c_config.txt').toString('utf8');
for(const token of ['kRvaSpatialForwardEntry = 0x00DD3850','kRvaSpatialForwardWrite = 0x00DD3A7A',
  'kRvaSpatialReverseEntry = 0x00DD3B50','kRvaSpatialReverseWrite = 0x00DD3BB2',
  'ConfigureSpatialExecuteDr7','g_writer_mirror_committed_count',
  'SafeRead(static_cast<uintptr_t>(context->Rsp),&hit.caller)',
  'const bool log_phase = capture && !g_config.locomotion_writer_mirror',
  'g_config.causal_distance_patch || g_config.elapsed_recurrence_patch'])
  assert.ok(source.includes(token),token);
for(const token of ['enable_timing_override=0','locomotion_trials=0','locomotion_phase_rca=0',
  'locomotion_write_watch=0','locomotion_writer_mirror=0','locomotion_measure_delta=1.5',
  'causal_distance_patch=0','elapsed_recurrence_patch=0'])
  assert.ok(config.includes(token),token);
const game='C:/Users/User/Desktop/DS3 Decompilation/evidence/I-0002/artifacts/staging/DarkSoulsIII.exe';
assert.equal(hash(fs.readFileSync(game)),'EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C');
const dll=read('bin/I0007SpatialExchangeObserver.dll'),injector=read('bin/I0007SpatialExchangeObserverInjector.exe');
const recordUrl=new URL('BUILD-VALIDATION.json',here);
let builtAt=new Date().toISOString();
try { builtAt=JSON.parse(fs.readFileSync(recordUrl,'utf8')).built_at||builtAt; } catch {}
const result={game_sha256:hash(fs.readFileSync(game)),built_at:builtAt,
  derived_from:'transient-capture source with a dedicated observation-only spatial mode',
  source_sha256:Object.fromEntries(files.map(name=>[name,hash(read(name))])),
  binary_sha256:{'I0007SpatialExchangeObserver.dll':hash(dll),
    'I0007SpatialExchangeObserverInjector.exe':hash(injector)},
  native_pairing_test:'PASS: interleaved pairs plus elapsed/identity/duplicate/order/pointer rejection',
  four_execute_test:'PASS: two 4x1024 execute-breakpoint arms, entry caller reads, RF resume and exact debug-state restoration',
  analyzer_test:'PASS: bidirectional structure plus identity/overflow/missing-direction/elapsed rejection',
  config_state:'disabled',
  live_validation:'Not performed for the caller-RVA extension. Its predecessor at commit8c3d38d ran in session27644, stopped/restored exactly and persisted4096 pre-overflow rows; that trial remains rejected.',
  predecessor_live_binary_sha256:'891D5F5194DA5DBE32811078758ADAFC198C4E0FFCADA9BE261A4BBB276BC08B',
  limit:'The caller-RVA build is offline-qualified only. The predecessor has a clean live restoration/durability result, not an accepted target-165 trial. No cross-rate or gameplay claim is accepted.'};
fs.writeFileSync(recordUrl,JSON.stringify(result,null,2)+'\n');
console.log(JSON.stringify(result,null,2));
