#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Check repository Markdown paths, heading anchors and roadmap tracking URLs.

No SDKs or network access are required. This intentionally checks syntax and
links, not semantic drift; document ownership is reviewed with the PR template.
"""

from __future__ import annotations

import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unicodedata
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]
LINK = re.compile(r'!?\[(?:[^\[\]]|\[[^\]]*\])*\]\(\s*<?([^\s)>]+)>?(?:\s+"[^"]*")?\s*\)')
INLINE_CODE = re.compile(r'(`+).*?\1')
SCHEME = re.compile(r'^[a-zA-Z][a-zA-Z0-9+.-]*:')
MACHINE_PATH = re.compile(r'^(?:[a-zA-Z]:[\\/]|[\\/])')


def prose(text: str):
    fence = None
    for number, line in enumerate(text.splitlines(), 1):
        marker = re.match(r'^\s*(`{3,}|~{3,})', line)
        if marker:
            token = marker.group(1)
            if fence is None:
                fence = token
            elif token[0] == fence[0] and len(token) >= len(fence):
                fence = None
            continue
        if fence is None:
            yield number, line


def anchors(text: str) -> set[str]:
    found: set[str] = set()
    for _, line in prose(text):
        match = re.match(r'^#{1,6}\s+(.+?)\s*#*\s*$', line)
        if not match:
            continue
        title = re.sub(r'!?\[([^]]*)\]\([^)]*\)', r'\1', match.group(1))
        title = re.sub(r'<[^>]*>', '', title).replace('`', '').lower().strip()
        base = ''.join(c for c in title if c in ' -_' or
                       unicodedata.category(c)[0] in {'L', 'N'} or
                       unicodedata.category(c) == 'Mn').replace(' ', '-')
        slug, suffix = base, 0
        while slug in found:
            suffix += 1
            slug = f'{base}-{suffix}'
        found.add(slug)
    return found


def exact_path(root: Path, path: Path) -> bool:
    """Check each spelling explicitly, even on case-insensitive Windows."""
    current = root
    for part in path.relative_to(root).parts:
        if not current.is_dir() or part not in {p.name for p in current.iterdir()}:
            return False
        current /= part
    return True


def check_file(root: Path, path: Path, cache: dict[Path, set[str]]) -> list[str]:
    errors = []
    roadmap = path.relative_to(root).parts[:2] == ('docs', 'roadmap')
    for number, line in prose(path.read_text(encoding='utf-8')):
        for match in LINK.finditer(INLINE_CODE.sub('', line)):
            target = match.group(1)
            where = f'{path.relative_to(root).as_posix()}:{number}'
            if MACHINE_PATH.match(target) or target.lower().startswith('file:'):
                errors.append(f'{where}: absolute or machine-local link: {target}')
                continue
            if SCHEME.match(target):
                url = urlsplit(target)
                if roadmap and url.netloc.lower() == 'github.com' and re.search(
                        r'/(?:issues|pull)(?:/|$)', url.path):
                    if not re.fullmatch(r'/[^/]+/[^/]+/(?:issues|pull)/[1-9][0-9]*/?', url.path):
                        errors.append(f'{where}: invalid Issue/PR URL: {target}')
                continue
            file_part, _, fragment = target.partition('#')
            file_part = unquote(file_part)
            # abspath normalizes '..' without fixing casing as resolve() can.
            destination = Path(os.path.abspath(path.parent / file_part))
            if not destination.is_relative_to(root):
                errors.append(f'{where}: link escapes repository: {target}')
                continue
            if not exact_path(root, destination):
                errors.append(f'{where}: missing path or incorrect case: {file_part}')
                continue
            if fragment:
                if destination.suffix.lower() != '.md' or not destination.is_file():
                    errors.append(f'{where}: heading on non-Markdown target: {target}')
                    continue
                if destination not in cache:
                    cache[destination] = anchors(destination.read_text(encoding='utf-8'))
                if unquote(fragment) not in cache[destination]:
                    errors.append(f'{where}: missing heading: {target}')
    return errors


def selftest() -> None:
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory).resolve()
        (root / 'docs' / 'roadmap').mkdir(parents=True)
        target = root / 'Target.md'
        target.write_text('# API `types`\n# Repeat\n# Repeat\n```md\n# Hidden\n```\n', encoding='utf-8')
        source = root / 'docs' / 'roadmap' / 'current.md'
        cases = [
            ('[ok](../../Target.md#api-types)', False),
            ('[ok](../../Target.md#repeat-1)', False),
            ('[bad](../../target.md)', True),
            ('[bad](../../missing.md)', True),
            ('[bad](../../Target.md#hidden)', True),
            ('[bad](/tmp/local.md)', True),
            ('[bad](C:/local.md)', True),
            ('[bad](../../../outside.md)', True),
            ('[ok](https://example.com/missing)', False),
            ('[ok](https://github.com/owner/repo/pull/12)', False),
            ('[bad](https://github.com/owner/repo/issues/no-number)', True),
            ('[bad](https://github.com/owner/repo/pull/0)', True),
            ('`[ignored](missing.md)`', False),
            ('```md\n[ignored](missing.md)\n```', False),
        ]
        for content, expected_error in cases:
            source.write_text(content, encoding='utf-8')
            errors = check_file(root, source, {})
            if bool(errors) != expected_error:
                raise AssertionError((content, errors))
    print(f'Documentation checker self-test passed ({len(cases)} cases).')


def main() -> int:
    if sys.argv[1:] == ['--selftest']:
        selftest()
        return 0
    if sys.argv[1:]:
        print('Usage: python scripts/check_docs.py [--selftest]', file=sys.stderr)
        return 2
    listing = subprocess.run(
        ['git', '-C', str(ROOT), 'ls-files', '-z', '--cached', '--others',
         '--exclude-standard', '--', '*.md'], check=True, capture_output=True)
    paths = sorted({ROOT / name for name in listing.stdout.decode('utf-8').split('\0')
                    if name and (ROOT / name).is_file()})
    cache: dict[Path, set[str]] = {}
    errors = [error for path in paths for error in check_file(ROOT, path, cache)]
    if errors:
        print('\n'.join(errors), file=sys.stderr)
        return 1
    print(f'Documentation links checked ({len(paths)} Markdown files).')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
