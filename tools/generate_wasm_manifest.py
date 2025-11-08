#!/usr/bin/env python3
"""
Generate a C header manifest for embedded WASM binaries from template
"""
import sys
import argparse
from pathlib import Path

def sanitize_name(name):
    """Convert name to valid C symbol (replace - and . with _)"""
    return name.replace('-', '_').replace('.', '_')

def generate_extern_declarations(wasm_names):
    """Generate extern declarations for all WASM symbols"""
    declarations = []
    for name in wasm_names:
        safe_name = sanitize_name(name)
        declarations.append(f"// {name}")
        declarations.append(f"extern const uint8_t _binary_{safe_name}_wasm_start[];")
        declarations.append(f"extern const uint8_t _binary_{safe_name}_wasm_end[];")
    return "\n".join(declarations)

def generate_manifest_entries(wasm_names):
    """Generate manifest array entries"""
    entries = []
    for name in wasm_names:
        safe_name = sanitize_name(name)
        entry = f"""    {{
        .name = "{name}",
        .data = _binary_{safe_name}_wasm_start,
        .size = (size_t)(_binary_{safe_name}_wasm_end - _binary_{safe_name}_wasm_start)
    }}"""
        entries.append(entry)
    return ",\n".join(entries)

def main():
    parser = argparse.ArgumentParser(description='Generate WASM manifest header from template')
    parser.add_argument('names', nargs='+', help='WASM container names')
    parser.add_argument('-t', '--template', required=True, help='Input template file')
    parser.add_argument('-o', '--output', required=True, help='Output header file')

    args = parser.parse_args()

    # Read template
    template_path = Path(args.template)
    if not template_path.exists():
        print(f"Error: Template file not found: {args.template}", file=sys.stderr)
        return 1

    template = template_path.read_text()

    # Generate content pieces
    extern_decls = generate_extern_declarations(args.names)
    manifest_entries = generate_manifest_entries(args.names)

    # Replace placeholders
    content = template.replace('@EXTERN_DECLARATIONS@', extern_decls)
    content = content.replace('@COUNT@', str(len(args.names)))
    content = content.replace('@MANIFEST_ENTRIES@', manifest_entries)

    # Write output
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(content)

    print(f"Generated manifest for {len(args.names)} WASM binaries: {', '.join(args.names)}")
    print(f"Template: {args.template}")
    print(f"Output: {args.output}")

    return 0

if __name__ == '__main__':
    sys.exit(main())
