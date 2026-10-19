#!/usr/bin/env python3
"""Enable explicit CNMT application versions in upstream hacbrewpack v3.x."""
import argparse,pathlib
p=argparse.ArgumentParser();p.add_argument('source',type=pathlib.Path);a=p.parse_args()
f=a.source/'cnmt.c';s=f.read_text()
if 'HACBREWPACK_TITLE_VERSION' not in s:
 needle='    // Common values'
 if needle not in s:raise SystemExit('Unsupported hacbrewpack source; cnmt.c was not modified')
 s=s.replace(needle,'''    /* Optional monotonic version for replacing previously installed homebrew. */
    const char *version = getenv("HACBREWPACK_TITLE_VERSION");
    if (version && *version) {
        char *end;
        unsigned long value = strtoul(version, &end, 0);
        if (*end || value > UINT32_MAX) {
            fprintf(stderr, "Invalid HACBREWPACK_TITLE_VERSION\\n");
            exit(EXIT_FAILURE);
        }
        cnmt_ctx->cnmt_header.title_version = (uint32_t)value;
    }

'''+needle,1)
 f.write_text(s)
print('CNMT version support ready; rebuild hacbrewpack before packaging.')
