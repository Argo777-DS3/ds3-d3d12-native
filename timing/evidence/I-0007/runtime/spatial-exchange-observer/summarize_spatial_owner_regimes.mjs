import fs from 'node:fs';

const [regimesFile,rttiFile,output]=process.argv.slice(2);
if(!regimesFile||!rttiFile||!output)
  throw new Error('usage: node summarize_spatial_owner_regimes.mjs <regimes.json> <rtti.json> <output.json>');

const regimes=JSON.parse(fs.readFileSync(regimesFile,'utf8'));
const rtti=JSON.parse(fs.readFileSync(rttiFile,'utf8'));
const owners=rtti.owners.map(owner=>{
  const counts=regimes.by_owner[owner.owner]||{};
  return {
    owner:owner.owner,
    captured_forward_entries:owner.captured_forward_entries,
    owner_type:owner.owner_rtti?.type_name??null,
    player:owner.player,
    player_type:owner.player_rtti?.type_name??null,
    module28_object:owner.module28_object,
    module28_type:owner.module28_rtti?.type_name??null,
    entry_rows:counts.entries??0,
    matches_delta:counts.matches_delta??0,
    matches_30x_delta:counts.matches_30x_delta??0,
    other:counts.other??0,
  };
});
function counts(field){
  const result={};
  for(const owner of owners){
    const key=owner[field]??'unknown';
    result[key]=(result[key]??0)+1;
  }
  return result;
}
const scaled=owners.filter(owner=>owner.matches_30x_delta>0);
const unscaled=owners.filter(owner=>owner.matches_30x_delta===0);
const maximumForwardEntries=Math.max(...owners.map(owner=>owner.captured_forward_entries));
const result={
  schema:'I-0007-spatial-owner-regimes-v1',
  owner_count:owners.length,
  owner_type_counts:counts('owner_type'),
  player_type_counts:counts('player_type'),
  module28_type_counts:counts('module28_type'),
  scaled_forward_owner_count:scaled.length,
  scaled_forward_player_type_counts:Object.fromEntries(
    [...new Set(scaled.map(owner=>owner.player_type))].map(type=>
      [type,scaled.filter(owner=>owner.player_type===type).length])),
  scaled_forward_entry_count_distribution:Object.fromEntries(
    [...new Set(scaled.map(owner=>owner.captured_forward_entries))].sort((a,b)=>a-b).map(value=>
      [value,scaled.filter(owner=>owner.captured_forward_entries===value).length])),
  scaled_transient_owner_count:scaled.filter(owner=>owner.captured_forward_entries<maximumForwardEntries).length,
  unscaled_only_owner_count:unscaled.length,
  unscaled_only_player_type_counts:Object.fromEntries(
    [...new Set(unscaled.map(owner=>owner.player_type))].map(type=>
      [type,unscaled.filter(owner=>owner.player_type===type).length])),
  persistent_unscaled_owner_count:unscaled.filter(owner=>owner.captured_forward_entries===maximumForwardEntries).length,
  maximum_captured_forward_entries:maximumForwardEntries,
  owners,
  limit:'RTTI was read after instrument stop from the still-live process. Type identity is observed for these instances only; it does not assign gameplay role to the 30x regime or prove all instances remain valid across scenes.',
};
fs.writeFileSync(output,JSON.stringify(result,null,2)+'\n');
console.log(JSON.stringify(result,null,2));
