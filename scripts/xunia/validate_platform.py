#!/usr/bin/env python3
import json
from pathlib import Path

root=Path(__file__).resolve().parents[2]
manifest=json.loads((root/"xunia-platform.json").read_text())
ontology=json.loads((root/"platform/ontology/xunia-core.ontology.json").read_text())
assert manifest["schemaVersion"]=="1.0.0"
assert ontology["schemaVersion"]=="1.0.0"
names=[x["apiName"] for x in ontology["objectTypes"]]
assert len(names)==len(set(names))
known=set(names)
prohibited={x.lower() for x in ontology["prohibitedProperties"]}
for obj in ontology["objectTypes"]:
    assert obj["primaryKey"] in obj["properties"]
    assert not prohibited.intersection(x.lower() for x in obj["properties"])
for link in ontology["links"]:
    assert link["from"] in known and link["to"] in known
for action in ontology["actions"]:
    assert action["target"] in known
for required in ("approveAgentDiagnosis","promoteCombinedRelease"):
    action=next(x for x in ontology["actions"] if x["apiName"]==required)
    assert action["approval"]=="HUMAN_REQUIRED"
print(f"XUNIA platform valid: {len(names)} objects, {len(ontology['links'])} links, {len(ontology['actions'])} actions")
