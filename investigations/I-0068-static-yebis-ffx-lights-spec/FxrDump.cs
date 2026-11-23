// I-0068 part AQ: structural dump of one DS3 FXR3 effect (nodes, configs, actions, properties, modifiers) for Read-Fxr.ps1. Read-only.
// Layout (OBSERVED on the sign effects 30000-30052): node: type i16@0, configCount@8, actionCount@12, nodeCount@16, offsets u64@24/32/40;
// config: actionCount@12, offset@24 (stride 32); action (stride 64): type@0, fields1@8, section10@12, props1@16, fields2@20, props2@28, offsets@32/40/48;
// property (stride 40): typeEnum@0 (valueType bits 0-1, function bits 4-7, loop bit 12), fieldCount@8, fieldOffset@16, modOffset@24, modCount@32;
// modifier (stride 32): typeEnum@0, fieldCount@8, propCount@12, fieldOffset@16, propOffset@24 (property stride 24). Offsets are stored as pointers: subtract mem = statesPtr - statesOff.
using System; using System.Collections.Generic; using System.IO; using System.Text;
public static class FxrDump {
  static byte[] b; static long mem;
  static int I32(long p){ return BitConverter.ToInt32(b,(int)p); }
  static short I16(long p){ return BitConverter.ToInt16(b,(int)p); }
  static long U64(long p){ return (long)BitConverter.ToUInt64(b,(int)p); }
  static float F32(long p){ return BitConverter.ToSingle(b,(int)p); }
  static long Off(long p){ return U64(p)-mem; }
  static StringBuilder sb;
  static string Num(long p,bool asInt){ return asInt? I32(p).ToString() : F32(p).ToString("G7",System.Globalization.CultureInfo.InvariantCulture); }
  static string Guess(long p){ int i=I32(p); float f=F32(p); double a=Math.Abs((double)f); if(i==0) return "0"; if((a>=1e-4 && a<1e6) && !(i>=-100000 && i<=100000 && i!=0 && i>0x100 && false)) { if(i>=-64 && i<=4096 && a<1e-3) return "i"+i; return "f"+f.ToString("G7",System.Globalization.CultureInfo.InvariantCulture)+"|i"+i; } return "i"+i; }
  public static string Dump(byte[] data){
    b=data; sb=new StringBuilder();
    if(I32(0)!=0x00525846) return "not FXR";
    int version=I16(6); int id=I32(12); int stateList=I32(16); int statesOff=I32(24); int nodeOff=I32(40);
    long statesPtr=U64(stateList+8); mem=statesPtr-statesOff;
    sb.AppendLine("FXR id="+id+" version="+version);
    Node(nodeOff,0);
    return sb.ToString();
  }
  static void Node(long p,int depth){
    string ind=new string(' ',depth*2);
    int type=I16(p), cc=I32(p+8), ac=I32(p+12), nc=I32(p+16);
    long co=Off(p+24), ao=Off(p+32), no=Off(p+40);
    sb.AppendLine(ind+"NODE type="+type+" configs="+cc+" actions="+ac+" children="+nc);
    for(int i=0;i<cc;i++) Config(co+32L*i,depth+1);
    for(int i=0;i<ac;i++) Action(ao+64L*i,depth+1,"A");
    for(int i=0;i<nc;i++) Node(no+48L*i,depth+1);
  }
  static void Config(long p,int depth){
    string ind=new string(' ',depth*2);
    int type=I16(p), ac=I32(p+12); long ao=Off(p+24);
    sb.AppendLine(ind+"CONFIG type="+type+" actions="+ac);
    for(int i=0;i<ac;i++) Action(ao+64L*i,depth+1,"a");
  }
  static void Action(long p,int depth,string tag){
    string ind=new string(' ',depth*2);
    int type=I16(p); int f1=I32(p+8), s10=I32(p+12), p1=I32(p+16), f2=I32(p+20), p2=I32(p+28);
    long fo=Off(p+32), so=Off(p+40), po=Off(p+48);
    sb.AppendLine(ind+"ACTION "+type+" fields1="+f1+" fields2="+f2+" props1="+p1+" props2="+p2+" s10="+s10);
    var l=new List<string>(); for(int i=0;i<f1;i++) l.Add(Guess(fo+4L*i)); sb.AppendLine(ind+"  F1 ["+string.Join(", ",l)+"]");
    l=new List<string>(); for(int i=0;i<f2;i++) l.Add(Guess(fo+4L*(f1+i))); sb.AppendLine(ind+"  F2 ["+string.Join(", ",l)+"]");
    for(int i=0;i<p1+p2;i++) Prop(po+40L*i,depth+1,(i<p1?"P1."+i:"P2."+(i-p1)),false);
  }
  static void Prop(long p,int depth,string name,bool inMod){
    string ind=new string(' ',depth*2);
    int ea=I16(p)&0xFFFF; int vt=ea&3; int fn=(ea>>4)&15; int loop=(ea>>12)&1;
    int fc=I32(p+8); long fo=Off(p+16);
    long mo=0; int mc=0;
    if(!inMod){ mo=Off(p+24); mc=I32(p+32); }
    string[] fnn={"Zero","One","Constant","Stepped","Linear","Bezier","Hermite","CompHermite"};
    var l=new List<string>();
    for(int i=0;i<fc;i++){ bool asInt=(fn>=3 && fn!=7 && i==0)||(fn==7 && i>0 && i<=1); l.Add(Num(fo+4L*i,asInt)); }
    sb.AppendLine(ind+name+" vt="+vt+" fn="+(fn<8?fnn[fn]:fn.ToString())+(loop==1?" loop":"")+" ["+string.Join(", ",l)+"]"+(mc>0?" mods="+mc:""));
    for(int m=0;m<mc;m++){
      long mp=mo+32L*m; int mea=I16(mp)&0xFFFF; int mt=((mea>>12)&3)<<4 | ((mea>>4)&15); int mvt=mea&3; int mfc=I32(mp+8); int mpc=I32(mp+12); long mfo=Off(mp+16); long mpo=Off(mp+24);
      var ml=new List<string>(); for(int i=0;i<mfc;i++) ml.Add(Guess(mfo+4L*i));
      sb.AppendLine(ind+"  MOD type="+mt+" vt="+mvt+" fields=["+string.Join(", ",ml)+"] props="+mpc);
      for(int k=0;k<mpc;k++) Prop(mpo+24L*k,depth+2,"mp"+k,true);
    }
  }
}
