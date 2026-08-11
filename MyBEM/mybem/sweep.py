"""Expand a sweep yaml into one experiment yaml per run.

configs/sweeps/<name>.yaml -> configs/nets/generated/<run name>.yaml,
then feed the printed list to submit.py --exp.
"""

import argparse

import yaml

from .paths import NET_CONFIGS, SWEEPS


def merge(base, over):
    out = dict(base)
    for k, v in over.items():
        out[k] = merge(out[k], v) if isinstance(v, dict) and isinstance(out.get(k), dict) else v
    return out


def expand(sweep):
    with open(SWEEPS / sweep) as f:
        cfg = yaml.safe_load(f)
    with open(NET_CONFIGS / cfg["base"]) as f:
        base = yaml.safe_load(f)

    out = NET_CONFIGS / "generated"
    out.mkdir(exist_ok=True)
    names = []
    for run in cfg["runs"]:
        if "name" not in run:
            raise SystemExit(f"run without a name: {run}")
        # net: is replaced wholesale, not merged — leftover keys of another
        # architecture would survive the merge.
        exp = merge(base, {k: v for k, v in run.items() if k != "net"})
        exp["net"] = run.get("net", base["net"])
        with open(out / f"{exp['name']}.yaml", "w") as f:
            yaml.safe_dump(exp, f, sort_keys=False)
        names.append(f"generated/{exp['name']}.yaml")
    return names


def main():
    p = argparse.ArgumentParser()
    p.add_argument("sweep", help="file under configs/sweeps/")
    a = p.parse_args()
    names = expand(a.sweep)
    print(f"{len(names)} experiments in {NET_CONFIGS / 'generated'}\n")
    print("--exp " + " ".join(names))


if __name__ == "__main__":
    main()
