#!/usr/bin/env python3
"""Build deterministic Stage-G.6 BatchNormalization fixture."""
import importlib.util, math, struct, sys
from pathlib import Path
HERE=Path(__file__).resolve().parent
SPEC=importlib.util.spec_from_file_location("g1",HERE/"make-stage-g1-fixtures.py")
G1=importlib.util.module_from_spec(SPEC); SPEC.loader.exec_module(G1); R=G1.R
def attr_float(name,bits): return R.s(1,name)+R.v(20,1)+R.f32(2,bits)
def main():
 root=Path(sys.argv[1]); root.mkdir(parents=True,exist_ok=True)
 x=[1.,2.,3.,4.,-1.,0.,1.,2.]; scale=[2.,.5]; bias=[.25,-.5]; mean=[2.5,.5]; var=[1.25,2.]
 tensors=[]; entries=[]
 for name,vals in (("bn_scale",scale),("bn_bias",bias),("bn_mean",mean),("bn_var",var)):
  tensor,raw=G1.float_tensor(name,(2,),vals); tensors.append(tensor); entries.append((name,(2,),raw))
 node=R.node(("x","bn_scale","bn_bias","bn_mean","bn_var"),("y",),"BatchNormalization",attrs=(attr_float("epsilon",0x3A83126F),))
 model=R.model(R.graph(nodes=[node],initializers=tensors,inputs=[R.value_info("x",(1,2,2,2))],outputs=[R.value_info("y",(1,2,2,2))]))
 epsilon=0.001
 expected=[]
 for i,value in enumerate(x):
  c=i//4; expected.append(scale[c]*(value-mean[c])/math.sqrt(var[c]+epsilon)+bias[c])
 G1.write_fixture(root,"batchnorm-reference",model,x,expected,entries)
 alias_node=R.node(("bn_scale",),("scale_alias",),"Identity")
 bad_bn=R.node(("x","scale_alias","bn_bias","bn_mean","bn_var"),("y",),"BatchNormalization")
 bad_model=R.model(R.graph(nodes=[alias_node,bad_bn],initializers=tensors,inputs=[R.value_info("x",(1,2,2,2))],outputs=[R.value_info("y",(1,2,2,2))]))
 (root/"unsupported-aliased-parameter.onnx").write_bytes(bad_model)
if __name__=="__main__": main()
