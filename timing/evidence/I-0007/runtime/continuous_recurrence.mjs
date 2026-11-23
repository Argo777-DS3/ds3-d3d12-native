const f=Math.fround,T=1/60,tolerance=T*1e-5;
const native=(s,r)=>r?(s>.5?f(s*f(.8)):.5):(s<1?f(s*f(1.2)):1);
export class ContinuousRecurrence {
  constructor(s){this.value=this.start=this.end=s;this.phase=0;this.branch=null;}
  advance(active,reduce,dt){
    if(!Number.isFinite(dt)||dt<=0||dt>.05)throw Error('Invalid caller delta');
    if(!active)return this.value;
    if(this.branch!==reduce){this.start=this.value;this.end=native(this.value,reduce);this.phase=0;this.branch=reduce;}
    this.phase+=dt;
    while(this.phase+tolerance>=T){this.value=this.start=this.end;this.phase=Math.max(0,this.phase-T);this.end=native(this.start,reduce);}
    if(this.phase>0)this.value=f(this.start+(this.end-this.start)*this.phase/T);
    return this.value;
  }
}
