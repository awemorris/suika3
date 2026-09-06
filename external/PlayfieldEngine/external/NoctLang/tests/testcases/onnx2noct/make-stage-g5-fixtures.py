#!/usr/bin/env python3
"""Build deterministic Stage-G.5 reduction and softmax fixtures."""
import importlib.util, math, struct, sys
from pathlib import Path
HERE=Path(__file__).resolve().parent
SPEC=importlib.util.spec_from_file_location("g1", HERE/"make-stage-g1-fixtures.py")
G1=importlib.util.module_from_spec(SPEC); SPEC.loader.exec_module(G1); R=G1.R
def attr_int(name,value): return R.s(1,name)+R.v(20,2)+R.v(3,value)
def write(root,name,model,values,expected):
    (root/f"{name}.onnx").write_bytes(model)
    (root/f"{name}.input.f32le").write_bytes(struct.pack(f"<{len(values)}f",*values))
    (root/f"{name}.output.f32le").write_bytes(struct.pack(f"<{len(expected)}f",*expected))
def main():
    root=Path(sys.argv[1]); root.mkdir(parents=True,exist_ok=True)
    values=[1.,2.,3.,4.,5.,6.]
    attrs=(G1.attr_ints("axes",[1,2]),attr_int("keepdims",1))
    nodes=[R.node(("x",),("sum",),"ReduceSum",attrs=attrs),
           R.node(("x",),("mean",),"ReduceMean",attrs=attrs),
           R.node(("sum","mean"),("y",),"Add")]
    model=R.model(R.graph(nodes=nodes,inputs=[R.value_info("x",(1,2,3))],outputs=[R.value_info("y",(1,1,1))]))
    write(root,"reduce-sum-mean",model,values,[sum(values)+sum(values)/6.])
    extrema_nodes=[R.node(("x",),("max",),"ReduceMax",attrs=(G1.attr_ints("axes",[2]),attr_int("keepdims",1))),
                   R.node(("max",),("y",),"ReduceMin",attrs=(G1.attr_ints("axes",[1]),attr_int("keepdims",0)))]
    extrema_model=R.model(R.graph(nodes=extrema_nodes,inputs=[R.value_info("x",(1,2,3))],outputs=[R.value_info("y",(1,1))]))
    write(root,"reduce-extrema",extrema_model,values,[3.])
    logits=[1000.,1001.,999.]
    soft=R.node(("x",),("s",),"Softmax",attrs=(attr_int("axis",1),))
    logsoft=R.node(("x",),("l",),"LogSoftmax",attrs=(attr_int("axis",1),))
    add=R.node(("s","l"),("y",),"Add")
    soft_model=R.model(R.graph(nodes=[soft,logsoft,add],inputs=[R.value_info("x",(1,3))],outputs=[R.value_info("y",(1,3))]))
    ex=[math.exp(x-max(logits)) for x in logits]; total=sum(ex)
    expected=[e/total+(x-max(logits)-math.log(total)) for e,x in zip(ex,logits)]
    write(root,"softmax-logsoftmax",soft_model,logits,expected)
if __name__=="__main__": main()
