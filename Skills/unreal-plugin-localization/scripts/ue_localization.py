"""
Unreal Engine Plugin Localization Helper
=========================================
Subcommands:
  gather  — Setup + GatherText + Export PO + Extract compact JSON
  compile — Inject translated JSON into PO + Import + Compile .locres
"""

import argparse
import glob
import json
import os
import re
import socket
import struct
import subprocess
import sys
import winreg
from typing import Dict, List, Optional, Tuple

# ---------------------------------------------------------------------------
# Path auto-detection
# ---------------------------------------------------------------------------

def find_uproject(uplugin_path: str) -> str:
    """Walk up from .uplugin dir to find the .uproject file."""
    d = os.path.dirname(os.path.abspath(uplugin_path))
    while True:
        hits = glob.glob(os.path.join(d, "*.uproject"))
        if hits:
            return hits[0]
        parent = os.path.dirname(d)
        if parent == d:
            break
        d = parent
    raise FileNotFoundError(f"No .uproject found above {uplugin_path}")


def resolve_engine_dir(uproject_path: str) -> str:
    """Read EngineAssociation from .uproject and resolve via registry."""
    with open(uproject_path, "r", encoding="utf-8-sig") as f:
        proj = json.load(f)
    assoc = proj.get("EngineAssociation", "")
    if not assoc:
        raise ValueError("EngineAssociation is empty in .uproject")

    if assoc.startswith("{"):
        # Source-build GUID — look up in HKCU\SOFTWARE\Epic Games\Unreal Engine\Builds
        try:
            key = winreg.OpenKey(winreg.HKEY_CURRENT_USER,
                                 r"SOFTWARE\Epic Games\Unreal Engine\Builds")
            val, _ = winreg.QueryValueEx(key, assoc)
            winreg.CloseKey(key)
            return val.replace("/", os.sep)
        except OSError:
            raise ValueError(f"Registry key not found for engine GUID {assoc}")
    else:
        # Launcher version like "5.6"
        reg_path = rf"SOFTWARE\EpicGames\Unreal Engine\{assoc}"
        for hive in (winreg.HKEY_LOCAL_MACHINE, winreg.HKEY_CURRENT_USER):
            try:
                key = winreg.OpenKey(hive, reg_path)
                val, _ = winreg.QueryValueEx(key, "InstalledDirectory")
                winreg.CloseKey(key)
                return val.replace("/", os.sep)
            except OSError:
                continue
        raise ValueError(f"Registry key not found for engine version {assoc}")


def get_editor_cmd(engine_dir: str) -> str:
    """Return path to UnrealEditor-Cmd.exe, fallback to other editor binaries."""
    candidates = [
        os.path.join(engine_dir, "Engine", "Binaries", "Win64", "UnrealEditor-Cmd.exe"),
        os.path.join(engine_dir, "Engine", "Binaries", "Win64", "UnrealEditor.exe"),
    ]
    for c in candidates:
        if os.path.isfile(c):
            return c
    raise FileNotFoundError(f"No editor binary found under {engine_dir}")

# ---------------------------------------------------------------------------
# Plugin helpers
# ---------------------------------------------------------------------------

def plugin_name(uplugin_path: str) -> str:
    return os.path.splitext(os.path.basename(uplugin_path))[0]


