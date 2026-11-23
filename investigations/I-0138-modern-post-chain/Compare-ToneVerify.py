# I-0138 tranche 8: the two verify flags are gone from the module; this reader stays for the pairs already in the evidence
# store, and no new pair can be made.
# I-0138 tranches 1, 2 and 4: reads the pairs of pictures a run with world-pictures.flag and a verify flag leaves in its folder.
# On each witness frame the module wrote our pass's output and the output of the original's programmes drawn from the same
# frame's inputs:
#   post-tone-verify.flag   <stem>-tone-ours.rgba and <stem>-tone-yebis.rgba: the tone map's picture, RGBA8 rows at the
#                           presentation extent;
#   post-glare-verify.flag  <stem>-glare-ours.r11g11b10 and <stem>-glare-yebis.r11g11b10: the glare family's output (the tone
#                           map's t1), packed R11G11B10 float texels, four bytes each; and, tranche 4, <stem>-dof-ours.rgba16f
#                           and -dof-yebis.rgba16f (the depth of field's output) and <stem>-mb-ours.rgba16f and -mb-yebis.rgba16f
#                           (the motion blur's), RGBA16F texels, eight bytes each, written only on the frames that draw them.
# Prints, a pair a line, the differing bytes (and, for the 8-bit pictures, the largest difference); exits 1 when a pair
# differs, a file is missing, or no pair of a needed kind is found (a verify stage that failed writes none). The needed
# kinds: the tone pairs when no --need is given (tranche 1's rule, unchanged); with --need, the kinds named (a run with both
# flags is read with --need tone --need glare; a run with post-glare-verify.flag alone with --need glare; --need dof and
# --need mb where the run must have drawn them).
# Usage: python -I Compare-ToneVerify.py [--need tone|glare|dof|mb]... <run folder> [<run folder> ...]
import glob,os,sys

KINDS=(('tone','-tone-ours.rgba','-tone-yebis.rgba',True),('glare','-glare-ours.r11g11b10','-glare-yebis.r11g11b10',False),
    ('dof','-dof-ours.rgba16f','-dof-yebis.rgba16f',False),('mb','-mb-ours.rgba16f','-mb-yebis.rgba16f',False))

def compare(ours_path,ours_suffix,theirs_suffix,eight_bit):
    theirs_path=ours_path[:-len(ours_suffix)]+theirs_suffix
    name=os.path.basename(ours_path)[:-len(ours_suffix)]+ours_suffix.rsplit('-ours',1)[0]
    if not os.path.exists(theirs_path):
        print('%s: no %s beside it'%(name,theirs_suffix));return False
    ours=open(ours_path,'rb').read();theirs=open(theirs_path,'rb').read()
    if len(ours)!=len(theirs) or not ours:
        print('%s: sizes %d and %d'%(name,len(ours),len(theirs)));return False
    if ours==theirs:
        print('%s: %d bytes, 0 differing'%(name,len(ours)));return True
    differing=0;largest=0
    for a,b in zip(ours,theirs):
        if a!=b:
            differing+=1;largest=max(largest,abs(a-b))
    if eight_bit:
        print('%s: %d bytes, %d differing, largest %d/255'%(name,len(ours),differing,largest))
    else:
        print('%s: %d bytes, %d differing'%(name,len(ours),differing))
    return False

def main(folders,need):
    ok=True
    for kind,ours_suffix,theirs_suffix,eight_bit in KINDS:
        pairs=0;equal=0
        for folder in folders:
            for ours in sorted(glob.glob(os.path.join(folder,'*'+ours_suffix))):
                pairs+=1;equal+=compare(ours,ours_suffix,theirs_suffix,eight_bit)
        print('%s pairs %d, equal %d'%(kind,pairs,equal))
        ok=ok and pairs==equal and (pairs>0 or kind not in need)
    return 0 if ok else 1

if __name__=='__main__':
    args=sys.argv[1:];need=set()   # empty: the tone pairs (below)
    while len(args)>=2 and args[0]=='--need':
        need.add(args[1]);args=args[2:]
    if not args or need-{'tone','glare','dof','mb'}:
        print('usage: python -I Compare-ToneVerify.py [--need tone|glare|dof|mb]... <run folder> [<run folder> ...]');sys.exit(2)
    sys.exit(main(args,need or {'tone'}))
