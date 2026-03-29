#!/usr/bin/env python3
"""
Skill usage report generator.

Reads session-log.jsonl (produced by Cursor hooks) and generates a summary
of skill usage frequency, failure correlation, co-occurrence, and coverage gaps.

Usage:
    python skill-report.py [--log PATH] [--skills PATH]
"""

import argparse
import json
import os
from collections import Counter, defaultdict
from pathlib import Path


def load_session_log(log_path: str) -> list[dict]:
    entries = []
    if not os.path.exists(log_path):
        return entries
    with open(log_path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if line:
                try:
                    entries.append(json.loads(line))
                except json.JSONDecodeError:
                    continue
    return entries


def get_all_skills(skills_dir: str) -> set[str]:
    skills = set()
    if not os.path.isdir(skills_dir):
        return skills
    for entry in os.listdir(skills_dir):
        skill_md = os.path.join(skills_dir, entry, "SKILL.md")
        if os.path.isfile(skill_md):
            skills.add(entry)
    return skills


def generate_report(entries: list[dict], all_skills: set[str]) -> str:
    lines = ["# Skill Usage Report", ""]

    if not entries:
        lines.append("No session data found. Ensure Cursor hooks are configured.")
        return "\n".join(lines)

    total_sessions = len(entries)
    completed = sum(1 for e in entries if e.get("status") == "completed")
    errored = sum(1 for e in entries if e.get("status") == "error")
    aborted = sum(1 for e in entries if e.get("status") == "aborted")

    lines.append(f"## Overview")
    lines.append(f"- Total sessions: {total_sessions}")
    lines.append(f"- Completed: {completed} | Error: {errored} | Aborted: {aborted}")
    lines.append("")

    # Usage frequency
    usage = Counter()
    for entry in entries:
        for skill in entry.get("skills_used", []):
            usage[skill] += 1

    lines.append("## Usage Frequency")
    lines.append("| Skill | Sessions | % |")
    lines.append("|-------|----------|---|")
    for skill, count in usage.most_common():
        pct = count / total_sessions * 100
        lines.append(f"| {skill} | {count} | {pct:.0f}% |")
    lines.append("")

    # Failure correlation
    failure_skills = Counter()
    for entry in entries:
        if entry.get("status") == "error":
            for skill in entry.get("skills_used", []):
                failure_skills[skill] += 1

    if failure_skills:
        lines.append("## Failure-Correlated Skills (self-iteration candidates)")
        lines.append("| Skill | Failed Sessions | Failure Rate |")
        lines.append("|-------|----------------|-------------|")
        for skill, fail_count in failure_skills.most_common():
            total_use = usage.get(skill, 1)
            rate = fail_count / total_use * 100
            lines.append(f"| {skill} | {fail_count} | {rate:.0f}% |")
        lines.append("")

    # Co-occurrence
    cooccur = Counter()
    for entry in entries:
        skills = sorted(set(entry.get("skills_used", [])))
        for i in range(len(skills)):
            for j in range(i + 1, len(skills)):
                cooccur[(skills[i], skills[j])] += 1

    if cooccur:
        lines.append("## Skill Co-occurrence (top 10)")
        lines.append("| Skill A | Skill B | Sessions |")
        lines.append("|---------|---------|----------|")
        for (a, b), count in cooccur.most_common(10):
            lines.append(f"| {a} | {b} | {count} |")
        lines.append("")

    # Coverage gaps
    used_skills = set(usage.keys())
    unused = all_skills - used_skills
    if unused:
        lines.append("## Unused Skills")
        lines.append("These skills were never loaded in any tracked session:")
        for skill in sorted(unused):
            lines.append(f"- {skill}")
        lines.append("")

    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description="Generate skill usage report")
    parser.add_argument(
        "--log",
        default=".cursor/hooks/state/session-log.jsonl",
        help="Path to session-log.jsonl",
    )
    parser.add_argument(
        "--skills",
        default="Plugins/UnrealClientProtocol/Agent/skills",
        help="Path to skills directory",
    )
    parser.add_argument(
        "--output",
        default=None,
        help="Output file path (default: stdout)",
    )
    args = parser.parse_args()

    entries = load_session_log(args.log)
    all_skills = get_all_skills(args.skills)
    report = generate_report(entries, all_skills)

    if args.output:
        Path(args.output).parent.mkdir(parents=True, exist_ok=True)
        with open(args.output, "w", encoding="utf-8") as f:
            f.write(report)
        print(f"Report written to {args.output}")
    else:
        print(report)


if __name__ == "__main__":
    main()
