# Audit helper (offline): print the execution flow of a Blueprint graph exported by
# GOTA_ExportCombatGraphs.py, starting from events whose name matches a filter.
#   python Tools/GOTA_GraphFlow.py Saved/GraphAudit/BP_Player__EventGraph.txt StartBlock [depth]
import re
import sys

text = open(sys.argv[1], encoding="utf-8", errors="ignore").read()
flt = sys.argv[2] if len(sys.argv) > 2 else ""
max_depth = int(sys.argv[3]) if len(sys.argv) > 3 else 40

nodes = {}
for block in re.split(r"\nBegin Object Class=", "\n" + text)[1:]:
    head = block.split("\n", 1)[0]
    m = re.match(r"(\S+) Name=\"([^\"]+)\"", head)
    if not m:
        continue
    cls, name = m.group(1).split(".")[-1], m.group(2)
    label = None
    for key in ["CustomFunctionName", "EventReference=\\(MemberName", "FunctionReference=\\(MemberName",
                "VariableReference=\\(MemberName", "DelegateReference=\\(MemberName", "TargetType"]:
        mm = re.search(key + r"=\"([^\"]+)\"", block)
        if mm:
            label = mm.group(1)
            break
    mm = re.search(r'StructType=[^\n]*?\.([A-Za-z_0-9]+)\'"', block)
    pins = []
    for pm in re.finditer(r"CustomProperties Pin \((.*)\)", block):
        p = pm.group(1)
        pname = re.search(r'PinName="([^"]*)"', p)
        cat = re.search(r'PinType\.PinCategory="([^"]*)"', p)
        direction = "out" if 'Direction="EGPD_Output"' in p else "in"
        links = re.findall(r"(K2Node_[A-Za-z_0-9]+|EdGraphNode_[A-Za-z_0-9]+) [0-9A-F]{32}", re.search(r"LinkedTo=\(([^)]*)\)", p).group(1)) if "LinkedTo=(" in p else []
        default = re.search(r'DefaultValue="([^"]*)"', p)
        pins.append(dict(name=pname.group(1) if pname else "", cat=cat.group(1) if cat else "", dir=direction,
                         links=links, default=default.group(1) if default else None))
    nodes[name] = dict(cls=cls, label=label, pins=pins)


def describe(n):
    node = nodes[n]
    defaults = [f"{p['name']}={p['default']}" for p in node["pins"]
                if p["dir"] == "in" and p["cat"] != "exec" and p["default"] not in (None, "") and not p["links"]]
    data_in = [f"{p['name']}<-{','.join(nodes[l]['label'] or nodes[l]['cls'] for l in p['links'] if l in nodes)}"
               for p in node["pins"] if p["dir"] == "in" and p["cat"] != "exec" and p["links"]]
    return f"{node['cls'].replace('K2Node_', '')}:{node['label']} " + " ".join(defaults + data_in)


def walk(n, depth, seen):
    if depth > max_depth or n in seen:
        if n in seen:
            print("  " * depth + f"(loop -> {nodes[n]['label']})")
        return
    seen = seen | {n}
    print("  " * depth + describe(n))
    for p in nodes[n]["pins"]:
        if p["dir"] == "out" and p["cat"] == "exec":
            for l in p["links"]:
                if l in nodes:
                    if p["name"] not in ("then", ""):
                        print("  " * (depth + 1) + f"[{p['name']}]")
                    walk(l, depth + 1, seen)


for n, node in nodes.items():
    if node["cls"] in ("K2Node_CustomEvent", "K2Node_Event", "K2Node_FunctionEntry", "K2Node_InputKey",
                       "K2Node_EnhancedInputAction", "K2Node_ComponentBoundEvent", "K2Node_InputDebugKey") \
            and flt.lower() in str(node["label"]).lower() + node["cls"].lower() + n.lower():
        print("=" * 8, n, node["label"])
        walk(n, 0, set())
