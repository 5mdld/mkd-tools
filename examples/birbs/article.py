"""XML helpers shared by the general and kanwa dictionaries."""

from html import escape
from pathlib import Path
import re
from string import Template

import mkd

ROOT = Path(__file__).resolve().parent


def inline(*elements):
    text = mkd.InlineText()
    for element in elements:
        text.append(element)
    return text


def article(directory, filename, title, *, target, targets=None, **fragments):
    source = directory / "articles" / filename
    template = Template(source.read_text(encoding="utf-8"))
    if targets:
        fragments.update({name: str(value) for name, value in targets.items()})

    body = template.substitute(fragments)
    # Keep source indentation from becoming whitespace in the native layout.
    body = re.sub(r">[ \t]*\n\s*<", "><", body)
    return f'''<html xmlns="http://www.w3.org/1999/xhtml" hmhtml="5" lang="ja">
<head><title>{escape(title)}</title>
<meta name="viewport" content="width=device-width, initial-scale=1.0"/>
<link rel="stylesheet" href="dictionary.css"/></head>
<body id="{target}">{body}</body></html>'''


def search_keys(entry, targets):
    """Expand the editorial key records without hiding any writer calls."""
    for scope_name, keys in entry["keys"].items():
        scope = getattr(mkd.SearchScope, scope_name)
        for value in keys:
            record = {"key": value} if isinstance(value, str) else value
            target = targets[record.get("target", "main")]
            yield scope, record["key"], target, record.get("exact_only", False)
