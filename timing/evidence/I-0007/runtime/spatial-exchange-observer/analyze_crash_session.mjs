import fs from 'node:fs';
import path from 'node:path';

function rows(file) {
  const lines=fs.readFileSync(file,'utf8').trim().split(/\r?\n/);
  const header=lines.shift().split(',');
  return lines.filter(Boolean).map(line=>{
    const values=line.split(',');
    return {
      ...Object.fromEntries(header.map((name,index)=>[name,values[index]??''])),
      __complete:values.length===header.length,
      __column_count:values.length,
      __expected_column_count:header.length,
    };
  });
}

function parseWer(file) {
  const fields={};
  for (const line of fs.readFileSync(file,'utf16le').split(/\r?\n/)) {
    const at=line.indexOf('=');
    if (at>0) fields[line.slice(0,at)]=line.slice(at+1);
  }
  return {
    event_type:fields.EventType??null,
    report_identifier:fields.ReportIdentifier??null,
    fault_module:fields['Sig[3].Value']??null,
    exception_code:fields['Sig[7].Value']??null,
    exception_data:fields['Sig[8].Value']??null,
  };
}

function faultAddress(session){
  const events=fs.readFileSync(path.join(session,'crash','application-events.txt'),'utf8');
  const match=/Fault offset:\s*(0x[0-9a-f]+)/i.exec(events);
  if(!match) return null;
  const address=BigInt(match[1]);
  let mapped=null,imageBase=null;
  for(const line of fs.readFileSync(path.join(session,'modules-start.csv'),'utf8').split(/\r?\n/).slice(1)){
    const module=/^"([^"]+)",(0x[0-9A-Fa-f]+),(\d+),/.exec(line);
    if(!module) continue;
    const base=BigInt(module[2]),size=BigInt(module[3]);
    if(module[1]==='DarkSoulsIII.exe') imageBase=base;
    if(address>=base&&address<base+size) mapped=module[1];
  }
  return {
    address:`0x${address.toString(16).padStart(16,'0')}`,
    mapped_start_module:mapped,
    image_base:imageBase===null?null:`0x${imageBase.toString(16).padStart(16,'0')}`,
    delta_from_image_base:imageBase===null?null:`0x${(address-imageBase).toString(16)}`,
  };
}

const [sessionArg,outputArg]=process.argv.slice(2);
if(!sessionArg||!outputArg) throw new Error('usage: node analyze_crash_session.mjs <session-dir> <output.json>');
const session=path.resolve(sessionArg);
const events=rows(path.join(session,'locomotion-events.csv'));
const phases=rows(path.join(session,'locomotion-phases.csv'));
const trial2Start=events.find(row=>row.trial==='2'&&row.event==='measure_start');
const trial2End=events.find(row=>row.trial==='2'&&row.event==='trial_end');
if(!trial2Start) throw new Error('trial 2 start is absent');

const startSequence=Number(trial2Start.seq);
const seen=new Set();
let maxSequence=0;
for(const name of fs.readdirSync(session).filter(name=>name.endsWith('.csv'))){
  const text=fs.readFileSync(path.join(session,name),'utf8');
  if(!text.startsWith('seq,')) continue;
  for(const line of text.split(/\r?\n/).slice(1)){
    const match=/^(\d+),/.exec(line);
    if(!match) continue;
    const sequence=Number(match[1]);
    if(sequence>=startSequence) seen.add(sequence);
    if(sequence>maxSequence) maxSequence=sequence;
  }
}
let missing=0;
for(let sequence=startSequence;sequence<=maxSequence;++sequence) if(!seen.has(sequence)) ++missing;

const lastPhase=phases.at(-1);
const qpcFrequency=Number((fs.readFileSync(path.join(session,'session-info.txt'),'utf8')
  .split(/\r?\n/).find(line=>line.startsWith('qpc_frequency='))??'').split('=')[1]);
const structural=JSON.parse(fs.readFileSync(path.join(session,'spatial-exchange-summary.json'),'utf8'));
const report={
  schema:'I-0007-spatial-crash-audit-v1',
  session:path.basename(session),
  accepted_control_60:structural,
  rejected_trial_165:{
    start_sequence:startSequence,
    start_qpc:Number(trial2Start.qpc),
    end_recorded:Boolean(trial2End),
    last_durable_sequence:maxSequence,
    last_phase:{
      sequence:Number(lastPhase.seq),
      qpc:Number(lastPhase.qpc),
      seconds_after_start:(Number(lastPhase.qpc)-Number(trial2Start.qpc))/qpcFrequency,
      csv_complete:lastPhase.__complete,
      csv_column_count:lastPhase.__column_count,
      csv_expected_column_count:lastPhase.__expected_column_count,
      site:lastPhase.phase,
      event:lastPhase.event,
      caller_rva:lastPhase.caller_rva,
      self:lastPhase.self,
      context:lastPhase.context,
      animation_id:Number(lastPhase.animation_id),
      animation_time:Number(lastPhase.animation_time),
    },
    missing_sequence_slots_through_last_serialized_prefix:missing,
    sequence_gap_interpretation:'Spatial rows were retained in memory until explicit stop, so they should dominate the gaps. Other CSV streams could also lose rows generated after their last periodic flush. The gap is therefore an upper bound, not an exact recovery of spatial hits.',
    upper_bound_total_spatial_hits_through_last_serialized_prefix:structural.rows+missing,
    per_trial_hit_limit:4096,
    global_hit_limit:8192,
    overflow_supported:false,
    operator_observation:'The crash visibly coincided with the character stopping after movement.',
    comparison_usable:false,
  },
  windows_error:{
    ...parseWer(path.join(session,'crash','Report.wer')),
    fault_target:faultAddress(session),
  },
  completion_marker_present:fs.existsSync(path.join(session,'COMPLETE')),
  conclusion:'Target 60 is structurally accepted. Target 165 is a rejected crash capture. The final phase line is a partially serialized before-row, so it does not prove that the hooked original function began. The CSV and WER evidence do not identify whether observer logging, the original dispatcher, or another path supplied the invalid execute target.',
};
fs.mkdirSync(path.dirname(path.resolve(outputArg)),{recursive:true});
fs.writeFileSync(outputArg,JSON.stringify(report,null,2)+'\n');
console.log(JSON.stringify(report,null,2));
