#!/usr/bin/env python3
"""Read-only lexical guard for the reviewed i_core_proxy surface.

Compare full normalized virtual signatures, including overload parameter types,
return types and cv/ref/noexcept qualifiers. Also bind the test's typed member
pointers to that complete surface. This is not a C++ parser or an execution test:
unsupported declaration syntax fails explicitly rather than being ignored.
No compiler, wallet/native code, network, imports of project code or writes run.
"""
from collections import Counter
from pathlib import Path
import json
import re
import sys


ROOT = Path(__file__).resolve().parents[2]
ALLOWED = "call_COMMAND_RPC_GET_BLOCKS_DIRECT"
STRIP = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')
TOKEN = re.compile(r"[A-Za-z_]\w*|::|&&|[<>&*,]")
QUALIFIERS = {"const", "volatile"}
BUILTINS = {"void", "bool", "char", "signed", "unsigned", "short", "int", "long",
            "float", "double", "wchar_t", "char16_t", "char32_t"}


def clean(source):
    return STRIP.sub(lambda match: "\n" * match.group().count("\n"), source)


def tokens(text):
    found = TOKEN.findall(text)
    if "".join(found) != re.sub(r"\s+", "", text):
        raise ValueError("unsupported signature token: " + text.strip())
    return found


def normalize_type(text, parameter=False):
    found = tokens(text)
    if parameter and len(found) >= 2 and re.fullmatch(r"[A-Za-z_]\w*", found[-1]):
        prefix = found[:-1]
        if (found[-1] not in BUILTINS and prefix[-1] != "::"
                and any(re.fullmatch(r"[A-Za-z_]\w*", t) and t not in QUALIFIERS
                        for t in prefix)):
            found = prefix  # Named parameter; qualifiers and references stay.
    if not found:
        raise ValueError("empty type")
    return " ".join(found)


def split_parameters(text):
    if not text.strip() or text.strip() == "void":
        return []
    result, depth, start = [], 0, 0
    for index, char in enumerate(text):
        if char == "<":
            depth += 1
        elif char == ">":
            depth -= 1
            if depth < 0:
                raise ValueError("unbalanced template parameter")
        elif char == "," and depth == 0:
            result.append(text[start:index])
            start = index + 1
    if depth:
        raise ValueError("unbalanced template parameter")
    result.append(text[start:])
    return result


def signature(return_type, name, parameters, suffix=""):
    suffix = re.sub(r"\b(?:override|final)\b", "", suffix)
    suffix = re.sub(r"=\s*0\s*$", "", suffix).strip()
    if not re.fullmatch(r"(?:(?:const|volatile|noexcept|&&|&)\s*)*", suffix):
        raise ValueError("unsupported method qualifier: " + suffix)
    return (normalize_type(return_type), name,
            tuple(normalize_type(p, parameter=True) for p in split_parameters(parameters)),
            " ".join(suffix.split()))


def matching_brace(source, start):
    depth = 1
    for index in range(start + 1, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return index
    raise ValueError("unbalanced class or method braces")


def class_body(source, name):
    source = clean(source)
    match = re.search(r"\b(?:class|struct)\s+" + re.escape(name) + r"\b[^;{]*\{", source)
    if not match:
        raise ValueError("class missing: " + name)
    start = match.end() - 1
    return source[start + 1:matching_brace(source, start)]


def method_headers(body):
    start = index = 0
    while index < len(body):
        if body[index] == "{":
            yield body[start:index].strip()
            index = matching_brace(body, index) + 1
            start = index
        elif body[index] == ";":
            yield body[start:index].strip()
            index += 1
            start = index
        else:
            index += 1


def class_surface(source, name, marker):
    body = class_body(source, name)
    methods = []
    seen_markers = 0
    for header in method_headers(body):
        if not re.search(r"\b" + marker + r"\b", header):
            continue
        seen_markers += len(re.findall(r"\b" + marker + r"\b", header))
        header = re.sub(r"^\s*(?:public|protected|private)\s*:\s*", "", header)
        if re.fullmatch(r"(?:virtual\s+)?~" + re.escape(name)
                        + r"\s*\(\s*\)\s*(?:override\s*)?(?:=\s*default\s*)?", header):
            continue  # Destruction is not an RPC/capability surface.
        header = re.sub(r"^\s*virtual\s+", "", header)
        match = re.fullmatch(r"(.+?)\s+([A-Za-z_]\w*)\s*\(([^()]*)\)\s*(.*)",
                             header, flags=re.S)
        if not match:
            raise ValueError("unsupported " + name + " declaration: " + header)
        methods.append(signature(*match.groups()))
    if seen_markers != len(re.findall(r"\b" + marker + r"\b", body)):
        raise ValueError("unparsed " + marker + " declaration in " + name)
    if not methods:
        raise ValueError("empty surface: " + name)
    return Counter(methods)


def typed_test_surface(source):
    pattern = re.compile(
        r"using\s+method_type\s*=\s*(.+?)\s*"
        r"\(\s*tools::i_core_proxy::\s*\*\s*\)\s*\(([^()]*)\)\s*;"
        r"\s*const\s+method_type\s+method\s*=\s*&tools::i_core_proxy::(\w+)\s*;",
        re.S)
    source = clean(source)
    matches = list(pattern.finditer(source))
    if len(matches) != len(re.findall(r"\busing\s+method_type\b", source)):
        raise ValueError("unparsed typed test invocation")
    return Counter(signature(m.group(1), m.group(3), m.group(2)) for m in matches)


def describe(surface):
    return [{"signature": repr(key), "count": count} for key, count in sorted(surface.items())]


def require_equal(expected, actual, label):
    if expected != actual:
        raise ValueError(json.dumps({"surface": label,
                                     "missing": describe(expected - actual),
                                     "extra": describe(actual - expected)}, sort_keys=True))
    if any(count != 1 for count in actual.values()):
        raise ValueError("duplicate signature: " + label)


def main():
    base = (ROOT / "src/wallet/i_core_rpc_proxy.h").read_text(encoding="utf-8")
    wrapper = (ROOT / "src/wallet/read_only_core_proxy.h").read_text(encoding="utf-8")
    test = (ROOT / "tests/unit_tests/read_only_core_proxy_surface.cpp").read_text(encoding="utf-8")
    expected = class_surface(base, "i_core_proxy", "virtual")
    require_equal(expected, class_surface(wrapper, "core_proxy", "override"), "read-only proxy")
    require_equal(expected, class_surface(test, "counting_delegate", "override"), "counting delegate")
    require_equal(expected, typed_test_surface(test), "typed test calls")
    allowed = sum(count for entry, count in expected.items() if entry[1] == ALLOWED)
    if allowed != 1:
        raise ValueError("expected exactly one allowed block-fetch signature")
    print(json.dumps({"virtual_signatures": sum(expected.values()),
                      "denied_signatures": sum(expected.values()) - allowed,
                      "allowed_signatures": allowed, "matching_surfaces": 4,
                      "source_only": True}, sort_keys=True))


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError) as error:
        print("proxy surface check failed: " + str(error), file=sys.stderr)
        raise SystemExit(1)