def ensure_localization_target(uplugin_path: str, name: str):
    """Ensure LocalizationTargets entry exists in .uplugin JSON."""
    with open(uplugin_path, "r", encoding="utf-8-sig") as f:
        data = json.load(f)

    targets = data.get("LocalizationTargets")
    if isinstance(targets, list):
        if any(t.get("Name") == name for t in targets):
            return
        targets.append({"Name": name, "LoadingPolicy": "Editor"})
    else:
        data["LocalizationTargets"] = [{"Name": name, "LoadingPolicy": "Editor"}]

    with open(uplugin_path, "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent="\t")

# ---------------------------------------------------------------------------
# INI generation (embedded templates)
# ---------------------------------------------------------------------------

def _loc_dir_relative(uplugin_path: str, uproject_path: str, target_name: str) -> str:
    """Localization content dir relative to project root (forward slashes)."""
    plugin_dir = os.path.dirname(os.path.abspath(uplugin_path))
    project_root = os.path.dirname(os.path.abspath(uproject_path))
    rel = os.path.relpath(plugin_dir, project_root).replace("\\", "/")
    return f"{rel}/Content/Localization/{target_name}"


def _source_dir_relative(uplugin_path: str, uproject_path: str) -> str:
    plugin_dir = os.path.dirname(os.path.abspath(uplugin_path))
    project_root = os.path.dirname(os.path.abspath(uproject_path))
    rel = os.path.relpath(plugin_dir, project_root).replace("\\", "/")
    return f"{rel}/Source/"


def _content_dir_relative(uplugin_path: str, uproject_path: str) -> str:
    plugin_dir = os.path.dirname(os.path.abspath(uplugin_path))
    project_root = os.path.dirname(os.path.abspath(uproject_path))
    rel = os.path.relpath(plugin_dir, project_root).replace("\\", "/")
    return f"{rel}/Content/"


def generate_gather_export_ini(
    uplugin_path: str,
    uproject_path: str,
    target_name: str,
    native: str,
    cultures: List[str],
) -> str:
    loc_dir = _loc_dir_relative(uplugin_path, uproject_path, target_name)
    src_dir = _source_dir_relative(uplugin_path, uproject_path)
    content_dir = _content_dir_relative(uplugin_path, uproject_path)

    cultures_lines = "\n".join(f"CulturesToGenerate={c}" for c in [native] + cultures)

    return f"""\
[CommonSettings]
SourcePath={loc_dir}
DestinationPath={loc_dir}
ManifestName={target_name}.manifest
ArchiveName={target_name}.archive
PortableObjectName={target_name}.po
NativeCulture={native}
{cultures_lines}

[GatherTextStep0]
CommandletClass=GatherTextFromSource
SearchDirectoryPaths={src_dir}
FileNameFilters=*.cpp
FileNameFilters=*.h
FileNameFilters=*.inl
ShouldGatherFromEditorOnlyData=true

[GatherTextStep1]
CommandletClass=GatherTextFromAssets
IncludePathFilters={content_dir}*
ExcludePathFilters={content_dir}Localization/*
PackageFileNameFilters=*.uasset
PackageFileNameFilters=*.umap
ShouldGatherFromEditorOnlyData=true

[GatherTextStep2]
CommandletClass=GatherTextFromMetaData
IncludePathFilters={src_dir}*
InputKeys=DisplayName
OutputNamespaces=UObjectDisplayNames
OutputKeys="{{FieldPath}}"
InputKeys=Category
OutputNamespaces=UObjectCategory
OutputKeys="{{MetaDataValue}}"
InputKeys=ToolTip
OutputNamespaces=UObjectToolTips
OutputKeys="{{FieldPath}}"
ShouldGatherFromEditorOnlyData=true

[GatherTextStep3]
CommandletClass=GenerateGatherManifest

[GatherTextStep4]
CommandletClass=GenerateGatherArchive
bPurgeOldEmptyEntries=true

[GatherTextStep5]
CommandletClass=InternationalizationExport
bExportLoc=true
"""


def generate_import_compile_ini(
    uplugin_path: str,
    uproject_path: str,
    target_name: str,
    native: str,
    cultures: List[str],
) -> str:
    loc_dir = _loc_dir_relative(uplugin_path, uproject_path, target_name)
    cultures_lines = "\n".join(f"CulturesToGenerate={c}" for c in [native] + cultures)

    return f"""\
[CommonSettings]
SourcePath={loc_dir}
DestinationPath={loc_dir}
ManifestName={target_name}.manifest
ArchiveName={target_name}.archive
PortableObjectName={target_name}.po
NativeCulture={native}
{cultures_lines}

[GatherTextStep0]
CommandletClass=InternationalizationExport
bImportLoc=true

[GatherTextStep1]
CommandletClass=GenerateTextLocalizationResource
ResourceName={target_name}.locres
"""

# ---------------------------------------------------------------------------
# Source-context extraction (context-enhanced mode)
# ---------------------------------------------------------------------------

_LOCTEXT_RE = re.compile(
    r'(?:LOCTEXT|NSLOCTEXT)\s*\(\s*'
    r'(?:TEXT\s*\(\s*"(?:[^"\\]|\\.)*"\s*\)\s*,\s*)?'  # optional namespace for NSLOCTEXT
    r'(?:TEXT\s*\(\s*)?"(?P<key>(?:[^"\\]|\\.)*)"(?:\s*\))?\s*,\s*'
    r'(?:TEXT\s*\(\s*)?"(?P<val>(?:[^"\\]|\\.)*)"(?:\s*\))?'
)

_METADATA_RE = re.compile(
    r'(?:DisplayName|ToolTip|Category)\s*=\s*"(?P<val>(?:[^"\\]|\\.)*)"'
)


def _extract_source_contexts(source_dir: str) -> Dict[str, str]:
    """Scan C++ sources for LOCTEXT/NSLOCTEXT calls and UPROPERTY metadata.

    Returns {msgid_text: context_hint} where context_hint is a compact
    summary of surrounding code (function name, comment above, etc.).
    """
    contexts: Dict[str, str] = {}
    if not os.path.isdir(source_dir):
        return contexts

    for root, _dirs, files in os.walk(source_dir):
        for fname in files:
            if not fname.endswith((".cpp", ".h", ".inl")):
                continue
            fpath = os.path.join(root, fname)
            try:
                with open(fpath, "r", encoding="utf-8-sig") as f:
                    lines = f.readlines()
            except (UnicodeDecodeError, OSError):
                continue

            content = "".join(lines)
            rel_path = os.path.relpath(fpath, source_dir).replace("\\", "/")

            for m in _LOCTEXT_RE.finditer(content):
                val = m.group("val")
                if not val:
                    continue
                line_no = content[:m.start()].count("\n") + 1
                hint = _build_hint(lines, line_no, rel_path)
                if val not in contexts or len(hint) > len(contexts.get(val, "")):
                    contexts[val] = hint

            for m in _METADATA_RE.finditer(content):
                val = m.group("val")
                if not val:
                    continue
                line_no = content[:m.start()].count("\n") + 1
                hint = _build_hint(lines, line_no, rel_path)
                if val not in contexts or len(hint) > len(contexts.get(val, "")):
                    contexts[val] = hint

    return contexts


def _build_hint(lines: List[str], line_no: int, rel_path: str) -> str:
    """Build a compact context hint from surrounding lines."""
    idx = line_no - 1
    parts = [f"[{rel_path}:{line_no}]"]

    comment_lines = []
    for i in range(max(0, idx - 5), idx):
        stripped = lines[i].strip()
        if stripped.startswith("//") or stripped.startswith("*") or stripped.startswith("/**") or stripped.startswith("///"):
            comment_lines.append(stripped.lstrip("/*/ ").rstrip())
        elif stripped.startswith("UPROPERTY") or stripped.startswith("UFUNCTION") or stripped.startswith("UCLASS"):
            comment_lines.append(stripped[:120])
    if comment_lines:
        parts.append(" ".join(comment_lines[-3:]))

    return " | ".join(parts)


# ---------------------------------------------------------------------------
# UCP communication (optional, for Blueprint context)
# ---------------------------------------------------------------------------

_UCP_HOST = os.environ.get("UE_HOST", "127.0.0.1")
_UCP_PORT = int(os.environ.get("UE_PORT", "9876"))
_UCP_TIMEOUT = float(os.environ.get("UE_TIMEOUT", "10"))


def _ucp_available() -> bool:
    """Quick TCP probe to check if UCP is listening."""
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(2)
    try:
        s.connect((_UCP_HOST, _UCP_PORT))
        return True
    except (ConnectionRefusedError, OSError):
        return False
    finally:
        s.close()


def _ucp_recv_exact(sock: socket.socket, n: int) -> bytes:
    buf = bytearray()
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            raise ConnectionError("UCP connection closed")
        buf.extend(chunk)
    return bytes(buf)


def _ucp_call(command: dict) -> Optional[dict]:
    """Send a single UCP command and return the simplified result, or None on error."""
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(_UCP_TIMEOUT)
    try:
        s.connect((_UCP_HOST, _UCP_PORT))
        body = json.dumps(command).encode("utf-8")
        s.sendall(struct.pack("<I", len(body)) + body)
        raw_len = _ucp_recv_exact(s, 4)
        resp_len = struct.unpack("<I", raw_len)[0]
        raw_body = _ucp_recv_exact(s, resp_len)
        resp = json.loads(raw_body.decode("utf-8"))
        if resp.get("success"):
            return resp.get("result")
        return None
    except (ConnectionError, ConnectionRefusedError, OSError, json.JSONDecodeError):
        return None
    finally:
        s.close()


def _extract_blueprint_contexts(plugin_content_path: str) -> Dict[str, str]:
    """Use UCP to scan Blueprint assets and extract variable/function metadata.

    plugin_content_path: UE content path like '/MyPlugin/' (with slashes).
    Returns {metadata_text: hint_string}.
    """
    contexts: Dict[str, str] = {}

    assets = _ucp_call({
        "object": "/Script/EditorScriptingUtilities.Default__EditorAssetLibrary",
        "function": "ListAssets",
        "params": {"DirectoryPath": plugin_content_path, "bRecursive": True, "bIncludeFolder": False}
    })
    if not isinstance(assets, list):
        return contexts

    for asset_path in assets:
        obj_path = asset_path if "." in asset_path else f"{asset_path}.{asset_path.rsplit('/', 1)[-1]}"

        desc = _ucp_call({
            "object": "/Script/UnrealClientProtocol.Default__ObjectOperationLibrary",
            "function": "DescribeObject",
            "params": {"ObjectPath": obj_path}
        })
        if not isinstance(desc, dict):
            continue

        class_name = desc.get("class", "")
        if "Blueprint" not in class_name:
            continue

        asset_short = obj_path.rsplit("/", 1)[-1].split(".")[0]
        gen_class = desc.get("properties", {}).get("GeneratedClass", "")
        if not gen_class:
            continue

        outline = _ucp_call({
            "object": "/Script/UnrealClientProtocolEditor.Default__NodeCodeEditingLibrary",
            "function": "Outline",
            "params": {"AssetPath": asset_path}
        })
        if not isinstance(outline, dict):
            continue

        # Extract variable metadata
        variables = outline.get("Variables", []) if isinstance(outline.get("Variables"), list) else []
        for var_info in variables:
            var_name = var_info if isinstance(var_info, str) else var_info.get("name", "")
            if not var_name:
                continue
            prop_result = _ucp_call({
                "object": "/Script/UnrealClientProtocol.Default__ObjectOperationLibrary",
                "function": "DescribeObjectProperty",
                "params": {"ObjectPath": gen_class, "PropertyName": var_name}
            })
            if isinstance(prop_result, dict):
                meta = prop_result.get("metadata", {})
                for mk in ("DisplayName", "ToolTip"):
                    mv = meta.get(mk, "")
                    if mv and mv not in contexts:
                        contexts[mv] = f"[{asset_short}.Variable:{var_name}] | {mk}: {mv}"

        # Extract function metadata
        sections = outline.get("sections", []) if isinstance(outline.get("sections"), list) else []
        for sec in sections:
            sec_name = sec if isinstance(sec, str) else sec.get("name", "")
            if not sec_name or not sec_name.startswith("Function:"):
                continue
            func_name = sec_name[len("Function:"):]
            func_desc = _ucp_call({
                "object": "/Script/UnrealClientProtocol.Default__ObjectOperationLibrary",
                "function": "DescribeObjectFunction",
                "params": {"ObjectPath": gen_class, "FunctionName": func_name}
            })
            if isinstance(func_desc, dict):
                for mk in ("DisplayName", "ToolTip"):
                    mv = func_desc.get(mk, "") or func_desc.get("metadata", {}).get(mk, "")
                    if mv and mv not in contexts:
                        contexts[mv] = f"[{asset_short}.{sec_name}] | {mk}: {mv}"

    return contexts


# ---------------------------------------------------------------------------
# PO parsing (robust, supports multiline)
# ---------------------------------------------------------------------------

_PO_ENTRY_RE = re.compile(
    r'(?P<comments>(?:^#[^\n]*\n)*)'
    r'^msgctxt\s+(?P<ctxt>"(?:[^"\\]|\\.)*"(?:\s*\n"(?:[^"\\]|\\.)*")*)\s*\n'
    r'^msgid\s+(?P<id>"(?:[^"\\]|\\.)*"(?:\s*\n"(?:[^"\\]|\\.)*")*)\s*\n'
    r'^msgstr\s+(?P<str>"(?:[^"\\]|\\.)*"(?:\s*\n"(?:[^"\\]|\\.)*")*)',
    re.MULTILINE,
)


def _unquote_po(raw: str) -> str:
    """Concatenate multi-line PO quoted strings and unescape."""
    parts = re.findall(r'"((?:[^"\\]|\\.)*)"', raw)
    joined = "".join(parts)
    return joined.replace("\\n", "\n").replace('\\"', '"').replace("\\\\", "\\")


def _quote_po(text: str) -> str:
    """Escape and quote a string for PO format."""
    escaped = text.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n")
    return f'"{escaped}"'


def parse_po_file(po_path: str) -> Tuple[str, List[dict]]:
    """Parse PO file. Returns (full_content, entries_list)."""
    try:
        with open(po_path, "r", encoding="utf-8-sig") as f:
            content = f.read()
    except UnicodeDecodeError:
        with open(po_path, "r", encoding="gbk") as f:
            content = f.read()

    entries = []
    for m in _PO_ENTRY_RE.finditer(content):
        entries.append({
            "comments": m.group("comments"),
            "msgctxt_raw": m.group("ctxt"),
            "msgid_raw": m.group("id"),
            "msgstr_raw": m.group("str"),
            "msgctxt": _unquote_po(m.group("ctxt")),
            "msgid": _unquote_po(m.group("id")),
            "msgstr": _unquote_po(m.group("str")),
            "span": (m.start(), m.end()),
        })
    return content, entries


def extract_pending(
    entries: List[dict],
    source_lang: str,
    target_lang: str,
    source_contexts: Optional[Dict[str, str]] = None,
) -> Tuple[dict, dict]:
    """Build compact JSON for Agent translation (incremental).

    Returns (pending_dict, existing_dict).
    Entries that already have a non-empty msgstr are treated as existing
    translations and skipped from the pending list.
    When source_contexts is provided (context-enhanced mode), a 'hint' field
    is added to pending entries to help the AI produce better translations.
    """
    pending_items = []
    existing_items = []
    for i, e in enumerate(entries):
        if not e["msgid"]:
            continue
        if e["msgstr"]:
            existing_items.append({"id": i, "dst": e["msgstr"]})
        else:
            item: dict = {"id": i, "ctx": e["msgctxt"], "src": e["msgid"]}
            if source_contexts:
                hint = source_contexts.get(e["msgid"])
                if hint:
                    item["hint"] = hint
            pending_items.append(item)

    pending = {
        "source_lang": source_lang,
        "target_lang": target_lang,
        "total": len(pending_items),
        "entries": pending_items,
    }
    existing = {
        "translations": existing_items,
    }
    return pending, existing


def inject_translations(po_content: str, entries: List[dict], translations: dict) -> str:
    """Merge translated.json back into PO content string."""
    id_to_dst = {t["id"]: t["dst"] for t in translations.get("translations", [])}

    replacements = []
    for i, e in enumerate(entries):
        if i in id_to_dst:
            dst = id_to_dst[i]
            old_msgstr_line = f"msgstr {e['msgstr_raw']}"
            new_msgstr_line = f"msgstr {_quote_po(dst)}"
            replacements.append((e["span"], old_msgstr_line, new_msgstr_line))

    # Apply replacements from end to start to preserve offsets
    for (start, end), old_line, new_line in reversed(replacements):
        chunk = po_content[start:end]
        chunk = chunk.replace(old_line, new_line, 1)
        po_content = po_content[:start] + chunk + po_content[end:]

    return po_content

# ---------------------------------------------------------------------------
# Commandlet runner
# ---------------------------------------------------------------------------

def run_commandlet(editor_cmd: str, uproject: str, config_path: str) -> bool:
    """Run UE GatherText commandlet with the given config INI."""
    config_rel = os.path.relpath(config_path, os.path.dirname(uproject)).replace("\\", "/")
    cmd = [editor_cmd, uproject, "-run=GatherText", f'-config="{config_rel}"']
    cmd_str = f'"{editor_cmd}" "{uproject}" -run=GatherText -config="{config_rel}"'
    result = subprocess.run(cmd_str, shell=True, capture_output=True, text=True)
    if result.returncode != 0:
        sys.stderr.write(result.stdout[-2000:] if len(result.stdout) > 2000 else result.stdout)
        sys.stderr.write(result.stderr[-2000:] if len(result.stderr) > 2000 else result.stderr)
    return result.returncode == 0

# ---------------------------------------------------------------------------
# Subcommand: gather
# ---------------------------------------------------------------------------

def cmd_gather(args):
    uplugin = os.path.abspath(args.uplugin)
    name = plugin_name(uplugin)
    cultures = args.cultures
    native = args.native
    use_context = getattr(args, "context", False)

    try:
        uproject = find_uproject(uplugin)
        engine_dir = resolve_engine_dir(uproject)
        editor_cmd = get_editor_cmd(engine_dir)
    except Exception as e:
        print(json.dumps({"status": "error", "message": str(e)}))
        return 1

    ensure_localization_target(uplugin, name)

    # Generate and write GatherExport INI
    project_root = os.path.dirname(uproject)
    config_dir = os.path.join(project_root, "Config", "Localization")
    os.makedirs(config_dir, exist_ok=True)
    ini_path = os.path.join(config_dir, f"{name}_GatherExport.ini")

    ini_content = generate_gather_export_ini(uplugin, uproject, name, native, cultures)
    with open(ini_path, "w", encoding="utf-8") as f:
        f.write(ini_content)

    # Run commandlet
    if not run_commandlet(editor_cmd, uproject, ini_path):
        print(json.dumps({"status": "error", "message": "GatherText commandlet failed. Check stderr for details."}))
        return 1

    # Context-enhanced mode: scan source files for surrounding code context
    source_contexts: Optional[Dict[str, str]] = None
    bp_context_count = 0
    if use_context:
        plugin_dir_abs = os.path.dirname(uplugin)
        src_dir = os.path.join(plugin_dir_abs, "Source")
        source_contexts = _extract_source_contexts(src_dir)

        # Blueprint context via UCP (optional — best-effort)
        if _ucp_available():
            plugin_content_path = f"/{name}/"
            bp_contexts = _extract_blueprint_contexts(plugin_content_path)
            bp_context_count = len(bp_contexts)
            for k, v in bp_contexts.items():
                if k not in source_contexts:
                    source_contexts[k] = v
        else:
            sys.stderr.write("Warning: UCP not available, skipping Blueprint context extraction\n")

    # Parse PO and extract pending JSON for each culture
    plugin_dir = os.path.dirname(uplugin)
    pending_files = []

    for culture in cultures:
        po_path = os.path.join(plugin_dir, "Content", "Localization", name, culture, f"{name}.po")
        if not os.path.isfile(po_path):
            sys.stderr.write(f"Warning: PO file not found: {po_path}\n")
            continue

        content, entries = parse_po_file(po_path)
        pending, existing = extract_pending(entries, native, culture, source_contexts)

        out_dir = os.path.dirname(po_path)
        pending_path = os.path.join(out_dir, "pending_translation.json")
        with open(pending_path, "w", encoding="utf-8") as f:
            json.dump(pending, f, ensure_ascii=False, indent=2)

        if existing["translations"]:
            existing_path = os.path.join(out_dir, "existing_translations.json")
            with open(existing_path, "w", encoding="utf-8") as f:
                json.dump(existing, f, ensure_ascii=False, indent=2)

        pending_files.append(pending_path.replace("\\", "/"))

    result = {
        "status": "ok",
        "message": f"Gathered {name}: {sum(1 for _ in pending_files)} culture(s) ready for translation.",
        "pending_files": pending_files,
    }
    if use_context and source_contexts:
        result["context_enhanced"] = True
        result["context_entries"] = len(source_contexts)
        if bp_context_count:
            result["context_blueprint_entries"] = bp_context_count

    print(json.dumps(result, ensure_ascii=False))
    return 0

# ---------------------------------------------------------------------------
# Subcommand: compile
# ---------------------------------------------------------------------------

def cmd_compile(args):
    uplugin = os.path.abspath(args.uplugin)
    name = plugin_name(uplugin)
    cultures = args.cultures
    native = args.native

    try:
        uproject = find_uproject(uplugin)
        engine_dir = resolve_engine_dir(uproject)
        editor_cmd = get_editor_cmd(engine_dir)
    except Exception as e:
        print(json.dumps({"status": "error", "message": str(e)}))
        return 1

    plugin_dir = os.path.dirname(uplugin)
    merged_count = 0

    for culture in cultures:
        po_path = os.path.join(plugin_dir, "Content", "Localization", name, culture, f"{name}.po")
        translated_path = os.path.join(os.path.dirname(po_path), "translated.json")

        if not os.path.isfile(po_path):
            sys.stderr.write(f"Warning: PO file not found: {po_path}\n")
            continue

        existing_path = os.path.join(os.path.dirname(po_path), "existing_translations.json")
        has_translated = os.path.isfile(translated_path)
        has_existing = os.path.isfile(existing_path)

        if not has_translated and not has_existing:
            sys.stderr.write(f"Warning: no translation files found for {culture}, skipping.\n")
            continue

        combined = {}
        if has_existing:
            with open(existing_path, "r", encoding="utf-8") as f:
                for t in json.load(f).get("translations", []):
                    combined[t["id"]] = t["dst"]
        if has_translated:
            with open(translated_path, "r", encoding="utf-8") as f:
                for t in json.load(f).get("translations", []):
                    combined[t["id"]] = t["dst"]

        translations = {"translations": [{"id": k, "dst": v} for k, v in combined.items()]}

        po_content, entries = parse_po_file(po_path)
        merged = inject_translations(po_content, entries, translations)

        with open(po_path, "w", encoding="utf-8") as f:
            f.write(merged)

        merged_count += len(combined)

    # Generate and write ImportCompile INI
    project_root = os.path.dirname(uproject)
    config_dir = os.path.join(project_root, "Config", "Localization")
    os.makedirs(config_dir, exist_ok=True)
    ini_path = os.path.join(config_dir, f"{name}_ImportCompile.ini")

    ini_content = generate_import_compile_ini(uplugin, uproject, name, native, cultures)
    with open(ini_path, "w", encoding="utf-8") as f:
        f.write(ini_content)

    # Run commandlet
    if not run_commandlet(editor_cmd, uproject, ini_path):
        print(json.dumps({"status": "error", "message": "Import/Compile commandlet failed. Check stderr for details."}))
        return 1

    print(json.dumps({
        "status": "ok",
        "message": f"Compiled {name}: {merged_count} translation(s) merged and .locres generated.",
    }, ensure_ascii=False))
    return 0

# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description="UE Plugin Localization Helper")
    sub = parser.add_subparsers(dest="command", required=True)

    for name, func in [("gather", cmd_gather), ("compile", cmd_compile)]:
        p = sub.add_parser(name)
        p.add_argument("--uplugin", required=True, help=".uplugin file path")
        p.add_argument("--cultures", nargs="+", default=["zh-Hans"], help="Target culture(s)")
        p.add_argument("--native", default="en", help="Native/source culture (default: en)")
        if name == "gather":
            p.add_argument("--context", action="store_true",
                           help="Context-enhanced mode: scan source files and attach code context hints to entries")
        p.set_defaults(func=func)

    args = parser.parse_args()
    sys.exit(args.func(args))


if __name__ == "__main__":
    main()
