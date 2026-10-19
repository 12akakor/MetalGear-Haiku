#!/usr/bin/env python3
# Copyright 2026, Akakor. Distributed under the terms of the MIT License.
"""Call graph of the disassembly: which routines are reachable from given
roots, their size in instructions and whether a C++ function with the same
name exists in src/game. Used to plan the rewrite.

    callgraph.py <disassembly dir> <src/game> root... [-todo]
"""
import os
import re
import sys

LABEL = re.compile(r"^([A-Za-z_][\w]*):")
INSTR = re.compile(r"^\s+([a-z]{2,4})\b\s*(.*)$", re.I)
WORD = re.compile(r"\b([A-Za-z_][\w]*)\b")


def files_in_order(root):
	out = []

	def walk(path):
		base = os.path.dirname(path)
		for line in open(path, encoding="latin-1"):
			m = re.match(r'\s*include\s+"?([^"\s;]+)"?', line, re.I)
			if m:
				inc = os.path.join(root, m.group(1))
				if not os.path.exists(inc):
					inc = os.path.join(base, m.group(1))
				if os.path.exists(inc) and inc.endswith(".asm"):
					walk(inc)
				continue
		out.append(path)
	walk(os.path.join(root, "MetalGear.asm"))
	return out


def parse(root):
	routines = {}	# name -> {"size": n, "refs": set, "next": name}
	order = []
	current = None
	for path in files_in_order(root):
		for raw in open(path, encoding="latin-1"):
			line = raw.split(";")[0].rstrip()
			if not line.strip():
				continue
			m = LABEL.match(line)
			if m:
				name = m.group(1)
				if current is not None and routines[current]["falls"]:
					routines[current]["refs"].add(name)
				current = name
				routines[name] = {"size": 0, "refs": set(), "falls": True, "file": path}
				order.append(name)
				line = line[m.end():]
				if not line.strip():
					continue
				line = "\t" + line.strip()
			if current is None:
				continue
			r = routines[current]
			mi = INSTR.match(line)
			if not mi:
				continue
			op, args = mi.group(1).lower(), mi.group(2)
			if op in ("db", "ds", "equ", "org", "page", "code", "defb", "defs"):
				r["falls"] = False
				continue
			if op == "dw":
				for w in WORD.findall(args):
					r["refs"].add(w)
				r["falls"] = False
				continue
			r["size"] += 1
			if op == "call":
				for w in WORD.findall(args):
					r.setdefault("calls", set()).add(w)
			if op in ("call", "jp", "jr", "djnz", "ld"):
				for w in WORD.findall(args):
					if w.lower() not in ("a", "b", "c", "d", "e", "h", "l", "af", "bc", "de",
						"hl", "ix", "iy", "sp", "z", "nz", "nc", "p", "m", "pe", "po", "ixh",
						"ixl", "iyh", "iyl", "i", "r"):
						r["refs"].add(w)
			if op in ("ret", "reti", "retn") and not args.strip():
				r["falls"] = False
			if op in ("jp", "jr") and "," not in args:
				r["falls"] = False
	return routines, order


def native_names(src):
	names = set()
	for f in os.listdir(src):
		if f.endswith(".cpp"):
			for m in re.finditer(r"^\w[\w\s\*&<>:]*?\b(\w+)\s*\(", open(os.path.join(src, f)).read(), re.M):
				names.add(m.group(1))
	return names


def main():
	root, src = sys.argv[1], sys.argv[2]
	roots = [a for a in sys.argv[3:] if not a.startswith("-")]
	routines, order = parse(root)
	native = native_names(src)
	seen = set()
	stack = list(roots)
	while stack:
		n = stack.pop()
		if n in seen or n not in routines:
			continue
		seen.add(n)
		stack.extend(routines[n]["refs"])
	code = [n for n in order if n in seen and routines[n]["size"] > 0]
	done = [n for n in code if n in native]
	total = sum(routines[n]["size"] for n in code)
	donesz = sum(routines[n]["size"] for n in done)
	print("%d code labels reachable, %d instructions; %d labels (%d instr) have a C++ function"
		% (len(code), total, len(done), donesz))
	if "-order" in sys.argv:
		# routines = labels that are called (or in tables); body = labels up
		# to the next routine; listed callees first
		called = set()
		for n in order:
			called |= routines[n].get("calls", set())
			if routines[n]["size"] == 0:
				called |= routines[n]["refs"]
		called |= set(roots)
		owner = {}
		cur = None
		for n in order:
			if n in called and routines[n]["size"] > 0:
				cur = n
			owner[n] = cur
		body = {}
		for n in order:
			if owner[n] is not None:
				body.setdefault(owner[n], []).append(n)
		def deps(rt):
			out = set()
			for lab in body.get(rt, []):
				for t in routines[lab]["refs"]:
					if t in owner and owner[t] is not None and owner[t] != rt and routines[t]["size"] > 0:
						out.add(owner[t])
			return out
		done_set, out = set(), []
		import sys as _s
		_s.setrecursionlimit(100000)
		def visit(rt, stack=()):
			if rt in done_set or rt in stack:
				return
			for d in sorted(deps(rt)):
				visit(d, stack + (rt,))
			done_set.add(rt)
			out.append(rt)
		for r0 in roots:
			if r0 in owner:
				visit(owner[r0])
		total = 0
		for rt in out:
			size = sum(routines[l]["size"] for l in body.get(rt, []))
			total += size
			mark = "done" if rt in native else ""
			print("%5d %6d  %-30s %-4s %s" % (size, total, rt, mark,
				os.path.relpath(routines[rt]["file"], root)))
	if "-todo" in sys.argv:
		for n in code:
			if n not in native:
				print("%5d  %-32s %s" % (routines[n]["size"], n,
					os.path.relpath(routines[n]["file"], root)))


if __name__ == "__main__":
	main()
